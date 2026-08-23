#!/usr/bin/env python3
import argparse
import base64
import pathlib
import sqlite3
import struct
import tempfile
import zlib

# Deliberately use structural lower bounds instead of frozen counts. Andrew's
# public MEMSTools archive can gain firmwares/tables over time. A changed count
# must not make a valid enrichment fail; missing families/core data must.
MINIMUMS = {
    "mems_knowledge_source": 1,
    "mems_firmware_catalog": 200,
    "mems_axis_definition": 100,
    "mems_scalar_definition": 1000,
    "mems_scalar_binding": 10000,
    "mems_table_definition": 500,
    "mems_table_binding": 5000,
    "mems_variable_correlation": 10000,
    "mems_source_asset": 100,
}


def unpack(path: pathlib.Path) -> str:
    packed = base64.b64decode(path.read_bytes().strip())
    if len(packed) < 5:
        raise RuntimeError(f"{path}: qz64 trop court")
    expected = struct.unpack(">I", packed[:4])[0]
    raw = zlib.decompress(packed[4:])
    if len(raw) != expected:
        raise RuntimeError(f"{path}: taille qCompress invalide")
    return raw.decode("utf-8")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("reference_dir", type=pathlib.Path)
    args = parser.parse_args()
    files = [args.reference_dir / f"research_enrichment_{n}.qz64" for n in (1500, 1510, 1520, 1530, 1540)]
    missing = [str(p) for p in files if not p.exists()]
    if missing:
        raise SystemExit("Fichiers manquants: " + ", ".join(missing))

    with tempfile.TemporaryDirectory() as temp:
        db = pathlib.Path(temp) / "memstools.sqlite"
        con = sqlite3.connect(db)
        try:
            for path in files:
                for statement in unpack(path).splitlines():
                    statement = statement.strip()
                    if statement and not statement.startswith("--"):
                        con.execute(statement)
            con.commit()

            for table, minimum in MINIMUMS.items():
                actual = con.execute(f"SELECT COUNT(*) FROM {table}").fetchone()[0]
                if actual < minimum:
                    raise SystemExit(f"{table}: {actual}, minimum structurel attendu {minimum}")
                print(f"OK {table}: {actual}")

            source = con.execute("""
                SELECT author, archive_sha256, verification_level
                FROM mems_knowledge_source
                WHERE source_key='andrew_revill_memstools'
            """).fetchone()
            if not source or source[0] != "Andrew Revill" or len(source[1] or "") != 64 or source[2] != "source_externe":
                raise SystemExit(f"Provenance Andrew Revill invalide: {source!r}")
            print(f"OK provenance Andrew Revill: sha256={source[1]}")

            families = {row[0] for row in con.execute(
                "SELECT DISTINCT family FROM mems_firmware_catalog WHERE source_key='andrew_revill_memstools'"
            )}
            required_families = {"1.3", "1.6", "1.9"}
            if not required_families.issubset(families):
                raise SystemExit(f"Familles MEMS manquantes: {sorted(required_families - families)}")
            print("OK familles MEMS 1.3 / 1.6 / 1.9")

            aanmp002 = con.execute("""
                SELECT family FROM mems_firmware_catalog
                WHERE source_key='andrew_revill_memstools' AND firmware_code='AANMP002'
            """).fetchone()
            if not aanmp002:
                raise SystemExit("Firmware AANMP002 absent du corpus structure")
            print(f"OK firmware AANMP002: MEMS {aanmp002[0]}")

            inj = con.execute("""
                SELECT COUNT(*), MIN(a.scale), MAX(a.scale), GROUP_CONCAT(DISTINCT a.suffix)
                FROM mems_scalar_binding b
                JOIN mems_scalar_definition d USING(source_key,class_key)
                LEFT JOIN mems_axis_definition a
                  ON a.source_key=d.source_key AND a.class_key=d.axis_class
                WHERE b.firmware_code='AANMP002' AND d.identifier='inj_pw_rosco'
            """).fetchone()
            if not inj or inj[0] <= 0:
                raise SystemExit("AANMP002: scalaire inj_pw_rosco absent")
            print(f"OK AANMP002 inj_pw_rosco: {inj[0]} liaison(s), unité={inj[3]}, échelle={inj[1]}..{inj[2]}")
        finally:
            con.close()


if __name__ == "__main__":
    main()
