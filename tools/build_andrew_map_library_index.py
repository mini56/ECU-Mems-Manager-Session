#!/usr/bin/env python3
"""Index Andrew Revill's public Map/Firmware Library without embedding large binaries.

The generated reference data keeps URLs, family, firmware code, part-number hints,
file kind and library area. It does not download ROM/ASM/PETA payloads into the
application package.
"""
from __future__ import annotations

import argparse
import html
import re
import time
import urllib.parse
import urllib.request
from html.parser import HTMLParser
from pathlib import Path

from build_memstools_enrichment import add_multi, sqlq, write_qz64

SOURCE_KEY = "andrew_revill_map_firmware_library"
ROOT = "https://andrewrevill.co.uk/MapFirmwareLibrary/"
OUTPUT_NAME = "research_enrichment_1590.qz64"
USER_AGENT = "ECU-Mems-Manager knowledge audit"

TARGETS = [
    ("1.3", "identified_library", "Rover MEMS 1.3/"),
    ("1.6", "identified_library", "Rover MEMS 1.6/"),
    ("1.9", "identified_library", "Rover MEMS 1.9/"),
    ("1.3", "firmware_archive", "Firmware Archive/Rover MEMS 1.3/"),
    ("1.6", "firmware_archive", "Firmware Archive/Rover MEMS 1.6/"),
    ("1.9", "firmware_archive", "Firmware Archive/Rover MEMS 1.9/"),
]

PART_RE = re.compile(r"\b(?:MNE|MKC|MSB|NNN|NNW|YWC|SLD)\s?_?\d{5,6}\b", re.I)
FW_RE = re.compile(r"\b[A-Z]{3,7}[0-9]{3,4}\b")


class Links(HTMLParser):
    def __init__(self):
        super().__init__()
        self.items = []
        self.href = None
        self.text = []

    def handle_starttag(self, tag, attrs):
        if tag.lower() == "a":
            self.href = dict(attrs).get("href")
            self.text = []

    def handle_data(self, data):
        if self.href is not None:
            self.text.append(data)

    def handle_endtag(self, tag):
        if tag.lower() == "a" and self.href is not None:
            self.items.append((self.href, "".join(self.text).strip()))
            self.href = None
            self.text = []


def fetch(url):
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=30) as response:
        return response.read().decode("utf-8", errors="replace")


def list_dir(url):
    parser = Links()
    parser.feed(fetch(url))
    result = []
    for href, label in parser.items:
        if not href or href.startswith(("?", "#")) or href in ("../", "./"):
            continue
        absolute = urllib.parse.urljoin(url, href)
        if not absolute.startswith(ROOT):
            continue
        result.append((absolute, urllib.parse.unquote(label or href)))
    return result


def kind(filename):
    ext = Path(filename).suffix.lower()
    return {
        ".rom": "rom", ".bin": "binary", ".hex": "firmware_hex",
        ".asm": "disassembly", ".xf1": "peta_xf1", ".xf3": "peta_xf3",
        ".map": "map", ".eep": "eeprom", ".csv": "csv", ".txt": "text",
    }.get(ext, ext.lstrip(".") or "file")


def part_numbers(text):
    values = []
    for match in PART_RE.finditer(text.replace("_", " ")):
        value = re.sub(r"\s|_", "", match.group(0)).upper()
        if value not in values:
            values.append(value)
    return ";".join(values) or None


def firmware_code(text):
    # Prefer the filename stem when it is itself a firmware ID (archive files).
    stem = Path(text).stem.upper()
    if re.fullmatch(r"[A-Z]{3,7}[0-9]{3,4}", stem):
        return stem
    # Identified-library names usually contain a software ID separated by dashes.
    candidates = FW_RE.findall(text.upper().replace("_", " "))
    ignored_prefixes = ("MEMS", "ROVER", "MGROVER")
    for value in candidates:
        if not value.startswith(ignored_prefixes):
            return value
    return None


def crawl(family, area, relative_root):
    start = urllib.parse.urljoin(ROOT, urllib.parse.quote(relative_root, safe="/"))
    pending = [start]
    seen = set()
    rows = []
    while pending:
        url = pending.pop(0)
        if url in seen:
            continue
        seen.add(url)
        try:
            entries = list_dir(url)
        except Exception as exc:
            print(f"WARN list {url}: {exc}")
            continue
        for child, label in entries:
            if child.endswith("/"):
                if child not in seen:
                    pending.append(child)
                continue
            filename = urllib.parse.unquote(urllib.parse.urlparse(child).path.rsplit("/", 1)[-1])
            rel = urllib.parse.unquote(child[len(ROOT):]) if child.startswith(ROOT) else filename
            rows.append((
                SOURCE_KEY, family, area, rel, filename, kind(filename),
                firmware_code(filename), part_numbers(filename), child,
                "source_externe"
            ))
        time.sleep(0.03)
    return rows


