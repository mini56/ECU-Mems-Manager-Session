#!/usr/bin/env python3
import argparse
import base64
import hashlib
import json
import sqlite3
import struct
import zlib
from pathlib import Path

import fitz

PAGES = [
    {
        "label": "20.3",
        "pdf_index": 17,
        "filename": "RCL0194ENG_20_3.png",
        "key": "RAVE RCL0194ENG 20.3 Mini SPi Japon 97MY MEMS lambda relais vilebrequin",
        "topics": "engine management MEMS SPi Japan oxygen lambda relay crankshaft fuel pump",
    },
    {
        "label": "20.4",
        "pdf_index": 18,
        "filename": "RCL0194ENG_20_4.png",
        "key": "RAVE RCL0194ENG 20.4 Mini SPi Japon 97MY MEMS injecteur IAC TPS ECT IAT",
        "topics": "engine management MEMS SPi Japan injector stepper purge ignition coolant throttle inlet air",
    },
    {
        "label": "39.3",
        "pdf_index": 26,
        "filename": "RCL0194ENG_39_3.png",
        "key": "RAVE RCL0194ENG 39.3 Mini SPi Japon 97MY ventilateur refroidissement",
        "topics": "cooling fan SPi Japan relay high temperature switch",
    },
]

SOURCE_DOCUMENT = "RCL0194ENG"
PILOT_BATCH = "1680"
ENRICHMENT_FILE = "research_enrichment_1750.qz64"
SOURCE_URL = "https://www.goclassic.eu/upload/content/e68ce7_9c0fd6a5b3a14a3585ecddb02b3ceb9d.pdf"


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def sql_quote(value: str) -> str:
    return "'" + value.replace("'", "''") + "'"


def qcompress_base64(payload: bytes) -> bytes:
    # Qt qCompress format: 4-byte big-endian uncompressed length + zlib stream.
    packed = struct.pack(">I", len(payload)) + zlib.compress(payload, 9)
    return base64.b64encode(packed) + b"\n"


def build_sql(pdf_hash: str, rendered):
    statements = [
        "CREATE TABLE IF NOT EXISTS mems_rave_illustration ("
        "illustration_key TEXT PRIMARY KEY, pilot_batch TEXT NOT NULL, source_document TEXT NOT NULL, "
        "source_page_label TEXT NOT NULL, source_pdf_page_index INTEGER NOT NULL, relative_path TEXT NOT NULL, "
        "source_url TEXT, source_pdf_sha256 TEXT NOT NULL, asset_sha256 TEXT NOT NULL, width_px INTEGER NOT NULL, "
        "height_px INTEGER NOT NULL, notes TEXT);",
        "CREATE TABLE IF NOT EXISTS mems_rave_illustration_link ("
        "image_ref TEXT NOT NULL, illustration_key TEXT NOT NULL, relation_type TEXT NOT NULL DEFAULT 'source_illustration', "
        "PRIMARY KEY(image_ref, illustration_key), "
        "FOREIGN KEY(illustration_key) REFERENCES mems_rave_illustration(illustration_key));",
        "CREATE INDEX IF NOT EXISTS idx_rave_illustration_document_page "
        "ON mems_rave_illustration(source_document, source_page_label);",
        "CREATE INDEX IF NOT EXISTS idx_rave_illustration_link_ref "
        "ON mems_rave_illustration_link(image_ref, illustration_key);",
    ]

    for item in rendered:
        illustration_key = f"rave:{SOURCE_DOCUMENT}:{item['label']}"
        relative = f"images/rave/{item['filename']}"
        notes = (
            f"Pilote lot {PILOT_BATCH}; page constructeur originale extraite du PDF {SOURCE_DOCUMENT}; "
            "aucun fait historique modifie."
        )
        statements.append(
            "INSERT OR REPLACE INTO mems_rave_illustration("
            "illustration_key,pilot_batch,source_document,source_page_label,source_pdf_page_index,relative_path,"
            "source_url,source_pdf_sha256,asset_sha256,width_px,height_px,notes) VALUES("
            + ",".join(
                [
                    sql_quote(illustration_key),
                    sql_quote(PILOT_BATCH),
                    sql_quote(SOURCE_DOCUMENT),
                    sql_quote(item["label"]),
                    str(item["pdf_index"]),
                    sql_quote(relative),
                    sql_quote(SOURCE_URL),
                    sql_quote(pdf_hash),
                    sql_quote(item["asset_sha256"]),
                    str(item["width_px"]),
                    str(item["height_px"]),
                    sql_quote(notes),
                ]
            )
            + ");"
        )
        statements.append(
            "INSERT OR REPLACE INTO mems_rave_illustration_link(image_ref,illustration_key,relation_type) VALUES("
            + ",".join(
                [
                    sql_quote(f"{SOURCE_DOCUMENT}:{item['label']}"),
                    sql_quote(illustration_key),
                    sql_quote("source_illustration"),
                ]
            )
            + ");"
        )

    # This view links existing historical facts without changing mems_rave_fact.
    statements.append(
        "CREATE VIEW IF NOT EXISTS mems_rave_fact_illustration_view AS "
        "SELECT f.fact_key,f.source_key,f.document,f.variant,f.topic,f.statement,f.image_ref,"
        "l.illustration_key,i.relative_path,i.source_page_label,i.source_pdf_sha256,i.asset_sha256 "
        "FROM mems_rave_fact f "
        "LEFT JOIN mems_rave_illustration_link l ON l.image_ref=f.image_ref "
        "LEFT JOIN mems_rave_illustration i ON i.illustration_key=l.illustration_key;"
    )
    return "\n".join(statements) + "\n"


