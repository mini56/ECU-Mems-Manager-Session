#!/usr/bin/env python3
"""Recover one unchanged original 4-PDF source ZIP using authenticated Git blob IDs.

The transferred objects are not installable fragments. They are consecutive
byte ranges of the single 46,940,984-byte French RAVE documentary ZIP supplied
by the user. This script fails closed if any source byte is missing or changed.
"""
from __future__ import annotations
from concurrent.futures import ThreadPoolExecutor, as_completed
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import struct
import time
import urllib.error
import urllib.request
import zipfile

EXPECTED_ZIP_SHA = "7e9cabd6cfe0584eb88dfb786b2441b6f20d706916732caa74c0d7303485b41a"
EXPECTED_SIZE = 46940984
EXPECTED_PIECES = 120
EXPECTED_SOURCE_NAMES = {
    "ir07301f.pdf": 24,
    "elxn010f.pdf": 210,
    "hr01795f.PDF": 98,
    "cdxn010f.pdf": 74,
}


def get_blob(item: dict) -> tuple[int, bytes]:
    sha = item["sha"]
    req = urllib.request.Request(
        "https://api.github.com/repos/mini56/"
        "ECU-Mems-Manager-Session/git/blobs/" + sha,
        headers={
            "Accept": "application/vnd.github+json",
            "X-GitHub-Api-Version": "2022-11-28",
            "User-Agent": "SESSION-original-french-documentary-restore",
            **({"Authorization": "Bearer " + os.environ["GH_TOKEN"]}
               if os.environ.get("GH_TOKEN") else {}),
        },
    )
    for attempt in range(4):
        try:
            with urllib.request.urlopen(req, timeout=60) as response:
                blob = json.load(response)
            if blob.get("encoding") != "base64":
                raise ValueError("GITHUB_BLOB_NOT_BASE64")
            payload = base64.b64decode(blob["content"], validate=False)
            header = f"blob {len(payload)}".encode("ascii") + bytes([0])
            if hashlib.sha1(header + payload).hexdigest() != sha:
                raise ValueError("SOURCE_BLOB_SHA_MISMATCH")
            if item["part"] < 119 and len(payload) != 393216:
                raise ValueError("SOURCE_PART_WRONG_LENGTH")
            return item["part"], payload
        except urllib.error.HTTPError as e:
            if e.code not in (429, 500, 502, 503, 504) or attempt == 3:
                raise
            time.sleep(2 ** attempt)
    raise RuntimeError("GITHUB_ORIGINAL_BINARY_SOURCE_UNAVAILABLE")


def recover(manifests: Path, output: Path) -> dict:
    pieces = []
    for p in sorted(manifests.glob("blobs_*.json")):
        objects = json.loads(p.read_text(encoding="utf-8"))
        if not isinstance(objects, list):
            raise ValueError("TRANSFER_MANIFEST_INVALID")
        pieces.extend(objects)
    if len(pieces) != EXPECTED_PIECES:
        raise ValueError(f"INCOMPLETE_120_SOURCE_BLOBS_FOUND_{len(pieces)}")
    indices = [x["part"] for x in pieces]
    if sorted(indices) != list(range(EXPECTED_PIECES)):
        raise ValueError("TRANSFER_BLOBS_MISSING_OR_DUPLICATED")
    if any(not isinstance(x["sha"], str) or len(x["sha"]) != 40 for x in pieces):
        raise ValueError("TRANSFER_BLOB_SHA_INVALID")
    output.parent.mkdir(parents=True, exist_ok=True)
    partial = output.with_suffix(".partial")
    digest = hashlib.sha256()
    try:
        with ThreadPoolExecutor(max_workers=6) as pool:
            tasks = {pool.submit(get_blob, item): item["part"] for item in pieces}
            collected: dict[int, bytes] = {}
            for task in as_completed(tasks):
                part, payload = task.result()
                collected[part] = payload
        with partial.open("wb") as file:
            for part in range(EXPECTED_PIECES):
                chunk = collected[part]
                file.write(chunk)
                digest.update(chunk)
        if partial.stat().st_size != EXPECTED_SIZE or digest.hexdigest() != EXPECTED_ZIP_SHA:
            raise ValueError("ORIGINAL_USER_RAVE_ZIP_SOURCE_CHECKSUM_FAILED")
        with zipfile.ZipFile(partial) as source:
            if source.testzip() is not None:
                raise ValueError("ORIGINAL_USER_RAVE_ZIP_CRC_FAILED")
            assets = source.namelist()
            if "mems_documentary_fr.sqlite" not in assets or len(assets) != 407:
                raise ValueError("ORIGINAL_SOURCE_BUNDLE_NOT_FOUR_PDFS_AND_FACSIMILES")
            import sqlite3
            with sqlite3.connect(":memory:") as db:
                db.deserialize(source.read("mems_documentary_fr.sqlite"))
                docs = dict(db.execute(
                    "SELECT source_filename,source_pages FROM documentary_documents"))
                if docs != EXPECTED_SOURCE_NAMES:
                    raise ValueError("ORIGINAL_SOURCE_MANUAL_IDENTITIES_NOT_PRESERVED")
                pages = db.execute("SELECT COUNT(*) FROM documentary_pages").fetchone()[0]
                images = db.execute("SELECT COUNT(*) FROM documentary_images").fetchone()[0]
                if pages != 406 or images != 406:
                    raise ValueError("ORIGINAL_SOURCE_DOCUMENTARY_INCOMPLETE")
        partial.replace(output)
    finally:
        partial.unlink(missing_ok=True)
    return {"source_sha256": digest.hexdigest(), "source_bytes": EXPECTED_SIZE,
            "documents": 4, "pages": 406, "images": 406, "chunks": 120}


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--manifests", type=Path, required=True)
    ap.add_argument("--output", type=Path, required=True)
    a = ap.parse_args()
    print("ORIGINAL_FR_RAVE_BUNDLE_RESTORED",
          json.dumps(recover(a.manifests, a.output), sort_keys=True))
