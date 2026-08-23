#!/usr/bin/env python3
"""Append completeness statistics for raw MEMSTools metadata to Andrew audit."""
from __future__ import annotations
import argparse, base64, json, pathlib, re, sqlite3, struct, tempfile, zlib


def n(p):
    m=re.search(r"_(\d+)\.qz64$",p.name,re.I); return int(m.group(1)) if m else 0
def unpack(b):
    p=base64.b64decode(b.strip()); size=struct.unpack(">I",p[:4])[0]; raw=zlib.decompress(p[4:])
    if len(raw)!=size: raise RuntimeError("qCompress mismatch")
    return raw.decode("utf-8")
def run(c,t):
    for s in t.splitlines():
        s=s.strip()
        if s and not s.startswith("--"): c.execute(s)
    c.commit()
def rebuild(root):
    f=tempfile.NamedTemporaryFile(delete=False,suffix=".sqlite"); f.close(); p=pathlib.Path(f.name); c=sqlite3.connect(p)
    run(c,unpack(b"".join(x.read_bytes().strip() for x in sorted(root.glob("mems_reference_seed_*.qz64"),key=n))))
    for x in sorted(root.rglob("research_enrichment*.qz64"),key=lambda q:(n(q),q.as_posix())): run(c,unpack(x.read_bytes()))
    return c,p
def main():
    ap=argparse.ArgumentParser(); ap.add_argument("reference_dir",type=pathlib.Path); ap.add_argument("--report",type=pathlib.Path,required=True); ap.add_argument("--json-output",type=pathlib.Path,required=True); a=ap.parse_args()
    c,p=rebuild(a.reference_dir)
    try:
        props=c.execute("SELECT COUNT(*) FROM mems_definition_property_external WHERE source_key='andrew_revill_memstools_complete_metadata'").fetchone()[0]
        dims=c.execute("SELECT COUNT(*) FROM mems_dimension_property_external WHERE source_key='andrew_revill_memstools_complete_metadata'").fetchone()[0]
        ds=list(c.execute("SELECT family,variant,dataset,row_count,column_count,non_empty_cells,relative_path FROM mems_correlation_dataset_external WHERE source_key='andrew_revill_memstools_complete_metadata' ORDER BY family,variant,dataset"))
        cells=c.execute("SELECT COUNT(*) FROM mems_correlation_cell_external WHERE source_key='andrew_revill_memstools_complete_metadata'").fetchone()[0]
        text=a.report.read_text(encoding="utf-8").rstrip()+"\n\n## Conservation complète des métadonnées MEMSTools\n\n"
        text+=f"En plus des tables sémantiques déjà décodées, la base conserve **{props} propriétés DEF**, **{dims} propriétés DIM** et **{cells} cellules de matrices de corrélation non vides**. Cette couche est volontairement générique : elle empêche de perdre aujourd'hui une donnée Andrew dont la signification ne serait exploitée que plus tard par le moteur expert.\n\n"
        text+="| Famille | Variante | Dataset | Lignes | Colonnes | Cellules non vides |\n|---|---|---|---:|---:|---:|\n"
        for family,variant,dataset,rows,cols,nonempty,path in ds:
            text+=f"| MEMS {family} | {variant or ''} | `{dataset}` | {rows} | {cols} | {nonempty} |\n"
        text+="\nLes tables sémantiques (`mems_scalar_*`, `mems_table_*`, `mems_variable_correlation`) restent préférées pour les champs déjà compris. La couche générique sert de conservation/audit et ne transforme pas automatiquement un champ brut en fait diagnostique.\n"
        a.report.write_text(text,encoding="utf-8")
        data=json.loads(a.json_output.read_text(encoding="utf-8")) if a.json_output.exists() else {}
        data["memstools_complete_metadata"]={"definition_properties":props,"dimension_properties":dims,"correlation_cells":cells,"datasets":[{"family":r[0],"variant":r[1],"dataset":r[2],"rows":r[3],"columns":r[4],"non_empty_cells":r[5],"path":r[6]} for r in ds]}
        a.json_output.write_text(json.dumps(data,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
        print(json.dumps(data["memstools_complete_metadata"],ensure_ascii=False,indent=2))
    finally:
        c.close(); p.unlink(missing_ok=True)
if __name__=="__main__": main()
