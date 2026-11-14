#!/usr/bin/env python3
"""Build ONE native English documentary SQLite with BOTH AKM7169ENG and RCL0193ENG.

Inputs are the exact existing English AKM source fragments and the original
RAVE PDF committed at rave/xn/wmxn990e.pdf. Do not translate, substitute
manuals, or modify FR, ECU, expert, or user databases.

The original AKM tables remain untouched in the resulting SQLite for audit.
Both documents are also indexed in the shared language-native schema used by
the SESSION documentary reader. Imported technical text is REVIEW_REQUIRED,
not a validated repair instruction.

Output is a private, unencrypted *candidate* ZIP; distribution/activation
requires the established authenticated administrator-pack process and rights.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import shutil
import sqlite3
import tempfile
import zipfile

try:
    import fitz
except ImportError as exc:
    raise SystemExit("PyMuPDF required: python -m pip install pymupdf") from exc

from build_rave_en_documentary import build as build_akm

RAVE_SHA = "c050a3eebe50c5a85bf8a69b7722bd2052079944e09d58578a498984ecf06715"
AKM_SHA = "c8bbb30d7d5a52932e7f92723ba5dc70520012ac3ceac21d19ab0a39b4d4c4e0"
RAVE_PAGES = 372
AKM_PAGES = 482
RAVE_BLANK = {
    2, 8, 10, 14, 16, 18, 30, 34, 42, 48, 52, 98, 106, 120, 138, 140,
    146, 148, 152, 154, 162, 176, 178, 192, 194, 202, 204, 212, 214,
    228, 230, 234, 236, 256, 258, 262, 274, 280, 282, 298, 304, 306,
    316, 326, 328, 360,
}
SCHEMA = """
PRAGMA foreign_keys=ON;
CREATE TABLE documentary_database(
 singleton INTEGER PRIMARY KEY CHECK(singleton=1),
 schema_version INTEGER NOT NULL CHECK(schema_version=1),
 language TEXT NOT NULL CHECK(language='en'),
 collection_revision INTEGER NOT NULL CHECK(collection_revision>0),
 publication_status TEXT NOT NULL CHECK(publication_status IN ('source_inventory','validated'))
);
CREATE TABLE documentary_documents(
 document_id TEXT NOT NULL, source_revision TEXT NOT NULL,
 language TEXT NOT NULL CHECK(language='en'), publisher TEXT NOT NULL,
 edition_code TEXT NOT NULL, source_filename TEXT NOT NULL,
 source_sha256 TEXT NOT NULL, source_pages INTEGER NOT NULL,
 extraction TEXT NOT NULL, PRIMARY KEY(document_id,source_revision)
);
CREATE TABLE documentary_pages(
 document_id TEXT NOT NULL, source_revision TEXT NOT NULL,
 physical_page INTEGER NOT NULL CHECK(physical_page BETWEEN 1 AND 999),
 structured_records INTEGER NOT NULL DEFAULT 0,
 indexed_state TEXT NOT NULL CHECK(indexed_state IN ('source_linked','not_yet_structured')),
 PRIMARY KEY(document_id,source_revision,physical_page),
 FOREIGN KEY(document_id,source_revision)
 REFERENCES documentary_documents(document_id,source_revision)
);
CREATE TABLE documentary_images(
 document_id TEXT NOT NULL, source_revision TEXT NOT NULL,
 physical_page INTEGER NOT NULL,
 relative_path TEXT NOT NULL, asset_sha256 TEXT NOT NULL,
 asset_bytes INTEGER NOT NULL,
 PRIMARY KEY(document_id,source_revision,physical_page),
 FOREIGN KEY(document_id,source_revision,physical_page)
 REFERENCES documentary_pages(document_id,source_revision,physical_page)
);
CREATE TABLE documentary_entries(
 document_id TEXT NOT NULL, source_revision TEXT NOT NULL,
 entry_id TEXT NOT NULL, entry_type TEXT NOT NULL CHECK(entry_type IN
 ('reference','procedure','step','technical_value','notice','figure','other')),
 source_page INTEGER CHECK(source_page BETWEEN 1 AND 999),
 title TEXT NOT NULL, source_text TEXT NOT NULL,
 verification_status TEXT NOT NULL CHECK(verification_status IN
 ('source_checked','review_required','technical_reviewed')),
 source_table TEXT NOT NULL, raw_json TEXT NOT NULL,
 PRIMARY KEY(document_id,source_revision,entry_id),
 FOREIGN KEY(document_id,source_revision)
 REFERENCES documentary_documents(document_id,source_revision)
);
CREATE INDEX idx_documentary_page ON documentary_entries(document_id,source_page);
CREATE INDEX idx_documentary_type ON documentary_entries(entry_type);
CREATE VIRTUAL TABLE documentary_fts USING fts5(title, source_text,
 content='documentary_entries', content_rowid='rowid',
 tokenize='unicode61 remove_diacritics 2');
