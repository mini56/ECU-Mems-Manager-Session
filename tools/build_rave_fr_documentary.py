#!/usr/bin/env python3
"""Reconstruire une base documentaire française à partir des LOTS RCL0193FRE.

N'extrait PAS l'anglais AKM7169ENG et ne traduit aucun document.
Rejoue une copie des enrichissements historiques dans une base temporaire
(jamais la base ECU de l'utilisateur), puis exporte la source française.
Un lot manquant ou une extraction vide provoque un échec, sans publication.
"""
from __future__ import annotations

import argparse
import base64
import hashlib
import json
import re
import sqlite3
import tempfile
import zlib
from collections import defaultdict
from pathlib import Path

SOURCE = "RCL0193FRE"
SOURCE_SHA = "0c7fef28d0d0f0673ba321d6625a019c005823103caa98afb3258114e1fec713"
LOTS = tuple(range(1790, 1870, 10))
TABLES = (
    "mems_knowledge_item", "mems_knowledge_scope",
    "mems_applicability_scope", "mems_scope_constraint",
    "mems_specification", "mems_specification_value",
    "mems_procedure", "mems_procedure_step", "mems_procedure_requirement",
    "mems_knowledge_relation", "mems_term_alias",
    "mems_rave_fact", "mems_expert_fact_external",
)
CONTENT_FIELDS = (
    "topic", "source_text", "source_section", "title", "description",
    "statement", "step_text", "instruction", "action_text",
    "requirement_text", "procedure_text", "value_text",
    "specification", "value", "unit", "notes", "reference",
)
SCHEMA = """
PRAGMA foreign_keys=ON;
CREATE TABLE documentary_database(
    singleton INTEGER PRIMARY KEY CHECK(singleton=1),
    schema_version INTEGER NOT NULL,
    language TEXT NOT NULL CHECK(language='fr'),
    collection_revision INTEGER NOT NULL CHECK(collection_revision>0),
    publication_status TEXT NOT NULL CHECK(publication_status IN ('source_inventory','validated'))
);
CREATE TABLE documentary_documents(
    document_id TEXT NOT NULL,
    source_revision TEXT NOT NULL,
    language TEXT NOT NULL CHECK(language='fr'),
    publisher TEXT NOT NULL,
    edition_code TEXT NOT NULL,
    source_filename TEXT NOT NULL,
    source_sha256 TEXT NOT NULL,
    source_pages INTEGER NOT NULL,
    extraction TEXT NOT NULL,
    PRIMARY KEY(document_id,source_revision)
);
CREATE TABLE documentary_pages(
    document_id TEXT NOT NULL,
    source_revision TEXT NOT NULL,
    physical_page INTEGER NOT NULL CHECK(physical_page BETWEEN 1 AND 371),
    structured_records INTEGER NOT NULL DEFAULT 0,
    indexed_state TEXT NOT NULL CHECK(indexed_state IN ('source_linked','not_yet_structured')),
    PRIMARY KEY(document_id,source_revision,physical_page),
    FOREIGN KEY(document_id,source_revision)
      REFERENCES documentary_documents(document_id,source_revision)
);
CREATE TABLE documentary_entries(
    document_id TEXT NOT NULL,
    source_revision TEXT NOT NULL,
    entry_id TEXT NOT NULL,
    entry_type TEXT NOT NULL CHECK(entry_type IN
      ('reference','procedure','step','technical_value','notice','figure','other')),
    source_page INTEGER CHECK(source_page BETWEEN 1 AND 371),
    title TEXT NOT NULL,
    source_text TEXT NOT NULL,
    verification_status TEXT NOT NULL CHECK(verification_status IN
      ('source_checked','review_required','technical_reviewed')),
    source_table TEXT NOT NULL,
    raw_json TEXT NOT NULL,
    PRIMARY KEY(document_id,source_revision,entry_id),
    FOREIGN KEY(document_id,source_revision)
      REFERENCES documentary_documents(document_id,source_revision)
);
CREATE TABLE documentary_source_rows(
    table_name TEXT NOT NULL,
    source_index INTEGER NOT NULL,
    row_json TEXT NOT NULL,
    PRIMARY KEY(table_name,source_index)
);
CREATE INDEX idx_documentary_page ON documentary_entries(source_page);
CREATE INDEX idx_documentary_type ON documentary_entries(entry_type);
CREATE VIRTUAL TABLE documentary_fts USING fts5(title, source_text,
  content='documentary_entries', content_rowid='rowid',
  tokenize='unicode61 remove_diacritics 2');
CREATE TRIGGER documentary_fts_insert AFTER INSERT ON documentary_entries BEGIN
 INSERT INTO documentary_fts(rowid,title,source_text)
 VALUES (new.rowid,new.title,new.source_text);
END;
"""

