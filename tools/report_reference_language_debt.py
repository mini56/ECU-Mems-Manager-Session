#!/usr/bin/env python3
import pathlib
import sqlite3
import sys
import xml.etree.ElementTree as ET

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import audit_reference_database as audit


def main() -> int:
    root = pathlib.Path("database/reference")
    db_path = audit.build_database(root)
    con = sqlite3.connect(db_path)
    try:
        print("SQLITE_LEGACY_UNIQUE_VALUES")
        seen = set()
        tables = [r[0] for r in con.execute("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%' ORDER BY name")]
        for table in tables:
            columns = [r[1] for r in con.execute(f'PRAGMA table_info("{table}")')]
            lower_map = {c.lower(): c for c in columns}
            for column in columns:
                lc = column.lower()
                if audit.LANG_SUFFIX_RE.match(lc) or not audit.is_translatable_column(lc):
                    continue
                if f"{lc}_fr" in lower_map:
                    continue
                for rowid, value in con.execute(f'SELECT rowid,"{column}" FROM "{table}" WHERE "{column}" IS NOT NULL'):
                    text = str(value).strip()
                    if not text or audit.looks_technical_text(text):
                        continue
                    key = (table, lc, text)
                    if key in seen:
                        continue
                    seen.add(key)
                    print(f"SQLITE_UNIQUE|{table}.{column}|{text}")

        print("XML_UNIQUE_VALUES")
        seen_xml = set()
        for path in sorted((root / "fiches").glob("*.xml.qz64")):
            raw = audit.unpack_qz64_bytes(path.read_bytes(), path.as_posix())
            tree = ET.fromstring(raw.decode("utf-8"))
            for element in tree.iter():
                texts = []
                if element.text and element.text.strip():
                    texts.append(element.text.strip())
                for key, value in element.attrib.items():
                    if key.lower() in {"titre", "title", "description", "fonction", "function", "note", "label"} and value.strip():
                        texts.append(value.strip())
                for text in texts:
                    if audit.looks_technical_text(text) or text in seen_xml:
                        continue
                    seen_xml.add(text)
                    print(f"XML_UNIQUE|{text}")

        print("SVG_UNIQUE_VALUES")
        seen_svg = set()
        for asset in sorted(root.rglob("*.svg")):
            tree = ET.parse(asset).getroot()
            for element in tree.iter():
                if audit.local_tag(element.tag) != "text":
                    continue
                text = " ".join("".join(element.itertext()).split())
                if not text or audit.looks_technical_text(text) or text in seen_svg:
                    continue
                seen_svg.add(text)
                print(f"SVG_UNIQUE|{text}")
        return 0
    finally:
        con.close()
        if db_path.exists():
            db_path.unlink()


if __name__ == "__main__":
    raise SystemExit(main())
