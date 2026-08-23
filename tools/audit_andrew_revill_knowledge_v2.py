#!/usr/bin/env python3
"""Audit all Andrew Revill batches against the reference database before Andrew.

The baseline excludes every Andrew batch while preserving research_enrichment_1550,
which belongs to the separate rover-mems-386 source.
"""
from __future__ import annotations

import argparse, base64, json, pathlib, re, sqlite3, struct, tempfile, zlib

ANDREW_FILES = {
    "research_enrichment_1500.qz64", "research_enrichment_1510.qz64",
    "research_enrichment_1520.qz64", "research_enrichment_1530.qz64",
    "research_enrichment_1540.qz64", "research_enrichment_1560.qz64",
    "research_enrichment_1570.qz64", "research_enrichment_1580.qz64",
    "research_enrichment_1590.qz64", "research_enrichment_1600.qz64",
}
PREFIX = "andrew_revill_"


def num(path):
    m = re.search(r"_(\d+)\.qz64$", path.name, re.I)
    return int(m.group(1)) if m else 0


def unpack_bytes(encoded):
    packed = base64.b64decode(encoded.strip())
    if len(packed) < 5:
        raise RuntimeError("qz64 trop court")
    expected = struct.unpack(">I", packed[:4])[0]
    raw = zlib.decompress(packed[4:])
    if len(raw) != expected:
        raise RuntimeError("taille qCompress invalide")
    return raw.decode("utf-8")


def execute(con, text):
    for statement in text.splitlines():
        statement = statement.strip()
        if statement and not statement.startswith("--"):
            con.execute(statement)
    con.commit()


def build(root, include_andrew):
    tmp = tempfile.NamedTemporaryFile(prefix="andrew-audit-", suffix=".sqlite", delete=False)
    tmp.close()
    path = pathlib.Path(tmp.name)
    con = sqlite3.connect(path)
    try:
        seeds = sorted(root.glob("mems_reference_seed_*.qz64"), key=num)
        execute(con, unpack_bytes(b"".join(p.read_bytes().strip() for p in seeds)))
        enrichments = sorted(root.rglob("research_enrichment*.qz64"), key=lambda p: (num(p), p.as_posix()))
        for p in enrichments:
            if p.name in ANDREW_FILES and not include_andrew:
                continue
            execute(con, unpack_bytes(p.read_bytes()))
        return con, path
    except Exception:
        con.close(); path.unlink(missing_ok=True); raise


def exists(con, table):
    return con.execute("SELECT 1 FROM sqlite_master WHERE type='table' AND name=?", (table,)).fetchone() is not None


def tables(con):
    return [r[0] for r in con.execute("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%' ORDER BY name")]


def scalar(con, sql, args=()):
    row = con.execute(sql, args).fetchone()
    return int(row[0] or 0) if row else 0


def token(value):
    return re.sub(r"[^a-z0-9_]+", "", str(value or "").lower())


def baseline_tokens(con):
    out = set()
    for table in tables(con):
        info = list(con.execute(f'PRAGMA table_info("{table}")'))
        cols = [r[1] for r in info if "TEXT" in (r[2] or "").upper()]
        if not cols:
            continue
        try:
            rows = con.execute("SELECT " + ",".join(f'"{c}"' for c in cols) + f' FROM "{table}"')
        except sqlite3.Error:
            continue
        for row in rows:
            for value in row:
                if value is None:
                    continue
                for t in re.findall(r"[a-z0-9_]{3,}", str(value).lower()):
                    out.add(token(t))
    return out


