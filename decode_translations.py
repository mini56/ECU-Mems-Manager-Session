#!/usr/bin/env python3
"""Build-time validation for the four additional translation catalogs.

The catalogs are stored directly in translations/.  No packed/base64 bundle is
used anymore: this script only checks that all four files exist and are valid
Qt TS XML before CMake runs lrelease.
"""
from pathlib import Path
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parent
outdir = root / "translations"

for lang in ("es", "it", "pt", "de"):
    path = outdir / f"ECUMemsManager_{lang}.ts"
    if not path.is_file():
        raise SystemExit(f"Missing translation catalog: {path}")
    try:
        tree = ET.parse(path)
    except ET.ParseError as exc:
        raise SystemExit(f"Invalid TS XML {path.name}: {exc}") from exc
    messages = tree.findall(".//message")
    if not messages:
        raise SystemExit(f"Empty translation catalog: {path.name}")
    print(f"Validated {path.name}: {len(messages)} messages")
