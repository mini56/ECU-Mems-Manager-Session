#!/usr/bin/env python3
import argparse
import base64
import json
import pathlib
import re
import sqlite3
import sys
import tempfile
import xml.etree.ElementTree as ET
import zlib

LANGUAGES = ("fr", "en", "es", "it", "pt", "de")
SUPPORTED_ASSETS = {".svg", ".png", ".jpg", ".jpeg", ".webp", ".gif", ".pdf", ".html", ".htm", ".md", ".txt", ".csv", ".xml"}
LANG_SUFFIX_RE = re.compile(r"^(.+)_(fr|en|es|it|pt|de)$", re.I)

# Columns whose values are intended to be read by the user and therefore
# must either have six language variants or an explicit localization layer.
TRANSLATABLE_HINTS = (
    "description", "note", "cause", "solution", "function", "capability",
    "availability", "access_method", "testing", "subject", "topic", "summary",
    "meaning", "behavior", "behaviour", "warning", "instruction", "comment",
    "component_name", "field_name", "setting_name", "rule_name", "parameter",
    "safety_level", "confidence", "status", "label", "title"
)

# These values are identifiers/proper names/technical data. They are searchable
# in every language but are intentionally not translated.
TECHNICAL_COLUMN_EXACT = {
    "id", "rowid", "part_number", "ecu_part_number", "alt_refs", "brand", "make",
    "makes", "model", "models", "variant", "engine", "market", "year", "year_from",
    "year_to", "system", "system_family", "injection", "mems_version", "mems_versions",
    "command_hex", "packet_command", "response_format", "default_hex", "minimum_hex",
    "maximum_hex", "increment_command", "decrement_command", "byte_offset", "bit_definition",
    "decoding_formula", "formula", "scaling", "unit", "address", "offset", "size", "filename",
    "file_type", "relative_path", "url", "sha", "hash", "source_url", "source_file", "source_ref",
    "source_name", "calibration_id", "firmware", "rom", "keywords", "n_fitments", "n_files",
    "nominal_resistance_ohms", "control_pinout", "pin", "pin_number", "code", "dtc", "generation"
}


def unpack_qz64_bytes(encoded: bytes, label: str) -> bytes:
    packed = base64.b64decode(encoded.strip(), validate=True)
    if len(packed) < 5:
        raise RuntimeError(f"{label}: qz64 trop court")
    expected = int.from_bytes(packed[:4], "big")
    raw = zlib.decompress(packed[4:])
    if len(raw) != expected:
        raise RuntimeError(f"{label}: {len(raw)} octets reconstruits, {expected} attendus")
    return raw


def numeric_suffix(path: pathlib.Path) -> int:
    match = re.search(r"_(\d+)\.qz64$", path.name, re.I)
    return int(match.group(1)) if match else 0


def execute_sql_lines(con: sqlite3.Connection, raw: bytes, label: str) -> None:
    text = raw.decode("utf-8")
    for line_no, statement in enumerate(text.splitlines(), 1):
        statement = statement.strip()
        if not statement or statement.startswith("--"):
            continue
        try:
            con.execute(statement)
        except sqlite3.Error as exc:
            raise RuntimeError(f"{label}:{line_no}: SQL invalide: {exc}\n{statement}") from exc


def is_translatable_column(column: str) -> bool:
    lc = column.lower()
    if lc in TECHNICAL_COLUMN_EXACT:
        return False
    if lc.endswith(("_url", "_path", "_file", "_filename", "_hex", "_bytes", "_byte", "_id")):
        return False
    if LANG_SUFFIX_RE.match(lc):
        return True
    return any(hint in lc for hint in TRANSLATABLE_HINTS)


def looks_technical_text(text: str) -> bool:
    value = text.strip()
    if not value:
        return True
    if re.fullmatch(r"(?:0x)?[0-9A-Fa-f]{1,8}(?:\s+(?:0x)?[0-9A-Fa-f]{1,8})*", value):
        return True
    if re.fullmatch(r"[A-Z0-9_.:/+\-]{1,64}", value) and " " not in value:
        return True
    if re.fullmatch(r"[-+]?\d+(?:[.,]\d+)?(?:\s*(?:V|mV|A|mA|ohm|Ω|ms|s|rpm|°C|%|baud|bps))?", value, re.I):
        return True
    return False