def build(output_dir):
    all_rows = []
    coverage = []
    for family, area, relative in TARGETS:
        rows = crawl(family, area, relative)
        all_rows.extend(rows)
        coverage.append((SOURCE_KEY, family, area, urllib.parse.urljoin(ROOT, urllib.parse.quote(relative, safe="/")), "indexed" if rows else "empty_or_unreachable", len(rows), "source_externe"))
        print(f"{family} {area}: {len(rows)}")

    # Andrew's current top-level MapFirmwareLibrary does not expose a Rover MEMS 1.2 directory.
    coverage.append((SOURCE_KEY, "1.2", "identified_library", ROOT + "Rover%20MEMS%201.2/", "not_listed_at_top_level", 0, "source_externe"))
    coverage.append((SOURCE_KEY, "1.2", "firmware_archive", ROOT + "Firmware%20Archive/Rover%20MEMS%201.2/", "not_confirmed_by_index", 0, "source_externe"))

    # Deduplicate exact URLs while preserving first classification.
    dedup = {}
    for row in all_rows:
        dedup.setdefault(row[8], row)
    all_rows = sorted(dedup.values(), key=lambda r: (r[1], r[2], r[3]))

    lines = ["BEGIN TRANSACTION;"]
    lines.extend([
        "CREATE TABLE IF NOT EXISTS mems_knowledge_source(source_key TEXT PRIMARY KEY,author TEXT,title TEXT,source_url TEXT,archive_sha256 TEXT,verification_level TEXT,scope TEXT,notes TEXT);",
        "CREATE TABLE IF NOT EXISTS andrew_map_library_asset(source_key TEXT NOT NULL,family TEXT NOT NULL,library_area TEXT NOT NULL,relative_path TEXT NOT NULL,filename TEXT NOT NULL,file_kind TEXT,firmware_code TEXT,part_numbers TEXT,url TEXT NOT NULL,verification_level TEXT NOT NULL,PRIMARY KEY(source_key,url));",
        "CREATE TABLE IF NOT EXISTS andrew_map_library_coverage(source_key TEXT NOT NULL,family TEXT NOT NULL,library_area TEXT NOT NULL,url TEXT NOT NULL,status TEXT NOT NULL,file_count INTEGER NOT NULL,verification_level TEXT NOT NULL,PRIMARY KEY(source_key,family,library_area));",
        "CREATE INDEX IF NOT EXISTS idx_andrew_library_fw ON andrew_map_library_asset(family,firmware_code);",
        "CREATE INDEX IF NOT EXISTS idx_andrew_library_part ON andrew_map_library_asset(part_numbers);",
    ])
    lines.append(f"DELETE FROM andrew_map_library_asset WHERE source_key={sqlq(SOURCE_KEY)};")
    lines.append(f"DELETE FROM andrew_map_library_coverage WHERE source_key={sqlq(SOURCE_KEY)};")
    lines.append(f"DELETE FROM mems_knowledge_source WHERE source_key={sqlq(SOURCE_KEY)};")
    source = (
        SOURCE_KEY, "Andrew Revill", "MEMS Map & Firmware Library", ROOT, None,
        "source_externe", "MEMS 1.3 / 1.6 / 1.9; explicit 1.2 coverage gap recorded",
        "Index metadata only. Large ROM, ASM and PETA files remain on Andrew Revill's public library and are not embedded in ECU MEMS Manager."
    )
    lines.append("INSERT INTO mems_knowledge_source VALUES(" + ",".join(sqlq(v) for v in source) + ");")
    add_multi(lines, "andrew_map_library_asset",
              ["source_key","family","library_area","relative_path","filename","file_kind","firmware_code","part_numbers","url","verification_level"],
              all_rows, batch=200)
    add_multi(lines, "andrew_map_library_coverage",
              ["source_key","family","library_area","url","status","file_count","verification_level"],
              coverage)
    lines.append("COMMIT;")
    output = output_dir / OUTPUT_NAME
    write_qz64(output, lines)
    print(f"output={output}")
    print(f"assets={len(all_rows)}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--output-dir", type=Path, default=Path("database/reference"))
    args = ap.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    build(args.output_dir)


if __name__ == "__main__":
    main()
