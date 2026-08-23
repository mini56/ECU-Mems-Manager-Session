#!/usr/bin/env python3
"""Build structured family-level facts documented by Andrew Revill.

Source: https://andrewrevill.co.uk/MEMS3FamiliesSupported.htm
The facts are intentionally stored in external/provenance-aware tables instead
of overwriting project-decoded protocol tables.
"""
from __future__ import annotations

import argparse
from pathlib import Path

from build_memstools_enrichment import add_multi, sqlq, write_qz64

SOURCE_KEY = "andrew_revill_family_support_page"
SOURCE_URL = "https://andrewrevill.co.uk/MEMS3FamiliesSupported.htm"
OUTPUT_NAME = "research_enrichment_1570.qz64"

FAMILIES = ("1.2", "1.3", "1.6", "1.9")

COMMON_CAPABILITIES = (
    ("read_save_full_rom", "available", "Reading and saving of full ROM (firmware and map)"),
    ("live_data", "available", "Live diagnostics - live data"),
    ("live_fault_codes", "available", "Live diagnostics - live fault codes"),
    ("live_dashboard", "available", "Live diagnostics - live dashboard"),
    ("immobiliser_pairing_status", "available", "Immobiliser pairing and status"),
    ("clear_adaptations", "available", "Clear adaptations"),
    ("service_adjustments", "available", "Service adjustments"),
)


def build(output_dir: Path):
    lines = ["BEGIN TRANSACTION;"]
    lines.extend([
        "CREATE TABLE IF NOT EXISTS mems_knowledge_source(source_key TEXT PRIMARY KEY,author TEXT,title TEXT,source_url TEXT,archive_sha256 TEXT,verification_level TEXT,scope TEXT,notes TEXT);",
        "CREATE TABLE IF NOT EXISTS mems_family_capability_external(source_key TEXT NOT NULL,family TEXT NOT NULL,capability TEXT NOT NULL,availability TEXT NOT NULL,access_method TEXT,source_text_summary TEXT,verification_level TEXT NOT NULL,PRIMARY KEY(source_key,family,capability));",
        "CREATE TABLE IF NOT EXISTS mems_family_transport_external(source_key TEXT NOT NULL,family TEXT NOT NULL,protocol TEXT,cable_type TEXT,diagnostic_connector TEXT,notes TEXT,verification_level TEXT NOT NULL,PRIMARY KEY(source_key,family));",
        "CREATE TABLE IF NOT EXISTS mems_connector_pin_external(source_key TEXT NOT NULL,family_scope TEXT NOT NULL,cable_pin INTEGER NOT NULL,signal TEXT NOT NULL,ecu_pin INTEGER,notes TEXT,verification_level TEXT NOT NULL,PRIMARY KEY(source_key,family_scope,cable_pin));",
    ])
    for table in ("mems_family_capability_external", "mems_family_transport_external", "mems_connector_pin_external"):
        lines.append(f"DELETE FROM {table} WHERE source_key={sqlq(SOURCE_KEY)};")
    lines.append(f"DELETE FROM mems_knowledge_source WHERE source_key={sqlq(SOURCE_KEY)};")
    source = (
        SOURCE_KEY,
        "Andrew Revill",
        "Full Support Added for All Rover MEMS ECU Families",
        SOURCE_URL,
        None,
        "source_externe",
        "MEMS 1.2 / 1.3 / 1.6 / 1.9",
        "Family-level capabilities and diagnostic cable facts transcribed from Andrew Revill's published support matrix. Kept separate from project-decoded protocol data and from MEMSTools firmware correlations.",
    )
    lines.append("INSERT INTO mems_knowledge_source VALUES(" + ",".join(sqlq(v) for v in source) + ");")

    capability_rows = []
    for family in FAMILIES:
        for capability, availability, summary in COMMON_CAPABILITIES:
            capability_rows.append((
                SOURCE_KEY, family, capability, availability, "diagnostic/service interface",
                summary, "source_externe"
            ))
    add_multi(lines, "mems_family_capability_external",
              ["source_key","family","capability","availability","access_method","source_text_summary","verification_level"],
              capability_rows)

    transport_rows = [
        (SOURCE_KEY, "1.2", "ROSCO", "3-pin UART ROSCO diagnostic cable", "Rover 3-pin diagnostic connector", "Andrew states MEMS 1.2 normally uses the 3-pin ROSCO cable.", "source_externe"),
        (SOURCE_KEY, "1.3", "ROSCO", "3-pin UART ROSCO diagnostic cable", "Rover 3-pin diagnostic connector", "Andrew states MEMS 1.3 normally uses the 3-pin ROSCO cable.", "source_externe"),
        (SOURCE_KEY, "1.6", "ROSCO", "3-pin UART ROSCO diagnostic cable", "Rover 3-pin diagnostic connector", "Andrew states MEMS 1.6 normally uses the 3-pin ROSCO cable.", "source_externe"),
        (SOURCE_KEY, "1.9", "ROSCO", "OBDII-style K-Line diagnostic cable", "OBDII-style diagnostic connector", "Andrew's matrix lists Rover MEMS 1.9 EU2 MKC MPI as ROSCO protocol over an OBDII-style K-Line cable.", "source_externe"),
    ]
    add_multi(lines, "mems_family_transport_external",
              ["source_key","family","protocol","cable_type","diagnostic_connector","notes","verification_level"],
              transport_rows)

    pin_rows = [
        (SOURCE_KEY, "MEMS 1.2/1.3/1.6", 1, "Signal Ground", 14, "3-pin ROSCO cable pin 1 to ECU pin 14", "source_externe"),
        (SOURCE_KEY, "MEMS 1.2/1.3/1.6", 2, "ECU Data Out -> PC Data In", 10, "3-pin ROSCO cable pin 2 to ECU pin 10", "source_externe"),
        (SOURCE_KEY, "MEMS 1.2/1.3/1.6", 3, "PC Data Out -> ECU Data In", 15, "3-pin ROSCO cable pin 3 to ECU pin 15", "source_externe"),
    ]
    add_multi(lines, "mems_connector_pin_external",
              ["source_key","family_scope","cable_pin","signal","ecu_pin","notes","verification_level"],
              pin_rows)

    lines.append("COMMIT;")
    output = output_dir / OUTPUT_NAME
    write_qz64(output, lines)
    print(f"output={output}")
    print(f"capabilities={len(capability_rows)}")
    print(f"transports={len(transport_rows)}")
    print(f"connector_pins={len(pin_rows)}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", type=Path, default=Path("database/reference"))
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    build(args.output_dir)


if __name__ == "__main__":
    main()
