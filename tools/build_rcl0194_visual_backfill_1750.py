#!/usr/bin/env python3
"""Build and validate additive RCL0194 visual backfill batch 1750.

This tool intentionally operates only on database/reference assets. It reconstructs
the BUILD #101 reference SQLite from the shipped seed/enrichment lots, derives
RCL0194 links from the historical facts already present, writes a Qt qCompress
compatible Base64 .qz64 batch, applies it to the reconstructed SQLite, and checks
the additive invariants before any Git commit is allowed.
"""

from __future__ import annotations

import argparse
import base64
import hashlib
import json
import re
import shutil
import sqlite3
import struct
import zlib
from pathlib import Path

EXPECTED_RAVE_FACTS = 93
EXPECTED_EXPERT_FACTS = 105
EXPECTED_RCL0194_FACTS = 55
EXPECTED_COLOR_FACTS = 26
EXPECTED_CORRECTIONS = 3
EXPECTED_USER_VERSION = 20

ASSETS = [
    ("COLOUR_CODES", "COLOUR CODES", 5, "legend",
     "RCL0194ENG_COLOUR_CODES.png",
     "718af9c48d7cc466b5c50278680ecb286de81a869d8e2293207796afe08833d3"),
    ("15.1", "15.1", 13, "wiring_diagram",
     "RCL0194ENG_15_1.png",
     "fa3a9f7620b660dc1405adf5bb330f74b9cf627e6ebc7bf2e3ac1b7a42016eeb"),
    ("20.1", "20.1", 15, "wiring_diagram",
     "RCL0194ENG_20_1.png",
     "334eea87abb0b9b5692c18ef157f51aa4a2edcc1580dd3d911ba7c3262bb9052"),
    ("20.2", "20.2", 16, "wiring_diagram",
     "RCL0194ENG_20_2.png",
     "3736908a793e46a5e3bee32d1d51544caf52557475317a2a5f96ca3d16177c9e"),
    ("20.3", "20.3", 17, "wiring_diagram",
     "RCL0194ENG_20_3.png",
     "2988614881aa2d713788eac084c97ae148595bbb19da21fd92b484041ef3e4c8"),
    ("20.4", "20.4", 18, "wiring_diagram",
     "RCL0194ENG_20_4.png",
     "7a04d84ee0296ab41333650f369aa3624f3ac7988a528584237a7b8fb021bea0"),
    ("39.2", "39.2", 25, "wiring_diagram",
     "RCL0194ENG_39_2.png",
     "e3e4c30e318aca2f9aeeb6984840397ec0bc5432d3c15034e2e68e6b2c0a6d22"),
    ("39.3", "39.3", 26, "wiring_diagram",
     "RCL0194ENG_39_3.png",
     "b8da82236a56034cd3afde692752ae6ccdef2bc792f5dacfe14392ffc0f00188"),
]

EXPECTED_39_1_FACT_KEYS = {
    "RAVE-WIR-MPI-006",
    "RAVE-WIR-MPI-008",
    "RAVE-WIR-MPI-012",
}


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    return sha256_bytes(path.read_bytes())


def qt_uncompress(payload: bytes) -> bytes:
    if len(payload) < 5:
        raise AssertionError("Qt compressed payload too short")
    expected = struct.unpack(">I", payload[:4])[0]
    raw = zlib.decompress(payload[4:])
    if len(raw) != expected:
        raise AssertionError(
            f"Qt qCompress length mismatch: expected {expected}, got {len(raw)}"
        )
    return raw


def decode_qz64(path: Path) -> bytes:
    data = path.read_bytes()
    stripped = data.strip()
    if stripped and re.fullmatch(rb"[A-Za-z0-9+/=\r\n\t ]+", stripped):
        return qt_uncompress(base64.b64decode(stripped))
    return qt_uncompress(data)


def encode_qz64(raw: bytes) -> bytes:
    payload = struct.pack(">I", len(raw)) + zlib.compress(raw, 9)
    return base64.b64encode(payload) + b"\n"


def clean_sql(raw: bytes) -> str:
    lines = []
    for line in raw.decode("utf-8").splitlines():
        if line.strip().startswith("--"):
            continue
        lines.append(line)
    return "\n".join(lines) + "\n"


def execute_project_sql(db: sqlite3.Connection, raw: bytes) -> None:
    sql = clean_sql(raw)
    if ";" in sql:
        db.executescript(sql)
        return
    for line in sql.splitlines():
        stmt = line.strip()
        if stmt:
            db.execute(stmt)


