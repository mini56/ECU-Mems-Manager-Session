#!/usr/bin/env python3
"""Build provenance-aware MEMS 1.9 architecture facts documented by Andrew Revill.

Source: https://andrewrevill.co.uk/MEMSMapperMems19.htm
These facts describe firmware/map representation and Andrew's correlation corpus.
They are deliberately separate from runtime protocol/RAM facts.
"""
from __future__ import annotations

import argparse
from pathlib import Path
from build_memstools_enrichment import add_multi, sqlq, write_qz64

SOURCE_KEY = "andrew_revill_mems19_architecture"
SOURCE_URL = "https://andrewrevill.co.uk/MEMSMapperMems19.htm"
OUTPUT_NAME = "research_enrichment_1580.qz64"

FACTS = [
    ("processor", "Intel AN87C196KD", "hardware", "MEMS 1.9 uses an Intel AN87C196KD processor rather than the later Motorola processors."),
    ("native_data_width", "8-bit", "architecture", "Andrew describes MEMS 1.9 as natively 8-bit."),
    ("multi_byte_endianness", "little-endian", "architecture", "Words and longer values are stored low byte first."),
    ("table_storage_width", "8-bit byte", "calibration", "Andrew states all MEMS 1.9 tables use byte data."),
    ("firmware_map_layout", "monolithic", "memory_layout", "Boot loader, firmware and map are not separated in the same way as later MEMS; map and firmware occupy one monolithic space."),
    ("rpm_table_encoding", "25 rpm per count", "calibration", "Engine speed values in 8-bit table representation use 25 rpm increments."),
    ("rpm_table_max_representable", "6375 rpm", "calibration", "An unsigned byte at 25 rpm/count represents at most 6375 rpm."),
    ("mapper_project_translation", "lossless pseudo-MEMS3 representation", "tooling", "Mapper internally reorganises MEMS 1.9 data into a pseudo-MEMS3 project and reverses the transformation when writing/saving."),
    ("peta_definition_origin", "ex-factory PETA transfer files", "provenance", "Andrew reports receiving factory data including PETA transfer files containing named scalar/table definitions."),
    ("correlation_method", "automated disassembly and code correlation", "provenance", "Andrew automated firmware disassembly and correlated equivalent code addresses, variables, tables and scalars across firmware versions."),
    ("correlation_scope", "MEMS 1.9 and MEMS 1.6", "provenance", "Andrew describes the resulting correlated library as fairly complete for MEMS 1.9 and MEMS 1.6."),
    ("firmware_archive_artifacts", ".HEX + .ASM + .XF1/.XF3 where available", "source_assets", "Andrew's firmware archive pairs raw firmware with disassembly and PETA transfer definition material where available."),
    ("mems1x_basic_file_structure", "1.2/1.3/1.6/1.9 same basic Mapper structure", "tooling", "Andrew notes MEMS 1.2, 1.3, 1.6 and 1.9 files share the same basic structure for the relevant Mapper/ICP operations."),
]


def build(output_dir: Path):
    lines = ["BEGIN TRANSACTION;"]
    lines.extend([
        "CREATE TABLE IF NOT EXISTS mems_knowledge_source(source_key TEXT PRIMARY KEY,author TEXT,title TEXT,source_url TEXT,archive_sha256 TEXT,verification_level TEXT,scope TEXT,notes TEXT);",
        "CREATE TABLE IF NOT EXISTS mems_architecture_fact_external(source_key TEXT NOT NULL,family TEXT NOT NULL,fact_key TEXT NOT NULL,fact_value TEXT NOT NULL,layer TEXT NOT NULL,notes TEXT,verification_level TEXT NOT NULL,PRIMARY KEY(source_key,family,fact_key));",
    ])
    lines.append(f"DELETE FROM mems_architecture_fact_external WHERE source_key={sqlq(SOURCE_KEY)};")
    lines.append(f"DELETE FROM mems_knowledge_source WHERE source_key={sqlq(SOURCE_KEY)};")
    source = (
        SOURCE_KEY, "Andrew Revill", "Remapping the Rover MEMS 1.9 ECU",
        SOURCE_URL, None, "source_externe", "MEMS 1.9 (with correlation context for MEMS 1.6 and MEMS 1.X)",
        "Architecture, calibration representation and provenance facts from Andrew Revill's MEMS 1.9 technical article. Kept outside live diagnostic RAM/protocol tables."
    )
    lines.append("INSERT INTO mems_knowledge_source VALUES(" + ",".join(sqlq(v) for v in source) + ");")
    rows = [(SOURCE_KEY, "1.9", key, value, layer, notes, "source_externe") for key, value, layer, notes in FACTS]
    add_multi(lines, "mems_architecture_fact_external",
              ["source_key","family","fact_key","fact_value","layer","notes","verification_level"], rows)
    lines.append("COMMIT;")
    output = output_dir / OUTPUT_NAME
    write_qz64(output, lines)
    print(f"output={output}")
    print(f"facts={len(rows)}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", type=Path, default=Path("database/reference"))
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    build(args.output_dir)


if __name__ == "__main__":
    main()
