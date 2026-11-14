#!/usr/bin/env python3
"""Rebuild an existing *language documentary ZIP* in the ONE SESSION contract.

No OCR, no translation and no change to existing manual text, page references
or images. Adds immutable original-source inventory and uniform procedure links.
The source ZIP stays untouched; output is an offline candidate, never published.
"""
from __future__ import annotations

import argparse
from contextlib import closing
import hashlib
import json
from pathlib import Path
import shutil
import sqlite3
import tempfile
import zipfile

from add_documentary_pdf import copy_snapshot, sha256, verify_images
from documentary_common import (
    LANGUAGES, documentary_import_report, ensure_import_history,
    ensure_structured_operations, validate_source_snapshot,
)


CORE_TABLES=("documentary_database","documentary_documents","documentary_pages",
             "documentary_entries","documentary_images")


def original_rows_fingerprint(db: sqlite3.Connection) -> dict[str,str]:
    result={}
    for table in CORE_TABLES:
        cols=[row[1] for row in db.execute(f"PRAGMA table_info({table})")]
        if not cols:
            raise ValueError("DOCUMENTARY_CORE_TABLE_MISSING_"+table)
        # Values are hashed, not printed; all original documentation survives.
        rows=db.execute(f'SELECT * FROM "{table}" ORDER BY rowid')
        digest=hashlib.sha256()
        for row in rows:
            digest.update(json.dumps(row,ensure_ascii=False,
                           separators=(",",":"),default=str).encode("utf-8"))
            digest.update(b"\n")
        result[table]=digest.hexdigest()
    return result


def normalize_snapshot(source: Path, language: str, output: Path) -> dict:
    if language not in LANGUAGES:
        raise ValueError("DOCUMENTARY_LANGUAGE_NOT_SUPPORTED")
    if not source.is_file() or output.resolve()==source.resolve():
        raise ValueError("SOURCE_SNAPSHOT_CANNOT_BE_OVERWRITTEN")
    output.parent.mkdir(parents=True,exist_ok=True)
    previous=sha256(source)
    with tempfile.TemporaryDirectory(prefix="mems-doc-normalize-") as folder:
        root=Path(folder)
        copy_snapshot(source,root,language)
        database=root/f"mems_documentary_{language}.sqlite"
        with closing(sqlite3.connect(database)) as db:
            db.execute("PRAGMA foreign_keys=ON")
            original=original_rows_fingerprint(db)
            # All historical sources get an explicitly undated baseline;
            # the original import date cannot be recovered from an old ZIP.
            operation_report=ensure_structured_operations(db)
            historical=ensure_import_history(db,
                legacy_snapshot_sha256=previous,
                existing_kind="existing_snapshot", imported_utc=None)
            db.commit()
            unchanged=original_rows_fingerprint(db)
            # Collection revision may advance; absolutely nothing else in the
            # core source rows may be changed, deleted or re-indexed.
            if original!=unchanged:
                raise ValueError("ORIGINAL_MANUFACTURER_ROWS_CHANGED")
            db.execute(
                "UPDATE documentary_database "
                "SET collection_revision=collection_revision+1 WHERE singleton=1")
            db.commit()
            summary=validate_source_snapshot(db,language)
            history=documentary_import_report(db,language)
            verify_images(db,root)
        images=sorted((root/"images").rglob("*"))
        partial=output.with_name(output.name+".part")
        try:
            with zipfile.ZipFile(partial,"w",zipfile.ZIP_DEFLATED,
                                 compresslevel=7) as archive:
                archive.write(database,database.name)
                for path in images:
                    if path.is_file():
                        archive.write(path,path.relative_to(root).as_posix())
            with zipfile.ZipFile(partial) as archive:
                if archive.testzip():
                    raise ValueError("REBUILT_DOCUMENTARY_ZIP_CORRUPT")
            partial.replace(output)
        finally:
            partial.unlink(missing_ok=True)
    report=dict(summary,source_zip_sha256=previous,
                normalized_zip_sha256=sha256(output),
                core_source_rows_sha256=unchanged,
                added_history_records=historical,
                structured_operation_contract=operation_report,
                original_document_rows_preserved=True,
                candidate_only=True,
                **history)
    output.with_suffix(output.suffix+".audit.json").write_text(
        json.dumps(report,ensure_ascii=False,indent=2,sort_keys=True)+"\n",
        encoding="utf-8")
    return report


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-zip",type=Path,required=True)
    parser.add_argument("--language",choices=sorted(LANGUAGES),required=True)
    parser.add_argument("--output",type=Path,required=True)
    args=parser.parse_args()
    report=normalize_snapshot(args.source_zip,args.language,args.output)
    print("DOCUMENTARY_SNAPSHOT_NORMALIZED",json.dumps({
        key:report[key] for key in (
          "language","document_count","source_pages",
          "indexed_entries","facsimiles","added_history_records",
          "source_zip_sha256","normalized_zip_sha256",
          "original_document_rows_preserved"
        )},sort_keys=True))


if __name__=="__main__":
    main()
