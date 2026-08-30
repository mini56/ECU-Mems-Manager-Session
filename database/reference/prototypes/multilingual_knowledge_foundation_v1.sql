PRAGMA foreign_keys=ON;
BEGIN IMMEDIATE;

CREATE TABLE IF NOT EXISTS mems_doc_locale (
    locale TEXT PRIMARY KEY,
    display_name TEXT NOT NULL,
    fallback_locale TEXT,
    text_direction TEXT NOT NULL DEFAULT 'ltr',
    enabled INTEGER NOT NULL DEFAULT 1,
    notes TEXT,
    CHECK (length(trim(locale)) > 0),
    CHECK (text_direction IN ('ltr','rtl')),
    CHECK (enabled IN (0,1)),
    FOREIGN KEY (fallback_locale) REFERENCES mems_doc_locale(locale)
);

INSERT OR IGNORE INTO mems_doc_locale(locale,display_name,fallback_locale,text_direction,enabled,notes) VALUES
('fr','Français',NULL,'ltr',1,'Langue actuelle de MEMS Manager'),
('en','English','fr','ltr',1,'Langue actuelle de MEMS Manager'),
('es','Español','fr','ltr',1,'Langue actuelle de MEMS Manager'),
('it','Italiano','fr','ltr',1,'Langue actuelle de MEMS Manager'),
('pt','Português','fr','ltr',1,'Langue actuelle de MEMS Manager'),
('de','Deutsch','fr','ltr',1,'Langue actuelle de MEMS Manager');

CREATE TABLE IF NOT EXISTS mems_doc_document (
    document_key TEXT PRIMARY KEY,
    source_key TEXT,
    title_source TEXT NOT NULL,
    publication_code TEXT,
    edition TEXT,
    source_locale TEXT NOT NULL,
    source_kind TEXT NOT NULL,
    file_name TEXT,
    file_sha256 TEXT,
    page_count INTEGER,
    verification_level TEXT NOT NULL,
    notes TEXT,
    CHECK (page_count IS NULL OR page_count >= 0),
    FOREIGN KEY (source_locale) REFERENCES mems_doc_locale(locale)
);

