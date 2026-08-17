#!/usr/bin/env python3
import pathlib
import sqlite3
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import audit_reference_database as audit


def main() -> int:
    root = pathlib.Path("database/reference")
    db_path = audit.build_database(root)
    con = sqlite3.connect(db_path)
    try:
        targets = (
            ("protocol_commands", "name"),
            ("protocol_data_fields", "field_name"),
            ("protocol_operations", "function"),
        )
        print("LANGUAGE_FAMILY_SOURCE_VALUES")
        for table, base in targets:
            columns = [r[1] for r in con.execute(f'PRAGMA table_info("{table}")')]
            source = f"{base}_fr" if f"{base}_fr" in columns else base
            print(f"FAMILY {table}.{base} SOURCE={source}")
            try:
                rows = con.execute(
                    f'SELECT rowid,"{source}" FROM "{table}" '
                    f'WHERE "{source}" IS NOT NULL AND TRIM(CAST("{source}" AS TEXT))<>\'\' '
                    f'ORDER BY rowid'
                ).fetchall()
            except sqlite3.Error as exc:
                print(f"REPORT_ERROR {table}.{source}: {exc}")
                continue
            for rowid, value in rows:
                print(f"SOURCE_VALUE|{table}.{base}|rowid={rowid}|{value}")
        return 0
    finally:
        con.close()
        if db_path.exists():
            db_path.unlink()


if __name__ == "__main__":
    raise SystemExit(main())
