#!/usr/bin/env python3
"""Add French OCR *to existing RCL0193FRE*, without adding another manual.

The uploaded 206 MB scanned 'wmxn980f.pdf' was conclusively matched against
21 of 22 sampled manufacturer pages already installed as RCL0193FRE.
This tool only enriches those same 371 pages. It never:
- duplicates the source or changes the 417 known workshop operations;
- rewrites repair steps, notices, figure assets, or document identifiers;
- treats unreviewed OCR as source_checked repair instructions;
- publishes licensed manufacturer material.
"""
from __future__ import annotations
import argparse
from contextlib import closing
import gzip
import hashlib
import json
from pathlib import Path
import sqlite3
import tempfile
import zipfile

from add_documentary_pdf import copy_snapshot, verify_images, sha256
from documentary_common import validate_source_snapshot, documentary_import_report

SOURCE_ID="RCL0193FRE"
SOURCE_PAGES=371
PRINTED_PDF_SHA256="9150471b587f0216408220cdfbc5dc19c00262d73dc447c6d10ab82a49961086"
MATCHED_ORIGINAL_SHA256="0c7fef28d0d0f0673ba321d6625a019c005823103caa98afb3258114e1fec713"
RECORD_PREFIX="page-ocr:wmxn980f:"


def core_records(db):
    """Hash unchanged source information in deterministic row order."""
    data={}
    for table in ("documentary_documents","documentary_pages",
                  "documentary_images","documentary_operations",
                  "documentary_operation_steps","documentary_operation_notices"):
        present=db.execute(
          "SELECT 1 FROM sqlite_master WHERE type='table' AND name=?",
          (table,)).fetchone()
        if not present:
            continue
        digest=hashlib.sha256()
        cursor=db.execute(f'SELECT * FROM "{table}" ORDER BY rowid')
        for row in cursor:
            digest.update(json.dumps(row,ensure_ascii=False,
                           separators=(",",":"),default=str).encode())
            digest.update(b"\n")
        data[table]=digest.hexdigest()
    digest=hashlib.sha256()
    cursor=db.execute(
      "SELECT * FROM documentary_entries WHERE entry_id NOT LIKE ? "
      "ORDER BY document_id,source_revision,entry_id",
      (RECORD_PREFIX+"%",))
    for row in cursor:
        digest.update(json.dumps(row,ensure_ascii=False,
                       separators=(",",":"),default=str).encode())
        digest.update(b"\n")
    data["unchanged_existing_entries"]=digest.hexdigest()
    return data


def load_ocr(source: Path) -> list[dict]:
    if not source.is_file() or source.stat().st_size>4*1024*1024:
        raise ValueError("FRENCH_OCR_SOURCE_MISSING_OR_EXCESSIVE")
    results=[]
    with gzip.open(source,'rt',encoding="utf-8") as input:
        for line in input:
            record=json.loads(line)
            if not isinstance(record,dict):
                raise ValueError("FRENCH_OCR_BAD_RECORD")
            page=record.get("physical_page")
            text=record.get("text")
            if page != len(results)+1 or not isinstance(text,str):
                raise ValueError("FRENCH_OCR_PAGES_NOT_CONSECUTIVE")
            if record.get("source_sha256")!=PRINTED_PDF_SHA256:
                raise ValueError("FRENCH_OCR_WRONG_ORIGINAL_PDF")
            if record.get("verification_status")!="review_required":
                raise ValueError("FRENCH_OCR_FALSE_REVIEW_STATUS")
            if hashlib.sha256(text.encode()).hexdigest()!=record.get("page_text_sha256"):
                raise ValueError("FRENCH_OCR_PAGE_TEXT_CORRUPT")
            results.append(record)
    if len(results)!=SOURCE_PAGES:
        raise ValueError("FRENCH_OCR_INCOMPLETE_371_SOURCE_PAGES")
    return results