def numeric_suffix(path: Path) -> int:
    match = re.search(r"_(\d+)\.qz64$", path.name, re.I)
    return int(match.group(1)) if match else 0


def rebuild_reference_database(reference: Path, destination: Path) -> None:
    if destination.exists():
        destination.unlink()
    db = sqlite3.connect(str(destination))
    try:
        seeds = sorted(reference.glob("mems_reference_seed_*.qz64"),
                       key=lambda p: (numeric_suffix(p), p.name))
        if not seeds:
            raise AssertionError("No reference seed files found")
        encoded = b"".join(p.read_bytes().strip() for p in seeds)
        execute_project_sql(db, qt_uncompress(base64.b64decode(encoded)))

        manifest = json.loads((reference / "manifest.json").read_text(encoding="utf-8-sig"))
        ordered: list[Path] = []
        seen: set[Path] = set()
        for name in manifest.get("research_enrichment_batches", []):
            p = reference / name
            if p.is_file() and p.name != "research_enrichment_1600.qz64" and numeric_suffix(p) < 1750:
                ordered.append(p)
                seen.add(p.resolve())

        discovered = sorted(reference.rglob("research_enrichment*.qz64"),
                            key=lambda p: (numeric_suffix(p), str(p)))
        for p in discovered:
            if p.name == "research_enrichment_1600.qz64":
                continue
            suffix = numeric_suffix(p)
            if suffix >= 1750:
                continue
            if p.resolve() not in seen:
                ordered.append(p)
                seen.add(p.resolve())

        for p in ordered:
            execute_project_sql(db, decode_qz64(p))

        db.execute(f"PRAGMA user_version={EXPECTED_USER_VERSION}")
        db.commit()
    finally:
        db.close()


def sql_quote(value: str | None) -> str:
    if value is None:
        return "NULL"
    return "'" + value.replace("'", "''") + "'"


def source_pages(text: str) -> list[str]:
    found: list[str] = []
    for page in ("15.1", "20.1", "20.2", "20.3", "20.4", "39.1", "39.2", "39.3"):
        if page in text and page not in found:
            found.append(page)
    return found


