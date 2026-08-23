#!/usr/bin/env python3
"""
Build deterministic ECU MEMS Manager reference enrichments from Andrew Revill's
MEMSTools archive.

The generated .qz64 files are text files containing:
    base64(qCompress(UTF-8 SQL))
where qCompress uses the Qt 4-byte big-endian uncompressed-size prefix followed
by a zlib stream.

Scope intentionally restricted to ECU MEMS Manager: Rover MEMS 1.3, 1.6 and
1.9. MEMSTools does not expose a RoverMems12 correlation/definition dataset in
the audited archive, so no MEMS 1.2 facts are invented.
"""
from __future__ import annotations
import argparse, base64, csv, hashlib, io, os, re, struct, urllib.parse, zipfile, zlib
from pathlib import Path

SOURCE_KEY = "andrew_revill_memstools"
SOURCE_URL = "https://andrewrevill.co.uk/MEMSToolsIndex.htm"

FAMILY_FILES = {
    "1.3": ("RoverMems13", "standard"),
    "1.6 NA": ("RoverMems16Na", "NA"),
    "1.6 Turbo": ("RoverMems16Turbo", "Turbo"),
    "1.9": ("RoverMems19", "standard"),
}

def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()

def parse_ini_sections(text: str):
    result, section, props = [], None, {}
    for raw in text.splitlines():
        line = raw.rstrip("\r\n")
        if line.startswith("[") and line.endswith("]"):
            if section is not None:
                result.append((section, props))
            section, props = line[1:-1], {}
        elif section is not None and "=" in line:
            key, value = line.split("=", 1)
            props[key] = value
    if section is not None:
        result.append((section, props))
    return result

def decode_value(value):
    if value is None:
        return None
    text = urllib.parse.unquote(value)
    return " ".join(text.replace("\r", " ").replace("\n", " ").split())

def sqlq(value):
    if value is None:
        return "NULL"
    if isinstance(value, bool):
        return "1" if value else "0"
    if isinstance(value, (int, float)):
        return str(value)
    return "'" + str(value).replace("'", "''") + "'"

def as_int(value, default=0):
    try:
        return int(value)
    except (TypeError, ValueError):
        return default

def as_num(value, type_=float):
    if value in (None, ""):
        return None
    try:
        return type_(value)
    except (TypeError, ValueError):
        return None

def address_int(value):
    if not value:
        return None
    value = value.strip()
    try:
        return int(value[1:], 16) if value.startswith("$") else int(value, 0)
    except ValueError:
        return None

def add_multi(lines, table, columns, rows, batch=250):
    for pos in range(0, len(rows), batch):
        chunk = rows[pos:pos + batch]
        values = ["(" + ",".join(sqlq(v) for v in row) + ")" for row in chunk]
        lines.append(
            "INSERT INTO %s(%s) VALUES%s;"
            % (table, ",".join(columns), ",".join(values))
        )

def qz64(sql: str) -> str:
    raw = sql.encode("utf-8")
    qcompressed = struct.pack(">I", len(raw)) + zlib.compress(raw, 9)
    return base64.b64encode(qcompressed).decode("ascii")

def write_qz64(path: Path, lines):
    sql = "\n".join(lines) + "\n"
    path.write_text(qz64(sql), encoding="ascii")

def family_catalog(zf: zipfile.ZipFile):
    metadata = {}
    for label, (suffix, variant) in FAMILY_FILES.items():
        path = f"Executables/Correlations/Firmware.{suffix}.csv"
        header = next(csv.reader(io.StringIO(zf.read(path).decode("utf-8-sig", errors="replace"))))
        family = "1.6" if label.startswith("1.6") else label
        for firmware in header[2:]:
            metadata[firmware] = {"family": family, "variant": variant, "suffix": suffix}
    return metadata

