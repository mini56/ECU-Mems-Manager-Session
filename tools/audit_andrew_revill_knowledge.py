#!/usr/bin/env python3
"""Audit Andrew Revill knowledge against the pre-Andrew MEMS reference database.

The report is intentionally conservative. Exact textual overlap is reported as
an indicator only; it never promotes an external reverse-engineered fact to a
project-verified fact. Calibration scalars/tables are also kept distinct from
runtime-variable correlations so a future expert engine cannot silently treat a
ROM mapping address as a live diagnostic RAM address.
"""
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

ANDREW_FILES = {
    "research_enrichment_1500.qz64",
    "research_enrichment_1510.qz64",
    "research_enrichment_1520.qz64",
    "research_enrichment_1530.qz64",
    "research_enrichment_1540.qz64",
    "research_enrichment_1560.qz64",
}
ANDREW_SOURCE_PREFIX = "andrew_revill_"


def numeric_suffix(path: pathlib.Path):
    match = re.search(r"_(\d+)\.qz64$", path.name, re.I)
    return int(match.group(1)) if match else 0


def unpack_text_bytes(encoded: bytes):
    packed = base64.b64decode(encoded.strip())
    if len(packed) < 5:
        raise RuntimeError("qz64 trop court")
    expected = struct.unpack(">I", packed[:4])[0]
    raw = zlib.decompress(packed[4:])
    if len(raw) != expected:
        raise RuntimeError(f"qCompress invalide: {len(raw)} / {expected}")
    return raw.decode("utf-8")


def unpack_file(path: pathlib.Path):
    return unpack_text_bytes(path.read_bytes())


def execute_lines(con: sqlite3.Connection, text: str):
    for statement in text.splitlines():
        statement = statement.strip()
        if statement and not statement.startswith("--"):
            con.execute(statement)
    con.commit()


def apply_seed(con: sqlite3.Connection, root: pathlib.Path):
    parts = sorted(root.glob("mems_reference_seed_*.qz64"), key=numeric_suffix)
    if not parts:
        raise RuntimeError("Aucun seed MEMS")
    encoded = b"".join(path.read_bytes().strip() for path in parts)
    execute_lines(con, unpack_text_bytes(encoded))


def enrichment_files(root: pathlib.Path):
    files = list(root.rglob("research_enrichment*.qz64"))
    return sorted(files, key=lambda p: (numeric_suffix(p), p.as_posix()))


def build_db(root: pathlib.Path, include_andrew: bool):
    temp = tempfile.NamedTemporaryFile(prefix="mems-audit-", suffix=".sqlite", delete=False)
    temp.close()
    db_path = pathlib.Path(temp.name)
    con = sqlite3.connect(db_path)
    try:
        apply_seed(con, root)
        for path in enrichment_files(root):
            is_andrew = path.name in ANDREW_FILES
            if is_andrew != include_andrew and is_andrew:
                continue
            execute_lines(con, unpack_file(path))
        return con, db_path
    except Exception:
        con.close()
        db_path.unlink(missing_ok=True)
        raise


def tables(con):
    return [row[0] for row in con.execute(
        "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%' ORDER BY name"
    )]


def table_exists(con, name):
    return con.execute(
        "SELECT 1 FROM sqlite_master WHERE type='table' AND name=?", (name,)
    ).fetchone() is not None


def count(con, sql, args=()):
    row = con.execute(sql, args).fetchone()
    return int(row[0] or 0) if row else 0


def normalized_token(value):
    if value is None:
        return ""
    return re.sub(r"[^a-z0-9_]+", "", str(value).lower())


def baseline_tokens(con):
    result = set()
    for table in tables(con):
        info = list(con.execute(f'PRAGMA table_info("{table}")'))
        cols = [row[1] for row in info if "TEXT" in (row[2] or "").upper()]
        if not cols:
            continue
        quoted = ",".join(f'"{c}"' for c in cols)
        try:
            cursor = con.execute(f'SELECT {quoted} FROM "{table}"')
        except sqlite3.Error:
            continue
        for row in cursor:
            for value in row:
                if value is None:
                    continue
                text = str(value).lower()
                for token in re.findall(r"[a-z0-9_]{3,}", text):
                    result.add(normalized_token(token))
    return result


def source_keys(con):
    if not table_exists(con, "mems_knowledge_source"):
        return []
    return [row[0] for row in con.execute(
        "SELECT source_key FROM mems_knowledge_source WHERE source_key LIKE ? ORDER BY source_key",
        (ANDREW_SOURCE_PREFIX + "%",),
    )]