def build_batch_sql(baseline: Path) -> bytes:
    db = sqlite3.connect(str(baseline))
    db.row_factory = sqlite3.Row
    try:
        rave_count = db.execute("SELECT COUNT(*) FROM mems_rave_fact").fetchone()[0]
        expert_count = db.execute(
            "SELECT COUNT(*) FROM mems_expert_fact_external"
        ).fetchone()[0]
        user_version = db.execute("PRAGMA user_version").fetchone()[0]
        assert rave_count == EXPECTED_RAVE_FACTS, rave_count
        assert expert_count == EXPECTED_EXPERT_FACTS, expert_count
        assert user_version == EXPECTED_USER_VERSION, user_version

        facts = db.execute(
            "SELECT fact_key, document, image_ref, source_section, topic, notes "
            "FROM mems_rave_fact WHERE document='RCL0194ENG' ORDER BY fact_key"
        ).fetchall()
        assert len(facts) == EXPECTED_RCL0194_FACTS, len(facts)

        lines: list[str] = [
            "-- RCL0194 visual backfill batch 1750",
            "CREATE TABLE IF NOT EXISTS mems_rave_illustration("
            "illustration_key TEXT PRIMARY KEY,"
            "document TEXT NOT NULL,"
            "source_page TEXT NOT NULL,"
            "pdf_index INTEGER,"
            "asset_kind TEXT NOT NULL,"
            "relative_path TEXT NOT NULL UNIQUE,"
            "sha256 TEXT NOT NULL,"
            "notes TEXT);",
            "CREATE TABLE IF NOT EXISTS mems_rave_illustration_link("
            "fact_key TEXT NOT NULL,"
            "illustration_key TEXT NOT NULL,"
            "link_role TEXT NOT NULL DEFAULT 'source',"
            "notes TEXT,"
            "PRIMARY KEY(fact_key,illustration_key,link_role));",
            "CREATE TABLE IF NOT EXISTS mems_rave_source_reference_correction("
            "correction_key TEXT PRIMARY KEY,"
            "fact_key TEXT NOT NULL UNIQUE,"
            "original_image_ref TEXT,"
            "effective_image_ref TEXT NOT NULL,"
            "reason TEXT NOT NULL,"
            "verification_level TEXT NOT NULL,"
            "notes TEXT);",
            "CREATE INDEX IF NOT EXISTS idx_rave_illustration_document_page "
            "ON mems_rave_illustration(document,source_page);",
            "CREATE INDEX IF NOT EXISTS idx_rave_illustration_link_fact "
            "ON mems_rave_illustration_link(fact_key);",
            "CREATE INDEX IF NOT EXISTS idx_rave_illustration_link_asset "
            "ON mems_rave_illustration_link(illustration_key);",
            "CREATE INDEX IF NOT EXISTS idx_rave_source_correction_fact "
            "ON mems_rave_source_reference_correction(fact_key);",
            "DROP VIEW IF EXISTS mems_rave_fact_illustration_view;",
            "CREATE VIEW mems_rave_fact_illustration_view AS "
            "SELECT r.fact_key,r.document,r.variant,r.topic,r.statement,"
            "r.source_section,r.verification_level,r.image_ref AS historical_image_ref,"
            "COALESCE(c.effective_image_ref,r.image_ref) AS effective_image_ref,"
            "l.link_role,i.illustration_key,i.source_page,i.asset_kind,"
            "i.relative_path,i.sha256 "
            "FROM mems_rave_fact r "
            "LEFT JOIN mems_rave_source_reference_correction c ON c.fact_key=r.fact_key "
            "LEFT JOIN mems_rave_illustration_link l ON l.fact_key=r.fact_key "
            "LEFT JOIN mems_rave_illustration i ON i.illustration_key=l.illustration_key;",
        ]

        for key, page, index, kind, filename, digest in ASSETS:
            illustration_key = f"RAVE:RCL0194ENG:{key}"
            notes = "Original Rover RAVE source visual retained additively; documentary only."
            lines.append(
                "INSERT OR REPLACE INTO mems_rave_illustration("
                "illustration_key,document,source_page,pdf_index,asset_kind,relative_path,sha256,notes"
                ") VALUES("
                f"{sql_quote(illustration_key)},'RCL0194ENG',{sql_quote(page)},{index},"
                f"{sql_quote(kind)},{sql_quote('images/rave/'+filename)},"
                f"{sql_quote(digest)},{sql_quote(notes)});"
            )

        corrections: list[str] = []
        unresolved: list[tuple[str, str]] = []
        direct_linked: set[str] = set()

        for row in facts:
            fact_key = str(row["fact_key"])
            evidence_text = " ".join(
                str(row[name] or "") for name in ("image_ref", "source_section", "notes")
            )
            pages = source_pages(evidence_text)
            if "39.1" in pages:
                pages = [p for p in pages if p != "39.1"]
                if "39.2" not in pages:
                    pages.append("39.2")
                corrections.append(fact_key)
                original = str(row["image_ref"] or "")
                lines.append(
                    "INSERT OR REPLACE INTO mems_rave_source_reference_correction("
                    "correction_key,fact_key,original_image_ref,effective_image_ref,reason,"
                    "verification_level,notes) VALUES("
                    f"{sql_quote('RCL0194ENG:39.1->39.2:'+fact_key)},"
                    f"{sql_quote(fact_key)},{sql_quote(original)},'RCL0194ENG:39.2',"
                    "'Factory page 39.1 is HEATER BLOWER; factory page 39.2 is COOLING FAN — MPi.',"
                    "'verifie_constructeur',"
                    "'Historical fact row preserved unchanged; effective source corrected additively.');"
                )

            source_candidates = [p for p in pages if p in {"15.1","20.1","20.2","20.3","20.4","39.2","39.3"}]
            if not source_candidates:
                unresolved.append((fact_key, evidence_text))
                continue

            for page in source_candidates:
                role = "effective_source" if fact_key in corrections and page == "39.2" else "source"
                lines.append(
                    "INSERT OR IGNORE INTO mems_rave_illustration_link("
                    "fact_key,illustration_key,link_role,notes) VALUES("
                    f"{sql_quote(fact_key)},{sql_quote('RAVE:RCL0194ENG:'+page)},"
                    f"{sql_quote(role)},'Source-page visual link derived from historical RAVE provenance.');"
                )
                direct_linked.add(fact_key)

            if fact_key.startswith("RAVE-COLOR-SPIJ-"):
                lines.append(
                    "INSERT OR IGNORE INTO mems_rave_illustration_link("
                    "fact_key,illustration_key,link_role,notes) VALUES("
                    f"{sql_quote(fact_key)},'RAVE:RCL0194ENG:COLOUR_CODES','legend',"
                    "'Rover colour-code legend supporting the wire-colour statement.');"
                )

        if unresolved:
            detail = "\n".join(f"{k}: {v}" for k, v in unresolved)
            raise AssertionError("Unresolved RCL0194 visual provenance:\n" + detail)

        if direct_linked != {str(r["fact_key"]) for r in facts}:
            missing = sorted({str(r["fact_key"]) for r in facts} - direct_linked)
            raise AssertionError(f"Facts without direct source visual: {missing}")

        if set(corrections) != EXPECTED_39_1_FACT_KEYS:
            raise AssertionError(
                f"Unexpected 39.1 correction keys: {sorted(corrections)}"
            )

        return ("\n".join(lines) + "\n").encode("utf-8")
    finally:
        db.close()


