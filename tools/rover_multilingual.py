#!/usr/bin/env python3
"""One Rover SQLite, immutable English manufacturer text, versioned translations.

Never translates text, rewrites manufacturer tables, or claims that machine
translations have technical approval.  Translation packs are independent
artifacts checked against the exact source and imported atomically.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sqlite3
from pathlib import Path

SCHEMA_VERSION = 1
SUPPORTED_LANGUAGES = ("fr", "en", "pt", "es", "it", "de")
NUMBER = re.compile(r"\d+(?:[.,]\d+)*")
UNITS = re.compile(r"\b(?:Nm|mm|kPa|bar|mV|rpm|psi|V|A|°C|°F)\b", re.I)

def sha(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()

def require(ok: bool, message: str) -> None:
    if not ok:
        raise ValueError(message)

def units_from_source(con: sqlite3.Connection):
    # Original rowids are immutable inside the SHA-verified English corpus.
    # Keys do not depend on a translated word or the user's search query.
    for ref, title in con.execute("SELECT procedure_id,title FROM procedures ORDER BY procedure_id"):
        if title and title.strip():
            yield ("procedure.title:" + ref, "procedure_title", ref, title)
    for rowid, ref, body in con.execute(
        "SELECT rowid,procedure_id,source_text FROM procedure_steps ORDER BY rowid"
    ):
        if body and body.strip():
            yield ("procedure.step:" + str(rowid), "procedure_step", ref, body)
    for page, body in con.execute(
        "SELECT physical_page,ocr_text FROM source_page_discovery ORDER BY physical_page"
    ):
        if body and body.strip():
            yield ("ocr.page:" + str(page), "page_ocr", str(page), body)
    candidates = {
        row[0] for row in con.execute("SELECT name FROM sqlite_master WHERE type='table'")
    }
    if "extracted_procedure_candidates" in candidates:
        for ref, title in con.execute(
            "SELECT candidate_id,candidate_title FROM extracted_procedure_candidates "
            "ORDER BY candidate_id"
        ):
            if title and title.strip():
                yield ("candidate.title:" + ref, "candidate_title", ref, title)
    if "extracted_procedure_candidate_steps" in candidates:
        for rowid, ref, body in con.execute(
            "SELECT rowid,candidate_id,ocr_instruction_candidate "
            "FROM extracted_procedure_candidate_steps ORDER BY rowid"
        ):
            if body and body.strip():
                yield ("candidate.step:" + str(rowid), "candidate_step", ref, body)

def create_schema(con: sqlite3.Connection, original_sha: str) -> dict:
    require(bool(re.fullmatch(r"[a-f0-9]{64}", original_sha)),
            "INVALID_SOURCE_DATABASE_SHA256")
    con.execute("PRAGMA foreign_keys=ON")
    # Execute schema and units in a single transaction; no partial DB on failure.
    con.execute("BEGIN IMMEDIATE")
    try:
        con.execute("""CREATE TABLE documentary_language_meta(
            key TEXT PRIMARY KEY NOT NULL,
            value TEXT NOT NULL
        ) WITHOUT ROWID""")
        con.execute("""CREATE TABLE documentary_text_units(
            unit_id TEXT PRIMARY KEY NOT NULL,
            unit_kind TEXT NOT NULL,
            owner_id TEXT NOT NULL,
            source_text TEXT NOT NULL,
            source_sha256 TEXT NOT NULL CHECK(length(source_sha256)=64)
        ) WITHOUT ROWID""")
        con.execute("""CREATE TABLE documentary_translations(
            unit_id TEXT NOT NULL,
            language TEXT NOT NULL CHECK(language IN ('fr','en','pt','es','it','de')),
            translated_text TEXT NOT NULL CHECK(length(trim(translated_text))>0),
            source_sha256 TEXT NOT NULL,
            translated_sha256 TEXT NOT NULL,
            review_status TEXT NOT NULL CHECK(review_status IN ('machine_draft','reviewed')),
            reviewer TEXT NOT NULL DEFAULT '',
            PRIMARY KEY(unit_id,language),
            FOREIGN KEY(unit_id) REFERENCES documentary_text_units(unit_id)
                ON DELETE RESTRICT
        ) WITHOUT ROWID""")
        con.execute("CREATE INDEX documentary_translation_language ON documentary_translations(language)")
        con.executemany(
            "INSERT INTO documentary_language_meta(key,value) VALUES(?,?)",
            (("schema_version", str(SCHEMA_VERSION)),
             ("original_sqlite_sha256", original_sha),
             ("original_language", "en"),
             ("translation_priority", "fr")),
        )
        count = 0
        kinds: dict[str, int] = {}
        for unit_id, kind, owner, original in units_from_source(con):
            con.execute(
                "INSERT INTO documentary_text_units(unit_id,unit_kind,owner_id,"
                "source_text,source_sha256) VALUES(?,?,?,?,?)",
                (unit_id, kind, owner, original, sha(original)),
            )
            count += 1
            kinds[kind] = kinds.get(kind, 0) + 1
        con.execute("COMMIT")
    except Exception:
        con.execute("ROLLBACK")
        raise
    return {"source_units": count, "kinds": kinds, "translated_units": 0,
            "language_priority": "fr"}

def validate_translation(original: str, translated: str) -> None:
    require(isinstance(translated, str) and bool(translated.strip()),
            "EMPTY_TRANSLATION")
    require(len(translated) <= max(250, len(original) * 8),
            "TRANSLATION_LENGTH_SUSPICIOUS")
    require(NUMBER.findall(original) == NUMBER.findall(translated),
            "NUMERIC_REFERENCE_CHANGED")
    source_units = [x.lower() for x in UNITS.findall(original)]
    translated_units = [x.lower() for x in UNITS.findall(translated)]
    require(source_units == translated_units, "TECHNICAL_UNITS_CHANGED")
    require("<|im_" not in translated and "<think>" not in translated,
            "MODEL_INTERNAL_TOKENS_IN_TRANSLATION")

def import_pack(con: sqlite3.Connection, pack: dict) -> int:
    require(isinstance(pack, dict), "INVALID_TRANSLATION_PACK")
    require(pack.get("schema_version") == SCHEMA_VERSION, "TRANSLATION_SCHEMA_VERSION")
    language = pack.get("language")
    require(language in SUPPORTED_LANGUAGES and language != "en",
            "UNSUPPORTED_TRANSLATION_LANGUAGE")
    source_sha = con.execute(
        "SELECT value FROM documentary_language_meta WHERE key='original_sqlite_sha256'"
    ).fetchone()
    require(source_sha and pack.get("original_sqlite_sha256") == source_sha[0],
            "TRANSLATION_PACK_WRONG_ROVER_SOURCE")
    entries = pack.get("entries")
    require(isinstance(entries, list), "TRANSLATIONS_NOT_AN_ARRAY")
    # Reject duplicate keys rather than silently keeping the last version.
    seen = set()
    pending = []
    for entry in entries:
        require(isinstance(entry, dict), "INVALID_TRANSLATION_ENTRY")
        unit_id = entry.get("unit_id")
        require(isinstance(unit_id, str) and unit_id and unit_id not in seen,
                "DUPLICATE_OR_INVALID_TRANSLATION_UNIT")
        seen.add(unit_id)
        source = con.execute(
            "SELECT source_text,source_sha256 FROM documentary_text_units "
            "WHERE unit_id=?", (unit_id,)
        ).fetchone()
        require(source is not None, "TRANSLATION_REFERENCES_UNKNOWN_SOURCE")
        require(source[1] == entry.get("source_sha256"),
                "TRANSLATION_SOURCE_CHANGED")
        translated = entry.get("translated_text")
        validate_translation(source[0], translated)
        status = entry.get("review_status", "machine_draft")
        require(status in ("machine_draft", "reviewed"), "INVALID_REVIEW_STATUS")
        reviewer = entry.get("reviewer", "")
        require(isinstance(reviewer, str), "INVALID_REVIEWER")
        require(status != "reviewed" or reviewer.strip(),
                "REVIEWED_TRANSLATION_REQUIRES_REVIEWER")
        pending.append((unit_id, language, translated, source[1], sha(translated),
                        status, reviewer.strip()))
    con.execute("PRAGMA foreign_keys=ON")
    con.execute("BEGIN IMMEDIATE")
    try:
        for row in pending:
            existing = con.execute(
                "SELECT translated_sha256,review_status FROM documentary_translations "
                "WHERE unit_id=? AND language=?", (row[0], row[1])
            ).fetchone()
            require(existing is None or existing[0] == row[4],
                    "CONFLICTING_TRANSLATION_REQUIRES_NEW_REVIEWED_PACK")
            # Reproducible: an identical re-import is idempotent.
            con.execute(
                "INSERT OR IGNORE INTO documentary_translations("
                "unit_id,language,translated_text,source_sha256,translated_sha256,"
                "review_status,reviewer) VALUES(?,?,?,?,?,?,?)", row
            )
        con.execute("COMMIT")
    except Exception:
        con.execute("ROLLBACK")
        raise
    return len(pending)

def check_localized_schema(con: sqlite3.Connection, original_sha: str) -> dict:
    con.execute("PRAGMA foreign_keys=ON")
    require(con.execute("PRAGMA integrity_check").fetchone()[0] == "ok",
            "LOCALIZED_SQLITE_INTEGRITY_FAILED")
    require(not con.execute("PRAGMA foreign_key_check").fetchall(),
            "LOCALIZED_SQLITE_FK_FAILED")
    meta = dict(con.execute("SELECT key,value FROM documentary_language_meta"))
    require(meta.get("schema_version") == str(SCHEMA_VERSION) and
            meta.get("original_sqlite_sha256") == original_sha,
            "LOCALIZATION_SOURCE_IDENTITY_FAILED")
    source = list(units_from_source(con))
    total = con.execute("SELECT COUNT(*) FROM documentary_text_units").fetchone()[0]
    require(total == len(source), "MISSING_OR_EXTRA_ORIGINAL_TEXT_UNITS")
    for uid, kind, owner, original in source:
        x = con.execute("SELECT unit_kind,owner_id,source_text,source_sha256 "
                        "FROM documentary_text_units WHERE unit_id=?", (uid,)).fetchone()
        require(x == (kind, owner, original, sha(original)), "SOURCE_TEXT_UNIT_MISMATCH")
    coverage = dict(con.execute(
        "SELECT language,COUNT(*) FROM documentary_translations GROUP BY language"
    ))
    for uid, language, translation, src_hash, translated_hash, status in con.execute(
        "SELECT unit_id,language,translated_text,source_sha256,translated_sha256,"
        "review_status FROM documentary_translations"
    ):
        original = con.execute(
            "SELECT source_text,source_sha256 FROM documentary_text_units WHERE unit_id=?",
            (uid,)
        ).fetchone()
        require(original is not None and original[1] == src_hash and
                sha(translation) == translated_hash, "CORRUPT_TRANSLATION_PROVENANCE")
        validate_translation(original[0], translation)
    reviewed_french = con.execute(
        "SELECT COUNT(*) FROM documentary_translations t "
        "JOIN documentary_text_units u ON u.unit_id=t.unit_id "
        "WHERE t.language='fr' AND t.review_status='reviewed' "
        "AND length(trim(t.reviewer))>0 "
        "AND t.source_sha256=u.source_sha256"
    ).fetchone()[0]
    translated_french = coverage.get("fr", 0)
    french = {
        "required": total,
        "translated": translated_french,
        "reviewed": reviewed_french,
        "missing": total - translated_french,
        "not_reviewed": translated_french - reviewed_french,
        "ready_for_user_testing": bool(total) and reviewed_french == total,
    }
    return {"source_units": total, "coverage": coverage,
            "french": french, "schema_version": SCHEMA_VERSION}


def require_complete_french(status: dict) -> None:
    """One quality gate, never a partial French/English display release."""
    french = status["french"]
    require(french["ready_for_user_testing"],
            "FRENCH_CORPUS_INCOMPLETE: "
            f"{french['translated']}/{french['required']} translated, "
            f"{french['reviewed']}/{french['required']} translation-reviewed. "
            "NO_USER_SETUP_OR_TRIAL_PERMITTED")

def make_export(con: sqlite3.Connection, source_sha: str, language: str) -> dict:
    require(language in SUPPORTED_LANGUAGES and language != "en", "BAD_EXPORT_LANGUAGE")
    entries = [
        dict(unit_id=uid, unit_kind=kind, source_sha256=original_hash,
             source_text=original, translated_text="", review_status="machine_draft")
        for uid, kind, original, original_hash in con.execute(
            "SELECT unit_id,unit_kind,source_text,source_sha256 "
            "FROM documentary_text_units ORDER BY unit_id"
        )
    ]
    return {"schema_version": SCHEMA_VERSION, "original_sqlite_sha256": source_sha,
            "language": language, "entries": entries}

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--database", type=Path, required=True)
    parser.add_argument("--source-sha256", required=True)
    parser.add_argument("--translation-pack", type=Path, action="append", default=[])
    parser.add_argument("--export-source", type=Path)
    parser.add_argument("--language", default="fr")
    parser.add_argument("--verify-only", action="store_true")
    parser.add_argument("--require-complete-french", action="store_true",
                        help="Reject any French release or user trial unless all"
                             " original text units have reviewed French entries")
    args = parser.parse_args()
    con = sqlite3.connect(args.database)
    try:
        if not args.verify_only:
            create_schema(con, args.source_sha256)
            for path in args.translation_pack:
                print("IMPORTED_TRANSLATIONS", path,
                      import_pack(con, json.loads(path.read_text(encoding="utf-8"))))
        status = check_localized_schema(con, args.source_sha256)
        if args.require_complete_french:
            require_complete_french(status)
        if args.export_source:
            args.export_source.parent.mkdir(parents=True, exist_ok=True)
            export = make_export(con, args.source_sha256, args.language)
            args.export_source.write_text(json.dumps(export, indent=2, ensure_ascii=False),
                                          encoding="utf-8")
        print("ROVER_MULTILINGUAL_SQLITE_SCHEMA_OK",
              json.dumps(status, sort_keys=True))
    finally:
        con.close()

if __name__ == "__main__":
    main()
