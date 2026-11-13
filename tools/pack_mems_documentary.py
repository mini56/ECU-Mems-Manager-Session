#!/usr/bin/env python3
"""Create a password-protected, authenticated MEMSDOC1 documentary ZIP asset.

Private-pack construction ONLY: publication of third-party manufacturer work
requires a separate confirmation of redistribution rights. No password is
written to a manifest, GitHub Actions log, asset name or result JSON.

Format:
  8 bytes magic "MEMSDOC1"
  1 byte version 1
  16 random salt
  12 random nonce
  8 little-endian original ZIP size
  AES-256-GCM(ciphertext, authenticated header as AAD) + 16-byte tag
Key: PBKDF2-HMAC-SHA256(password UTF-8, salt, 210000 iterations, 32 bytes).
"""
from __future__ import annotations

import argparse
from contextlib import closing
import hashlib
import json
import os
import re
import sqlite3
import struct
import tempfile
import zipfile
from pathlib import Path

from cryptography.hazmat.primitives.ciphers.aead import AESGCM

MAX_ZIP = 256 * 1024 * 1024
MAX_UNPACKED = 700 * 1024 * 1024
MAGIC = b"MEMSDOC1"
ITERATIONS = 210000


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def inspect_zip(path: Path, language: str) -> dict:
    require(path.is_file() and path.stat().st_size < MAX_ZIP, "ZIP_MISSING_OR_TOO_LARGE")
    exact = f"mems_documentary_{language}.sqlite"
    with zipfile.ZipFile(path) as zip_:
        entries = zip_.infolist()
        files = [e for e in entries if not e.is_dir()]
        names = [e.filename for e in files]
        require(len(names) == len(set(names)), "DUPLICATE_ZIP_PATH")
        require(len(files) < 1000, "ZIP_TOO_MANY_FILES")
        require(exact in names and names.count(exact) == 1, "ZIP_SQLITE_LANGUAGE_MISMATCH")
        require(not any(n.startswith("/") or "\\" in n or
                        n in (".", "..") or
                        any(p in (".", "..", "") for p in n.split("/"))
                        for n in names), "ZIP_PATH_TRAVERSAL")
        require(not any((e.external_attr >> 16) & 0o170000 == 0o120000
                        for e in files), "ZIP_SYMLINK_REJECTED")
        require(sum(e.file_size for e in files) < MAX_UNPACKED,
                "ZIP_UNPACKED_TOO_LARGE")
        require(zip_.testzip() is None, "ZIP_CRC_ERROR")
        with tempfile.TemporaryDirectory() as folder:
            source_db = Path(folder) / exact
            source_db.write_bytes(zip_.read(exact))
            # sqlite3.Connection.__exit__ alone does NOT close the handle:
            # Windows refuses to delete the temporary SQLite otherwise.
            with closing(sqlite3.connect(f"file:{source_db}?mode=ro", uri=True)) as db:
                require(db.execute("PRAGMA integrity_check").fetchone()[0] == "ok",
                        "SQLITE_INTEGRITY_ERROR")
                require(not db.execute("PRAGMA foreign_key_check").fetchall(),
                        "SQLITE_FOREIGN_KEYS_ERROR")
                if language == "fr":
                    docs = db.execute(
                        "SELECT document_id,source_pages,source_sha256 "
                        "FROM documentary_documents ORDER BY document_id"
                    ).fetchall()
                    RCL=("RCL0193FRE",371,
                         "0c7fef28d0d0f0673ba321d6625a019c005823103caa98afb3258114e1fec713")
                    SPI=("AKM7169FRE",20,
                         "37cd0434347fe4009963d075e4b91b0b21ebc905bae5657519a8080c3026d4f5")
                    expected_single=[RCL]
                    expected_multi=[SPI,RCL]
                    require(docs in (expected_single,expected_multi),
                            "FRENCH_MANUFACTURER_PDF_IDENTITIES_INVALID")
                    multi=docs==expected_multi
                    expected_entries=6387 if multi else 5958
                    expected_images=345 if multi else 325
                    require(db.execute("SELECT count(*) FROM documentary_entries")
                            .fetchone()[0] == expected_entries,
                            "FRENCH_ENTRIES_INCOMPLETE")
                    require(db.execute("SELECT count(*) FROM documentary_fts")
                            .fetchone()[0] == expected_entries,
                            "FRENCH_FTS_INDEX_INCOMPLETE")
                    require(db.execute(
                        "SELECT count(*) FROM documentary_entries WHERE verification_status"
                        " != 'source_checked'"
                    ).fetchone()[0] == 0, "FRENCH_UNREVIEWED_ROWS_REJECTED")
                    require(db.execute("SELECT COUNT(*) FROM documentary_page_visual_status"
                                       " WHERE document_id='RCL0193FRE' AND is_blank=0")
                            .fetchone()[0] == 325,"RCL_IMAGE_VISUAL_PROVENANCE_INCOMPLETE")
                    require(db.execute("SELECT COUNT(*) FROM documentary_pages")
                            .fetchone()[0] == (391 if multi else 371),
                            "FRENCH_PAGE_COVERAGE_INVALID")
                    images = db.execute(
                        "SELECT relative_path,asset_sha256,asset_bytes "
                        "FROM documentary_images"
                    ).fetchall()
                    require(len(images) == expected_images,
                            "FRENCH_IMAGE_COVERAGE_INVALID")
                    for relative, digest, size in images:
                        require(relative in names and size == zip_.getinfo(relative).file_size,
                                "MANUFACTURER_IMAGE_MISSING")
                        require(hashlib.sha256(zip_.read(relative)).hexdigest() == digest,
                                "MANUFACTURER_IMAGE_SHA256_MISMATCH")
                    if multi:
                        require(db.execute(
                            "SELECT count(*) FROM documentary_entries WHERE "
                            "document_id='AKM7169FRE' AND entry_type='step'"
                        ).fetchone()[0] == 267, "SPI_REPAIR_STEPS_INCOMPLETE")
                        require(db.execute(
                            "SELECT count(*) FROM documentary_entries WHERE "
                            "document_id='AKM7169FRE' AND entry_type='technical_value'"
                        ).fetchone()[0] == 22,"SPI_TECHNICAL_VALUES_INCOMPLETE")
                        require(db.execute(
                            "SELECT count(*) FROM documentary_entries WHERE "
                            "document_id='AKM7169FRE' AND source_table="
                            "'akm7169fre_description_relie'"
                        ).fetchone()[0] == 19,"SPI_DESCRIPTIVE_SECTIONS_INCOMPLETE")
                else:
                    docs = db.execute("SELECT total_pages FROM source_documents").fetchall()
                    require(docs == [(482,)], "ENGLISH_MANUFACTURER_PAGES_INCOMPLETE")
                    require(db.execute("SELECT count(*) FROM procedures").fetchone()[0] == 336,
                            "ENGLISH_MANUFACTURER_PROCEDURES_INCOMPLETE")
                    images = []
    return {
        "language": language,
        "source_zip_sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
        "source_zip_bytes": path.stat().st_size,
        "documents": len(docs),
        "manufacturer_image_count": len(images),
        "verified_sqlite": exact,
    }