def validate_after_apply(database: Path) -> dict[str, int | str]:
    db = sqlite3.connect(str(database))
    try:
        integrity = db.execute("PRAGMA integrity_check").fetchone()[0]
        user_version = db.execute("PRAGMA user_version").fetchone()[0]
        rave = db.execute("SELECT COUNT(*) FROM mems_rave_fact").fetchone()[0]
        expert = db.execute("SELECT COUNT(*) FROM mems_expert_fact_external").fetchone()[0]
        assets = db.execute(
            "SELECT COUNT(*) FROM mems_rave_illustration WHERE document='RCL0194ENG'"
        ).fetchone()[0]
        direct = db.execute(
            "SELECT COUNT(DISTINCT r.fact_key) "
            "FROM mems_rave_fact r "
            "JOIN mems_rave_illustration_link l ON l.fact_key=r.fact_key "
            "WHERE r.document='RCL0194ENG' AND l.link_role IN ('source','effective_source')"
        ).fetchone()[0]
        colors = db.execute(
            "SELECT COUNT(DISTINCT fact_key) FROM mems_rave_illustration_link "
            "WHERE fact_key LIKE 'RAVE-COLOR-SPIJ-%' "
            "AND illustration_key='RAVE:RCL0194ENG:COLOUR_CODES' AND link_role='legend'"
        ).fetchone()[0]
        corrections = db.execute(
            "SELECT COUNT(*) FROM mems_rave_source_reference_correction "
            "WHERE effective_image_ref='RCL0194ENG:39.2'"
        ).fetchone()[0]
        wrong_rewrite = db.execute(
            "SELECT COUNT(*) FROM mems_rave_fact "
            "WHERE fact_key IN ('RAVE-WIR-MPI-006','RAVE-WIR-MPI-008','RAVE-WIR-MPI-012') "
            "AND image_ref NOT LIKE '%39.1%'"
        ).fetchone()[0]

        assert str(integrity).lower() == "ok", integrity
        assert user_version == EXPECTED_USER_VERSION, user_version
        assert rave == EXPECTED_RAVE_FACTS, rave
        assert expert == EXPECTED_EXPERT_FACTS, expert
        assert assets == len(ASSETS), assets
        assert direct == EXPECTED_RCL0194_FACTS, direct
        assert colors == EXPECTED_COLOR_FACTS, colors
        assert corrections == EXPECTED_CORRECTIONS, corrections
        assert wrong_rewrite == 0, wrong_rewrite

        return {
            "integrity": str(integrity),
            "user_version": int(user_version),
            "rave_facts": int(rave),
            "expert_facts": int(expert),
            "rcl0194_assets": int(assets),
            "rcl0194_direct_visual_coverage": int(direct),
            "color_legend_links": int(colors),
            "source_corrections": int(corrections),
        }
    finally:
        db.close()