def grouped(con, sql, args=()):
    return [tuple(row) for row in con.execute(sql, args)]


def quality_stats(con, keys):
    if not keys:
        return {}
    placeholders = ",".join("?" for _ in keys)
    q = {}
    q["orphan_scalar_bindings"] = count(con, f"""
        SELECT COUNT(*) FROM mems_scalar_binding b
        LEFT JOIN mems_scalar_definition d
          ON d.source_key=b.source_key AND d.class_key=b.class_key
        WHERE b.source_key IN ({placeholders}) AND d.class_key IS NULL
    """, keys)
    q["orphan_table_bindings"] = count(con, f"""
        SELECT COUNT(*) FROM mems_table_binding b
        LEFT JOIN mems_table_definition d
          ON d.source_key=b.source_key AND d.class_key=b.class_key
        WHERE b.source_key IN ({placeholders}) AND d.class_key IS NULL
    """, keys)
    q["variable_null_addresses"] = count(con, f"""
        SELECT COUNT(*) FROM mems_variable_correlation
        WHERE source_key IN ({placeholders}) AND address_int IS NULL
    """, keys)
    q["variable_unknown_firmware"] = count(con, f"""
        SELECT COUNT(*) FROM mems_variable_correlation v
        LEFT JOIN mems_firmware_catalog f
          ON f.source_key=v.source_key AND f.firmware_code=v.firmware_code
        WHERE v.source_key IN ({placeholders}) AND f.firmware_code IS NULL
    """, keys)
    q["shared_variable_addresses"] = count(con, f"""
        SELECT COUNT(*) FROM (
          SELECT source_key,family,firmware_code,address_int
          FROM mems_variable_correlation
          WHERE source_key IN ({placeholders}) AND address_int IS NOT NULL
          GROUP BY source_key,family,firmware_code,address_int
          HAVING COUNT(DISTINCT class_key)>1
        )
    """, keys)
    q["scalar_unresolved_axis"] = count(con, f"""
        SELECT COUNT(*) FROM mems_scalar_definition d
        LEFT JOIN mems_axis_definition a
          ON a.source_key=d.source_key AND a.class_key=d.axis_class
        WHERE d.source_key IN ({placeholders})
          AND COALESCE(d.axis_class,'')<>'' AND a.class_key IS NULL
    """, keys)
    return q


