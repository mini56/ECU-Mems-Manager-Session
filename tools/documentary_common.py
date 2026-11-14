#!/usr/bin/env python3
"""Single language-native documentary SQLite contract for SESSION.

The same schema is used for FR, EN and every future language. Extra historical
source tables are retained as provenance, never used to choose an importer.
Keep this schema in sync with the C++ MemsDocumentaryRepository and installer.
"""
from __future__ import annotations

LANGUAGES = frozenset(("fr", "en", "es", "pt", "it", "de"))
SCHEMA_VERSION = 1
MAX_SOURCE_PAGES = 50000

OPERATION_SCHEMA = """
CREATE TABLE IF NOT EXISTS documentary_operations(
 document_id TEXT NOT NULL, source_revision TEXT NOT NULL,
 operation_id TEXT NOT NULL, source_entry_id TEXT NOT NULL,
 title TEXT NOT NULL, opening_page INTEGER,
 verification_status TEXT NOT NULL,
 PRIMARY KEY(document_id,source_revision,operation_id),
 UNIQUE(document_id,source_revision,source_entry_id),
 FOREIGN KEY(document_id,source_revision)
 REFERENCES documentary_documents(document_id,source_revision),
 FOREIGN KEY(document_id,source_revision,source_entry_id)
 REFERENCES documentary_entries(document_id,source_revision,entry_id)
);
CREATE TABLE IF NOT EXISTS documentary_operation_steps(
 document_id TEXT NOT NULL, source_revision TEXT NOT NULL,
 operation_id TEXT NOT NULL, source_entry_id TEXT NOT NULL,
 step_number INTEGER NOT NULL, phase TEXT NOT NULL,
 source_page INTEGER, source_text TEXT NOT NULL,
 verification_status TEXT NOT NULL,
 PRIMARY KEY(document_id,source_revision,source_entry_id),
 FOREIGN KEY(document_id,source_revision,operation_id)
 REFERENCES documentary_operations(document_id,source_revision,operation_id),
 FOREIGN KEY(document_id,source_revision,source_entry_id)
 REFERENCES documentary_entries(document_id,source_revision,entry_id)
);
CREATE INDEX IF NOT EXISTS idx_doc_operation_steps_order
 ON documentary_operation_steps(document_id,source_revision,operation_id,phase,step_number);
"""