def render_assets(pdf_path: Path, reference: Path) -> None:
    import fitz

    expected_pdf = "64e64f8a7c24f362913e2661403bc474e4e7ef07f96db618ef661645e0d0f051"
    actual_pdf = sha256_file(pdf_path)
    if actual_pdf != expected_pdf:
        raise AssertionError(f"RCL0194ENG PDF hash mismatch: {actual_pdf}")

    out = reference / "images" / "rave"
    out.mkdir(parents=True, exist_ok=True)
    doc = fitz.open(str(pdf_path))
    assert doc.page_count == 41, doc.page_count
    try:
        for key, page, index, kind, filename, wanted in ASSETS:
            p = doc.load_page(index)
            text = " ".join(p.get_text("text").split()).lower()
            tokens = {
                "COLOUR_CODES": ("colour codes", "black", "green", "slate"),
                "15.1": ("charging", "starting", "mpi"),
                "20.1": ("engine management", "mems", "mpi"),
                "20.2": ("engine management", "mems", "mpi", "map sensor"),
                "20.3": ("engine management", "mems", "spi"),
                "20.4": ("engine management", "mems", "spi"),
                "39.2": ("cooling fan", "mpi"),
                "39.3": ("cooling fan", "spi"),
            }[key]
            for token in tokens:
                if token not in text:
                    raise AssertionError(f"Page token missing: {key} / {token}")
            pix = p.get_pixmap(matrix=fitz.Matrix(2.0, 2.0), alpha=False)
            target = out / filename
            pix.save(target)
            got = sha256_file(target)
            if got != wanted:
                raise AssertionError(f"PNG hash mismatch {filename}: {got}")
            print(f"PNG_PASS {key} index={index} sha256={got} {pix.width}x{pix.height}")
    finally:
        doc.close()


