#!/usr/bin/env python3
from pathlib import Path

path = Path("ravemems/v2/core_extract.py")
text = path.read_text(encoding="utf-8")
old = '''    publication_match = re.search(profile["publication_code_regex"], head_text, re.IGNORECASE)\n    if not publication_match:\n        raise SystemExit("Expected document identity pattern not found in source PDF")\n    publication_code = publication_match.group(0).upper()\n'''
new = '''    publication_match = re.search(profile["publication_code_regex"], head_text, re.IGNORECASE)\n    publication_code = publication_match.group(0).upper() if publication_match else None\n'''
count = text.count(old)
if count != 1:
    raise SystemExit(f"expected exactly one legacy publication gate, found {count}")
path.write_text(text.replace(old, new, 1), encoding="utf-8")
print("GENERIC_UNKNOWN_PUBLICATION_FALLBACK_PATCHED")