CREATE TRIGGER documentary_fts_insert AFTER INSERT ON documentary_entries BEGIN
 INSERT INTO documentary_fts(rowid,title,source_text)
 VALUES(new.rowid,new.title,new.source_text);
END;
"""


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise ValueError(reason)


def sha_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        while block := f.read(1024 * 1024):
            h.update(block)
    return h.hexdigest()


def insert_entry(db: sqlite3.Connection, doc: str, revision: str, key: str,
                 kind: str, page: int | None, title: str, text: str,
                 table: str, raw: object = None) -> None:
    db.execute(
        "INSERT INTO documentary_entries VALUES(?,?,?,?,?,?,?,?,?,?)",
        (doc, revision, key, kind, page, title[:600], text,
         "review_required", table,
         json.dumps(raw if raw is not None else {}, ensure_ascii=False, default=str)),
    )


def source_rows(db: sqlite3.Connection, name: str):
    cols = {row[1] for row in db.execute('PRAGMA table_info("' + name + '")')}
    require(bool(cols), "AKM_SOURCE_TABLE_MISSING:" + name)
    for row in db.execute('SELECT rowid AS __source_rowid__, * FROM "' + name + '"'):
        yield dict(row)


def page_number(value: object, maximum: int) -> int | None:
    try:
        page = int(value)
    except (ValueError, TypeError):
        return None
    return page if 1 <= page <= maximum else None


def first_text(row: dict, *keys: str) -> str:
    for key in keys:
        value = row.get(key)
        if value is not None and str(value).strip():
            return str(value).strip()
    return ""


def add_akm(db: sqlite3.Connection) -> dict:
    revision = "AKM7169ENG-original-482"
    source = db.execute(
        "SELECT original_pdf_sha256,total_pages FROM source_documents"
    ).fetchone()
    require(source == (AKM_SHA, AKM_PAGES), "AKM_SOURCE_PROVENANCE_CHANGED")
    db.execute("INSERT INTO documentary_documents VALUES(?,?,?,?,?,?,?,?,?)",
               ("AKM7169ENG", revision, "en", "Rover", "AKM7169ENG",
                "AKM7169ENG.pdf", AKM_SHA, AKM_PAGES,
                "Original source SQLite; native OCR and structured-index migration"))
    for page in range(1, AKM_PAGES + 1):
        db.execute("INSERT INTO documentary_pages VALUES(?,?,?,?,?)",
                   ("AKM7169ENG", revision, page, 0, "not_yet_structured"))
    counters = Counter()
    for row in source_rows(db, "source_page_discovery"):
        page = page_number(row.get("physical_page"), AKM_PAGES)
        require(page is not None, "AKM_BAD_PAGE_ID")
        text = first_text(row, "ocr_text")
        if text:
            insert_entry(db, "AKM7169ENG", revision, f"page:{page:03d}",
                         "reference", page, f"AKM7169ENG — page {page}",
                         text, "akm7169eng_page_ocr", row)
            counters["indexed_pages"] += 1
    for row in source_rows(db, "procedures"):
        key = first_text(row, "procedure_id")
        require(bool(key), "AKM_PROCEDURE_WITHOUT_ID")
        title = first_text(row, "title", "manufacturer_repair_no")
        text = " — ".join(x for x in [
            first_text(row, "manufacturer_repair_no"), title] if x)
        page = page_number(row.get("source_page"), AKM_PAGES)
        insert_entry(db, "AKM7169ENG", revision, "procedure:" + key,
                     "procedure", page, title, text,
                     "akm7169eng_procedures", row)
        counters["procedures"] += 1
    for row in source_rows(db, "procedure_steps"):
        rowid = row["__source_rowid__"]
        text = first_text(row, "source_text")
        page = page_number(row.get("source_page"), AKM_PAGES)
        if text:
            insert_entry(db, "AKM7169ENG", revision, f"step:{rowid}",
                         "step", page,
                         " — ".join(filter(None, [
                             first_text(row, "procedure_id"),
                             first_text(row, "phase"),
                             first_text(row, "step_number")])),
                         text, "akm7169eng_procedure_steps", row)
            counters["steps"] += 1
    for row in source_rows(db, "technical_values"):
        rowid = row["__source_rowid__"]
        title = first_text(row, "parameter", "description",
                           "title", "component", "name")
        details = [f"{key}: {value}" for key, value in row.items()
                   if key not in ("__source_rowid__", "id", "value_id")
                   and value is not None and str(value).strip()]
        page = page_number(row.get("source_page"), AKM_PAGES)
        if details:
            insert_entry(db, "AKM7169ENG", revision, f"value:{rowid}",
                         "technical_value", page,
                         title or "AKM7169ENG — technical specification",
                         "\n".join(details), "akm7169eng_technical_values", row)
            counters["values"] += 1
    require(counters["procedures"] == 336 and counters["steps"] == 5055
            and counters["values"] == 630,
            "AKM_STRUCTURED_COUNTS_CHANGED:" + str(dict(counters)))
    return dict(counters)


def add_rave(db: sqlite3.Connection, path: Path, image_dir: Path) -> dict:
    require(path.is_file() and sha_file(path) == RAVE_SHA,
            "RCL0193ENG_ORIGINAL_PDF_SHA_MISMATCH")
    revision = "RCL0193ENG-5th-edition"
    with fitz.open(path) as pdf:
        require(pdf.page_count == RAVE_PAGES, "RCL0193ENG_PAGE_COUNT_INVALID")
        db.execute("INSERT INTO documentary_documents VALUES(?,?,?,?,?,?,?,?,?)",
                   ("RCL0193ENG", revision, "en", "Rover", "RCL0193ENG",
                    "rave/xn/wmxn990e.pdf", RAVE_SHA, RAVE_PAGES,
                    "PDF native text and manufacturer-page facsimiles, review required"))
        counters = Counter()
        for index in range(pdf.page_count):
            number = index + 1
            page = pdf.load_page(index)
            source_text = page.get_text("text", sort=True).strip()
            blank = number in RAVE_BLANK
            require(blank or bool(source_text),
                    f"RCL0193ENG_UNEXTRACTED_NONBLANK_PAGE:{number}")
            db.execute("INSERT INTO documentary_pages VALUES(?,?,?,?,?)",
                       ("RCL0193ENG", revision, number, 0,
                        "not_yet_structured" if blank else "source_linked"))
            if blank:
                require(not source_text, f"RCL0193ENG_BLANK_MISMATCH:{number}")
                counters["blank_pages"] += 1
                continue
            insert_entry(db, "RCL0193ENG", revision, f"page:{number:03d}",
                         "reference", number,
                         f"RCL0193ENG Mini Workshop Manual — page {number}",
                         source_text, "rcl0193eng_pdf_text",
                         {"physical_page": number, "source_pdf_sha256": RAVE_SHA,
                          "extraction": "native_pdf_text", "review": "pending"})
            counters["indexed_pages"] += 1
            image_dir.mkdir(parents=True, exist_ok=True)
            rel = f"images/RCL0193ENG_PDF_{number:03d}.png"
            png = page.get_pixmap(matrix=fitz.Matrix(1.35, 1.35),
                                  colorspace=fitz.csGRAY, alpha=False).tobytes("png")
            image_path = image_dir / Path(rel).name
            image_path.write_bytes(png)
            db.execute("INSERT INTO documentary_images VALUES(?,?,?,?,?,?)",
                       ("RCL0193ENG", revision, number, rel,
                        hashlib.sha256(png).hexdigest(), len(png)))
            counters["facsimiles"] += 1
        require(counters == Counter({"blank_pages": 46, "indexed_pages": 326,
                                     "facsimiles": 326}),
                "RCL0193ENG_SOURCE_COVERAGE_MISMATCH:" + str(dict(counters)))
    return dict(counters)


def build(parts: Path, rave: Path, out: Path) -> dict:
    require(not out.resolve().is_relative_to(parts.resolve()),
            "OUTPUT_MUST_NOT_OVERWRITE_ORIGINAL_PARTS")
    out.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="mems-rave-en-") as temporary:
        tmp = Path(temporary)
        legacy_folder = tmp / "akm"
        build_akm(parts, legacy_folder)
        target = tmp / "mems_documentary_en.sqlite"
        shutil.copy2(legacy_folder / "mems_documentary_en.sqlite", target)
        image_dir = tmp / "images"
        with sqlite3.connect(target) as db:
            db.row_factory = sqlite3.Row
            db.execute("PRAGMA foreign_keys=ON")
            db.executescript(SCHEMA)
            db.execute("INSERT INTO documentary_database VALUES(1,1,'en',2,'source_inventory')")
            akm = add_akm(db)
            rave_data = add_rave(db, rave, image_dir)
            db.commit()
            require(db.execute("PRAGMA quick_check").fetchone()[0] == "ok",
                    "NATIVE_EN_INTEGRITY_FAILED")
            require(not db.execute("PRAGMA foreign_key_check").fetchall(),
                    "NATIVE_EN_FOREIGN_KEYS_FAILED")
            docs = db.execute(
                "SELECT document_id,source_pages FROM documentary_documents ORDER BY document_id"
            ).fetchall()
            require([tuple(r) for r in docs] ==
                    [("AKM7169ENG", 482), ("RCL0193ENG", 372)],
                    "NATIVE_EN_DOCUMENT_MISMATCH")
            n = db.execute("SELECT COUNT(*) FROM documentary_entries").fetchone()[0]
            fts = db.execute("SELECT COUNT(*) FROM documentary_fts").fetchone()[0]
            pages = db.execute("SELECT COUNT(*) FROM documentary_pages").fetchone()[0]
            require(n == fts and pages == 854, "NATIVE_EN_FTS_OR_PAGE_LOSS")
            require(db.execute(
                "SELECT COUNT(*) FROM documentary_entries WHERE verification_status!='review_required'"
            ).fetchone()[0] == 0, "UNAPPROVED_TEXT_WAS_MARKED_VERIFIED")
            require(db.execute("SELECT COUNT(*) FROM procedures").fetchone()[0] == 336,
                    "ORIGINAL_AKM_PROCEDURES_CHANGED")
            report = {"language": "en", "documents": [
                {"id": "AKM7169ENG", "pages": AKM_PAGES, "source_sha256": AKM_SHA},
                {"id": "RCL0193ENG", "pages": RAVE_PAGES, "source_sha256": RAVE_SHA}],
                "akm": akm, "rave": rave_data,
                "native_entries": n, "native_fts": fts, "native_pages": pages,
                "approval": "source_inventory / review_required",
                "sqlite_sha256": sha_file(target),
                "status": "private_unencrypted_candidate_only",
                "published": False}
        package = out / "MEMS-DOC-EN-AKM7169ENG-RCL0193ENG-PRIVATE-CANDIDATE.zip"
        with zipfile.ZipFile(package, "w", zipfile.ZIP_DEFLATED,
                             compresslevel=7) as archive:
            archive.write(target, "mems_documentary_en.sqlite")
            for asset in sorted(image_dir.glob("*.png")):
                archive.write(asset, "images/" + asset.name)
        require(zipfile.ZipFile(package).testzip() is None, "CANDIDATE_ZIP_CRC_FAILED")
        report["candidate_zip_sha256"] = sha_file(package)
        report["candidate_zip_bytes"] = package.stat().st_size
        report_file = out / "RAVE_EN_MERGE_REPORT.json"
        report_file.write_text(json.dumps(report, ensure_ascii=False, indent=2),
                               encoding="utf-8")
        print(json.dumps(report, ensure_ascii=False, indent=2))
        return report


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--parts", type=Path, default=Path("database/documentary/parts"))
    p.add_argument("--rave", type=Path, default=Path("rave/xn/wmxn990e.pdf"))
    p.add_argument("--output", type=Path, default=Path("output/rave-en-multidoc"))
    args = p.parse_args()
    build(args.parts, args.rave, args.output)
