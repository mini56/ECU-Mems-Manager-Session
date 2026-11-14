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
from documentary_common import LANGUAGES, validate_source_snapshot

MAX_ZIP = 256 * 1024 * 1024
MAX_UNPACKED = 700 * 1024 * 1024
MAGIC = b"MEMSDOC1"
ITERATIONS = 210000


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def inspect_zip(path: Path, language: str) -> dict:
    """One validator for ALL languages, editions and document counts.

    No manual names, page counts, fixed numbers of images, or hidden source
    substitutions. Every stored original image is verified by its SHA256.
    """
    require(language in LANGUAGES, "UNKNOWN_LANGUAGE")
    require(path.is_file() and path.stat().st_size < MAX_ZIP,
            "ZIP_MISSING_OR_TOO_LARGE")
    exact=f"mems_documentary_{language}.sqlite"
    with zipfile.ZipFile(path) as archive:
        entries=archive.infolist()
        files=[entry for entry in entries if not entry.is_dir()]
        names=[entry.filename for entry in files]
        require(len(files)==len(set(names)) and len(files)<2500,
                "ZIP_PATH_COLLISION_OR_TOO_MANY_FILES")
        require(exact in names and names.count(exact)==1,
                "ZIP_SQLITE_LANGUAGE_MISMATCH")
        require(not any(name.startswith("/") or "\\" in name or
                        any(part in ("", ".", "..") for part in name.split("/"))
                        for name in names), "ZIP_PATH_TRAVERSAL")
        require(not any((entry.external_attr >> 16) & 0o170000==0o120000
                        for entry in files), "ZIP_SYMLINK_REJECTED")
        require(sum(entry.file_size for entry in files)<MAX_UNPACKED,
                "ZIP_UNPACKED_TOO_LARGE")
        require(archive.testzip() is None, "ZIP_CRC_ERROR")
        with tempfile.TemporaryDirectory() as folder:
            target=Path(folder)/exact
            target.write_bytes(archive.read(exact))
            with closing(sqlite3.connect(f"file:{target}?mode=ro",uri=True)) as db:
                counts=validate_source_snapshot(db,language)
                images=db.execute(
                    "SELECT relative_path,asset_sha256,asset_bytes "
                    "FROM documentary_images").fetchall()
                found=set()
                for relative,digest,size in images:
                    require(isinstance(relative,str) and
                            re.fullmatch(
                                r"images/[A-Za-z0-9_.-]+\.(png|jpg|jpeg|webp)",
                                relative,re.IGNORECASE) is not None and
                            relative not in found and relative in names and
                            isinstance(size,int) and 0<size<=15*1024*1024,
                            "DOC_IMAGE_PATH_OR_SIZE_INVALID")
                    require(archive.getinfo(relative).file_size==size and
                            hashlib.sha256(archive.read(relative)).hexdigest()==digest,
                            "DOC_IMAGE_SHA256_INVALID")
                    found.add(relative)
                require(set(names)==found|{exact},
                        "DOC_SNAPSHOT_UNDECLARED_FILES")
    return dict(
        counts,source_zip_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
        source_zip_bytes=path.stat().st_size,
        documents=counts["document_count"],
        manufacturer_image_count=counts["facsimiles"],
        verified_sqlite=exact)




def create_pack(source: Path, language: str, output: Path, password: str) -> dict:
    require(language in LANGUAGES, "UNKNOWN_LANGUAGE")
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
    parser.add_argument("--language", required=True, choices=sorted(LANGUAGES))
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
