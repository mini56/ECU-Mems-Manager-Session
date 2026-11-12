#!/usr/bin/env python3
"""Regression checks for the unique, source-preserving Rover language schema."""
from __future__ import annotations
import json
import sqlite3
import sys
import tempfile
from contextlib import closing
from pathlib import Path

from rover_multilingual import (
    check_localized_schema, create_schema, import_pack, make_export, sha,
)

SOURCE = "a" * 64

def ensure(test: bool, name: str) -> None:
    if not test:
        raise AssertionError(name)

def run() -> None:
    with tempfile.TemporaryDirectory() as directory:
        db = Path(directory) / "rover_french_test.sqlite"
        with closing(sqlite3.connect(db)) as con:
            con.executescript("""
                CREATE TABLE procedures(procedure_id TEXT PRIMARY KEY,title TEXT);
                CREATE TABLE procedure_steps(
                    procedure_id TEXT,phase TEXT,step_number INTEGER,source_text TEXT,
                    source_page INTEGER);
                CREATE TABLE source_page_discovery(
                    physical_page INTEGER PRIMARY KEY,ocr_text TEXT);
                CREATE TABLE extracted_procedure_candidates(
                    candidate_id TEXT PRIMARY KEY,candidate_title TEXT);
                CREATE TABLE extracted_procedure_candidate_steps(
                    candidate_id TEXT,ocr_instruction_candidate TEXT);
                INSERT INTO procedures VALUES
                    ('rover-akm7169eng-12.65.12','Cylinder head gasket removal'),
                    ('rover-akm7169eng-70.10.03','Rear brake drum');
                INSERT INTO procedure_steps VALUES
                    ('rover-akm7169eng-12.65.12','remove',1,
                     'Tighten cylinder head bolts to 88 Nm.',93);
                INSERT INTO source_page_discovery VALUES
                    (93,'Caution: verify the original service manual.');
                INSERT INTO extracted_procedure_candidates VALUES
                    ('rover:ocr:93:test','Unverified adjustment');
                INSERT INTO extracted_procedure_candidate_steps VALUES
                    ('rover:ocr:93:test','Check the operation carefully.');
            """)
            con.commit()
            before = list(con.execute("SELECT procedure_id,title FROM procedures ORDER BY procedure_id"))
            lines_before = list(con.execute("SELECT source_text FROM procedure_steps"))
            status = create_schema(con, SOURCE)
            ensure(status["source_units"] == 6, "EVERY_SOURCE_KIND_HAS_UNITS")
            draft = make_export(con, SOURCE, "fr")
            ensure(draft["language"] == "fr", "FRENCH_PRIORITIZED")

            lookup = {entry["unit_id"]: entry for entry in draft["entries"]}
            expected = {
                "procedure.title:rover-akm7169eng-12.65.12":
                    "Dépose du joint de culasse",
                "procedure.step:1":
                    "Serrer les boulons de culasse à 88 Nm.",
            }
            for key, translated in expected.items():
                entry = lookup[key]
                entry["translated_text"] = translated
                entry["review_status"] = "machine_draft"
            pack = {**draft, "entries": [lookup[uid] for uid in expected]}
            ensure(import_pack(con, pack) == 2, "TWO_SOURCE_BOUND_FRENCH_TRANSLATIONS")
            ensure(import_pack(con, pack) == 2, "IDENTICAL_REIMPORT_IS_IDEMPOTENT")
            ensure(con.execute(
                "SELECT COUNT(*) FROM documentary_translations").fetchone()[0] == 2,
                "NO_DUPLICATE_FR_ROWS")
            ensure(con.execute("SELECT title FROM procedures WHERE procedure_id=?",
                               ("rover-akm7169eng-12.65.12",)).fetchone()[0] ==
                   "Cylinder head gasket removal", "MANUFACTURER_TITLE_IMMUTABLE")
            ensure(list(con.execute("SELECT procedure_id,title FROM procedures ORDER BY procedure_id")) ==
                   before, "SOURCE_PROCEDURES_UNMODIFIED")
            ensure(list(con.execute("SELECT source_text FROM procedure_steps")) ==
                   lines_before, "SOURCE_STEPS_UNMODIFIED")
            fr_row = con.execute(
                "SELECT translated_text,review_status FROM documentary_translations "
                "WHERE unit_id=? AND language='fr'",
                ("procedure.title:rover-akm7169eng-12.65.12",)).fetchone()
            ensure(fr_row == ("Dépose du joint de culasse", "machine_draft"),
                   "FRENCH_TEXT_AND_UNVERIFIED_STATUS")
            ensure(check_localized_schema(con, SOURCE)["coverage"] == {"fr": 2},
                   "COVERAGE_IS_MEASURED")

            invalid = json.loads(json.dumps(pack))
            invalid["entries"][0]["translated_text"] = "Serrer à 89 Nm."
            try:
                import_pack(con, invalid)
            except ValueError:
                pass
            else:
                raise AssertionError("CHANGED_TORQUE_NOT_REJECTED")
            ensure(con.execute(
                "SELECT COUNT(*) FROM documentary_translations").fetchone()[0] == 2,
                "FAILED_TRANSLATION_CANNOT_PARTIALLY_IMPORT")

            wrong_corpus = json.loads(json.dumps(pack))
            wrong_corpus["original_sqlite_sha256"] = "b" * 64
            try:
                import_pack(con, wrong_corpus)
            except ValueError:
                pass
            else:
                raise AssertionError("FOREIGN_OR_REBUILT_ROVER_SOURCE_WAS_ACCEPTED")

            unknown = json.loads(json.dumps(pack))
            unknown["entries"][0]["unit_id"] = "procedure.title:invented"
            try:
                import_pack(con, unknown)
            except ValueError:
                pass
            else:
                raise AssertionError("TRANSLATION_WITHOUT_SOURCE_WAS_ACCEPTED")

            unreviewed = json.loads(json.dumps(pack))
            unreviewed["entries"][0]["review_status"] = "reviewed"
            try:
                import_pack(con, unreviewed)
            except ValueError:
                pass
            else:
                raise AssertionError("UNREVIEWED_TECHNICAL_CONTENT_MISLABELED")
            ensure(check_localized_schema(con, SOURCE)["coverage"] == {"fr": 2},
                   "AFTER_REJECTION_DB_REMAINS_HEALTHY")
        print("PASS ROVER_FR_ONE_SQLITE_IMMUTABLE_SOURCE_TRANSLATION_CONTRACT")

if __name__ == "__main__":
    run()
