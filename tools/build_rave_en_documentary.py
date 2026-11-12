#!/usr/bin/env python3
"""Build the already-complete *English* AKM7169ENG SQLite from its verified 29 parts.

This is a DOCUMENTARY-ONLY database, not an ECU/user/expert database.
No machine translation is performed.  Unlike the original installer builder,
the PDF itself is not required: its recorded SHA in the source SQLite is checked,
but no PDF is distributed without a separate rights review.
"""
from __future__ import annotations

import argparse
import gzip
import hashlib
import json
import shutil
import sqlite3
import tempfile
from pathlib import Path

from build_rover_session_documentary import (
    PART_COUNT, DATABASE_BYTES, DATABASE_SHA, GZIP_SHA, DOCUMENT_SHA,
    check_database, digest,
)
from rover_multilingual import create_schema, check_localized_schema


def require(ok: bool, message: str) -> None:
    if not ok:
        raise RuntimeError(message)


def build(parts_dir: Path, output_dir: Path) -> None:
    parts = sorted(parts_dir.glob("part-*.gz.part"))
    require(len(parts) == PART_COUNT and [p.name for p in parts] ==
            [f"part-{n:03d}.gz.part" for n in range(PART_COUNT)],
            "RAVE_EN_29_SOURCE_PARTS_MISSING")
    output_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp:
        compressed = Path(tmp) / "verified-rover.gz"
        with compressed.open("wb") as stream:
            for part in parts:
                with part.open("rb") as source:
                    shutil.copyfileobj(source, stream)
        require(digest(compressed) == GZIP_SHA, "RAVE_EN_COMPRESSED_SHA_INVALID")
        source_db = Path(tmp) / "rover_original.sqlite"
        with gzip.open(compressed, "rb") as src, source_db.open("wb") as dst:
            shutil.copyfileobj(src, dst)
        require(source_db.stat().st_size == DATABASE_BYTES
                and digest(source_db) == DATABASE_SHA,
                "RAVE_EN_SOURCE_SQLITE_SHA_INVALID")
        checked = check_database(source_db)
        # Keep the original source untouched, work on a new separate language DB.
        target = output_dir / "mems_documentary_en.sqlite"
        shutil.copy2(source_db, target)
        with sqlite3.connect(target) as con:
            create_schema(con, DATABASE_SHA)
            metadata = check_localized_schema(con, DATABASE_SHA)
            require(con.execute("PRAGMA integrity_check").fetchone()[0] == "ok",
                    "RAVE_EN_SQLITE_INTEGRITY_FAILED")
            require(not con.execute("PRAGMA foreign_key_check").fetchall(),
                    "RAVE_EN_FOREIGN_KEY_FAILURE")
            require(con.execute("SELECT COUNT(*) FROM documentary_translations")
                    .fetchone()[0] == 0, "RAVE_EN_LANGUAGE_MIX")
            require(con.execute(
                "SELECT original_pdf_sha256,total_pages FROM source_documents"
            ).fetchone() == (DOCUMENT_SHA, 482), "RAVE_EN_PDF_PROVENANCE_INVALID")
        report = {
            "language": "en",
            "document_id": "AKM7169ENG",
            "original_pages": 482,
            "original_pdf_sha256": DOCUMENT_SHA,
            "source_verified_sqlite_sha256": DATABASE_SHA,
            "distribution_sqlite_sha256": digest(target),
            "sqlite_bytes": target.stat().st_size,
            "counts": checked,
            "schema": "MEMS original Rover documentary repository",
            "documentary_language_meta": metadata,
            "english_native_source": True,
            "translated_into_french": False,
            "pdf_included": False,
            "source_facts_individually_approved": False,
            "installable_via_existing_password_button": False,
            "personal_or_ecu_database_included": False,
            "status": "private_checked_documentary_only",
        }
        (output_dir / "mems_documentary_en_report.json").write_text(
            json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8"
        )
        print("RAVE_EN_SQLITE_COMPLETE_AND_INTEGRITY_PASS",
              json.dumps(report, ensure_ascii=False))


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--parts", type=Path, default=Path("database/documentary/parts"))
    p.add_argument("--output", type=Path, default=Path("output/rave-en"))
    args = p.parse_args()
    build(args.parts, args.output)