def schema(language: str) -> str:
    if language not in LANGUAGES:
        raise ValueError("DOCUMENTARY_LANGUAGE_NOT_SUPPORTED")
    # These are exact existing production columns. Do not require a new Setup
    # or silently migrate existing user-installed snapshots.
    return """
PRAGMA foreign_keys=ON;
CREATE TABLE documentary_database(
 singleton INTEGER PRIMARY KEY CHECK(singleton=1),
 schema_version INTEGER NOT NULL CHECK(schema_version=1),
 language TEXT NOT NULL CHECK(language='LANGCODE'),
 collection_revision INTEGER NOT NULL CHECK(collection_revision>0),
 publication_status TEXT NOT NULL CHECK(publication_status IN ('source_inventory','validated'))
);
CREATE TABLE documentary_documents(
 document_id TEXT NOT NULL, source_revision TEXT NOT NULL,
 language TEXT NOT NULL CHECK(language='LANGCODE'),
 publisher TEXT NOT NULL, edition_code TEXT NOT NULL,
 source_filename TEXT NOT NULL, source_sha256 TEXT NOT NULL,
 source_pages INTEGER NOT NULL CHECK(source_pages BETWEEN 1 AND 50000),
 extraction TEXT NOT NULL,
 PRIMARY KEY(document_id,source_revision)
);
CREATE TABLE documentary_pages(
 document_id TEXT NOT NULL, source_revision TEXT NOT NULL,
 physical_page INTEGER NOT NULL CHECK(physical_page BETWEEN 1 AND 50000),
 structured_records INTEGER NOT NULL DEFAULT 0,
 indexed_state TEXT NOT NULL CHECK(indexed_state IN ('source_linked','not_yet_structured')),
 PRIMARY KEY(document_id,source_revision,physical_page),
 FOREIGN KEY(document_id,source_revision)
 REFERENCES documentary_documents(document_id,source_revision)
);
CREATE TABLE documentary_images(
 document_id TEXT NOT NULL, source_revision TEXT NOT NULL,
 physical_page INTEGER NOT NULL, relative_path TEXT NOT NULL,
 asset_sha256 TEXT NOT NULL, asset_bytes INTEGER NOT NULL,
 PRIMARY KEY(document_id,source_revision,physical_page),
 FOREIGN KEY(document_id,source_revision,physical_page)
 REFERENCES documentary_pages(document_id,source_revision,physical_page)
);
CREATE TABLE documentary_entries(
 document_id TEXT NOT NULL, source_revision TEXT NOT NULL,
 entry_id TEXT NOT NULL,
 entry_type TEXT NOT NULL CHECK(entry_type IN
 ('reference','procedure','step','technical_value','notice','figure','other')),
 source_page INTEGER CHECK(source_page BETWEEN 1 AND 50000),
 title TEXT NOT NULL, source_text TEXT NOT NULL,
 verification_status TEXT NOT NULL CHECK(verification_status IN
 ('source_checked','review_required','technical_reviewed')),
 source_table TEXT NOT NULL, raw_json TEXT NOT NULL,
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
CREATE TABLE documentary_import_history(
 import_sequence INTEGER PRIMARY KEY AUTOINCREMENT,
 document_id TEXT NOT NULL, source_revision TEXT NOT NULL,
 source_sha256 TEXT NOT NULL, source_pages INTEGER NOT NULL,
 import_kind TEXT NOT NULL CHECK(import_kind IN (
  'source_build','added_pdf','existing_snapshot')),
 imported_utc TEXT, previous_snapshot_sha256 TEXT,
 entries_at_import INTEGER NOT NULL, procedures_at_import INTEGER NOT NULL,
 steps_at_import INTEGER NOT NULL, images_at_import INTEGER NOT NULL,
 UNIQUE(document_id,source_revision),
 FOREIGN KEY(document_id,source_revision)
 REFERENCES documentary_documents(document_id,source_revision)
);
CREATE TRIGGER documentary_import_history_no_update
 BEFORE UPDATE ON documentary_import_history BEGIN
 SELECT RAISE(ABORT,'DOCUMENTARY_IMPORT_LOG_IMMUTABLE'); END;
CREATE TRIGGER documentary_import_history_no_delete
 BEFORE DELETE ON documentary_import_history BEGIN
 SELECT RAISE(ABORT,'DOCUMENTARY_IMPORT_LOG_IMMUTABLE'); END;
CREATE INDEX idx_documentary_page ON documentary_entries(document_id,source_page);
CREATE INDEX idx_documentary_type ON documentary_entries(entry_type);
CREATE VIRTUAL TABLE documentary_fts USING fts5(title,source_text,
 content='documentary_entries', content_rowid='rowid',
 tokenize='unicode61 remove_diacritics 2');
CREATE TRIGGER documentary_fts_insert AFTER INSERT ON documentary_entries BEGIN
 INSERT INTO documentary_fts(rowid,title,source_text)
 VALUES(new.rowid,new.title,new.source_text);
END;
""".replace("LANGCODE", language) + OPERATION_SCHEMA


