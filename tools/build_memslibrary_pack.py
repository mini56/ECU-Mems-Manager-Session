#!/usr/bin/env python3
import argparse, glob, hashlib, json, os, pathlib, re, sqlite3

PACK_SCHEMA_VERSION = 2
PACK_FORMAT = "MEMSLibraryKnowledgePack"
SOURCE_RUN_ID = "33810202288"
SOURCE_ARTIFACT_ID = "9914590689"
ENGINE_SHA = "5232215f5853241572f580cf1696c9f881b254f4"

LOCAL_KEY_COLUMNS = {
    "ravemems_page": {"page_key"},
    "ravemems_visual": {"visual_key"},
}
REFERENCE_COLUMNS = {
    ("ravemems_provenance", "page_key"): "page",
    ("ravemems_visual", "page_key"): "page",
    ("ravemems_visual_link", "visual_key"): "visual",
}

def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()

def qident(name):
    return '"' + name.replace('"', '""') + '"'

def list_source_dbs(root):
    return sorted(glob.glob(os.path.join(root, "**", "ravemems_v2.sqlite"), recursive=True))

def get_tables(cx):
    return [r[0] for r in cx.execute(
        "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%' ORDER BY name"
    )]

def get_create_sql(cx, table):
    row = cx.execute("SELECT sql FROM sqlite_master WHERE type='table' AND name=?", (table,)).fetchone()
    if not row or not row[0]:
        raise RuntimeError(f"missing CREATE TABLE for {table}")
    return row[0]

def columns(cx, table):
    return [r[1] for r in cx.execute(f"PRAGMA table_info({qident(table)})")]

def doc_context(cx):
    doc = cx.execute("SELECT document_key FROM ravemems_document").fetchall()
    rev = cx.execute("SELECT revision_key, document_key FROM ravemems_document_revision").fetchall()
    if len(doc) != 1 or len(rev) != 1 or rev[0][1] != doc[0][0]:
        raise RuntimeError("each source database must contain exactly one matching document/revision")
    return doc[0][0], rev[0][0]

def local_key(revision_key, old_key):
    if old_key is None:
        return None
    return f"{revision_key}::{old_key}"

def transformed_row(table, cols, row, revision_key):
    vals = list(row)
    for i, col in enumerate(cols):
        if vals[i] is None:
            continue
        if col in LOCAL_KEY_COLUMNS.get(table, set()):
            vals[i] = local_key(revision_key, vals[i])
        ref_kind = REFERENCE_COLUMNS.get((table, col))
        if ref_kind in ("page", "visual"):
            vals[i] = local_key(revision_key, vals[i])
    return vals

def normalize_search_text(*parts):
    text = " ".join(str(part) for part in parts if part).casefold()
    text = re.sub(r"[^\w-]+", " ", text, flags=re.UNICODE)
    return " ".join(text.split())

def create_search_schema(out):
    out.executescript("""
    CREATE TABLE memslibrary_pack (
        pack_id TEXT PRIMARY KEY,
        pack_format TEXT NOT NULL,
        pack_schema_version INTEGER NOT NULL,
        source_run_id TEXT NOT NULL,
        source_artifact_id TEXT NOT NULL,
        source_engine_sha TEXT NOT NULL,
        document_count INTEGER NOT NULL,
        source_database_count INTEGER NOT NULL
    );
    CREATE TABLE memslibrary_search (
        search_key TEXT PRIMARY KEY,
        document_key TEXT NOT NULL,
        revision_key TEXT NOT NULL,
        page_number INTEGER,
        entity_kind TEXT NOT NULL,
        entity_key TEXT NOT NULL,
        source_language TEXT NOT NULL,
        title TEXT,
        body TEXT,
        search_text TEXT NOT NULL,
        FOREIGN KEY(document_key) REFERENCES ravemems_document(document_key),
        FOREIGN KEY(revision_key) REFERENCES ravemems_document_revision(revision_key)
    );
    CREATE INDEX idx_memslibrary_search_document ON memslibrary_search(document_key);
    CREATE INDEX idx_memslibrary_search_revision ON memslibrary_search(revision_key);
    CREATE INDEX idx_memslibrary_search_page ON memslibrary_search(document_key, page_number);
    CREATE INDEX idx_memslibrary_search_kind ON memslibrary_search(entity_kind);
    CREATE INDEX idx_memslibrary_search_language ON memslibrary_search(source_language);
    CREATE INDEX idx_memslibrary_search_language_kind ON memslibrary_search(source_language, entity_kind);
    """)