def build_database(root: pathlib.Path) -> pathlib.Path:
    seed_parts = sorted(root.glob("mems_reference_seed_*.qz64"), key=lambda p: (numeric_suffix(p), p.name))
    if not seed_parts:
        raise RuntimeError("Aucun seed MEMS")
    seed_encoded = b"".join(path.read_bytes().strip() for path in seed_parts)
    seed_sql = unpack_qz64_bytes(seed_encoded, "seeds MEMS")

    enrichment_paths = sorted(root.rglob("research_enrichment*.qz64"), key=lambda p: (numeric_suffix(p), p.as_posix()))
    if not enrichment_paths:
        raise RuntimeError("Aucun lot d'enrichissement MEMS")

    smoke = pathlib.Path(tempfile.gettempdir()) / "ecu_mems_reference_full_audit.sqlite"
    if smoke.exists():
        smoke.unlink()
    con = sqlite3.connect(smoke)
    try:
        execute_sql_lines(con, seed_sql, "seeds MEMS")
        for path in enrichment_paths:
            execute_sql_lines(con, unpack_qz64_bytes(path.read_bytes(), path.as_posix()), path.as_posix())
        con.commit()
    finally:
        con.close()
    return smoke


def audit_xml(root: pathlib.Path, violations: list[str]) -> tuple[int, int]:
    files = sorted((root / "fiches").glob("*.xml.qz64"))
    if not files:
        violations.append("XML|Aucune fiche XML MEMS")
        return 0, 0

    human_nodes = 0
    multilingual_nodes = 0
    for path in files:
        try:
            raw = unpack_qz64_bytes(path.read_bytes(), path.as_posix())
            tree = ET.fromstring(raw.decode("utf-8"))
        except Exception as exc:
            violations.append(f"XML_INVALID|{path.name}|{exc}")
            continue

        for element in tree.iter():
            texts = []
            if element.text and element.text.strip():
                texts.append(element.text.strip())
            for key, value in element.attrib.items():
                if key.lower() in {"titre", "title", "description", "fonction", "function", "note", "label"} and value.strip():
                    texts.append(value.strip())
            for text in texts:
                if looks_technical_text(text):
                    continue
                human_nodes += 1
                lang = (element.attrib.get("lang") or element.attrib.get("xml:lang") or "").lower()
                if lang in LANGUAGES:
                    multilingual_nodes += 1
                else:
                    violations.append(f"XML_LANGUAGE|{path.name}|<{element.tag}>|{text[:180]}")
    return human_nodes, multilingual_nodes


def audit_assets(root: pathlib.Path, violations: list[str]) -> int:
    assets = [p for p in root.rglob("*") if p.is_file() and p.suffix.lower() in SUPPORTED_ASSETS and not p.name.endswith(".qz64")]
    for asset in assets:
        if asset.stat().st_size <= 0:
            violations.append(f"ASSET_EMPTY|{asset.relative_to(root).as_posix()}")
    return len(assets)