def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def decode_qz64(data: bytes) -> str:
    compact = b"".join(data.split())
    try:
        raw = base64.b64decode(compact, validate=True)
    except ValueError:
        raw = data  # ancien lot 1620 : qCompress binaire
    if len(raw) < 5:
        raise ValueError("QZ64 trop court")
    expected = int.from_bytes(raw[:4], "big")
    decoded = zlib.decompress(raw[4:])
    if expected and len(decoded) != expected:
        raise ValueError("Taille Qt qUncompress incorrecte")
    return decoded.decode("utf-8-sig")

def seed_reference(reference_root: Path, conn: sqlite3.Connection) -> None:
    parts = sorted(reference_root.glob("mems_reference_seed_*.qz64"))
    if len(parts) != 4:
        raise ValueError("Les 4 segments du seed sont requis")
    seed_sql = decode_qz64(b"".join(p.read_bytes().strip() for p in parts))
    for line in seed_sql.splitlines():
        statement = line.strip()
        if statement:
            conn.execute(statement)
    manifest = json.loads((reference_root / "manifest.json").read_text(encoding="utf-8"))
    listed = [reference_root / p for p in manifest["research_enrichment_batches"]]
    extras = sorted(reference_root.glob("research_enrichment*.qz64"),
                    key=lambda p: (int(re.search(r"_(\d+)\.qz64$", p.name).group(1))
                         if re.search(r"_(\d+)\.qz64$", p.name) else 0, p.name))
    already = set()
    applied = []
    for path in listed + extras:
        if path.name in already or path.name == "research_enrichment_1600.qz64":
            continue  # archive de corrélations brute exclue du runtime de référence
        if not path.is_file():
            raise FileNotFoundError(f"Lot référencé absent : {path}")
        already.add(path.name)
        sql = decode_qz64(path.read_bytes())
        sql = "\n".join(s for s in sql.splitlines()
                        if not s.lstrip().startswith("--"))
        try:
            if ";" in sql:
                conn.executescript(sql)
            else:
                for line in sql.splitlines():
                    if line.strip():
                        conn.execute(line.strip())
        except sqlite3.Error as exc:
            raise RuntimeError(f"Erreur lot {path.name}: {exc}") from exc
        applied.append(path.name)
    missing = {f"research_enrichment_{lot}.qz64" for lot in LOTS} - set(applied)
    if missing:
        raise RuntimeError(f"Lots français manquants : {sorted(missing)}")

def table_names(conn: sqlite3.Connection) -> set[str]:
    return {row[0] for row in conn.execute(
        "SELECT name FROM sqlite_master WHERE type='table'")}

def rows(conn: sqlite3.Connection, table: str) -> list[dict]:
    if table not in table_names(conn):
        return []
    return [dict(r) for r in conn.execute(f'SELECT * FROM "{table}"')]

def as_json(row: dict) -> str:
    def clean(value):
        if isinstance(value, bytes):
            return {"_base64": base64.b64encode(value).decode("ascii")}
        return value
    return json.dumps({k: clean(v) for k,v in row.items()},
                      ensure_ascii=False, sort_keys=True)

def linked(row: dict, parent_key: str, parents: set) -> bool:
    return row.get(parent_key) in parents if parent_key in row else False

def contains_source(row: dict) -> bool:
    return any(SOURCE in str(row.get(k) or "").upper() for k in
               ("knowledge_key","source_key","document","legacy_rave_fact_key",
                "fact_key","source_section","topic","notes"))