def validate_source_snapshot(db, language: str) -> dict:
    """Structural validation for any number of independent documents."""
    if language not in LANGUAGES:
        raise ValueError("DOCUMENTARY_LANGUAGE_NOT_SUPPORTED")
    meta = db.execute(
        "SELECT schema_version,language,publication_status "
        "FROM documentary_database WHERE singleton=1").fetchone()
    if not meta or tuple(meta)[:2] != (SCHEMA_VERSION,language):
        raise ValueError("DOCUMENTARY_SCHEMA_LANGUAGE_MISMATCH")
    if meta[2] not in ("source_inventory","validated"):
        raise ValueError("DOCUMENTARY_INVALID_PUBLICATION_STATUS")
    docs = db.execute(
        "SELECT document_id,source_revision,source_pages FROM documentary_documents"
    ).fetchall()
    if not docs:
        raise ValueError("DOCUMENTARY_EMPTY_SNAPSHOT")
    for document,revision,pages in docs:
        if not (document and revision and 1 <= pages <= MAX_SOURCE_PAGES):
            raise ValueError("DOCUMENTARY_BAD_SOURCE_ID_OR_PAGE_COUNT")
        actual = db.execute(
            "SELECT COUNT(*),MIN(physical_page),MAX(physical_page) "
            "FROM documentary_pages WHERE document_id=? AND source_revision=?",
            (document,revision)).fetchone()
        if actual != (pages,1,pages):
            raise ValueError("DOCUMENTARY_SOURCE_PAGES_INCOMPLETE")
    counts = db.execute(
        "SELECT (SELECT COUNT(*) FROM documentary_entries),"
        "(SELECT COUNT(*) FROM documentary_fts),"
        "(SELECT COUNT(*) FROM documentary_pages)").fetchone()
    if counts[0] == 0 or counts[0] != counts[1] or counts[2] != sum(d[2] for d in docs):
        raise ValueError("DOCUMENTARY_INDEX_COUNTS_INVALID")
    unlinked = db.execute(
        "SELECT COUNT(*) FROM documentary_entries e "
        "WHERE e.source_page IS NOT NULL AND NOT EXISTS "
        "(SELECT 1 FROM documentary_pages p WHERE p.document_id=e.document_id "
        "AND p.source_revision=e.source_revision "
        "AND p.physical_page=e.source_page)").fetchone()[0]
    if unlinked:
        raise ValueError("DOCUMENTARY_ENTRY_WITHOUT_ORIGINAL_PAGE")
    if meta[2] == "validated":
        unchecked = db.execute(
            "SELECT COUNT(*) FROM documentary_entries WHERE verification_status='review_required'"
        ).fetchone()[0]
        if unchecked:
            raise ValueError("DOCUMENTARY_UNREVIEWED_TEXT_MARKED_VALIDATED")
    # Previously distributed snapshots can legitimately lack an audit table.
    # All NEW snapshots are required to be stamped by the importer/build.
    history_exists=db.execute(
        "SELECT 1 FROM sqlite_master WHERE type='table' "
        "AND name='documentary_import_history'").fetchone() is not None
    if history_exists:
        tracked=db.execute(
            "SELECT COUNT(*) FROM documentary_import_history").fetchone()[0]
        if tracked!=len(docs):
            raise ValueError("DOCUMENTARY_IMPORT_HISTORY_INCOMPLETE")
        altered=db.execute("""
            SELECT COUNT(*) FROM documentary_import_history h
            JOIN documentary_documents d ON
              d.document_id=h.document_id AND
              d.source_revision=h.source_revision
            WHERE h.source_sha256<>d.source_sha256 OR
                  h.source_pages<>d.source_pages OR
                  h.entries_at_import > (SELECT COUNT(*) FROM documentary_entries e
                      WHERE e.document_id=d.document_id AND e.source_revision=d.source_revision)
            """).fetchone()[0]
        if altered:
            raise ValueError("DOCUMENTARY_IMPORT_HISTORY_SOURCE_MISMATCH")
    operation_tables={
        row[0] for row in db.execute(
            "SELECT name FROM sqlite_master WHERE type='table' AND name IN "
            "('documentary_operations','documentary_operation_steps')")}
    if bool(operation_tables) and len(operation_tables)!=2:
        raise ValueError("DOCUMENTARY_OPERATION_SCHEMA_INCOMPLETE")
    if len(operation_tables)==2:
        dangling=db.execute("""
            SELECT COUNT(*) FROM documentary_operation_steps s
            WHERE NOT EXISTS(
               SELECT 1 FROM documentary_operations p
               WHERE p.document_id=s.document_id
                 AND p.source_revision=s.source_revision
                 AND p.operation_id=s.operation_id)""").fetchone()[0]
        if dangling:
            raise ValueError("DOCUMENTARY_UNLINKED_OPERATION_STEP")
    if db.execute("PRAGMA integrity_check").fetchone()[0] != "ok":
        raise ValueError("DOCUMENTARY_SQLITE_INTEGRITY_FAILED")
    if db.execute("PRAGMA foreign_key_check").fetchall():
        raise ValueError("DOCUMENTARY_FOREIGN_KEY_FAILURE")
    return {"language":language,"document_count":len(docs),
            "source_pages":sum(d[2] for d in docs),
            "indexed_entries":counts[0],
            "facsimiles":db.execute("SELECT COUNT(*) FROM documentary_images").fetchone()[0]}