def first_page_map(out):
    rows = out.execute("""
       SELECT entity_kind, entity_key, MIN(p.physical_page)
       FROM ravemems_provenance pr
       JOIN ravemems_page p ON p.page_key=pr.page_key
       GROUP BY entity_kind, entity_key
    """).fetchall()
    return {(r[0], r[1]): r[2] for r in rows}

def add_search_rows(out):
    page_map = first_page_map(out)
    revision_for_doc = dict(out.execute("SELECT document_key, revision_key FROM ravemems_document_revision"))
    language_for_doc = dict(out.execute("SELECT document_key, source_language FROM ravemems_document"))
    doc_for_revision = dict(out.execute("SELECT revision_key, document_key FROM ravemems_document_revision"))
    rows = []

    for document_key, canonical_name, source_language, document_kind, manufacturer, title_source, _created in out.execute(
        "SELECT document_key, canonical_name, source_language, document_kind, manufacturer, title_source, created_at_utc FROM ravemems_document ORDER BY document_key"
    ):
        rev = revision_for_doc[document_key]
        body = " ".join(x for x in [canonical_name, document_kind, manufacturer, title_source] if x)
        rows.append((f"document::{document_key}", document_key, rev, None, "document", document_key,
                     source_language, title_source or canonical_name, body))

    for entity_key, revision_key, sequence_no, section_kind, manufacturer_identifier, title_source, source_language in out.execute(
        "SELECT section_key, revision_key, sequence_no, section_kind, manufacturer_identifier, title_source, source_language FROM ravemems_section ORDER BY revision_key, sequence_no, section_key"
    ):
        doc = doc_for_revision[revision_key]
        body = " ".join(x for x in [section_kind, manufacturer_identifier, title_source] if x)
        rows.append((f"section::{entity_key}", doc, revision_key, page_map.get(("section", entity_key), sequence_no),
                     "section", entity_key, source_language or language_for_doc.get(doc), title_source, body))

    for entity_key, revision_key, section_key, sequence_no, op_no, op_kind, title_source, source_language, completeness in out.execute(
        "SELECT operation_key,revision_key,section_key,sequence_no,manufacturer_operation_no,operation_kind,title_source,source_language,completeness_status FROM ravemems_operation ORDER BY revision_key,sequence_no,operation_key"
    ):
        doc = doc_for_revision[revision_key]
        body = " ".join(x for x in [op_no, op_kind, title_source] if x)
        rows.append((f"operation::{entity_key}", doc, revision_key, page_map.get(("operation", entity_key)),
                     "operation", entity_key, source_language or language_for_doc.get(doc), title_source, body))

    op_ctx = dict(out.execute("SELECT operation_key, revision_key FROM ravemems_operation"))
    for entity_key, operation_key, sequence_no, source_kind, norm_kind, title_source, completeness in out.execute(
        "SELECT phase_key,operation_key,sequence_no,phase_kind_source,normalized_phase_kind,title_source,completeness_status FROM ravemems_phase ORDER BY operation_key,sequence_no,phase_key"
    ):
        rev = op_ctx[operation_key]; doc = doc_for_revision[rev]
        body = " ".join(x for x in [source_kind, norm_kind, title_source] if x)
        rows.append((f"phase::{entity_key}", doc, rev, page_map.get(("phase", entity_key)),
                     "phase", entity_key, language_for_doc.get(doc), title_source, body))

    phase_ctx = {}
    for phase_key, operation_key in out.execute("SELECT phase_key, operation_key FROM ravemems_phase"):
        phase_ctx[phase_key] = op_ctx[operation_key]

    for entity_key, phase_key, sequence_no, manufacturer_step_no, instruction, condition_text, completeness, page_start, page_end in out.execute(
        "SELECT step_key,phase_key,sequence_no,manufacturer_step_no,instruction_source,condition_text,completeness_status,source_page_start,source_page_end FROM ravemems_step ORDER BY phase_key,sequence_no,step_key"
    ):
        rev = phase_ctx[phase_key]; doc = doc_for_revision[rev]
        body = " ".join(x for x in [manufacturer_step_no, instruction, condition_text] if x)
        page = page_map.get(("step", entity_key), page_start)
        rows.append((f"step::{entity_key}", doc, rev, page, "step", entity_key,
                     language_for_doc.get(doc), None, body))

    for entity_key, operation_key, sequence_no, notice_kind, source_text, scope_kind, target_operation, target_phase, target_step in out.execute(
        "SELECT notice_key,operation_key,sequence_no,notice_kind,source_text,scope_kind,target_operation_key,target_phase_key,target_step_key FROM ravemems_notice ORDER BY operation_key,sequence_no,notice_key"
    ):
        rev = op_ctx[operation_key]; doc = doc_for_revision[rev]
        body = " ".join(x for x in [notice_kind, source_text, scope_kind] if x)
        rows.append((f"notice::{entity_key}", doc, rev, page_map.get(("notice", entity_key)),
                     "notice", entity_key, language_for_doc.get(doc), notice_kind, body))

    for entity_key, operation_key, phase_key, step_key, sequence_no, req_type, req_source, part_no, quantity, unit, before_start, figure_ref in out.execute(
        "SELECT requirement_key,operation_key,phase_key,step_key,sequence_no,requirement_type,requirement_source,part_number,quantity,unit,before_start,figure_ref FROM ravemems_requirement ORDER BY operation_key,sequence_no,requirement_key"
    ):
        rev = op_ctx[operation_key]; doc = doc_for_revision[rev]
        body = " ".join(str(x) for x in [req_type, req_source, part_no, quantity, unit, figure_ref] if x is not None)
        rows.append((f"requirement::{entity_key}", doc, rev, page_map.get(("requirement", entity_key)),
                     "requirement", entity_key, language_for_doc.get(doc), req_type, body))

    for search_key, doc, rev, page, kind, entity, lang, title, body in rows:
        if not lang:
            raise RuntimeError(f"search row without source language: {search_key}")
        normalized = normalize_search_text(title, body)
        out.execute("""
            INSERT INTO memslibrary_search
            (search_key,document_key,revision_key,page_number,entity_kind,entity_key,source_language,title,body,search_text)
            VALUES (?,?,?,?,?,?,?,?,?,?)
        """, (search_key, doc, rev, page, kind, entity, lang, title, body, normalized))
    return len(rows)

