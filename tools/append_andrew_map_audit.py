#!/usr/bin/env python3
"""Append Map/Firmware Library index statistics to the Andrew audit report."""
from __future__ import annotations
import argparse, base64, json, pathlib, re, sqlite3, struct, tempfile, zlib


def num(p):
    m=re.search(r"_(\d+)\.qz64$",p.name,re.I); return int(m.group(1)) if m else 0

def unpack(b):
    p=base64.b64decode(b.strip()); n=struct.unpack(">I",p[:4])[0]; raw=zlib.decompress(p[4:])
    if len(raw)!=n: raise RuntimeError("qCompress mismatch")
    return raw.decode("utf-8")
def execsql(c,t):
    for s in t.splitlines():
        s=s.strip()
        if s and not s.startswith("--"): c.execute(s)
    c.commit()
def rebuild(root):
    f=tempfile.NamedTemporaryFile(delete=False,suffix=".sqlite"); f.close(); p=pathlib.Path(f.name); c=sqlite3.connect(p)
    seed=b"".join(x.read_bytes().strip() for x in sorted(root.glob("mems_reference_seed_*.qz64"),key=num)); execsql(c,unpack(seed))
    for x in sorted(root.rglob("research_enrichment*.qz64"),key=lambda q:(num(q),q.as_posix())): execsql(c,unpack(x.read_bytes()))
    return c,p

def main():
    ap=argparse.ArgumentParser(); ap.add_argument("reference_dir",type=pathlib.Path); ap.add_argument("--report",type=pathlib.Path,required=True); ap.add_argument("--json-output",type=pathlib.Path,required=True); a=ap.parse_args()
    c,p=rebuild(a.reference_dir)
    try:
        rows=list(c.execute("SELECT family,library_area,status,file_count FROM andrew_map_library_coverage WHERE source_key='andrew_revill_map_firmware_library' ORDER BY family,library_area"))
        total=c.execute("SELECT COUNT(*) FROM andrew_map_library_asset WHERE source_key='andrew_revill_map_firmware_library'").fetchone()[0]
        fw=c.execute("SELECT COUNT(DISTINCT firmware_code) FROM andrew_map_library_asset WHERE source_key='andrew_revill_map_firmware_library' AND COALESCE(firmware_code,'')<>''").fetchone()[0]
        parts=c.execute("SELECT COUNT(*) FROM andrew_map_library_asset WHERE source_key='andrew_revill_map_firmware_library' AND COALESCE(part_numbers,'')<>''").fetchone()[0]
        kinds=list(c.execute("SELECT file_kind,COUNT(*) FROM andrew_map_library_asset WHERE source_key='andrew_revill_map_firmware_library' GROUP BY file_kind ORDER BY COUNT(*) DESC,file_kind"))
        text=a.report.read_text(encoding="utf-8").rstrip()+"\n\n## Index Andrew Map/Firmware Library\n\n"
        text+=f"La bibliothèque publique Andrew est indexée dans la base sans embarquer les gros binaires : **{total} fichiers/liens**, **{fw} identifiants firmware distincts** et **{parts} entrées contenant au moins une référence MNE/MKC/etc.**\n\n"
        text+="| Famille | Zone | Statut | Fichiers |\n|---|---|---|---:|\n"
        for family,area,status,n in rows: text+=f"| MEMS {family} | {area} | {status} | {n} |\n"
        text+="\nRépartition des types de fichiers : "+", ".join(f"`{k}` {n}" for k,n in kinds)+".\n\n"
        text+="Pour MEMS 1.2, l'absence de répertoire de bibliothèque/corrélation confirmé reste explicitement enregistrée. Les capacités 1.2 publiées par Andrew sont conservées séparément dans les tables de capacités/transport ; aucune cartographie ou adresse 1.2 n'est inventée.\n"
        a.report.write_text(text,encoding="utf-8")
        data=json.loads(a.json_output.read_text(encoding="utf-8")) if a.json_output.exists() else {}
        data["map_firmware_library"]={"assets":total,"distinct_firmware_codes":fw,"assets_with_part_numbers":parts,"coverage":[{"family":r[0],"area":r[1],"status":r[2],"file_count":r[3]} for r in rows],"file_kinds":{k:n for k,n in kinds}}
        a.json_output.write_text(json.dumps(data,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
        print(json.dumps(data["map_firmware_library"],ensure_ascii=False,indent=2))
    finally:
        c.close(); p.unlink(missing_ok=True)
if __name__=="__main__": main()
