#!/usr/bin/env python3
"""Preserve complete Andrew MEMSTools definition/correlation metadata.

The semantic 1500-1540 batches expose the fields ECU MEMS Manager already
understands. This complementary batch keeps every DEF/DIM property and every
non-empty correlation-matrix cell for the in-scope Rover MEMS families, so
future expert-engine work can use information that is not decoded today without
having to re-import the archive.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import io
import json
import re
import zipfile
from pathlib import Path

from build_memstools_enrichment import decode_value, parse_ini_sections, sha256_file, sqlq, add_multi, write_qz64

SOURCE_KEY = "andrew_revill_memstools_complete_metadata"
SOURCE_URL = "https://andrewrevill.co.uk/MEMSToolsIndex.htm"
OUTPUT_NAME = "research_enrichment_1600.qz64"

SUFFIX_FAMILY = {
    "RoverMems13": ("1.3", "standard"),
    "RoverMems16Na": ("1.6", "NA"),
    "RoverMems16Turbo": ("1.6", "Turbo"),
    "RoverMems19": ("1.9", "standard"),
}


def infer_scope(section: str):
    if section.startswith("AxisClass."):
        return "axis_class"
    if section.startswith("ScalarClass."):
        return "scalar_class"
    if section.startswith("TableClass."):
        return "table_class"
    if section.startswith("Scalar."):
        return "scalar_binding"
    if section.startswith("Table."):
        return "table_binding"
    if section.startswith("Firmware."):
        return "firmware"
    return "other"


def family_from_firmware(firmware, firmware_family):
    return firmware_family.get(firmware, (None, None))[0]


def build(zip_path: Path, output_dir: Path):
    archive_sha = sha256_file(zip_path)
    with zipfile.ZipFile(zip_path) as zf:
        definition_path = "Executables/MemsMapper.RoverMems19.def"
        dimension_path = "Executables/MemsMapper.RoverMems19.dim"
        definition_text = zf.read(definition_path).decode("utf-8-sig", errors="replace")
        dimension_text = zf.read(dimension_path).decode("utf-8-sig", errors="replace")

        firmware_family = {}
        for suffix, (family, variant) in SUFFIX_FAMILY.items():
            name = f"Executables/Correlations/Firmware.{suffix}.csv"
            rows = list(csv.reader(io.StringIO(zf.read(name).decode("utf-8-sig", errors="replace"))))
            if rows:
                for fw in rows[0][2:]:
                    if fw:
                        firmware_family[fw] = (family, variant)

        property_rows = []
        for section, props in parse_ini_sections(definition_text):
            scope = infer_scope(section)
            firmware = None
            family = None
            if scope in ("scalar_binding", "table_binding"):
                rest = section.split(".", 1)[1]
                firmware = rest.split(".", 1)[0] if "." in rest else None
                family = family_from_firmware(firmware, firmware_family)
            elif scope == "firmware":
                firmware = section.split(".", 1)[1] if "." in section else None
                family = family_from_firmware(firmware, firmware_family)
            for key, value in props.items():
                property_rows.append((
                    SOURCE_KEY, section, scope, family, firmware, key,
                    decode_value(value), value
                ))

        dimension_rows = []
        for raw_line in dimension_text.splitlines():
            if "=" not in raw_line:
                continue
            key, value = raw_line.split("=", 1)
            key = key.strip()
            value = value.strip()
            scope = "other"
            if key.startswith("TableClass."):
                scope = "table_class"
            elif key.startswith("Table."):
                scope = "table_binding"
            dimension_rows.append((SOURCE_KEY, key, scope, value))

        dataset_rows = []
        cell_rows = []
        correlation_rx = re.compile(r"^Executables/Correlations/([^/]+)\.([^./]+)\.csv$", re.I)
        for name in sorted(zf.namelist()):
            match = correlation_rx.match(name)
            if not match:
                continue
            dataset, suffix = match.group(1), match.group(2)
            if suffix not in SUFFIX_FAMILY:
                continue
            family, variant = SUFFIX_FAMILY[suffix]
            data = zf.read(name)
            rows = list(csv.reader(io.StringIO(data.decode("utf-8-sig", errors="replace"))))
            if not rows:
                continue
            header = rows[0]
            non_empty = 0
            for row_index, row in enumerate(rows[1:], 1):
                row_key = row[0] if row else ""
                row_meta = row[1] if len(row) > 1 else ""
                for column_index, column_name in enumerate(header):
                    if column_index >= len(row):
                        continue
                    value = row[column_index]
                    if value == "":
                        continue
                    non_empty += 1
                    cell_rows.append((
                        SOURCE_KEY, family, variant, dataset, suffix, name,
                        row_index, row_key, row_meta, column_index, column_name,
                        value
                    ))
            dataset_rows.append((
                SOURCE_KEY, family, variant, dataset, suffix, name,
                len(rows) - 1, len(header), non_empty,
                hashlib.sha256(data).hexdigest(), json.dumps(header, ensure_ascii=False)
            ))

        lines = ["BEGIN TRANSACTION;"]
        lines.extend([
            "CREATE TABLE IF NOT EXISTS mems_knowledge_source(source_key TEXT PRIMARY KEY,author TEXT,title TEXT,source_url TEXT,archive_sha256 TEXT,verification_level TEXT,scope TEXT,notes TEXT);",
            "CREATE TABLE IF NOT EXISTS mems_definition_property_external(source_key TEXT NOT NULL,section TEXT NOT NULL,section_scope TEXT,family TEXT,firmware_code TEXT,property_key TEXT NOT NULL,value_decoded TEXT,value_raw TEXT,PRIMARY KEY(source_key,section,property_key));",
            "CREATE TABLE IF NOT EXISTS mems_dimension_property_external(source_key TEXT NOT NULL,property_key TEXT NOT NULL,property_scope TEXT,value_raw TEXT,PRIMARY KEY(source_key,property_key));",
            "CREATE TABLE IF NOT EXISTS mems_correlation_dataset_external(source_key TEXT NOT NULL,family TEXT NOT NULL,variant TEXT,dataset TEXT NOT NULL,suffix TEXT NOT NULL,relative_path TEXT NOT NULL,row_count INTEGER,column_count INTEGER,non_empty_cells INTEGER,sha256 TEXT,header_json TEXT,PRIMARY KEY(source_key,relative_path));",
            "CREATE TABLE IF NOT EXISTS mems_correlation_cell_external(source_key TEXT NOT NULL,family TEXT NOT NULL,variant TEXT,dataset TEXT NOT NULL,suffix TEXT NOT NULL,relative_path TEXT NOT NULL,row_index INTEGER NOT NULL,row_key TEXT,row_meta TEXT,column_index INTEGER NOT NULL,column_name TEXT,value_text TEXT NOT NULL,PRIMARY KEY(source_key,relative_path,row_index,column_index));",
            "CREATE INDEX IF NOT EXISTS idx_mems_def_prop_section ON mems_definition_property_external(section_scope,family,firmware_code);",
            "CREATE INDEX IF NOT EXISTS idx_mems_corr_dataset ON mems_correlation_dataset_external(family,dataset);",
            "CREATE INDEX IF NOT EXISTS idx_mems_corr_cell_fw ON mems_correlation_cell_external(family,dataset,column_name);",
        ])
        for table in ("mems_definition_property_external", "mems_dimension_property_external", "mems_correlation_dataset_external", "mems_correlation_cell_external"):
            lines.append(f"DELETE FROM {table} WHERE source_key={sqlq(SOURCE_KEY)};")
        lines.append(f"DELETE FROM mems_knowledge_source WHERE source_key={sqlq(SOURCE_KEY)};")
        source = (
            SOURCE_KEY, "Andrew Revill", "MEMSTools complete DEF/DIM/correlation metadata",
            SOURCE_URL, archive_sha, "source_externe", "MEMS 1.3 / 1.6 / 1.9",
            "Loss-preserving structured copy of all properties in MemsMapper.RoverMems19.def/.dim and all non-empty cells in in-scope correlation CSVs. Semantic tables remain authoritative for understood fields."
        )
        lines.append("INSERT INTO mems_knowledge_source VALUES(" + ",".join(sqlq(v) for v in source) + ");")
        add_multi(lines, "mems_definition_property_external",
                  ["source_key","section","section_scope","family","firmware_code","property_key","value_decoded","value_raw"], property_rows, batch=250)
        add_multi(lines, "mems_dimension_property_external",
                  ["source_key","property_key","property_scope","value_raw"], dimension_rows, batch=300)
        add_multi(lines, "mems_correlation_dataset_external",
                  ["source_key","family","variant","dataset","suffix","relative_path","row_count","column_count","non_empty_cells","sha256","header_json"], dataset_rows, batch=100)
        add_multi(lines, "mems_correlation_cell_external",
                  ["source_key","family","variant","dataset","suffix","relative_path","row_index","row_key","row_meta","column_index","column_name","value_text"], cell_rows, batch=300)
        lines.append("COMMIT;")
        output = output_dir / OUTPUT_NAME
        write_qz64(output, lines)
        print(f"output={output}")
        print(f"definition_properties={len(property_rows)}")
        print(f"dimension_properties={len(dimension_rows)}")
        print(f"correlation_datasets={len(dataset_rows)}")
        print(f"correlation_non_empty_cells={len(cell_rows)}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("memstools_zip", type=Path)
    ap.add_argument("--output-dir", type=Path, default=Path("database/reference"))
    args = ap.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    build(args.memstools_zip, args.output_dir)


if __name__ == "__main__":
    main()
