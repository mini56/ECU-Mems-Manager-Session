#!/usr/bin/env python3
"""Append ANY PDF to a language-specific SESSION documentary snapshot.

One importer and one on-disk schema for French, English and future languages.
The source PDF need not be RAVE or AKM. A new PDF never replaces existing
documents, procedural rows, images, or the user's ECU database.
Output is a *candidate* ZIP, never an automatic public publication.
"""
from __future__ import annotations
import argparse
from contextlib import closing
import hashlib
import json
from datetime import datetime, timezone
import re
import shutil
import sqlite3
import tempfile
import zipfile
from pathlib import Path

from documentary_common import (
    LANGUAGES, MAX_SOURCE_PAGES, schema, validate_source_snapshot,
    ensure_import_history, register_added_pdf, documentary_import_report,
)

MAX_FILES = 2500
MAX_ZIP_BYTES = 256 * 1024 * 1024
MAX_UNPACKED_BYTES = 700 * 1024 * 1024


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def document_id(filename: str, digest: str) -> str:
    # Same identity across repeated imports, different identity when the
    # original source changes: no ambiguous replacement or row renumbering.
    normalized = re.sub(r"[^A-Z0-9]+", "_", Path(filename).stem.upper()).strip("_")
    return "DOC_" + normalized[:38].rstrip("_") + "_" + digest[:12].upper()


def verify_images(db: sqlite3.Connection, root: Path) -> None:
    recorded = list(db.execute(
        "SELECT relative_path,asset_sha256,asset_bytes FROM documentary_images"))
    known: set[str] = set()
    for relative,digest,size in recorded:
        if (not isinstance(relative,str)
                or not re.fullmatch(r"images/[A-Za-z0-9_.-]+\.(?:png|jpg|jpeg|webp)",relative,re.I)
                or relative in known or not isinstance(size,int) or not 0<size<=15*1024*1024):
            raise ValueError("INVALID_IMAGE_METADATA:" + str(relative))
        path = root / relative
        if not path.is_file() or path.is_symlink() or path.stat().st_size!=size or sha256(path)!=digest:
            raise ValueError("SOURCE_IMAGE_MISSING_OR_CORRUPT:" + relative)
        known.add(relative)
    folder = root / "images"
    actual = set()
    if folder.exists():
        for item in folder.rglob("*"):
            if item.is_symlink():
                raise ValueError("SOURCE_SNAPSHOT_SYMLINK_FORBIDDEN")
            if item.is_file():
                actual.add(item.relative_to(root).as_posix())
    if known != actual:
        raise ValueError("UNDECLARED_SOURCE_IMAGE_IN_SNAPSHOT")


def copy_snapshot(source: Path, root: Path, language: str) -> None:
    if not source.is_file() or source.suffix.lower() != ".zip":
        raise ValueError("SOURCE_SNAPSHOT_MUST_BE_UNENCRYPTED_ZIP")
    dbname = f"mems_documentary_{language}.sqlite"
    with zipfile.ZipFile(source) as archive:
        members = [entry for entry in archive.infolist() if not entry.is_dir()]
        names = [entry.filename for entry in members]
        if (len(members) >= MAX_FILES or len(names)!=len(set(names)) or dbname not in names
                or source.stat().st_size > MAX_ZIP_BYTES
                or sum(e.file_size for e in members)>MAX_UNPACKED_BYTES):
            raise ValueError("SOURCE_SNAPSHOT_LIMITS_OR_DATABASE_MISSING")
        for entry in members:
            name=entry.filename
            if (name!=dbname and
                    not re.fullmatch(r"images/[A-Za-z0-9_.-]+\.(?:png|jpg|jpeg|webp)",name,re.I)):
                raise ValueError("UNEXPECTED_SNAPSHOT_ASSET:" + name)
            if entry.is_dir() or (entry.external_attr >> 16) & 0o170000 == 0o120000:
                raise ValueError("ARCHIVE_LINK_OR_DIRECTORY_FORBIDDEN")
            target=root / name
            target.parent.mkdir(parents=True,exist_ok=True)
            with archive.open(entry) as stream, target.open("wb") as output:
                shutil.copyfileobj(stream, output)
        if archive.testzip() is not None:
            raise ValueError("SOURCE_SNAPSHOT_CRC_ERROR")
    with closing(sqlite3.connect(f"file:{root/dbname}?mode=ro",uri=True)) as db:
        validate_source_snapshot(db,language)
        verify_images(db,root)