def ensure_import_history(db, *, legacy_snapshot_sha256: str | None = None,
                          existing_kind: str = "existing_snapshot",
                          imported_utc: str | None = None) -> int:
    """Give old snapshots a visibly UNDATED inventory without claiming an import time.

    This is an additive schema extension: never alter source rows, page ids,
    procedural texts, images, or the previously published language.
    """
    if existing_kind not in ("source_build", "existing_snapshot"):
        raise ValueError("INVALID_DOCUMENTARY_BASELINE_EVENT")
    db.executescript("""
    CREATE TABLE IF NOT EXISTS documentary_import_history(
     import_sequence INTEGER PRIMARY KEY AUTOINCREMENT,
     document_id TEXT NOT NULL, source_revision TEXT NOT NULL,
     source_sha256 TEXT NOT NULL, source_pages INTEGER NOT NULL,
     import_kind TEXT NOT NULL CHECK(import_kind IN (
      'source_build','added_pdf','existing_snapshot')),
     imported_utc TEXT, previous_snapshot_sha256 TEXT,
     entries_at_import INTEGER NOT NULL, procedures_at_import INTEGER NOT NULL,
     steps_at_import INTEGER NOT NULL, images_at_import INTEGER NOT NULL,
     UNIQUE(document_id,source_revision),
     FOREIGN KEY(document_id,source_revision)
     REFERENCES documentary_documents(document_id,source_revision)
    );
    CREATE TRIGGER IF NOT EXISTS documentary_import_history_no_update
     BEFORE UPDATE ON documentary_import_history BEGIN
     SELECT RAISE(ABORT,'DOCUMENTARY_IMPORT_LOG_IMMUTABLE'); END;
    CREATE TRIGGER IF NOT EXISTS documentary_import_history_no_delete
     BEFORE DELETE ON documentary_import_history BEGIN
     SELECT RAISE(ABORT,'DOCUMENTARY_IMPORT_LOG_IMMUTABLE'); END;
    """)
    missing=db.execute("""
        SELECT d.document_id,d.source_revision,d.source_sha256,d.source_pages,
        (SELECT COUNT(*) FROM documentary_entries e
          WHERE e.document_id=d.document_id AND e.source_revision=d.source_revision),
        (SELECT COUNT(*) FROM documentary_entries e
          WHERE e.document_id=d.document_id AND e.source_revision=d.source_revision
          AND e.entry_type='procedure'),
        (SELECT COUNT(*) FROM documentary_entries e
          WHERE e.document_id=d.document_id AND e.source_revision=d.source_revision
          AND e.entry_type='step'),
        (SELECT COUNT(*) FROM documentary_images i
          WHERE i.document_id=d.document_id AND i.source_revision=d.source_revision)
        FROM documentary_documents d
        LEFT JOIN documentary_import_history h
          ON h.document_id=d.document_id AND h.source_revision=d.source_revision
        WHERE h.import_sequence IS NULL
        ORDER BY d.document_id,d.source_revision
        """).fetchall()
    for doc,revision,digest,pages,entries,procedures,steps,images in missing:
        db.execute("""
            INSERT INTO documentary_import_history(
              document_id,source_revision,source_sha256,source_pages,
              import_kind,imported_utc,previous_snapshot_sha256,
              entries_at_import,procedures_at_import,steps_at_import,
              images_at_import) VALUES(?,?,?,?,?,?,?,?,?,?,?)""",
            (doc,revision,digest,pages,existing_kind,imported_utc,
             legacy_snapshot_sha256,entries,procedures,steps,images))
    return len(missing)


def register_added_pdf(db, *, document_id: str, revision: str,
                       previous_snapshot_sha256: str | None,
                       imported_utc: str) -> None:
    """Add exactly one verifiable immutable event for a newly imported PDF."""
    row=db.execute("""
        SELECT source_sha256,source_pages FROM documentary_documents
        WHERE document_id=? AND source_revision=?""",(document_id,revision)).fetchone()
    if row is None:
        raise ValueError("DOCUMENTARY_SOURCE_NOT_REGISTERED")
    counts=db.execute("""
        SELECT (SELECT COUNT(*) FROM documentary_entries
                WHERE document_id=? AND source_revision=?),
               (SELECT COUNT(*) FROM documentary_entries
                WHERE document_id=? AND source_revision=? AND entry_type='procedure'),
               (SELECT COUNT(*) FROM documentary_entries
                WHERE document_id=? AND source_revision=? AND entry_type='step'),
               (SELECT COUNT(*) FROM documentary_images
                WHERE document_id=? AND source_revision=?)""",
        (document_id,revision)*4).fetchone()
    db.execute("""
        INSERT INTO documentary_import_history(
          document_id,source_revision,source_sha256,source_pages,
          import_kind,imported_utc,previous_snapshot_sha256,
          entries_at_import,procedures_at_import,steps_at_import,
          images_at_import) VALUES(?,?,?,?,'added_pdf',?,?,?,?,?,?)""",
        (document_id,revision,row[0],row[1],imported_utc,
         previous_snapshot_sha256,*counts))