def quality(con):
    result = {}
    result["orphan_scalar_bindings"] = scalar(con, """
        SELECT COUNT(*) FROM mems_scalar_binding b
        LEFT JOIN mems_scalar_definition d ON d.source_key=b.source_key AND d.class_key=b.class_key
        WHERE b.source_key LIKE 'andrew_revill_%' AND d.class_key IS NULL
    """) if exists(con, "mems_scalar_binding") else 0
    result["orphan_table_bindings"] = scalar(con, """
        SELECT COUNT(*) FROM mems_table_binding b
        LEFT JOIN mems_table_definition d ON d.source_key=b.source_key AND d.class_key=b.class_key
        WHERE b.source_key LIKE 'andrew_revill_%' AND d.class_key IS NULL
    """) if exists(con, "mems_table_binding") else 0
    result["variable_null_addresses"] = scalar(con, "SELECT COUNT(*) FROM mems_variable_correlation WHERE source_key LIKE 'andrew_revill_%' AND address_int IS NULL") if exists(con, "mems_variable_correlation") else 0
    result["variable_unknown_firmware"] = scalar(con, """
        SELECT COUNT(*) FROM mems_variable_correlation v
        LEFT JOIN mems_firmware_catalog f ON f.source_key=v.source_key AND f.firmware_code=v.firmware_code
        WHERE v.source_key LIKE 'andrew_revill_%' AND f.firmware_code IS NULL
    """) if exists(con, "mems_variable_correlation") else 0
    result["shared_variable_addresses"] = scalar(con, """
        SELECT COUNT(*) FROM (
          SELECT source_key,family,firmware_code,address_int
          FROM mems_variable_correlation
          WHERE source_key LIKE 'andrew_revill_%' AND address_int IS NOT NULL
          GROUP BY source_key,family,firmware_code,address_int
          HAVING COUNT(DISTINCT class_key)>1)
    """) if exists(con, "mems_variable_correlation") else 0
    result["scalar_unresolved_axis"] = scalar(con, """
        SELECT COUNT(*) FROM mems_scalar_definition d
        LEFT JOIN mems_axis_definition a ON a.source_key=d.source_key AND a.class_key=d.axis_class
        WHERE d.source_key LIKE 'andrew_revill_%' AND COALESCE(d.axis_class,'')<>'' AND a.class_key IS NULL
    """) if exists(con, "mems_scalar_definition") else 0
    return result


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("reference_dir", type=pathlib.Path)
    ap.add_argument("--output", type=pathlib.Path, required=True)
    ap.add_argument("--json-output", type=pathlib.Path, required=True)
    args = ap.parse_args()

    base, base_path = build(args.reference_dir, False)
    full, full_path = build(args.reference_dir, True)
    try:
        tokens = baseline_tokens(base)
        sources = list(full.execute("""
            SELECT source_key,title,scope,verification_level,source_url,COALESCE(archive_sha256,'')
            FROM mems_knowledge_source WHERE source_key LIKE ? ORDER BY source_key
        """, (PREFIX + "%",))) if exists(full, "mems_knowledge_source") else []

        semantic_tables = [
            ("mems_firmware_catalog", "Firmwares"),
            ("mems_axis_definition", "Définitions d'axes / unités / échelles"),
            ("mems_scalar_definition", "Définitions de scalaires"),
            ("mems_scalar_binding", "Liaisons scalaire <-> firmware/adresse"),
            ("mems_table_definition", "Définitions de tables/cartographies"),
            ("mems_table_binding", "Liaisons table <-> firmware/index"),
            ("mems_variable_correlation", "Corrélations de variables"),
            ("mems_source_asset", "Fichiers sources MEMSTools indexés"),
        ]
        counts = {t: scalar(full, f"SELECT COUNT(*) FROM {t} WHERE source_key LIKE ?", (PREFIX + "%",)) if exists(full, t) else 0 for t,_ in semantic_tables}
        family_rows = list(full.execute("""
            SELECT source_key,family,COUNT(*) FROM mems_firmware_catalog
            WHERE source_key LIKE ? GROUP BY source_key,family ORDER BY family,source_key
        """, (PREFIX + "%",))) if exists(full, "mems_firmware_catalog") else []

        firmware_ids = [r[0] for r in full.execute("SELECT DISTINCT firmware_code FROM mems_firmware_catalog WHERE source_key LIKE ?", (PREFIX + "%",))] if exists(full, "mems_firmware_catalog") else []
        fw_overlap = sum(1 for v in firmware_ids if token(v) in tokens)
        identifiers = []
        for table in ("mems_scalar_definition", "mems_table_definition"):
            if exists(full, table):
                identifiers.extend(r[0] for r in full.execute(f"SELECT DISTINCT identifier FROM {table} WHERE source_key LIKE ? AND COALESCE(identifier,'')<>''", (PREFIX + "%",)))
        ident_overlap = sum(1 for v in identifiers if token(v) in tokens)

        mems12 = {
            "firmwares": scalar(full, "SELECT COUNT(*) FROM mems_firmware_catalog WHERE source_key='andrew_revill_memstools_mems12'") if exists(full, "mems_firmware_catalog") else 0,
            "scalars": scalar(full, "SELECT COUNT(*) FROM mems_scalar_binding WHERE source_key='andrew_revill_memstools_mems12'") if exists(full, "mems_scalar_binding") else 0,
            "tables": scalar(full, "SELECT COUNT(*) FROM mems_table_binding WHERE source_key='andrew_revill_memstools_mems12'") if exists(full, "mems_table_binding") else 0,
            "variables": scalar(full, "SELECT COUNT(*) FROM mems_variable_correlation WHERE source_key='andrew_revill_memstools_mems12'") if exists(full, "mems_variable_correlation") else 0,
            "assets": scalar(full, "SELECT COUNT(*) FROM mems_source_asset WHERE source_key='andrew_revill_memstools_mems12'") if exists(full, "mems_source_asset") else 0,
        }
        aanmp = full.execute("""
            SELECT b.address_hex,d.identifier,d.name,a.suffix,a.scale,a.offset
            FROM mems_scalar_binding b
            JOIN mems_scalar_definition d USING(source_key,class_key)
            LEFT JOIN mems_axis_definition a ON a.source_key=d.source_key AND a.class_key=d.axis_class
            WHERE b.source_key='andrew_revill_memstools' AND b.firmware_code='AANMP002' AND d.identifier='inj_pw_rosco'
            ORDER BY b.address_int LIMIT 1
        """).fetchone() if exists(full, "mems_scalar_binding") else None
        q = quality(full)

        summary = {
            "baseline_table_count": len(tables(base)), "full_table_count": len(tables(full)),
            "sources": [dict(source_key=r[0], title=r[1], scope=r[2], verification_level=r[3], url=r[4], archive_sha256=r[5]) for r in sources],
            "datasets": counts,
            "family_firmware_counts": [list(r) for r in family_rows],
            "firmware_exact_text_overlap": {"matched": fw_overlap, "total": len(firmware_ids)},
            "identifier_exact_text_overlap": {"matched": ident_overlap, "total": len(identifiers)},
            "quality": q, "mems_1_2": mems12,
            "aanmp002_inj_pw_rosco": list(aanmp) if aanmp else None,
        }

        lines = [
            "# Audit Andrew Revill -> base de connaissance ECU MEMS Manager", "",
            "## Principe d'audit", "",
            "La base `avant Andrew` exclut explicitement tous les lots Andrew 1500-1540 et 1560-1600. Le lot 1550 reste dans la base de comparaison car il appartient à la source distincte rover-mems-386.",
            "Toutes les données Andrew restent `source_externe`; aucune corrélation n'est promue automatiquement en donnée validée projet ou constructeur.", "",
            "Les scalaires/tables de cartographie, le protocole ROSCO 0x7D/0x80 et la RAM Mode 4 restent trois couches distinctes.", "",
            "## Sources Andrew intégrées", "",
        ]
        for r in sources:
            sha = f" — SHA-256 `{r[5]}`" if r[5] else ""
            lines.append(f"- `{r[0]}` — {r[1]} — **{r[2]}** — `{r[3]}`{sha}")
        lines += ["", "## Volume sémantique MEMSTools", "", "| Jeu de données | Lignes |", "|---|---:|"]
        for table,label in semantic_tables:
            lines.append(f"| {label} | {counts[table]} |")
        lines += ["", "## Couverture firmware", "", "| Source | Famille | Firmwares |", "|---|---|---:|"]
        for source,family,n in family_rows:
            lines.append(f"| `{source}` | MEMS {family} | {n} |")
        lines += [
            "", "## Comparaison avec la base antérieure", "",
            f"- Tables avant Andrew : **{len(tables(base))}** ; après tous les lots Andrew actuellement présents : **{len(tables(full))}**.",
            f"- Firmwares Andrew déjà cités textuellement dans l'ancienne base : **{fw_overlap}/{len(firmware_ids)}**.",
            f"- Identifiants scalaires/tables Andrew retrouvés comme jetons exacts dans l'ancienne base : **{ident_overlap}/{len(identifiers)}**.",
            "Ces recouvrements sont documentaires uniquement; ils ne constituent pas une validation technique.", "",
            "## Contrôles d'intégrité", "",
        ]
        lines.extend(f"- `{k}` : **{v}**" for k,v in q.items())
        lines += ["", "## MEMS 1.2", "",
                  f"- Firmwares corrélés dans le ZIP MEMSTools : **{mems12['firmwares']}**",
                  f"- Liaisons scalaires : **{mems12['scalars']}**",
                  f"- Liaisons tables : **{mems12['tables']}**",
                  f"- Corrélations variables : **{mems12['variables']}**",
                  f"- Fichiers 1.2 détectés par l'extracteur : **{mems12['assets']}**"]
        if not any(mems12.values()):
            lines.append("Le ZIP courant ne fournit donc pas de corpus de corrélation 1.2 équivalent à 1.3/1.6/1.9. Ce manque reste explicite; aucune adresse n'est inventée.")
        lines += ["", "## Point de contrôle AANMP002", ""]
        if aanmp:
            lines.append(f"`inj_pw_rosco` : scalaire `{aanmp[0]}`, unité `{aanmp[3]}`, échelle `{aanmp[4]}`, offset `{aanmp[5]}`.")
            lines.append("Cette information appartient au modèle de calibration MEMS Mapper; elle n'est pas assimilée à l'adresse RAM Mode 4 utilisée par ECU MEMS Manager.")
        else:
            lines.append("Contrôle `AANMP002 / inj_pw_rosco` non retrouvé.")
        lines += ["", "## Règle pour le moteur expert", "",
                  "- Utiliser la provenance et le niveau de confiance de chaque fait.",
                  "- Une connaissance Andrew seule peut soutenir une hypothèse ou un contrôle, pas une conclusion forte.",
                  "- Une adresse corrélée ne devient une adresse RAM live qu'après validation du chemin de lecture et de sa conversion pour le firmware concerné.", ""]

        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text("\n".join(lines), encoding="utf-8")
        args.json_output.parent.mkdir(parents=True, exist_ok=True)
        args.json_output.write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        print("\n".join(lines))
    finally:
        base.close(); full.close(); base_path.unlink(missing_ok=True); full_path.unlink(missing_ok=True)


if __name__ == "__main__":
    main()