def extract_source(conn: sqlite3.Connection) -> dict[str,list[dict]]:
    all_rows = {name: rows(conn,name) for name in TABLES}
    knowledge = [r for r in all_rows["mems_knowledge_item"] if contains_source(r)]
    if len(knowledge) < 100:
        raise RuntimeError(f"Inventaire français anormalement vide : {len(knowledge)} connaissances")
    keys = {r["knowledge_key"] for r in knowledge if r.get("knowledge_key")}
    selection = {"mems_knowledge_item": knowledge}
    selection["mems_knowledge_scope"] = [
        r for r in all_rows["mems_knowledge_scope"] if linked(r,"knowledge_key",keys)]
    scopes = {r.get("scope_key") for r in selection["mems_knowledge_scope"]}
    selection["mems_applicability_scope"] = [
        r for r in all_rows["mems_applicability_scope"] if r.get("scope_key") in scopes]
    selection["mems_scope_constraint"] = [
        r for r in all_rows["mems_scope_constraint"] if linked(r,"scope_key",scopes)]
    selection["mems_specification"] = [
        r for r in all_rows["mems_specification"]
        if linked(r,"knowledge_key",keys) or contains_source(r)]
    specs = {r.get("specification_key") for r in selection["mems_specification"]}
    selection["mems_specification_value"] = [
        r for r in all_rows["mems_specification_value"]
        if linked(r,"specification_key",specs)]
    selection["mems_procedure"] = [
        r for r in all_rows["mems_procedure"]
        if linked(r,"knowledge_key",keys) or contains_source(r)]
    procs = {r.get("procedure_key") for r in selection["mems_procedure"]}
    selection["mems_procedure_step"] = [
        r for r in all_rows["mems_procedure_step"]
        if linked(r,"procedure_key",procs)]
    selection["mems_procedure_requirement"] = [
        r for r in all_rows["mems_procedure_requirement"]
        if linked(r,"procedure_key",procs)]
    selection["mems_knowledge_relation"] = [
        r for r in all_rows["mems_knowledge_relation"]
        if any(value in keys for name,value in r.items() if name.endswith("knowledge_key"))]
    selection["mems_term_alias"] = [
        r for r in all_rows["mems_term_alias"]
        if linked(r,"knowledge_key",keys) or contains_source(r)]
    selection["mems_rave_fact"] = [
        r for r in all_rows["mems_rave_fact"] if contains_source(r)]
    selection["mems_expert_fact_external"] = [
        r for r in all_rows["mems_expert_fact_external"] if contains_source(r)]
    return selection

def physical_page(row: dict) -> int | None:
    for key in ("image_ref","source_section","source_key","knowledge_key","notes",
                "page","physical_page"):
        text = str(row.get(key) or "")
        for pattern in (r"RCL0193FRE[_:]PDF[_:](\d{1,3})",
                        r"\bPDF[: #p.]+(\d{1,3})\b"):
            match = re.search(pattern,text,re.I)
            if match:
                page = int(match.group(1))
                if 1 <= page <= 371:
                    return page
    return None

def record_text(row: dict) -> str:
    values = []
    for field in CONTENT_FIELDS:
        value = row.get(field)
        if value is not None and str(value).strip():
            values.append(f"{field}: {value}")
    return "\n".join(values) if values else "Source structurée : " + as_json(row)