def append_pdf(snapshot: Path | None, pdf: Path, language: str,
               output: Path, publisher: str="Source documentaire") -> dict:
    if language not in LANGUAGES:
        raise ValueError("DOCUMENTARY_LANGUAGE_NOT_SUPPORTED")
    if not pdf.is_file() or pdf.is_symlink() or pdf.suffix.lower()!=".pdf":
        raise ValueError("ORIGINAL_PDF_REQUIRED")
    if pdf.stat().st_size>MAX_UNPACKED_BYTES:
        raise ValueError("SOURCE_PDF_TOO_LARGE")
    import fitz
    digest=sha256(pdf)
    source_id=document_id(pdf.name,digest)
    revision="source-"+digest[:24]
    dbname=f"mems_documentary_{language}.sqlite"
    output.parent.mkdir(parents=True,exist_ok=True)
    if snapshot and snapshot.resolve()==output.resolve():
        raise ValueError("SOURCE_SNAPSHOT_MUST_NOT_BE_OVERWRITTEN")
    previous_zip_sha256=sha256(snapshot) if snapshot else None
    with tempfile.TemporaryDirectory(prefix="mems-doc-add-") as folder:
        root=Path(folder)
        dbpath=root / dbname
        if snapshot:
            copy_snapshot(snapshot,root,language)
        with closing(sqlite3.connect(dbpath)) as db:
            db.execute("PRAGMA foreign_keys=ON")
            if snapshot is None:
                db.executescript(schema(language))
                db.execute(
                    "INSERT INTO documentary_database VALUES(1,1,?,1,'source_inventory')",
                    (language,))
                db.commit()
            ensure_import_history(db,
                legacy_snapshot_sha256=previous_zip_sha256,
                existing_kind="existing_snapshot")
            existing=db.execute(
                "SELECT 1 FROM documentary_documents WHERE document_id=? AND source_revision=?",
                (source_id,revision)).fetchone()
            if existing:
                raise ValueError("ORIGINAL_DOCUMENT_ALREADY_IN_LANGUAGE_SNAPSHOT")
            images=root / "images"
            images.mkdir(exist_ok=True)
            with fitz.open(pdf) as original:
                count=original.page_count
                if not 1<=count<=MAX_SOURCE_PAGES:
                    raise ValueError("PDF_PAGE_COUNT_UNSUPPORTED")
                db.execute(
                    "INSERT INTO documentary_documents VALUES(?,?,?,?,?,?,?,?,?)",
                    (source_id,revision,language,publisher,
                     pdf.stem,pdf.name,digest,count,
                     "Original PDF pages, native text and page facsimiles; "
                     "unreviewed text is never a validated repair instruction"))
                for index in range(count):
                    page=original.load_page(index)
                    number=index+1
                    source_text=page.get_text("text",sort=True).strip()
                    db.execute(
                        "INSERT INTO documentary_pages VALUES(?,?,?,?,?)",
                        (source_id,revision,number,0,
                         "source_linked" if source_text else "not_yet_structured"))
                    # The only universal assertion from an unreviewed PDF is
                    # that this original page exists and contains source text.
                    # Never invent a procedure, torque value or engine scope.
                    raw=json.dumps({"source_pdf_sha256":digest,
                                    "source_page":number,
                                    "extraction":"native_pdf_text",
                                    "review":"pending"},sort_keys=True)
                    db.execute(
                        "INSERT INTO documentary_entries VALUES(?,?,?,?,?,?,?,?,?,?)",
                        (source_id,revision,f"page:{number:05d}","reference",number,
                         f"{pdf.stem} — page {number}",source_text,
                         "review_required","original_pdf_page",raw))
                    filename=f"{source_id}_PDF_{number:03d}.png"
                    image=page.get_pixmap(
                        matrix=fitz.Matrix(1.15,1.15),
                        colorspace=fitz.csGRAY,alpha=False).tobytes("png")
                    if not image or len(image)>15*1024*1024:
                        raise ValueError("ORIGINAL_PAGE_IMAGE_TOO_LARGE")
                    target=images / filename
                    target.write_bytes(image)
                    db.execute(
                        "INSERT INTO documentary_images VALUES(?,?,?,?,?,?)",
                        (source_id,revision,number,
                         "images/"+filename,
                         hashlib.sha256(image).hexdigest(),len(image)))
            register_added_pdf(db,document_id=source_id,revision=revision,
                               previous_snapshot_sha256=previous_zip_sha256,
                               imported_utc=datetime.now(timezone.utc).isoformat())
            db.execute(
                "UPDATE documentary_database SET collection_revision=collection_revision+1,"
                "publication_status='source_inventory' WHERE singleton=1")
            db.commit()
            details=validate_source_snapshot(db,language)
            audit=documentary_import_report(db,language)
            verify_images(db,root)
        assets=sorted((root/"images").glob("*"))
        if len(assets)+1>=MAX_FILES:
            raise ValueError("DOC_SNAPSHOT_FILE_COUNT_EXCEEDED")
        if dbpath.stat().st_size+sum(x.stat().st_size for x in assets)>MAX_UNPACKED_BYTES:
            raise ValueError("DOC_SNAPSHOT_TOO_LARGE_FOR_WINDOWS_INSTALLER")
        partial=output.with_name(output.name+".part")
        try:
            with zipfile.ZipFile(partial,"w",zipfile.ZIP_DEFLATED,
                                 compresslevel=7) as archive:
                archive.write(dbpath,dbname)
                for path in assets:
                    archive.write(path,"images/"+path.name)
            if partial.stat().st_size>MAX_ZIP_BYTES:
                raise ValueError("DOC_SNAPSHOT_ZIP_TOO_LARGE_FOR_WINDOWS_INSTALLER")
            with zipfile.ZipFile(partial) as archive:
                if archive.testzip() is not None:
                    raise ValueError("DOC_SNAPSHOT_CORRUPT_ZIP")
            partial.replace(output)
        finally:
            partial.unlink(missing_ok=True)
    result=dict(details,new_document_id=source_id,
                new_source_pages=count,original_pdf_sha256=digest,
                candidate_zip_sha256=sha256(output),
                production_installed=False)
    # The sidecar is human-readable; the immutable source history is inside
    # SQLite, so the trace survives encrypted distribution and installation.
    report_path=output.with_suffix(output.suffix+".audit.json")
    report_path.write_text(json.dumps(dict(result,**audit),ensure_ascii=False,
                                      indent=2,sort_keys=True)+"\n",encoding="utf-8")
    result["audit_report"]=str(report_path)
    result["import_sequence"]=audit["history"][-1]["sequence"]
    return result


def main() -> None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--language",choices=sorted(LANGUAGES),required=True)
    parser.add_argument("--pdf",type=Path,required=True)
    parser.add_argument("--snapshot",type=Path,
                        help="Previously verified full snapshot ZIP of this language")
    parser.add_argument("--output",type=Path,required=True)
    parser.add_argument("--publisher",default="Source documentaire")
    args=parser.parse_args()
    result=append_pdf(args.snapshot,args.pdf,args.language,
                      args.output,args.publisher)
    print(json.dumps(result,ensure_ascii=False,sort_keys=True))


if __name__=="__main__":
    main()