def update_manifest(reference: Path) -> None:
    path = reference / "manifest.json"
    data = json.loads(path.read_text(encoding="utf-8-sig"))
    assert data.get("database_revision") == EXPECTED_USER_VERSION
    batches = data.setdefault("research_enrichment_batches", [])
    if "research_enrichment_1750.qz64" not in batches:
        batches.append("research_enrichment_1750.qz64")

    diagrams = data.setdefault("diagrams", {})
    diagrams.update({
        "RAVE RCL0194ENG COLOUR CODES Rover codes couleurs fils":
            "images/rave/RCL0194ENG_COLOUR_CODES.png",
        "RAVE RCL0194ENG 15.1 Mini MPi 97MY charge demarrage relais MEMS":
            "images/rave/RCL0194ENG_15_1.png",
        "RAVE RCL0194ENG 20.1 Mini MPi 97MY MEMS lambda CKP CMP pompe relais":
            "images/rave/RCL0194ENG_20_1.png",
        "RAVE RCL0194ENG 20.2 Mini MPi 97MY MEMS injecteurs IAC MAP TPS ECT IAT ventilateur":
            "images/rave/RCL0194ENG_20_2.png",
        "RAVE RCL0194ENG 39.2 Mini MPi 97MY ventilateur refroidissement ECT":
            "images/rave/RCL0194ENG_39_2.png",
        "RAVE RCL0194ENG 20.3 Mini SPi Japon 97MY MEMS lambda relais vilebrequin":
            "images/rave/RCL0194ENG_20_3.png",
        "RAVE RCL0194ENG 20.4 Mini SPi Japon 97MY MEMS injecteur IAC TPS ECT IAT":
            "images/rave/RCL0194ENG_20_4.png",
        "RAVE RCL0194ENG 39.3 Mini SPi Japon 97MY ventilateur refroidissement":
            "images/rave/RCL0194ENG_39_3.png",
    })

    visual = data.setdefault("visual_assets", {})
    for key, page, index, kind, filename, digest in ASSETS:
        visual[f"rave:RCL0194ENG:{key}"] = {
            "path": f"images/rave/{filename}",
            "asset_kind": kind,
            "document": "RCL0194ENG",
            "source_page": page,
            "pdf_index": index,
            "sha256": digest,
        }

    path.write_text(
        json.dumps(data, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
        newline="\n",
    )


def write_audit(reference: Path, qz64: Path, sql_raw: bytes, validation: dict[str, int | str]) -> None:
    audit = reference / "audits" / "rcl0194_visual_backfill_1750_audit.md"
    audit.parent.mkdir(parents=True, exist_ok=True)
    qhash = sha256_file(qz64)
    sqlhash = sha256_bytes(sql_raw)
    text = f"""# RCL0194 visual backfill — batch 1750 audit

Date: 2026-08-29

Production remains `MEMSX64` BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`.
This batch is documentary only. It does **not** modify ECU communication, protocol,
acquisition, RAM reads, ECU writes/resets, safety routing, Qwen/ONNX runtime, or
the working 32-bit path. Database/IA/RAVE knowledge remains consultative and can
never take control of ECU communication.

Primary source: Rover `RCL0194ENG`, MINI Electrical Circuit Diagrams, 3rd Edition,
41 PDF pages, SHA-256 `64e64f8a7c24f362913e2661403bc474e4e7ef07f96db618ef661645e0d0f051`.

Eight source visuals are retained once: COLOUR CODES, 15.1, 20.1, 20.2, 20.3,
20.4, 39.2 and 39.3. They supplement structured database facts; they do not
replace structured knowledge.

Important source findings:
- `20.2` directly confirms an external MAP SENSOR (204) on MPi, distinct from
  the integrated MAP arrangement established for SPi.
- Factory `39.1` is HEATER BLOWER.
- Factory `39.2` is COOLING FAN — MPi.
- Historical facts `RAVE-WIR-MPI-006`, `RAVE-WIR-MPI-008`,
  `RAVE-WIR-MPI-012` are preserved unchanged and receive an additive effective
  source correction to `RCL0194ENG:39.2`.

Batch `research_enrichment_1750.qz64` is generated deterministically from the
historical RCL0194 facts reconstructed from the BUILD #101 seed/enrichment set.
It adds only visual-evidence/link/correction structures.

Validation:
- `PRAGMA integrity_check`: `{validation['integrity']}`
- `PRAGMA user_version`: `{validation['user_version']}`
- historical RAVE facts: `{validation['rave_facts']}`
- historical expert facts: `{validation['expert_facts']}`
- RCL0194 visual assets: `{validation['rcl0194_assets']}`
- RCL0194 direct/effective visual coverage: `{validation['rcl0194_direct_visual_coverage']}/{EXPECTED_RCL0194_FACTS}`
- SPi Japan colour facts with Rover colour-code legend: `{validation['color_legend_links']}/{EXPECTED_COLOR_FACTS}`
- explicit historical source corrections: `{validation['source_corrections']}`

Generated qz64:
- bytes: `{qz64.stat().st_size}`
- SHA-256: `{qhash}`
- decompressed SQL bytes: `{len(sql_raw)}`
- SQL SHA-256: `{sqlhash}`
"""
    audit.write_text(text, encoding="utf-8", newline="\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path("."))
    parser.add_argument("--pdf", type=Path, required=True)
    parser.add_argument("--work", type=Path, default=Path(".tmp-rcl0194-1750"))
    args = parser.parse_args()

    root = args.root.resolve()
    reference = root / "database" / "reference"
    work = (root / args.work).resolve() if not args.work.is_absolute() else args.work
    work.mkdir(parents=True, exist_ok=True)

    baseline = work / "baseline.sqlite"
    candidate = work / "candidate.sqlite"

    rebuild_reference_database(reference, baseline)
    sql_raw = build_batch_sql(baseline)
    qz64 = reference / "research_enrichment_1750.qz64"
    qz64.write_bytes(encode_qz64(sql_raw))

    if decode_qz64(qz64) != sql_raw:
        raise AssertionError("Generated qz64 round-trip mismatch")

    shutil.copy2(baseline, candidate)
    db = sqlite3.connect(str(candidate))
    try:
        execute_project_sql(db, decode_qz64(qz64))
        db.execute(f"PRAGMA user_version={EXPECTED_USER_VERSION}")
        db.commit()
    finally:
        db.close()

    validation = validate_after_apply(candidate)
    render_assets(args.pdf.resolve(), reference)
    update_manifest(reference)
    write_audit(reference, qz64, sql_raw, validation)

    manifest = json.loads((reference / "manifest.json").read_text(encoding="utf-8"))
    assert manifest["database_revision"] == EXPECTED_USER_VERSION
    assert "research_enrichment_1750.qz64" in manifest["research_enrichment_batches"]
    rcl = {
        k: v for k, v in manifest.get("visual_assets", {}).items()
        if k.startswith("rave:RCL0194ENG:")
    }
    assert len(rcl) == len(ASSETS), len(rcl)
    for item in rcl.values():
        asset = reference / item["path"]
        assert asset.is_file(), asset
        assert sha256_file(asset) == item["sha256"], asset

    print("RCL0194_1750_PASS")
    for key, value in validation.items():
        print(f"{key}={value}")
    print(f"qz64_sha256={sha256_file(qz64)}")
    print(f"sql_sha256={sha256_bytes(sql_raw)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