def validate_sql(sql_text: str):
    db = sqlite3.connect(":memory:")
    try:
        db.executescript(
            "CREATE TABLE mems_rave_fact("
            "fact_key TEXT PRIMARY KEY,source_key TEXT,document TEXT,variant TEXT,topic TEXT,statement TEXT,image_ref TEXT);"
        )
        db.execute(
            "INSERT INTO mems_rave_fact VALUES(?,?,?,?,?,?,?)",
            ("pilot", "rave", SOURCE_DOCUMENT, "SPi Japan 97MY", "oxygen", "pilot", f"{SOURCE_DOCUMENT}:20.3"),
        )
        db.executescript(sql_text)
        row = db.execute(
            "SELECT relative_path FROM mems_rave_fact_illustration_view WHERE fact_key='pilot'"
        ).fetchone()
        if not row or row[0] != "images/rave/RCL0194ENG_20_3.png":
            raise RuntimeError("RAVE fact/illustration link self-test failed")
    finally:
        db.close()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pdf", required=True)
    parser.add_argument("--dist", required=True)
    parser.add_argument("--expected-pdf-sha256", default="")
    args = parser.parse_args()

    pdf = Path(args.pdf).resolve()
    dist = Path(args.dist).resolve()
    ref = dist / "database" / "reference"
    manifest_path = ref / "manifest.json"
    if not pdf.is_file():
        raise SystemExit(f"RAVE source PDF missing: {pdf}")
    if not manifest_path.is_file():
        raise SystemExit(f"#101 reference manifest missing: {manifest_path}")

    pdf_hash = sha256(pdf)
    expected = args.expected_pdf_sha256.strip().lower()
    if expected and pdf_hash.lower() != expected:
        raise SystemExit(f"RAVE source SHA256 mismatch: got={pdf_hash} expected={expected}")

    image_dir = ref / "images" / "rave"
    image_dir.mkdir(parents=True, exist_ok=True)
    document = fitz.open(pdf)
    try:
        if document.page_count != 41:
            raise SystemExit(f"Unexpected RCL0194ENG page count: {document.page_count}")
        rendered = []
        for item in PAGES:
            page = document.load_page(item["pdf_index"])
            pix = page.get_pixmap(matrix=fitz.Matrix(2.0, 2.0), alpha=False)
            target = image_dir / item["filename"]
            pix.save(target)
            rendered.append(
                {
                    **item,
                    "asset_sha256": sha256(target),
                    "width_px": pix.width,
                    "height_px": pix.height,
                }
            )
    finally:
        document.close()

    sql_text = build_sql(pdf_hash, rendered)
    validate_sql(sql_text)
    (ref / ENRICHMENT_FILE).write_bytes(qcompress_base64(sql_text.encode("utf-8")))

    manifest = json.loads(manifest_path.read_text(encoding="utf-8-sig"))
    batches = manifest.setdefault("research_enrichment_batches", [])
    if ENRICHMENT_FILE not in batches:
        batches.append(ENRICHMENT_FILE)
    diagrams = manifest.setdefault("diagrams", {})
    for item in rendered:
        diagrams[item["key"]] = f"images/rave/{item['filename']}"
    manifest_path.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    metadata = {
        "pilot": "RAVE illustrations lot 1680",
        "base": "MEMSX64 BUILD #101 22dbe75ed14e0a61e694159d505ef72245116b48",
        "source_document": SOURCE_DOCUMENT,
        "source_url": SOURCE_URL,
        "source_pdf_sha256": pdf_hash,
        "database_revision": manifest.get("database_revision"),
        "historical_tables_modified": False,
        "assets": [
            {
                "page": item["label"],
                "pdf_index": item["pdf_index"],
                "path": f"database/reference/images/rave/{item['filename']}",
                "sha256": item["asset_sha256"],
                "width": item["width_px"],
                "height": item["height_px"],
            }
            for item in rendered
        ],
        "additive_enrichment": f"database/reference/{ENRICHMENT_FILE}",
    }
    (dist / "RAVE1680_PILOT.json").write_text(
        json.dumps(metadata, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
    )

    # Final package checks.
    manifest_check = json.loads(manifest_path.read_text(encoding="utf-8"))
    for item in rendered:
        rel = f"images/rave/{item['filename']}"
        if manifest_check.get("diagrams", {}).get(item["key"]) != rel:
            raise RuntimeError(f"Manifest diagram missing: {item['key']}")
        if not (ref / rel).is_file():
            raise RuntimeError(f"Packaged RAVE illustration missing: {rel}")
    if manifest_check.get("database_revision") != 20:
        raise RuntimeError("Pilot must keep database_revision=20")

    print(f"RAVE_SOURCE_SHA256={pdf_hash}")
    print("PASS RAVE 1680 pilot: original pages 20.3, 20.4, 39.3 packaged and linked additively")
    for item in rendered:
        print(
            f"RAVE_PAGE_{item['label'].replace('.', '_')}={item['asset_sha256']} "
            f"{item['width_px']}x{item['height_px']}"
        )


if __name__ == "__main__":
    main()
