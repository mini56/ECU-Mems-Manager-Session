#!/usr/bin/env python3
"""Append published-page knowledge to the generated Andrew MEMSTools audit."""
from __future__ import annotations

import argparse
import base64
import json
import pathlib
import re
import sqlite3
import struct
import tempfile
import zlib


def suffix(path):
    m = re.search(r"_(\d+)\.qz64$", path.name, re.I)
    return int(m.group(1)) if m else 0


def unpack(encoded: bytes):
    packed = base64.b64decode(encoded.strip())
    expected = struct.unpack(">I", packed[:4])[0]
    raw = zlib.decompress(packed[4:])
    if len(raw) != expected:
        raise RuntimeError("qCompress size mismatch")
    return raw.decode("utf-8")


def execute(con, text):
    for statement in text.splitlines():
        statement = statement.strip()
        if statement and not statement.startswith("--"):
            con.execute(statement)
    con.commit()


def rebuild(root):
    tmp = tempfile.NamedTemporaryFile(prefix="andrew-full-", suffix=".sqlite", delete=False)
    tmp.close()
    path = pathlib.Path(tmp.name)
    con = sqlite3.connect(path)
    seed = b"".join(p.read_bytes().strip() for p in sorted(root.glob("mems_reference_seed_*.qz64"), key=suffix))
    execute(con, unpack(seed))
    for p in sorted(root.rglob("research_enrichment*.qz64"), key=lambda x: (suffix(x), x.as_posix())):
        execute(con, unpack(p.read_bytes()))
    return con, path


def count(con, table, where="1=1", args=()):
    exists = con.execute("SELECT 1 FROM sqlite_master WHERE type='table' AND name=?", (table,)).fetchone()
    if not exists:
        return 0
    return con.execute(f"SELECT COUNT(*) FROM {table} WHERE {where}", args).fetchone()[0]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("reference_dir", type=pathlib.Path)
    ap.add_argument("--core-report", type=pathlib.Path, required=True)
    ap.add_argument("--json-output", type=pathlib.Path, required=True)
    args = ap.parse_args()

    con, db_path = rebuild(args.reference_dir)
    try:
        sources = list(con.execute("""
            SELECT source_key,title,scope,verification_level,source_url
            FROM mems_knowledge_source
            WHERE source_key LIKE 'andrew_revill_%'
            ORDER BY source_key
        """))
        capabilities = count(con, "mems_family_capability_external", "source_key='andrew_revill_family_support_page'")
        transports = count(con, "mems_family_transport_external", "source_key='andrew_revill_family_support_page'")
        pins = count(con, "mems_connector_pin_external", "source_key='andrew_revill_family_support_page'")
        architecture = count(con, "mems_architecture_fact_external", "source_key='andrew_revill_mems19_architecture'")
        mems12_fw = count(con, "mems_firmware_catalog", "source_key='andrew_revill_memstools_mems12'")
        mems12_var = count(con, "mems_variable_correlation", "source_key='andrew_revill_memstools_mems12'")
        mems12_scalar = count(con, "mems_scalar_binding", "source_key='andrew_revill_memstools_mems12'")
        mems12_table = count(con, "mems_table_binding", "source_key='andrew_revill_memstools_mems12'")

        core = args.core_report.read_text(encoding="utf-8").rstrip()
        extra = [
            "",
            "## Complément Andrew publié hors tables MEMSTools",
            "",
            "Les pages techniques Andrew ont été intégrées comme sources séparées afin de ne pas confondre une capacité documentée, une architecture firmware et une corrélation d'adresse.",
            "",
            "### Sources Andrew structurées au total",
            "",
        ]
        for key, title, scope, level, url in sources:
            extra.append(f"- `{key}` — {title} — **{scope}** — `{level}` — {url}")
        extra += [
            "",
            f"- Capacités famille documentées : **{capabilities}**",
            f"- Descriptions de transport/câble : **{transports}**",
            f"- Correspondances de broches câble 3-pin : **{pins}**",
            f"- Faits d'architecture/provenance MEMS 1.9 : **{architecture}**",
            "",
            "### Statut MEMS 1.2 après audit complet",
            "",
            "Andrew publie explicitement le support de MEMS 1.2 pour ROSCO, lecture ROM, données live, défauts, tableau de bord, antidémarrage, adaptations et réglages de service. Ces capacités et le câblage sont maintenant structurés dans la base.",
            "",
            f"En revanche le ZIP MEMSTools courant fournit **{mems12_fw} firmware(s) 1.2 corrélé(s), {mems12_scalar} liaison(s) scalaire(s), {mems12_table} liaison(s) table(s) et {mems12_var} corrélation(s) variable(s)** dans le format de corrélation utilisé pour 1.3/1.6/1.9. L'absence reste donc enregistrée comme une limite réelle, pas comblée artificiellement.",
            "",
            "### Règle d'utilisation par le moteur expert",
            "",
            "1. Les connaissances `source_externe` peuvent proposer une hypothèse ou un contrôle, mais ne doivent pas à elles seules déclencher une conclusion forte.",
            "2. Les corrélations de variables firmware ne deviennent des adresses RAM live qu'après validation explicite du chemin de lecture et de la conversion sur le firmware concerné.",
            "3. Les scalaires/tables de cartographie ne doivent jamais être assimilés aux paquets ROSCO 0x7D/0x80 ou à la RAM Mode 4.",
            "4. Une connaissance recoupée avec décodage projet ou mesure ECU réelle peut ensuite être promue vers `recoupee` ou `decoded_by_project`, sans modifier la source Andrew originale.",
            "",
        ]
        args.core_report.write_text(core + "\n" + "\n".join(extra), encoding="utf-8")
        summary = {
            "andrew_sources": [dict(source_key=r[0], title=r[1], scope=r[2], verification_level=r[3], url=r[4]) for r in sources],
            "family_capabilities": capabilities,
            "family_transports": transports,
            "connector_pins": pins,
            "architecture_facts": architecture,
            "mems_1_2_correlated_firmwares": mems12_fw,
            "mems_1_2_scalar_bindings": mems12_scalar,
            "mems_1_2_table_bindings": mems12_table,
            "mems_1_2_variable_correlations": mems12_var,
        }
        args.json_output.write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        print(json.dumps(summary, ensure_ascii=False, indent=2))
    finally:
        con.close()
        db_path.unlink(missing_ok=True)


if __name__ == "__main__":
    main()
