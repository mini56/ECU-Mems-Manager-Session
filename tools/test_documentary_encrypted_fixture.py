#!/usr/bin/env python3
"""Synthetic-only MEMSDOC1 roundtrip source for the native Windows CI test."""
from __future__ import annotations

import argparse
import hashlib
import os
import struct
import zipfile
from pathlib import Path
from cryptography.hazmat.primitives.ciphers.aead import AESGCM

PASSWORD = "SyntheticTestOnlyPassword456!"


def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("--output", type=Path, required=True)
    a = p.parse_args()
    a.output.mkdir(parents=True, exist_ok=True)
    plain = a.output / "test_plain.zip"
    with zipfile.ZipFile(plain, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        archive.writestr("readme.txt", "Synthetic test-only encrypted ZIP.\n")
    content = plain.read_bytes()
    salt, nonce = os.urandom(16), os.urandom(12)
    header = b"MEMSDOC1" + bytes((1,)) + salt + nonce + struct.pack("<Q", len(content))
    key = hashlib.pbkdf2_hmac("sha256", PASSWORD.encode("utf-8"),
                             salt, 210000, 32)
    encrypted = header + AESGCM(key).encrypt(nonce, content, header)
    (a.output / "test_plain.encrypted").write_bytes(encrypted)
    decrypted = AESGCM(key).decrypt(nonce, encrypted[45:], encrypted[:45])
    assert decrypted == content
    assert len(encrypted) == len(content) + 61
    print("MEMSDOC1_PYTHON_CROSS_PLATFORM_FIXTURE_PASS")


if __name__ == "__main__":
    main()
