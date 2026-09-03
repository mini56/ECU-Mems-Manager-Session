#!/usr/bin/env python3
from pathlib import Path

path = Path("ravemems/v2/core_extract.py")
lines = path.read_text(encoding="utf-8").splitlines(keepends=True)
needle = 'raise SystemExit("Expected document identity pattern not found in source PDF")'
matches = [i for i, line in enumerate(lines) if needle in line]
if len(matches) != 1:
    raise SystemExit(f"expected exactly one legacy publication error line, found {len(matches)}")

i = matches[0]
if i < 1 or i + 1 >= len(lines):
    raise SystemExit("legacy publication gate has unexpected file boundaries")
if lines[i - 1].strip() != "if not publication_match:":
    raise SystemExit(f"unexpected guard line before legacy publication error: {lines[i - 1].strip()!r}")
if lines[i + 1].strip() != "publication_code = publication_match.group(0).upper()":
    raise SystemExit(f"unexpected assignment line after legacy publication error: {lines[i + 1].strip()!r}")

indent = lines[i + 1][: len(lines[i + 1]) - len(lines[i + 1].lstrip())]
newline = "\r\n" if lines[i + 1].endswith("\r\n") else "\n"
replacement = f"{indent}publication_code = publication_match.group(0).upper() if publication_match else None{newline}"
lines[i - 1 : i + 2] = [replacement]
path.write_text("".join(lines), encoding="utf-8")
print("GENERIC_UNKNOWN_PUBLICATION_FALLBACK_PATCHED")