CREATE TABLE IF NOT EXISTS mems_doc_unit (
    unit_key TEXT PRIMARY KEY,
    document_key TEXT NOT NULL,
    unit_kind TEXT NOT NULL,
    sequence_no INTEGER,
    physical_page INTEGER,
    printed_ref TEXT,
    section_ref TEXT,
    source_hash TEXT,
    audit_status TEXT NOT NULL DEFAULT 'pending',
    integration_status TEXT NOT NULL DEFAULT 'pending',
    audit_reason TEXT,
    notes TEXT,
    CHECK (sequence_no IS NULL OR sequence_no >= 0),
    CHECK (physical_page IS NULL OR physical_page >= 1),
    CHECK (audit_status IN ('pending','useful','already_covered','out_of_scope','duplicate','needs_review')),
    CHECK (integration_status IN ('pending','integrated','backlog','not_required')),
    FOREIGN KEY (document_key) REFERENCES mems_doc_document(document_key) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS mems_doc_entity (
    entity_key TEXT PRIMARY KEY,
    document_key TEXT NOT NULL,
    unit_key TEXT,
    entity_kind TEXT NOT NULL,
    parent_entity_key TEXT,
    domain TEXT,
    component_key TEXT,
    verification_level TEXT NOT NULL,
    source_locale TEXT NOT NULL,
    source_locator TEXT,
    legacy_table TEXT,
    legacy_key TEXT,
    sort_order INTEGER NOT NULL DEFAULT 0,
    notes TEXT,
    FOREIGN KEY (document_key) REFERENCES mems_doc_document(document_key) ON DELETE CASCADE,
    FOREIGN KEY (unit_key) REFERENCES mems_doc_unit(unit_key) ON DELETE SET NULL,
    FOREIGN KEY (parent_entity_key) REFERENCES mems_doc_entity(entity_key) ON DELETE CASCADE,
    FOREIGN KEY (source_locale) REFERENCES mems_doc_locale(locale),
    UNIQUE (legacy_table, legacy_key, entity_kind)
);

CREATE TABLE IF NOT EXISTS mems_doc_text (
    entity_key TEXT NOT NULL,
    field_key TEXT NOT NULL,
    locale TEXT NOT NULL,
    text_value TEXT NOT NULL,
    translation_status TEXT NOT NULL DEFAULT 'draft',
    translation_method TEXT,
    source_locale TEXT,
    notes TEXT,
    PRIMARY KEY (entity_key, field_key, locale),
    CHECK (length(trim(field_key)) > 0),
    CHECK (translation_status IN ('source','draft','machine','reviewed','approved')),
    FOREIGN KEY (entity_key) REFERENCES mems_doc_entity(entity_key) ON DELETE CASCADE,
    FOREIGN KEY (locale) REFERENCES mems_doc_locale(locale),
    FOREIGN KEY (source_locale) REFERENCES mems_doc_locale(locale)
);

CREATE TABLE IF NOT EXISTS mems_doc_visual (
    entity_key TEXT PRIMARY KEY,
    relative_path TEXT NOT NULL,
    sha256 TEXT NOT NULL,
    visual_kind TEXT NOT NULL,
    source_width INTEGER,
    source_height INTEGER,
    original_intact INTEGER NOT NULL DEFAULT 1,
    legacy_illustration_key TEXT,
    notes TEXT,
    CHECK (source_width IS NULL OR source_width > 0),
    CHECK (source_height IS NULL OR source_height > 0),
    CHECK (original_intact IN (0,1)),
    FOREIGN KEY (entity_key) REFERENCES mems_doc_entity(entity_key) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS mems_doc_visual_region (
    region_entity_key TEXT PRIMARY KEY,
    visual_entity_key TEXT NOT NULL,
    region_kind TEXT NOT NULL,
    x_norm REAL,
    y_norm REAL,
    width_norm REAL,
    height_norm REAL,
    anchor_ref TEXT,
    z_order INTEGER NOT NULL DEFAULT 0,
    notes TEXT,
    CHECK (x_norm IS NULL OR (x_norm >= 0.0 AND x_norm <= 1.0)),
    CHECK (y_norm IS NULL OR (y_norm >= 0.0 AND y_norm <= 1.0)),
    CHECK (width_norm IS NULL OR (width_norm >= 0.0 AND width_norm <= 1.0)),
    CHECK (height_norm IS NULL OR (height_norm >= 0.0 AND height_norm <= 1.0)),
    FOREIGN KEY (region_entity_key) REFERENCES mems_doc_entity(entity_key) ON DELETE CASCADE,
    FOREIGN KEY (visual_entity_key) REFERENCES mems_doc_visual(entity_key) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS mems_doc_table (
    entity_key TEXT PRIMARY KEY,
    row_count INTEGER NOT NULL,
    column_count INTEGER NOT NULL,
    header_rows INTEGER NOT NULL DEFAULT 0,
    header_columns INTEGER NOT NULL DEFAULT 0,
    notes TEXT,
    CHECK (row_count >= 0),
    CHECK (column_count >= 0),
    CHECK (header_rows >= 0),
    CHECK (header_columns >= 0),
    FOREIGN KEY (entity_key) REFERENCES mems_doc_entity(entity_key) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS mems_doc_table_cell (
    cell_entity_key TEXT PRIMARY KEY,
    table_entity_key TEXT NOT NULL,
    row_no INTEGER NOT NULL,
    column_no INTEGER NOT NULL,
    row_span INTEGER NOT NULL DEFAULT 1,
    column_span INTEGER NOT NULL DEFAULT 1,
    value_numeric REAL,
    value_min REAL,
    value_max REAL,
    unit_code TEXT,
    notes TEXT,
    CHECK (row_no >= 0),
    CHECK (column_no >= 0),
    CHECK (row_span >= 1),
    CHECK (column_span >= 1),
    FOREIGN KEY (cell_entity_key) REFERENCES mems_doc_entity(entity_key) ON DELETE CASCADE,
    FOREIGN KEY (table_entity_key) REFERENCES mems_doc_table(entity_key) ON DELETE CASCADE,
    UNIQUE (table_entity_key, row_no, column_no)
);

CREATE TABLE IF NOT EXISTS mems_doc_value (
    entity_key TEXT PRIMARY KEY,
    value_kind TEXT NOT NULL,
    sequence_no INTEGER NOT NULL DEFAULT 1,
    value_numeric REAL,
    value_min REAL,
    value_max REAL,
    tolerance_minus REAL,
    tolerance_plus REAL,
    angle_deg REAL,
    unit_code TEXT,
    notes TEXT,
    CHECK (sequence_no >= 1),
    FOREIGN KEY (entity_key) REFERENCES mems_doc_entity(entity_key) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS mems_doc_relation (
    from_entity_key TEXT NOT NULL,
    relation_type TEXT NOT NULL,
    to_entity_key TEXT NOT NULL,
    notes TEXT,
    PRIMARY KEY (from_entity_key, relation_type, to_entity_key),
    FOREIGN KEY (from_entity_key) REFERENCES mems_doc_entity(entity_key) ON DELETE CASCADE,
    FOREIGN KEY (to_entity_key) REFERENCES mems_doc_entity(entity_key) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_mems_doc_unit_document ON mems_doc_unit(document_key, sequence_no);
CREATE INDEX IF NOT EXISTS idx_mems_doc_unit_audit ON mems_doc_unit(document_key, audit_status, integration_status);
CREATE INDEX IF NOT EXISTS idx_mems_doc_entity_document ON mems_doc_entity(document_key, entity_kind);
CREATE INDEX IF NOT EXISTS idx_mems_doc_entity_unit ON mems_doc_entity(unit_key, sort_order);
CREATE INDEX IF NOT EXISTS idx_mems_doc_entity_legacy ON mems_doc_entity(legacy_table, legacy_key);
CREATE INDEX IF NOT EXISTS idx_mems_doc_text_locale ON mems_doc_text(locale, field_key);
CREATE INDEX IF NOT EXISTS idx_mems_doc_text_entity_locale ON mems_doc_text(entity_key, locale);
CREATE INDEX IF NOT EXISTS idx_mems_doc_visual_path ON mems_doc_visual(relative_path);
CREATE INDEX IF NOT EXISTS idx_mems_doc_visual_region_visual ON mems_doc_visual_region(visual_entity_key, z_order);
CREATE INDEX IF NOT EXISTS idx_mems_doc_table_cell_table ON mems_doc_table_cell(table_entity_key, row_no, column_no);
CREATE INDEX IF NOT EXISTS idx_mems_doc_relation_to ON mems_doc_relation(to_entity_key, relation_type);

PRAGMA user_version=21;
COMMIT;
