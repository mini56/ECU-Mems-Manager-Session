#!/usr/bin/env python3
"""Block unintended changes to user-approved SESSION styles.

This is a build-time source verification, NOT a runtime UI patch. The
baseline is the Git blob checksum of every relevant approved style owner.
Changing any protected file fails CI BEFORE compilation and publication.
When the user explicitly approves a visual change, review the diff and
update the baseline in a dedicated, visible change. The Qt smoke test
independently checks fonts, alignment, panels and the 14-page navigation.
"""
import hashlib
import json
from pathlib import Path
import sys


def git_blob_hash(data: bytes) -> str:
    return hashlib.sha1(
        b"blob " + str(len(data)).encode("ascii") + b"\0" + data
    ).hexdigest()


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    config = root / "tools" / "ui_style_baseline.json"
    manifest = json.loads(config.read_text(encoding="utf-8"))
    if manifest.get("schema") != 1:
        print("STYLE_GUARD_INVALID_BASELINE_SCHEMA", file=sys.stderr)
        return 1
    hashes = manifest.get("git_blob_sha1", {})
    if len(hashes) < 10:
        print("STYLE_GUARD_INCOMPLETE_APPROVED_FILE_SET", file=sys.stderr)
        return 1
    failed = []
    for relpath, approved_sha in sorted(hashes.items()):
        source = root / relpath
        if not source.is_file():
            failed.append(f"{relpath}: missing approved source")
            continue
        sha = git_blob_hash(source.read_bytes())
        if sha != approved_sha:
            failed.append(f"{relpath}: approved {approved_sha}, current {sha}")
    if failed:
        print("SESSION_APPROVED_UI_STYLE_REGRESSION_BLOCKED", file=sys.stderr)
        for item in failed:
            print("  " + item, file=sys.stderr)
        print("Revert unauthorized style changes. Change the approved baseline only after explicit user acceptance.", file=sys.stderr)
        return 1
    print(f"SESSION_APPROVED_UI_STYLE_LOCK_PASS ({len(hashes)} protected source files)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