def create_pack(source: Path, language: str, output: Path, password: str) -> dict:
    require(language in ("fr", "en"), "UNKNOWN_LANGUAGE")
    require(isinstance(password, str) and len(password) >= 12, "PACK_PASSWORD_TOO_SHORT")
    details = inspect_zip(source, language)
    plain = source.read_bytes()
    salt, nonce = os.urandom(16), os.urandom(12)
    header = MAGIC + bytes([1]) + salt + nonce + struct.pack("<Q", len(plain))
    key = hashlib.pbkdf2_hmac("sha256", password.encode("utf-8"),
                             salt, ITERATIONS, dklen=32)
    encrypted = header + AESGCM(key).encrypt(nonce, plain, header)
    require(len(encrypted) == len(plain)+61, "ENVELOPE_LENGTH_INVALID")
    output.parent.mkdir(parents=True, exist_ok=True)
    tmp = output.with_name(output.name + ".part")
    try:
        with tmp.open("wb") as stream:
            stream.write(encrypted)
            stream.flush()
            os.fsync(stream.fileno())
        tmp.replace(output)
    finally:
        tmp.unlink(missing_ok=True)
    report = {
        **details, "format": "MEMSDOC1", "cipher": "AES-256-GCM",
        "kdf": "PBKDF2-HMAC-SHA256", "pbkdf2_iterations": ITERATIONS,
        "encrypted_asset_name": output.name,
        "encrypted_sha256": hashlib.sha256(encrypted).hexdigest(),
        "encrypted_size": len(encrypted),
        "third_party_redistribution_rights": "NOT_VERIFIED_BY_TOOL",
        "release_published": False,
        "password_in_report": False,
    }
    return report


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-zip", type=Path, required=True)
    parser.add_argument("--language", required=True, choices=("fr", "en"))
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--password-env", default="MEMS_DOCUMENTARY_PUBLISH_PASSWORD")
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    password = os.environ.get(args.password_env, "")
    require(password, "SECRET_PASSWORD_NOT_CONFIGURED")
    report = create_pack(args.source_zip, args.language, args.output, password)
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n",
                               encoding="utf-8")
    print("MEMSDOC1_PRIVATE_PACK_VERIFIED",
          json.dumps({k: report[k] for k in (
              "language", "encrypted_asset_name", "encrypted_size", "encrypted_sha256",
              "manufacturer_image_count")}, sort_keys=True))


if __name__ == "__main__":
    main()