def apply_to_snapshot(original: Path, ocr_gzip: Path, output: Path) -> dict:
    if original.resolve()==output.resolve():
        raise ValueError("FRENCH_SOURCE_SNAPSHOT_CANNOT_BE_OVERWRITTEN")
    records=load_ocr(ocr_gzip)
    output.parent.mkdir(parents=True,exist_ok=True)
    baseline_sha=sha256(original)
    with tempfile.TemporaryDirectory(prefix="mems-ocr-fr-") as tmp:
        root=Path(tmp)
        copy_snapshot(original,root,"fr")
        path=root/"mems_documentary_fr.sqlite"
        with closing(sqlite3.connect(path)) as db:
            db.execute("PRAGMA foreign_keys=ON")
            db.execute("BEGIN")
            document=db.execute("""
                SELECT source_revision,source_pages,source_sha256
                FROM documentary_documents WHERE document_id=?""",
                (SOURCE_ID,)).fetchall()
            if len(document)!=1 or document[0][1]!=SOURCE_PAGES or document[0][2]!=MATCHED_ORIGINAL_SHA256:
                raise ValueError("FRENCH_RAVE_EXISTING_MANUAL_IDENTITY_MISMATCH")
            revision=document[0][0]
            old_count=db.execute("SELECT COUNT(*) FROM documentary_entries").fetchone()[0]
            old_fingerprint=core_records(db)
            if db.execute("SELECT 1 FROM documentary_entries WHERE document_id=? "
                          "AND entry_id LIKE ? LIMIT 1",
                          (SOURCE_ID,RECORD_PREFIX+"%")).fetchone():
                raise ValueError("FRENCH_OCR_ALREADY_APPLIED")
            db.execute("""
              CREATE TABLE IF NOT EXISTS documentary_enrichment_history(
                event_id TEXT PRIMARY KEY,document_id TEXT NOT NULL,
                source_revision TEXT NOT NULL,uploaded_pdf_sha256 TEXT NOT NULL,
                ocr_archive_sha256 TEXT NOT NULL,
                pages_processed INTEGER NOT NULL,ocr_pages_with_text INTEGER NOT NULL,
                entries_added INTEGER NOT NULL,previous_snapshot_sha256 TEXT NOT NULL,
                verification_status TEXT NOT NULL,
                FOREIGN KEY(document_id,source_revision)
                REFERENCES documentary_documents(document_id,source_revision)
              )""")
            with ocr_gzip.open("rb") as file:
                ocr_hash=hashlib.file_digest(file,'sha256').hexdigest()
            text_count=0
            for record in records:
                page=record["physical_page"]
                if not db.execute("""
                    SELECT 1 FROM documentary_pages WHERE
                      document_id=? AND source_revision=? AND physical_page=?""",
                    (SOURCE_ID,revision,page)).fetchone():
                    raise ValueError("FRENCH_ORIGINAL_PAGE_IS_MISSING")
                source_text=record["text"]
                if len(source_text)>=20:text_count+=1
                candidates=record.get("headings",[])
                display=next((str(x).strip() for x in candidates
                              if isinstance(x,str) and x.strip()),"")
                title=f"RAVE français MINI — page {page}"
                if display:title+=" — "+display[:100]
                source_json=json.dumps({
                    "source_pdf_sha256":PRINTED_PDF_SHA256,
                    "original_page":page,
                    "page_text_sha256":record["page_text_sha256"],
                    "extraction":"ocr_fra",
                    "source_document_id":SOURCE_ID,
                    "review":"pending",
                    "operation_numbers_seen":record.get("operation_numbers",[]),
                    "original_page_images_remain_authoritative":True
                },sort_keys=True,ensure_ascii=False)
                db.execute("""
                    INSERT INTO documentary_entries
                    (document_id,source_revision,entry_id,entry_type,source_page,
                     title,source_text,verification_status,source_table,raw_json)
                    VALUES(?,?,?,'reference',?,?,?,'review_required',?,?)""",
                    (SOURCE_ID,revision,f"{RECORD_PREFIX}{page:05d}",page,
                     title,source_text,"original_rave_fr_ocr_scan",source_json))
            db.execute("""
                INSERT INTO documentary_enrichment_history
                  (event_id,document_id,source_revision,uploaded_pdf_sha256,
                   ocr_archive_sha256,pages_processed,ocr_pages_with_text,
                   entries_added,previous_snapshot_sha256,verification_status)
                VALUES(?,?,?,?,?,?,?,?,?,'review_required')""",
                ("rave-fr-ocr-"+PRINTED_PDF_SHA256[:24],SOURCE_ID,revision,
                 PRINTED_PDF_SHA256,ocr_hash,len(records),text_count,len(records),
                 baseline_sha))
            # Source inventory is an honest publication state: OCR is searchable
            # but cannot become a workshop instruction without human review.
            db.execute("UPDATE documentary_database "
                       "SET publication_status='source_inventory',"
                       "collection_revision=collection_revision+1 WHERE singleton=1")
            if old_fingerprint!=core_records(db):
                raise ValueError("FRENCH_EXISTING_DOCUMENTARY_SOURCE_CHANGED")
            db.commit()
            result=validate_source_snapshot(db,"fr")
            if result["indexed_entries"]!=old_count+len(records):
                raise ValueError("FRENCH_ADDED_ENTRIES_NOT_EXACTLY_371")
            verify_images(db,root)
            history=documentary_import_report(db,"fr")
            if history["document_count"]!=result["document_count"]:
                raise ValueError("FRENCH_DOCUMENTARY_IMPORT_HISTORY_CHANGED")
        archive_partial=output.with_name(output.name+".part")
        try:
            with zipfile.ZipFile(archive_partial,"w",zipfile.ZIP_DEFLATED,
                                 compresslevel=7) as z:
                z.write(path,path.name)
                for image in sorted((root/"images").glob("*")):
                    if image.is_file():z.write(image,"images/"+image.name)
            with zipfile.ZipFile(archive_partial) as z:
                if z.testzip() is not None:
                    raise ValueError("FRENCH_OCR_ARCHIVE_CORRUPT")
            archive_partial.replace(output)
        finally:
            archive_partial.unlink(missing_ok=True)
    audit=dict(result,original_source_document_id=SOURCE_ID,
               original_source_revision=revision,
               input_sha256=baseline_sha,
               source_pdf_sha256=PRINTED_PDF_SHA256,
               ocr_gzip_sha256=ocr_hash,
               pages_ocr_added=len(records),
               pages_with_ocr_text=text_count,
               existing_records_preserved=True,
               documents_added=0,
               source_ocr_review_status="review_required",
               original_french_manuals_kept=result["document_count"],
               output_sha256=sha256(output))
    output.with_suffix(".audit.json").write_text(
        json.dumps(audit,ensure_ascii=False,indent=2,sort_keys=True)+"\n",
        encoding="utf-8")
    return audit


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--snapshot",type=Path,required=True)
    p.add_argument("--ocr-gzip",type=Path,required=True)
    p.add_argument("--output",type=Path,required=True)
    a=p.parse_args()
    result=apply_to_snapshot(a.snapshot,a.ocr_gzip,a.output)
    print("EXISTING_FRENCH_RAVE_ENRICHED_WITH_OCR",json.dumps(result,ensure_ascii=False))


if __name__=="__main__":
    main()