def audit_sqlite(db_path: pathlib.Path, violations: list[str]) -> dict:
    con = sqlite3.connect(db_path)
    stats = {
        "tables": 0,
        "rows": 0,
        "language_families": 0,
        "incomplete_language_families": 0,
        "legacy_translatable_cells": 0,
        "legacy_translatable_unique": 0,
    }
    unique_legacy = set()
    try:
        integrity = con.execute("PRAGMA integrity_check").fetchone()[0]
        if integrity != "ok":
            violations.append(f"SQLITE_INTEGRITY|{integrity}")

        tables = [r[0] for r in con.execute("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%' ORDER BY name")]
        stats["tables"] = len(tables)
        for table in tables:
            row_count = con.execute(f'SELECT COUNT(*) FROM "{table}"').fetchone()[0]
            stats["rows"] += row_count
            info = con.execute(f'PRAGMA table_info("{table}")').fetchall()
            columns = [r[1] for r in info]
            lower_map = {c.lower(): c for c in columns}

            families = {}
            for column in columns:
                match = LANG_SUFFIX_RE.match(column)
                if match:
                    families.setdefault(match.group(1).lower(), set()).add(match.group(2).lower())

            for base, present in sorted(families.items()):
                stats["language_families"] += 1
                missing_columns = sorted(set(LANGUAGES) - present)
                if missing_columns:
                    stats["incomplete_language_families"] += 1
                    violations.append(f"LANG_COLUMNS|{table}.{base}|missing={','.join(missing_columns)}")
                    continue

                names = [lower_map[f"{base}_{lang}"] for lang in LANGUAGES]
                select_cols = ",".join(f'"{name}"' for name in names)
                for rowid, *values in con.execute(f'SELECT rowid,{select_cols} FROM "{table}"'):
                    populated = [str(v).strip() if v is not None else "" for v in values]
                    if not any(populated):
                        continue
                    missing_values = [LANGUAGES[i] for i, value in enumerate(populated) if not value]
                    if missing_values:
                        violations.append(f"LANG_VALUES|{table}.{base}|rowid={rowid}|missing={','.join(missing_values)}")

            # Unsuffixed user-facing columns are legacy localization debt. They
            # are fully enumerated, not merely guessed by English detection.
            for column in columns:
                lc = column.lower()
                if LANG_SUFFIX_RE.match(lc) or not is_translatable_column(lc):
                    continue
                if f"{lc}_fr" in lower_map:
                    continue
                try:
                    rows = con.execute(f'SELECT rowid,"{column}" FROM "{table}" WHERE "{column}" IS NOT NULL').fetchall()
                except sqlite3.Error:
                    continue
                for rowid, value in rows:
                    text = str(value).strip()
                    if not text or looks_technical_text(text):
                        continue
                    stats["legacy_translatable_cells"] += 1
                    unique_legacy.add((table, lc, text))
                    violations.append(f"LANG_LEGACY|{table}.{column}|rowid={rowid}|{text[:220]}")

        stats["legacy_translatable_unique"] = len(unique_legacy)
    finally:
        con.close()
    return stats


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default="database/reference")
    parser.add_argument("--strict-language", action="store_true", help="Fail on any language violation")
    args = parser.parse_args()

    root = pathlib.Path(args.root)
    violations: list[str] = []
    db_path = None
    try:
        manifest_path = root / "manifest.json"
        if not manifest_path.is_file():
            raise RuntimeError("manifest.json absent")
        json.loads(manifest_path.read_text(encoding="utf-8"))

        db_path = build_database(root)
        stats = audit_sqlite(db_path, violations)
        xml_human, xml_multilingual = audit_xml(root, violations)
        asset_count = audit_assets(root, violations)

        language_violations = [v for v in violations if v.startswith(("LANG_", "XML_LANGUAGE"))]
        structural_violations = [v for v in violations if not v.startswith(("LANG_", "XML_LANGUAGE"))]

        print("AUDIT MEMS REFERENCE DATABASE")
        print(f"SQLITE tables={stats['tables']} rows={stats['rows']}")
        print(f"LANGUAGE families={stats['language_families']} incomplete_families={stats['incomplete_language_families']}")
        print(f"LANGUAGE legacy_cells={stats['legacy_translatable_cells']} unique_values={stats['legacy_translatable_unique']}")
        print(f"XML human_text_nodes={xml_human} explicitly_localized={xml_multilingual}")
        print(f"ASSETS files={asset_count}")
        print(f"VIOLATIONS structural={len(structural_violations)} language={len(language_violations)} total={len(violations)}")

        # Print every structural issue, and a complete language inventory. This
        # log is the authoritative cleanup list until strict mode reaches zero.
        for item in violations:
            print("AUDIT_VIOLATION", item)

        if structural_violations:
            return 2
        if args.strict_language and language_violations:
            return 3
        return 0
    except Exception as exc:
        print(f"AUDIT_FATAL {exc}", file=sys.stderr)
        return 4
    finally:
        if db_path and db_path.exists():
            db_path.unlink()


if __name__ == "__main__":
    raise SystemExit(main())