def build(zip_path: Path, output_dir: Path):
    archive_sha = sha256_file(zip_path)
    with zipfile.ZipFile(zip_path) as zf:
        fw_meta = family_catalog(zf)
        relevant_fw = set(fw_meta)
        definition_text = zf.read("Executables/MemsMapper.RoverMems19.def").decode(
            "utf-8-sig", errors="replace"
        )
        dimension_text = zf.read("Executables/MemsMapper.RoverMems19.dim").decode(
            "utf-8-sig", errors="replace"
        )
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

        scalar_bindings, table_bindings = [], []
        scalar_classes, table_classes = set(), set()
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
                    scalar_bindings.append(
                        (firmware, address, class_key, as_int(props.get("Correlated")))
                    )
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
                    table_bindings.append(
                        (firmware, index, class_key, as_int(props.get("Correlated")))
                    )

        axis_classes = set()
        for class_key in scalar_classes:
            props = section_map.get("ScalarClass." + class_key, {})
            if props.get("AxisClass"):
                axis_classes.add(props["AxisClass"])
        for class_key in table_classes:
            props = section_map.get("TableClass." + class_key, {})
            for key in ("XAxis.AxisClass", "YAxis.AxisClass", "ZAxis.AxisClass"):
                if props.get(key):
                    axis_classes.add(props[key])

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
            "CREATE INDEX IF NOT EXISTS idx_mems_fw_family ON mems_firmware_catalog(family,firmware_code);",
            "CREATE INDEX IF NOT EXISTS idx_mems_scalar_fw_addr ON mems_scalar_binding(firmware_code,address_int);",
            "CREATE INDEX IF NOT EXISTS idx_mems_scalar_identifier ON mems_scalar_definition(identifier);",
            "CREATE INDEX IF NOT EXISTS idx_mems_table_fw_idx ON mems_table_binding(firmware_code,table_index);",
            "CREATE INDEX IF NOT EXISTS idx_mems_table_identifier ON mems_table_definition(identifier);",
            "CREATE INDEX IF NOT EXISTS idx_mems_variable_fw_addr ON mems_variable_correlation(firmware_code,address_int);",
        ])
        for table in (
            "mems_firmware_catalog", "mems_axis_definition", "mems_scalar_definition",
            "mems_scalar_binding", "mems_table_definition", "mems_table_binding",
            "mems_variable_correlation", "mems_source_asset"
        ):
            lines.append(f"DELETE FROM {table} WHERE source_key={sqlq(SOURCE_KEY)};")
        lines.append(f"DELETE FROM mems_knowledge_source WHERE source_key={sqlq(SOURCE_KEY)};")
        source_row = (
            SOURCE_KEY, "Andrew Revill", "MEMSTools – Rover MEMS knowledge corpus",
            SOURCE_URL, archive_sha, "source_externe", "MEMS 1.3 / 1.6 / 1.9",
            "Structured extraction from the supplied MEMSTools archive. Reverse-engineered external source; preserve provenance and validate safety-critical conclusions against project or vehicle evidence."
        )
        lines.append("INSERT INTO mems_knowledge_source VALUES(" + ",".join(sqlq(v) for v in source_row) + ");")

        firmware_rows = []
        for firmware in sorted(relevant_fw):
            props = section_map.get("Firmware." + firmware, {})
            meta = fw_meta[firmware]
            firmware_rows.append((
                SOURCE_KEY, meta["family"], meta["variant"], firmware,
                as_int(props.get("Correlated"))
            ))
        add_multi(lines, "mems_firmware_catalog",
                  ["source_key","family","variant","firmware_code","correlated"],
                  firmware_rows)

        axis_rows = []
        for class_key in sorted(axis_classes):
            props = section_map.get("AxisClass." + class_key, {})
            axis_rows.append((
                SOURCE_KEY, class_key, decode_value(props.get("Name")),
                decode_value(props.get("Suffix")), as_num(props.get("Width"), int),
                as_num(props.get("Decimals"), int), as_num(props.get("Scale")),
                as_num(props.get("Offset")), as_num(props.get("Signed"), int),
                as_num(props.get("ManifoldPressure"), int)
            ))
        add_multi(lines, "mems_axis_definition",
                  ["source_key","class_key","name","suffix","width","decimals","scale","offset","signed","manifold_pressure"],
                  axis_rows)

        asset_rows = []
        for name in zf.namelist():
            if name.endswith("/"):
                continue
            lower = name.lower()
            family = firmware = kind = None
            correlated = None
            if "disassemblies/rover mems 1.3/" in lower:
                family, kind = "1.3", "disassembly"
                correlated = 1 if "/correlated/" in lower else 0
            elif "disassemblies/rover mems 1.6/" in lower:
                family, kind = "1.6", "disassembly"
                correlated = 1 if "/correlated/" in lower else 0
            elif "disassemblies/rover mems 1.9/" in lower:
                family, kind = "1.9", "disassembly"
                correlated = 1 if "/correlated/" in lower else 0
            elif "correlations/" in lower and any(
                token in lower for token in ("rovermems13","rovermems16na","rovermems16turbo","rovermems19")
            ):
                kind = "correlation"
                family = "1.3" if "rovermems13" in lower else ("1.9" if "rovermems19" in lower else "1.6")
            elif name in (
                "Executables/MemsMapper.RoverMems19.def",
                "Executables/MemsMapper.RoverMems19.dim"
            ):
                kind, family = "definition", "1.3/1.6/1.9"
            elif re.search(r"(?i)Firmwares/Rover MEMS 1\.(3|6|9)", name):
                kind = "firmware_image"
                match = re.search(r"Rover MEMS 1\.(3|6|9)", name, re.I)
                family = "1." + match.group(1) if match else None
            elif "help/" in lower and ("rovermems16" in lower or "rovermems19" in lower):
                kind = "help_asset"
                family = "1.6" if "rovermems16" in lower else "1.9"
            if not kind:
                continue
            stem = os.path.splitext(os.path.basename(name))[0].upper()
            if stem in relevant_fw:
                firmware = stem
            data = zf.read(name)
            asset_rows.append((
                SOURCE_KEY, name, kind, family, firmware, len(data),
                sha256_bytes(data), correlated
            ))
        add_multi(lines, "mems_source_asset",
                  ["source_key","relative_path","file_kind","family","firmware_code","size_bytes","sha256","correlated"],
                  asset_rows, batch=120)
        lines.append("COMMIT;")
        write_qz64(output_dir / "research_enrichment_1500.qz64", lines)

        lines = ["BEGIN TRANSACTION;"]
        rows = []
        for class_key in sorted(scalar_classes):
            props = section_map.get("ScalarClass." + class_key, {})
            rows.append((
                SOURCE_KEY, class_key, decode_value(props.get("Identifier")),
                decode_value(props.get("Name")), props.get("AxisClass"),
                decode_value(props.get("RoverComments")),
                as_int(props.get("PetaHidden")), as_int(props.get("PetaUpdated")),
                as_int(props.get("ManifoldPressure")), props.get("Edited")
            ))
        add_multi(lines, "mems_scalar_definition",
                  ["source_key","class_key","identifier","name","axis_class","rover_comments","peta_hidden","peta_updated","manifold_pressure","edited"],
                  rows, batch=180)
        lines.append("COMMIT;")
        write_qz64(output_dir / "research_enrichment_1510.qz64", lines)

        lines = ["BEGIN TRANSACTION;"]
        rows = [
            (SOURCE_KEY, fw, address, address_int(address), class_key, correlated)
            for fw, address, class_key, correlated in scalar_bindings
        ]
        add_multi(lines, "mems_scalar_binding",
                  ["source_key","firmware_code","address_hex","address_int","class_key","correlated"],
                  rows, batch=350)
        lines.append("COMMIT;")
        write_qz64(output_dir / "research_enrichment_1520.qz64", lines)

        lines = ["BEGIN TRANSACTION;"]
        rows = []
        for class_key in sorted(table_classes):
            props = section_map.get("TableClass." + class_key, {})
            rows.append((
                SOURCE_KEY, class_key, decode_value(props.get("Identifier")),
                decode_value(props.get("Name")), props.get("XAxis.AxisClass"),
                props.get("YAxis.AxisClass"), props.get("ZAxis.AxisClass"),
                dimensions.get("TableClass." + class_key + ".X"),
                dimensions.get("TableClass." + class_key + ".Y"),
                as_int(props.get("PetaHidden")), as_int(props.get("PetaUpdated")),
                props.get("Edited")
            ))
        add_multi(lines, "mems_table_definition",
                  ["source_key","class_key","identifier","name","x_axis_class","y_axis_class","z_axis_class","x_count","y_count","peta_hidden","peta_updated","edited"],
                  rows, batch=180)
        rows = []
        for fw, index, class_key, correlated in table_bindings:
            try:
                table_index = int(index)
            except ValueError:
                continue
            rows.append((
                SOURCE_KEY, fw, table_index, class_key, correlated,
                dimensions.get(f"Table.{fw}.{index}.X"),
                dimensions.get(f"Table.{fw}.{index}.Y")
            ))
        add_multi(lines, "mems_table_binding",
                  ["source_key","firmware_code","table_index","class_key","correlated","x_count","y_count"],
                  rows, batch=300)
        lines.append("COMMIT;")
        write_qz64(output_dir / "research_enrichment_1530.qz64", lines)

        lines = ["BEGIN TRANSACTION;"]
        variable_rows = []
        for label, (suffix, variant) in FAMILY_FILES.items():
            family = "1.6" if label.startswith("1.6") else label
            path = f"Executables/Correlations/Variables.{suffix}.csv"
            rows = list(csv.reader(io.StringIO(zf.read(path).decode("utf-8-sig", errors="replace"))))
            header = rows[0]
            for row in rows[1:]:
                if not row or not row[0]:
                    continue
                class_key = row[0]
                member_count = as_int(row[1])
                for index, firmware in enumerate(header[2:], 2):
                    if index < len(row) and row[index]:
                        variable_rows.append((
                            SOURCE_KEY, family, class_key, firmware, row[index],
                            address_int(row[index]), member_count
                        ))
        add_multi(lines, "mems_variable_correlation",
                  ["source_key","family","class_key","firmware_code","address_hex","address_int","class_member_count"],
                  variable_rows, batch=350)
        lines.append("COMMIT;")
        write_qz64(output_dir / "research_enrichment_1540.qz64", lines)

        return {
            "archive_sha256": archive_sha,
            "firmwares": len(relevant_fw),
            "axis_classes": len(axis_classes),
            "scalar_classes": len(scalar_classes),
            "scalar_bindings": len(scalar_bindings),
            "table_classes": len(table_classes),
            "table_bindings": len(table_bindings),
            "variable_bindings": len(variable_rows),
            "source_assets": len(asset_rows),
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