def make_report(root: pathlib.Path):
    baseline, baseline_path = build_db(root, include_andrew=False)
    full, full_path = build_db(root, include_andrew=True)
    try:
        keys = source_keys(full)
        tokens = baseline_tokens(baseline)
        sources = grouped(full, """
            SELECT source_key,author,title,source_url,archive_sha256,verification_level,scope,notes
            FROM mems_knowledge_source WHERE source_key LIKE ? ORDER BY source_key
        """, (ANDREW_SOURCE_PREFIX + "%",)) if keys else []

        family_counts = grouped(full, """
            SELECT source_key,family,COUNT(*)
            FROM mems_firmware_catalog WHERE source_key LIKE ?
            GROUP BY source_key,family ORDER BY family,source_key
        """, (ANDREW_SOURCE_PREFIX + "%",)) if table_exists(full, "mems_firmware_catalog") else []
        asset_counts = grouped(full, """
            SELECT source_key,family,file_kind,COUNT(*)
            FROM mems_source_asset WHERE source_key LIKE ?
            GROUP BY source_key,family,file_kind ORDER BY family,file_kind
        """, (ANDREW_SOURCE_PREFIX + "%",)) if table_exists(full, "mems_source_asset") else []

        datasets = {}
        for table in (
            "mems_firmware_catalog", "mems_axis_definition", "mems_scalar_definition",
            "mems_scalar_binding", "mems_table_definition", "mems_table_binding",
            "mems_variable_correlation", "mems_source_asset",
        ):
            datasets[table] = count(full, f"SELECT COUNT(*) FROM {table} WHERE source_key LIKE ?", (ANDREW_SOURCE_PREFIX + "%",)) if table_exists(full, table) else 0

        fw_rows = grouped(full, "SELECT DISTINCT firmware_code FROM mems_firmware_catalog WHERE source_key LIKE ?", (ANDREW_SOURCE_PREFIX + "%",))
        fw_overlap = sum(1 for (fw,) in fw_rows if normalized_token(fw) in tokens)

        identifiers = []
        for table in ("mems_scalar_definition", "mems_table_definition"):
            if table_exists(full, table):
                identifiers.extend(row[0] for row in full.execute(
                    f"SELECT DISTINCT identifier FROM {table} WHERE source_key LIKE ? AND COALESCE(identifier,'')<>''",
                    (ANDREW_SOURCE_PREFIX + "%",),
                ))
        identifier_overlap = sum(1 for ident in identifiers if normalized_token(ident) in tokens)

        quality = quality_stats(full, keys)
        mems12 = {
            "firmwares": count(full, "SELECT COUNT(*) FROM mems_firmware_catalog WHERE source_key='andrew_revill_memstools_mems12'"),
            "scalars": count(full, "SELECT COUNT(*) FROM mems_scalar_binding WHERE source_key='andrew_revill_memstools_mems12'"),
            "tables": count(full, "SELECT COUNT(*) FROM mems_table_binding WHERE source_key='andrew_revill_memstools_mems12'"),
            "variables": count(full, "SELECT COUNT(*) FROM mems_variable_correlation WHERE source_key='andrew_revill_memstools_mems12'"),
            "assets": count(full, "SELECT COUNT(*) FROM mems_source_asset WHERE source_key='andrew_revill_memstools_mems12'"),
        }
        mems12_assets = [row[0] for row in full.execute(
            "SELECT relative_path FROM mems_source_asset WHERE source_key='andrew_revill_memstools_mems12' ORDER BY relative_path LIMIT 30"
        )] if table_exists(full, "mems_source_asset") else []

        aanmp = None
        if table_exists(full, "mems_scalar_binding"):
            aanmp = full.execute("""
                SELECT b.address_hex,d.identifier,d.name,a.suffix,a.scale,a.offset
                FROM mems_scalar_binding b
                JOIN mems_scalar_definition d USING(source_key,class_key)
                LEFT JOIN mems_axis_definition a
                  ON a.source_key=d.source_key AND a.class_key=d.axis_class
                WHERE b.source_key='andrew_revill_memstools'
                  AND b.firmware_code='AANMP002' AND d.identifier='inj_pw_rosco'
                ORDER BY b.address_int LIMIT 1
            """).fetchone()

        summary = {
            "sources": sources,
            "datasets": datasets,
            "family_firmware_counts": family_counts,
            "asset_counts": asset_counts,
            "baseline_table_count": len(tables(baseline)),
            "full_table_count": len(tables(full)),
            "firmware_exact_text_overlap": {"matched": fw_overlap, "total": len(fw_rows)},
            "identifier_exact_text_overlap": {"matched": identifier_overlap, "total": len(identifiers)},
            "quality": quality,
            "mems_1_2": mems12,
            "mems_1_2_asset_sample": mems12_assets,
            "aanmp002_inj_pw_rosco": tuple(aanmp) if aanmp else None,
        }

        lines = [
            "# Audit Andrew Revill -> base de connaissance ECU MEMS Manager",
            "",
            "## Principe d'audit",
            "",
            "Cet audit compare la base de référence reconstruite sans les enrichissements Andrew Revill à la base reconstruite avec eux. ",
            "Les données Andrew restent au niveau `source_externe`; un recoupement lexical n'est jamais promu automatiquement en `decoded_by_project` ou `verifie_constructeur`.",
            "",
            "Les objets `mems_scalar_*` et `mems_table_*` décrivent des éléments de firmware/calibration. Ils ne doivent pas être interprétés automatiquement comme des adresses RAM de diagnostic en temps réel. `mems_variable_correlation` est la famille de données la plus pertinente pour les variables internes, mais elle reste une corrélation externe tant qu'elle n'est pas validée par le projet ou sur véhicule.",
            "",
            "## Sources Andrew intégrées",
            "",
        ]
        for row in sources:
            lines.append(f"- `{row[0]}` — {row[2]} — portée **{row[6]}** — niveau `{row[5]}` — archive SHA-256 `{row[4]}`")
        lines += ["", "## Volume structuré", "", "| Jeu de données | Lignes Andrew |", "|---|---:|"]
        labels = {
            "mems_firmware_catalog": "Firmwares",
            "mems_axis_definition": "Définitions d'axes / unités / échelles",
            "mems_scalar_definition": "Définitions de scalaires",
            "mems_scalar_binding": "Liaisons scalaire <-> firmware/adresse",
            "mems_table_definition": "Définitions de tables/cartographies",
            "mems_table_binding": "Liaisons table <-> firmware/index",
            "mems_variable_correlation": "Corrélations de variables",
            "mems_source_asset": "Fichiers sources indexés",
        }
        for key, label in labels.items():
            lines.append(f"| {label} | {datasets[key]} |")

        lines += ["", "## Couverture firmware par famille", "", "| Source | Famille | Firmwares |", "|---|---|---:|"]
        for source, family, n in family_counts:
            lines.append(f"| `{source}` | MEMS {family} | {n} |")

        lines += [
            "",
            "## Comparaison avec la base antérieure",
            "",
            f"- Tables dans la base avant Andrew : **{len(tables(baseline))}** ; après intégration : **{len(tables(full))}**.",
            f"- Identifiants firmware Andrew déjà cités textuellement quelque part dans l'ancienne base : **{fw_overlap}/{len(fw_rows)}**.",
            f"- Identifiants de scalaires/tables Andrew retrouvés exactement comme jetons textuels dans l'ancienne base : **{identifier_overlap}/{len(identifiers)}**.",
            "- Ces deux derniers nombres sont des indicateurs de recouvrement documentaire, pas des validations sémantiques. Une absence signifie 'nouvelle connaissance structurée'; une présence signifie seulement 'déjà mentionné quelque part' tant qu'un recoupement technique précis n'est pas établi.",
            "",
            "## Contrôles d'intégrité",
            "",
        ]
        for key, value in quality.items():
            lines.append(f"- `{key}` : **{value}**")

        lines += ["", "## MEMS 1.2", ""]
        if any(mems12.values()):
            lines += [
                f"- Firmwares 1.2 structurés : **{mems12['firmwares']}**",
                f"- Liaisons scalaires 1.2 : **{mems12['scalars']}**",
                f"- Liaisons tables 1.2 : **{mems12['tables']}**",
                f"- Corrélations variables 1.2 : **{mems12['variables']}**",
                f"- Fichiers source 1.2 indexés : **{mems12['assets']}**",
            ]
            if mems12_assets:
                lines += ["", "Exemples de fichiers 1.2 détectés directement dans MEMSTools :"]
                lines.extend(f"- `{name}`" for name in mems12_assets)
        else:
            lines.append("Aucun élément MEMS 1.2 détecté dans les structures/fichiers indexés par le ZIP courant. Ce serait un trou explicite à compléter par une autre source Andrew, pas une donnée à inventer.")

        lines += ["", "## Point de contrôle AANMP002", ""]
        if aanmp:
            lines.append(f"`inj_pw_rosco` est présent pour AANMP002 : adresse de scalaire `{aanmp[0]}`, libellé `{aanmp[2]}`, unité `{aanmp[3]}`, échelle `{aanmp[4]}`, offset `{aanmp[5]}`.")
            lines.append("Cette adresse appartient au modèle scalaire/calibration de MEMS Mapper; elle n'est pas assimilée ici à l'adresse RAM Mode 4 utilisée par ECU MEMS Manager.")
        else:
            lines.append("Le contrôle AANMP002 / inj_pw_rosco n'a pas été retrouvé dans le corpus reconstruit.")

        lines += [
            "",
            "## Statut pour le futur moteur expert",
            "",
            "- **Utilisable immédiatement comme contexte externe** : familles/firmwares, noms de scalaires et tables, unités/échelles, corrélations variables, inventaire des désassemblages et fichiers de référence.",
            "- **À recouper avant décision diagnostique forte** : toute valeur ou adresse issue uniquement de MEMSTools, particulièrement les variables internes et les éléments marqués non corrélés.",
            "- **Ne pas mélanger** : cartographie/scalaires ROM, données de protocole 7D/80, et RAM Mode 4 sont trois couches différentes.",
            "- **Conserver la provenance** : le moteur expert devra pouvoir indiquer si une conclusion vient de `verifie_constructeur`, `decoded_by_project`, `recoupee` ou `source_externe`.",
            "",
        ]
        return "\n".join(lines), summary
    finally:
        baseline.close()
        full.close()
        baseline_path.unlink(missing_ok=True)
        full_path.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("reference_dir", type=pathlib.Path)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    parser.add_argument("--json-output", type=pathlib.Path)
    args = parser.parse_args()
    report, summary = make_report(args.reference_dir)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(report + "\n", encoding="utf-8")
    if args.json_output:
        args.json_output.parent.mkdir(parents=True, exist_ok=True)
        args.json_output.write_text(json.dumps(summary, ensure_ascii=False, indent=2, default=str) + "\n", encoding="utf-8")
    print(report)


if __name__ == "__main__":
    main()
