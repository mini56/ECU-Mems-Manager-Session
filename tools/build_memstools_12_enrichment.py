#!/usr/bin/env python3
"""Extract Rover MEMS 1.2 knowledge from Andrew Revill's current MEMSTools ZIP.

This is intentionally separate from the already validated 1.3/1.6/1.9 import.
It discovers the 1.2 correlation file names from the archive instead of assuming
that a fixed RoverMems12 layout exists forever. If semantic correlations are not
present, it still inventories every 1.2 source asset so the gap is explicit and
traceable rather than silently invented.
"""
from __future__ import annotations

import argparse
import csv
import io
import os
import re
import zipfile
from pathlib import Path

from build_memstools_enrichment import (
    add_multi,
    address_int,
    as_int,
    as_num,
    decode_value,
    parse_ini_sections,
    sha256_bytes,
    sha256_file,
    sqlq,
    write_qz64,
)

SOURCE_KEY = "andrew_revill_memstools_mems12"
SOURCE_URL = "https://andrewrevill.co.uk/MEMS3FamiliesSupported.htm"
OUTPUT_NAME = "research_enrichment_1560.qz64"


def lower_map(zf: zipfile.ZipFile):
    return {name.lower(): name for name in zf.namelist()}


def find_name(zf: zipfile.ZipFile, wanted: str):
    return lower_map(zf).get(wanted.lower())


def discover_suffixes(zf: zipfile.ZipFile):
    found = []
    rx = re.compile(r"^Executables/Correlations/Firmware\.([^/]+)\.csv$", re.I)
    for name in zf.namelist():
        match = rx.match(name)
        if not match:
            continue
        suffix = match.group(1)
        compact = re.sub(r"[^a-z0-9]", "", suffix.lower())
        if "rovermems12" in compact or compact.endswith("mems12"):
            found.append((suffix, name))
    return sorted(set(found), key=lambda item: item[0].lower())


def looks_like_12_asset(name: str, suffixes):
    lower = name.lower()
    compact = re.sub(r"[^a-z0-9]", "", lower)
    if any(suffix.lower() in lower for suffix, _ in suffixes):
        return True
    tokens = (
        "rover mems 1.2",
        "rover_mems_1_2",
        "rover-mems-1-2",
        "mems 1.2",
        "mems_1_2",
        "mems1.2",
        "rovermems12",
    )
    return any(token in lower for token in tokens) or "rovermems12" in compact


def asset_kind(name: str):
    lower = name.lower()
    if "disassembl" in lower:
        return "disassembly"
    if "correlations/" in lower:
        return "correlation"
    if "firmware" in lower or "firmwares/" in lower:
        return "firmware_image"
    if "/help/" in lower or lower.startswith("help/"):
        return "help_asset"
    if lower.endswith((".def", ".dim")):
        return "definition"
    return "source_asset"


