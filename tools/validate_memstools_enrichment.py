#!/usr/bin/env python3
import argparse
import base64
import pathlib
import sqlite3
import struct
import tempfile
import zlib

EXPECTED = {
    "mems_knowledge_source": 1,
    "mems_firmware_catalog": 286,
    "mems_axis_definition": 320,
    "mems_scalar_definition": 18519,
    "mems_scalar_binding": 96240,
    "mems_table_definition": 1815,
    "mems_table_binding": 17458,
    "mems_variable_correlation": 64706,
    "mems_source_asset": 433,
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
    files = [args.reference_dir / f"research_enrichment_{n}.qz64" for n in (1500,1510,1520,1530,1540)]
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
            for table, expected in EXPECTED.items():
                actual = con.execute(f"SELECT COUNT(*) FROM {table}").fetchone()[0]
                if actual != expected:
                    raise SystemExit(f"{table}: {actual}, attendu {expected}")
                print(f"OK {table}: {actual}")

            row = con.execute("""
                SELECT b.address_hex,d.identifier,a.suffix,a.scale,a.offset
                FROM mems_scalar_binding b
                JOIN mems_scalar_definition d USING(source_key,class_key)
                LEFT JOIN mems_axis_definition a
                  ON a.source_key=d.source_key AND a.class_key=d.axis_class
                WHERE b.firmware_code='AANMP002' AND d.identifier='inj_pw_rosco'
                ORDER BY b.address_int LIMIT 1
            """).fetchone()
            if row != ("$00E", "inj_pw_rosco", "ms", 0.001333, 0.0):
                raise SystemExit(f"AANMP002 inj_pw_rosco inattendu: {row!r}")
            print("OK AANMP002 inj_pw_rosco: $00E, ms, scale 0.001333")
        finally:
            con.close()

if __name__ == "__main__":
    main()