def build(source_root, output_dir, pack_id):
    src_dbs = list_source_dbs(source_root)
    if len(src_dbs) != 47:
        raise RuntimeError(f"expected exactly 47 validated source databases, got {len(src_dbs)}")

    output_dir = pathlib.Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    db_path = output_dir / "knowledge.sqlite"
    if db_path.exists():
        db_path.unlink()

    first = sqlite3.connect(src_dbs[0])
    source_tables = get_tables(first)
    schema_sql = [(t, get_create_sql(first, t)) for t in source_tables]
    source_columns = {t: columns(first,t) for t in source_tables}
    first.close()

    source_manifest = []
    for path in src_dbs:
        cx = sqlite3.connect(path)
        if get_tables(cx) != source_tables:
            raise RuntimeError(f"schema table mismatch: {path}")
        for t in source_tables:
            if columns(cx,t) != source_columns[t]:
                raise RuntimeError(f"schema column mismatch: {path}: {t}")
        if cx.execute("PRAGMA integrity_check").fetchone()[0] != "ok":
            raise RuntimeError(f"source integrity failure: {path}")
        if cx.execute("PRAGMA foreign_key_check").fetchall():
            raise RuntimeError(f"source foreign key failure: {path}")
        doc, rev = doc_context(cx)
        language = cx.execute("SELECT source_language FROM ravemems_document WHERE document_key=?", (doc,)).fetchone()[0]
        if not language:
            raise RuntimeError(f"document without source language: {doc}")
        source_manifest.append({
            "document_key": doc,
            "revision_key": rev,
            "source_language": language,
            "source_database_sha256": sha256_file(path)
        })
        cx.close()

    out = sqlite3.connect(db_path)
    out.execute("PRAGMA foreign_keys=OFF")
    out.execute("PRAGMA journal_mode=DELETE")
    out.execute("PRAGMA synchronous=FULL")
    for _t, sql in schema_sql:
        out.execute(sql)

    for path in src_dbs:
        cx = sqlite3.connect(path)
        doc, rev = doc_context(cx)
        for t in source_tables:
            cols = source_columns[t]
            q = f"SELECT {','.join(qident(c) for c in cols)} FROM {qident(t)}"
            rows = [transformed_row(t, cols, row, rev) for row in cx.execute(q)]
            if rows:
                placeholders = ",".join("?" for _ in cols)
                out.executemany(
                    f"INSERT INTO {qident(t)} ({','.join(qident(c) for c in cols)}) VALUES ({placeholders})",
                    rows
                )
        cx.close()

    create_search_schema(out)
    search_count = add_search_rows(out)
    out.execute("""
       INSERT INTO memslibrary_pack
       (pack_id,pack_format,pack_schema_version,source_run_id,source_artifact_id,source_engine_sha,document_count,source_database_count)
       VALUES (?,?,?,?,?,?,?,?)
    """, (pack_id, PACK_FORMAT, PACK_SCHEMA_VERSION, SOURCE_RUN_ID, SOURCE_ARTIFACT_ID, ENGINE_SHA, 47, 47))
    out.commit()
    out.execute("PRAGMA foreign_keys=ON")
    integrity = out.execute("PRAGMA integrity_check").fetchone()[0]
    fk = out.execute("PRAGMA foreign_key_check").fetchall()
    doc_count = out.execute("SELECT COUNT(*) FROM ravemems_document").fetchone()[0]
    page_count = out.execute("SELECT COUNT(*) FROM ravemems_page").fetchone()[0]
    visual_count = out.execute("SELECT COUNT(*) FROM ravemems_visual").fetchone()[0]
    review_count = out.execute("SELECT COUNT(*) FROM ravemems_review_flag WHERE status IS NULL OR lower(status) NOT IN ('resolved','closed')").fetchone()[0]
    missing_language_count = out.execute("SELECT COUNT(*) FROM memslibrary_search WHERE source_language IS NULL OR trim(source_language)='' ").fetchone()[0]
    language_rows = out.execute("SELECT source_language, COUNT(*) FROM memslibrary_search GROUP BY source_language ORDER BY source_language").fetchall()
    document_language_rows = out.execute("SELECT source_language, COUNT(*) FROM ravemems_document GROUP BY source_language ORDER BY source_language").fetchall()
    if integrity != "ok" or fk or doc_count != 47 or review_count != 0 or missing_language_count != 0:
        raise RuntimeError(
            f"pack validation failed: integrity={integrity} fk={len(fk)} docs={doc_count} "
            f"reviews={review_count} missing_language={missing_language_count}")
    out.execute("VACUUM")
    out.close()

    db_sha = sha256_file(db_path)
    manifest = {
        "pack_format": PACK_FORMAT,
        "pack_schema_version": PACK_SCHEMA_VERSION,
        "pack_id": pack_id,
        "knowledge_file": "knowledge.sqlite",
        "knowledge_sha256": db_sha,
        "document_count": doc_count,
        "page_count": page_count,
        "visual_count": visual_count,
        "search_entry_count": search_count,
        "languages": [
            {"code": language, "search_entry_count": count,
             "document_count": dict(document_language_rows).get(language, 0)}
            for language, count in language_rows
        ],
        "language_isolation_required": True,
        "source_run_id": SOURCE_RUN_ID,
        "source_artifact_id": SOURCE_ARTIFACT_ID,
        "source_engine_sha": ENGINE_SHA,
        "source_databases": sorted(source_manifest, key=lambda x:x["document_key"])
    }
    manifest_path = output_dir / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, ensure_ascii=False, sort_keys=True, separators=(",",":"))+"\n", encoding="utf-8")
    return manifest

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("source_root")
    ap.add_argument("output_dir")
    ap.add_argument("--pack-id", default="MEMSLibrary_Pack_001")
    args=ap.parse_args()
    manifest=build(args.source_root,args.output_dir,args.pack_id)
    print(json.dumps({
        "pack_id": manifest["pack_id"],
        "pack_schema_version": manifest["pack_schema_version"],
        "document_count": manifest["document_count"],
        "page_count": manifest["page_count"],
        "visual_count": manifest["visual_count"],
        "search_entry_count": manifest["search_entry_count"],
        "languages": manifest["languages"],
        "knowledge_sha256": manifest["knowledge_sha256"]
    }, indent=2, ensure_ascii=False))

if __name__=="__main__":
    main()