def documentary_import_report(db,language: str) -> dict:
    """The same human-readable audit inventory for every language."""
    document_rows=db.execute("""
        SELECT d.document_id,d.source_revision,d.source_filename,d.source_sha256,
               d.source_pages,h.import_sequence,h.import_kind,h.imported_utc,
               h.previous_snapshot_sha256,h.entries_at_import,
               h.procedures_at_import,h.steps_at_import,h.images_at_import
        FROM documentary_documents d
        JOIN documentary_import_history h
          ON d.document_id=h.document_id AND d.source_revision=h.source_revision
        ORDER BY h.import_sequence""").fetchall()
    return {
        "language":language,
        "document_count":len(document_rows),
        "history":[dict(zip((
          "document_id","source_revision","original_filename",
          "source_sha256","source_pages","sequence","import_kind",
          "imported_utc","previous_snapshot_sha256",
          "indexed_entries_at_import","procedures_at_import",
          "steps_at_import","images_at_import"),record))
          for record in document_rows],
    }


def ensure_structured_operations(db) -> dict:
    """Normalize existing reviewed/unreviewed procedure links, without new OCR.

    Once imported, all languages expose one operation_id -> steps relation.
    Transitional source adapters only recognize historic procedure_key and
    procedure_id. Future annotation files use procedure_id exclusively.
    Missing/unlinked steps remain visible in the report, never invented.
    """
    import json
    db.executescript(OPERATION_SCHEMA)
    inserted_operations=0
    linked_steps=0
    missing_keys=0
    orphan_steps=0
    procedures=db.execute("""
        SELECT document_id,source_revision,entry_id,title,source_page,
               verification_status,raw_json
        FROM documentary_entries WHERE entry_type='procedure'
        ORDER BY document_id,source_revision,entry_id""").fetchall()
    for doc,revision,entry,title,page,status,raw in procedures:
        source=json.loads(raw or "{}")
        key=str(source.get("procedure_id") or source.get("procedure_key") or "").strip()
        if not key:
            missing_keys+=1
            continue
        before=db.total_changes
        db.execute("""
            INSERT OR IGNORE INTO documentary_operations(
              document_id,source_revision,operation_id,source_entry_id,
              title,opening_page,verification_status)
            VALUES(?,?,?,?,?,?,?)""",
            (doc,revision,key,entry,title,page,status))
        inserted_operations+=db.total_changes-before
    steps=db.execute("""
        SELECT document_id,source_revision,entry_id,source_page,
               verification_status,raw_json,source_text
        FROM documentary_entries WHERE entry_type='step'
        ORDER BY document_id,source_revision,entry_id""").fetchall()
    for doc,revision,entry,page,status,raw,text in steps:
        source=json.loads(raw or "{}")
        key=str(source.get("procedure_id") or source.get("procedure_key") or "").strip()
        if not key or db.execute("""
            SELECT 1 FROM documentary_operations
            WHERE document_id=? AND source_revision=? AND operation_id=?""",
            (doc,revision,key)).fetchone() is None:
            orphan_steps+=1
            continue
        order=source.get("step_number",source.get("step_no",0))
        try:
            order=int(order)
        except (TypeError,ValueError):
            order=0
        if order<0:
            order=0
        original_text=next((str(source[k]).strip() for k in
            ("instruction_source","instruction_text","step_text","action_text",
             "instruction","source_text","statement")
            if k in source and source[k] is not None and str(source[k]).strip()),text)
        phase=str(source.get("phase") or "").strip()
        before=db.total_changes
        db.execute("""
            INSERT OR IGNORE INTO documentary_operation_steps(
             document_id,source_revision,operation_id,source_entry_id,
             step_number,phase,source_page,source_text,verification_status)
            VALUES(?,?,?,?,?,?,?,?,?)""",
            (doc,revision,key,entry,order,phase,page,original_text,status))
        linked_steps+=db.total_changes-before
    return {
        "new_operations":inserted_operations,
        "new_linked_steps":linked_steps,
        "procedures_without_keys":missing_keys,
        "unlinked_steps":orphan_steps,
        "total_operations":db.execute("SELECT COUNT(*) FROM documentary_operations").fetchone()[0],
        "total_linked_steps":db.execute("SELECT COUNT(*) FROM documentary_operation_steps").fetchone()[0],
    }