def build(reference_root: Path, destination: Path) -> dict:
    destination.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp:
        scratch = sqlite3.connect(Path(tmp)/"reference-rebuilt.sqlite")
        scratch.row_factory = sqlite3.Row
        try:
            seed_reference(reference_root,scratch)
            selection = extract_source(scratch)
        finally:
            scratch.close()
    if destination.exists():
        destination.unlink()  # sortie explicite uniquement, jamais la base utilisateur
    fr = sqlite3.connect(destination)
    try:
        fr.executescript(SCHEMA)
        fr.execute("INSERT INTO documentary_database VALUES(1,1,'fr',1,'source_inventory')")
        fr.execute("INSERT INTO documentary_documents VALUES(?,?,?,?,?,?,?, ?,?)",
            (SOURCE,"source-original-auditee-371","fr","Rover Group",SOURCE,
             "Manuel Rover MPI.pdf",SOURCE_SHA,371,"lots originaux 1790-1860"))
        fr.executemany("INSERT INTO documentary_pages VALUES(?,?,?,?,?)",
            ((SOURCE,"source-original-auditee-371",p,0,"not_yet_structured")
             for p in range(1,372)))
        types = {
            "mems_knowledge_item":"reference",
            "mems_specification":"technical_value",
            "mems_specification_value":"technical_value",
            "mems_procedure":"procedure",
            "mems_procedure_step":"step",
            "mems_procedure_requirement":"notice",
            "mems_term_alias":"other",
            "mems_rave_fact":"reference",
            "mems_expert_fact_external":"reference",
        }
        source_pages = defaultdict(int)
        entries = 0
        for table in TABLES:
            for index,row in enumerate(selection.get(table,[]),start=1):
                fr.execute("INSERT INTO documentary_source_rows VALUES(?,?,?)",
                           (table,index,as_json(row)))
                if table not in types:
                    continue
                page = physical_page(row)
                text = record_text(row)
                title = str(row.get("topic") or row.get("title") or
                            row.get("name") or row.get("source_section") or
                            row.get("knowledge_key") or table)
                status = ("source_checked"
                          if str(row.get("verification_level") or "").lower()
                             in ("verifie_constructeur","verifie_document","source_checked")
                          else "review_required")
                fr.execute("INSERT INTO documentary_entries VALUES(?,?,?,?,?,?,?,?,?,?)",
                    (SOURCE,"source-original-auditee-371",f"{table}:{index}",
                     types[table],page,title,text,status,table,as_json(row)))
                entries += 1
                if page:
                    source_pages[page]+=1
        fr.executemany(
            "UPDATE documentary_pages SET structured_records=?,indexed_state='source_linked' "
            "WHERE document_id=? AND physical_page=?",
            ((count,SOURCE,page) for page,count in source_pages.items()))
        fr.commit()
        integrity = fr.execute("PRAGMA integrity_check").fetchone()[0]
        fk = fr.execute("PRAGMA foreign_key_check").fetchall()
        fts_count = fr.execute("SELECT count(*) FROM documentary_fts").fetchone()[0]
        if integrity != "ok" or fk or fts_count != entries:
            raise RuntimeError(f"Intégrité documentaire: {integrity}, FK={len(fk)}, FTS={fts_count}/{entries}")
        if fr.execute("SELECT COUNT(*) FROM documentary_pages").fetchone()[0] != 371:
            raise RuntimeError("371 pages source exigées")
        report = {
            "source": SOURCE,
            "source_pdf_original_sha256": SOURCE_SHA,
            "language": "fr",
            "translation_from_english": False,
            "english_akm7169_included": False,
            "reference_lots": list(LOTS),
            "source_pages": 371,
            "pages_with_explicit_structured_links": len(source_pages),
            "pages_without_structured_links": 371-len(source_pages),
            "documentary_entries": entries,
            "source_rows_by_table": {k:len(v) for k,v in selection.items()},
            "sqlite_integrity": integrity,
            "publication_ready": False,
            "reason": "Inventaire de migration. Contrôler les pages non structurées et adapter le lecteur C++ avant diffusion.",
        }
        return report
    except Exception:
        fr.close()
        destination.unlink(missing_ok=True)
        raise
    finally:
        try:
            fr.close()
        except Exception:
            pass

def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--reference-root",type=Path,default=Path("database/reference"))
    parser.add_argument("--output",type=Path,default=Path("/tmp/mems_documentary_fr.sqlite"))
    args = parser.parse_args()
    report = build(args.reference_root,args.output)
    (args.output.parent/"mems_documentary_fr_report.json").write_text(
        json.dumps(report,ensure_ascii=False,indent=2),encoding="utf8")
    print("RAVE_FR_SOURCE_MIGRATION",json.dumps(report,ensure_ascii=False))

if __name__=="__main__":
    main()