def build(zip_path: Path, output_dir: Path):
    archive_sha = sha256_file(zip_path)
    with zipfile.ZipFile(zip_path) as zf:
        suffixes = discover_suffixes(zf)
        fw_meta = {}
        for suffix, firmware_path in suffixes:
            rows = list(csv.reader(io.StringIO(zf.read(firmware_path).decode("utf-8-sig", errors="replace"))))
            if not rows:
                continue
            for firmware in rows[0][2:]:
                firmware = firmware.strip()
                if firmware:
                    fw_meta[firmware] = {"family": "1.2", "variant": "standard", "suffix": suffix}

        relevant_fw = set(fw_meta)
        def_path = find_name(zf, "Executables/MemsMapper.RoverMems19.def")
        dim_path = find_name(zf, "Executables/MemsMapper.RoverMems19.dim")
        definition_text = zf.read(def_path).decode("utf-8-sig", errors="replace") if def_path else ""
        dimension_text = zf.read(dim_path).decode("utf-8-sig", errors="replace") if dim_path else ""
        sections = parse_ini_sections(definition_text)
        section_map = dict(sections)

        dimensions = {}
        for line in dimension_text.splitlines():
            if "=" not in line:
                continue
            key, value = line.split("=", 1)
            if key.startswith(("TableClass.", "Table.")) and key.endswith((".X", ".Y")):
                try:
                    dimensions[key] = int(value)
                except ValueError:
                    pass

        scalar_bindings = []
        table_bindings = []
        scalar_classes = set()
        table_classes = set()
        for section, props in sections:
            if section.startswith("Scalar."):
                rest = section[len("Scalar."):]
                if "." not in rest:
                    continue
                firmware, address = rest.split(".", 1)
                if firmware not in relevant_fw:
                    continue
                class_key = props.get("ScalarClass")
                if class_key:
                    scalar_classes.add(class_key)
                    scalar_bindings.append((firmware, address, class_key, as_int(props.get("Correlated"))))
            elif section.startswith("Table."):
                rest = section[len("Table."):]
                if "." not in rest:
                    continue
                firmware, index = rest.split(".", 1)
                if firmware not in relevant_fw:
                    continue
                class_key = props.get("TableClass")
                if class_key:
                    table_classes.add(class_key)
                    table_bindings.append((firmware, index, class_key, as_int(props.get("Correlated"))))

        axis_classes = set()
        for class_key in scalar_classes:
            props = section_map.get("ScalarClass." + class_key, {})
            axis = props.get("AxisClass")
            if axis:
                axis_classes.add(axis)
        for class_key in table_classes:
            props = section_map.get("TableClass." + class_key, {})
            for key in ("XAxis.AxisClass", "YAxis.AxisClass", "ZAxis.AxisClass"):
                axis = props.get(key)
                if axis:
                    axis_classes.add(axis)

        variable_rows = []
        for suffix, _ in suffixes:
            variable_name = find_name(zf, f"Executables/Correlations/Variables.{suffix}.csv")
            if not variable_name:
                continue
            rows = list(csv.reader(io.StringIO(zf.read(variable_name).decode("utf-8-sig", errors="replace"))))
            if not rows:
                continue
            header = rows[0]
            for row in rows[1:]:
                if not row or not row[0]:
                    continue
                class_key = row[0]
                member_count = as_int(row[1] if len(row) > 1 else 0)
                for index, firmware in enumerate(header[2:], 2):
                    if firmware not in relevant_fw or index >= len(row) or not row[index]:
                        continue
                    variable_rows.append((
                        SOURCE_KEY, "1.2", class_key, firmware, row[index],
                        address_int(row[index]), member_count
                    ))

        asset_rows = []
        for name in zf.namelist():
            if name.endswith("/") or not looks_like_12_asset(name, suffixes):
                continue
            data = zf.read(name)
            stem = os.path.splitext(os.path.basename(name))[0].upper()
            firmware = stem if stem in relevant_fw else None
            correlated = None
            if "disassembl" in name.lower():
                correlated = 1 if "/correlated/" in name.lower() else 0
            asset_rows.append((
                SOURCE_KEY, name, asset_kind(name), "1.2", firmware,
                len(data), sha256_bytes(data), correlated
            ))

        lines = ["BEGIN TRANSACTION;"]
        lines.extend([
            "CREATE TABLE IF NOT EXISTS mems_knowledge_source(source_key TEXT PRIMARY KEY,author TEXT,title TEXT,source_url TEXT,archive_sha256 TEXT,verification_level TEXT,scope TEXT,notes TEXT);",
            "CREATE TABLE IF NOT EXISTS mems_firmware_catalog(source_key TEXT NOT NULL,family TEXT NOT NULL,variant TEXT,firmware_code TEXT NOT NULL,correlated INTEGER NOT NULL DEFAULT 0,PRIMARY KEY(source_key,firmware_code));",
            "CREATE TABLE IF NOT EXISTS mems_axis_definition(source_key TEXT NOT NULL,class_key TEXT NOT NULL,name TEXT,suffix TEXT,width INTEGER,decimals INTEGER,scale REAL,offset REAL,signed INTEGER,manifold_pressure INTEGER,PRIMARY KEY(source_key,class_key));",
            "CREATE TABLE IF NOT EXISTS mems_scalar_definition(source_key TEXT NOT NULL,class_key TEXT NOT NULL,identifier TEXT,name TEXT,axis_class TEXT,rover_comments TEXT,peta_hidden INTEGER,peta_updated INTEGER,manifold_pressure INTEGER,edited TEXT,PRIMARY KEY(source_key,class_key));",
            "CREATE TABLE IF NOT EXISTS mems_scalar_binding(source_key TEXT NOT NULL,firmware_code TEXT NOT NULL,address_hex TEXT NOT NULL,address_int INTEGER,class_key TEXT NOT NULL,correlated INTEGER NOT NULL DEFAULT 0,PRIMARY KEY(source_key,firmware_code,address_hex));",
            "CREATE TABLE IF NOT EXISTS mems_table_definition(source_key TEXT NOT NULL,class_key TEXT NOT NULL,identifier TEXT,name TEXT,x_axis_class TEXT,y_axis_class TEXT,z_axis_class TEXT,x_count INTEGER,y_count INTEGER,peta_hidden INTEGER,peta_updated INTEGER,edited TEXT,PRIMARY KEY(source_key,class_key));",
            "CREATE TABLE IF NOT EXISTS mems_table_binding(source_key TEXT NOT NULL,firmware_code TEXT NOT NULL,table_index INTEGER NOT NULL,class_key TEXT NOT NULL,correlated INTEGER NOT NULL DEFAULT 0,x_count INTEGER,y_count INTEGER,PRIMARY KEY(source_key,firmware_code,table_index));",
            "CREATE TABLE IF NOT EXISTS mems_variable_correlation(source_key TEXT NOT NULL,family TEXT NOT NULL,class_key TEXT NOT NULL,firmware_code TEXT NOT NULL,address_hex TEXT NOT NULL,address_int INTEGER,class_member_count INTEGER,PRIMARY KEY(source_key,family,class_key,firmware_code));",
            "CREATE TABLE IF NOT EXISTS mems_source_asset(source_key TEXT NOT NULL,relative_path TEXT NOT NULL,file_kind TEXT,family TEXT,firmware_code TEXT,size_bytes INTEGER,sha256 TEXT,correlated INTEGER,PRIMARY KEY(source_key,relative_path));",
        ])
        for table in (
            "mems_firmware_catalog", "mems_axis_definition", "mems_scalar_definition",
            "mems_scalar_binding", "mems_table_definition", "mems_table_binding",
            "mems_variable_correlation", "mems_source_asset"
        ):
            lines.append(f"DELETE FROM {table} WHERE source_key={sqlq(SOURCE_KEY)};")
        lines.append(f"DELETE FROM mems_knowledge_source WHERE source_key={sqlq(SOURCE_KEY)};")
        notes = (
            "Dedicated MEMS 1.2 extraction from the current Andrew Revill MEMSTools archive. "
            "No unsupported semantic fact is fabricated: if the ZIP exposes only assets and no correlation CSV, "
            "the database records that partial coverage explicitly."
        )
        source_row = (
            SOURCE_KEY, "Andrew Revill", "MEMSTools – Rover MEMS 1.2 corpus",
            SOURCE_URL, archive_sha, "source_externe", "MEMS 1.2", notes
        )
        lines.append("INSERT INTO mems_knowledge_source VALUES(" + ",".join(sqlq(v) for v in source_row) + ");")

        firmware_rows = []
        for firmware in sorted(relevant_fw):
            props = section_map.get("Firmware." + firmware, {})
            firmware_rows.append((SOURCE_KEY, "1.2", "standard", firmware, as_int(props.get("Correlated"))))
        add_multi(lines, "mems_firmware_catalog",
                  ["source_key","family","variant","firmware_code","correlated"], firmware_rows)

        axis_rows = []
        for class_key in sorted(axis_classes):
            props = section_map.get("AxisClass." + class_key, {})
            axis_rows.append((
                SOURCE_KEY, class_key, decode_value(props.get("Name")), decode_value(props.get("Suffix")),
                as_num(props.get("Width"), int), as_num(props.get("Decimals"), int), as_num(props.get("Scale")),
                as_num(props.get("Offset")), as_num(props.get("Signed"), int), as_num(props.get("ManifoldPressure"), int)
            ))
        add_multi(lines, "mems_axis_definition",
                  ["source_key","class_key","name","suffix","width","decimals","scale","offset","signed","manifold_pressure"], axis_rows)

        scalar_def_rows = []
        for class_key in sorted(scalar_classes):
            props = section_map.get("ScalarClass." + class_key, {})
            scalar_def_rows.append((
                SOURCE_KEY, class_key, decode_value(props.get("Identifier")), decode_value(props.get("Name")),
                props.get("AxisClass"), decode_value(props.get("RoverComments")), as_int(props.get("PetaHidden")),
                as_int(props.get("PetaUpdated")), as_int(props.get("ManifoldPressure")), props.get("Edited")
            ))
        add_multi(lines, "mems_scalar_definition",
                  ["source_key","class_key","identifier","name","axis_class","rover_comments","peta_hidden","peta_updated","manifold_pressure","edited"], scalar_def_rows, batch=180)

        scalar_rows = [
            (SOURCE_KEY, fw, address, address_int(address), class_key, correlated)
            for fw, address, class_key, correlated in scalar_bindings
        ]
        add_multi(lines, "mems_scalar_binding",
                  ["source_key","firmware_code","address_hex","address_int","class_key","correlated"], scalar_rows, batch=300)

        table_def_rows = []
        for class_key in sorted(table_classes):
            props = section_map.get("TableClass." + class_key, {})
            table_def_rows.append((
                SOURCE_KEY, class_key, decode_value(props.get("Identifier")), decode_value(props.get("Name")),
                props.get("XAxis.AxisClass"), props.get("YAxis.AxisClass"), props.get("ZAxis.AxisClass"),
                dimensions.get("TableClass." + class_key + ".X"), dimensions.get("TableClass." + class_key + ".Y"),
                as_int(props.get("PetaHidden")), as_int(props.get("PetaUpdated")), props.get("Edited")
            ))
        add_multi(lines, "mems_table_definition",
                  ["source_key","class_key","identifier","name","x_axis_class","y_axis_class","z_axis_class","x_count","y_count","peta_hidden","peta_updated","edited"], table_def_rows, batch=180)

        table_rows = []
        for fw, index, class_key, correlated in table_bindings:
            try:
                table_index = int(index)
            except ValueError:
                continue
            table_rows.append((
                SOURCE_KEY, fw, table_index, class_key, correlated,
                dimensions.get(f"Table.{fw}.{index}.X"), dimensions.get(f"Table.{fw}.{index}.Y")
            ))
        add_multi(lines, "mems_table_binding",
                  ["source_key","firmware_code","table_index","class_key","correlated","x_count","y_count"], table_rows, batch=300)

        add_multi(lines, "mems_variable_correlation",
                  ["source_key","family","class_key","firmware_code","address_hex","address_int","class_member_count"], variable_rows, batch=300)
        add_multi(lines, "mems_source_asset",
                  ["source_key","relative_path","file_kind","family","firmware_code","size_bytes","sha256","correlated"], asset_rows, batch=120)
        lines.append("COMMIT;")
        output = output_dir / OUTPUT_NAME
        write_qz64(output, lines)
        return {
            "archive_sha256": archive_sha,
            "discovered_suffixes": [suffix for suffix, _ in suffixes],
            "firmwares_12": len(relevant_fw),
            "axis_classes_12": len(axis_classes),
            "scalar_classes_12": len(scalar_classes),
            "scalar_bindings_12": len(scalar_bindings),
            "table_classes_12": len(table_classes),
            "table_bindings_12": len(table_rows),
            "variable_bindings_12": len(variable_rows),
            "source_assets_12": len(asset_rows),
            "output": str(output),
        }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("memstools_zip", type=Path)
    parser.add_argument("--output-dir", type=Path, default=Path("database/reference"))
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    stats = build(args.memstools_zip, args.output_dir)
    for key, value in stats.items():
        print(f"{key}={value}")


if __name__ == "__main__":
    main()
