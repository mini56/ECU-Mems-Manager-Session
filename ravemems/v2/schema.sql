PRAGMA foreign_keys = ON;
PRAGMA user_version = 2;

CREATE TABLE ravemems_document (
    document_key TEXT PRIMARY KEY,
    canonical_name TEXT NOT NULL,
    source_language TEXT NOT NULL,
    document_kind TEXT,
    manufacturer TEXT,
    title_source TEXT,
    created_at_utc TEXT
);

CREATE TABLE ravemems_document_revision (
    revision_key TEXT PRIMARY KEY,
    document_key TEXT NOT NULL REFERENCES ravemems_document(document_key) ON DELETE CASCADE,
    edition_label TEXT,
    revision_label TEXT,
    publication_date TEXT,
    source_relative_path TEXT NOT NULL,
    source_blob_sha TEXT,
    source_sha256 TEXT NOT NULL,
    source_size INTEGER,
    page_count INTEGER NOT NULL CHECK(page_count >= 0),
    is_current INTEGER NOT NULL DEFAULT 0 CHECK(is_current IN (0,1)),
    UNIQUE(document_key, source_sha256)
);

CREATE TABLE ravemems_page (
    page_key TEXT PRIMARY KEY,
    revision_key TEXT NOT NULL REFERENCES ravemems_document_revision(revision_key) ON DELETE CASCADE,
    physical_page INTEGER NOT NULL CHECK(physical_page > 0),
    printed_page_label TEXT,
    source_text_sha256 TEXT,
    extraction_status TEXT NOT NULL DEFAULT 'pending'
        CHECK(extraction_status IN ('pending','complete','uncertain','failed')),
    UNIQUE(revision_key, physical_page)
);

CREATE TABLE ravemems_section (
    section_key TEXT PRIMARY KEY,
    revision_key TEXT NOT NULL REFERENCES ravemems_document_revision(revision_key) ON DELETE CASCADE,
    parent_section_key TEXT REFERENCES ravemems_section(section_key) ON DELETE CASCADE,
    sequence_no INTEGER NOT NULL CHECK(sequence_no > 0),
    section_kind TEXT,
    manufacturer_identifier TEXT,
    title_source TEXT NOT NULL,
    source_language TEXT NOT NULL,
    UNIQUE(revision_key, parent_section_key, sequence_no)
);

CREATE TABLE ravemems_applicability_scope (
    scope_key TEXT PRIMARY KEY,
    scope_kind TEXT NOT NULL DEFAULT 'vehicle',
    make TEXT,
    model TEXT,
    generation TEXT,
    body_style TEXT,
    year_from INTEGER,
    year_to INTEGER,
    market TEXT,
    engine_family TEXT,
    engine_code TEXT,
    engine_variant TEXT,
    displacement_cc INTEGER,
    fuel_delivery_kind TEXT,
    induction_kind TEXT,
    engine_management_kind TEXT,
    engine_controller_state TEXT NOT NULL DEFAULT 'unknown'
        CHECK(engine_controller_state IN ('unknown','absent','present')),
    ecu_family TEXT,
    ecu_reference TEXT,
    ecu_firmware TEXT,
    transmission_kind TEXT,
    transmission_code TEXT,
    catalyst_state TEXT,
    air_conditioning_state TEXT,
    source_scope_text TEXT,
    CHECK(year_from IS NULL OR year_to IS NULL OR year_from <= year_to),
    CHECK(displacement_cc IS NULL OR displacement_cc > 0),
    CHECK(
        engine_controller_state <> 'absent'
        OR (
            COALESCE(TRIM(ecu_family),'') = ''
            AND COALESCE(TRIM(ecu_reference),'') = ''
            AND COALESCE(TRIM(ecu_firmware),'') = ''
        )
    )
);

CREATE TABLE ravemems_scope_attribute (
    scope_key TEXT NOT NULL REFERENCES ravemems_applicability_scope(scope_key) ON DELETE CASCADE,
    attribute_key TEXT NOT NULL,
    attribute_value TEXT NOT NULL,
    source_text TEXT,
    PRIMARY KEY(scope_key, attribute_key)
);

CREATE TABLE ravemems_revision_scope (
    revision_key TEXT NOT NULL REFERENCES ravemems_document_revision(revision_key) ON DELETE CASCADE,
    scope_key TEXT NOT NULL REFERENCES ravemems_applicability_scope(scope_key) ON DELETE CASCADE,
    applicability TEXT NOT NULL DEFAULT 'applies'
        CHECK(applicability IN ('applies','excludes','uncertain')),
    PRIMARY KEY(revision_key, scope_key)
);

CREATE TABLE ravemems_operation (
    operation_key TEXT PRIMARY KEY,
    revision_key TEXT NOT NULL REFERENCES ravemems_document_revision(revision_key) ON DELETE CASCADE,
    section_key TEXT REFERENCES ravemems_section(section_key) ON DELETE SET NULL,
    sequence_no INTEGER NOT NULL CHECK(sequence_no > 0),
    manufacturer_operation_no TEXT,
    operation_kind TEXT,
    title_source TEXT NOT NULL,
    source_language TEXT NOT NULL,
    completeness_status TEXT NOT NULL DEFAULT 'complete'
        CHECK(completeness_status IN ('complete','incomplete','uncertain')),
    UNIQUE(revision_key, sequence_no)
);

CREATE TABLE ravemems_operation_scope (
    operation_key TEXT NOT NULL REFERENCES ravemems_operation(operation_key) ON DELETE CASCADE,
    scope_key TEXT NOT NULL REFERENCES ravemems_applicability_scope(scope_key) ON DELETE CASCADE,
    applicability TEXT NOT NULL DEFAULT 'applies'
        CHECK(applicability IN ('applies','excludes','uncertain')),
    PRIMARY KEY(operation_key, scope_key)
);

CREATE TABLE ravemems_phase (
    phase_key TEXT PRIMARY KEY,
    operation_key TEXT NOT NULL REFERENCES ravemems_operation(operation_key) ON DELETE CASCADE,
    sequence_no INTEGER NOT NULL CHECK(sequence_no > 0),
    phase_kind_source TEXT NOT NULL,
    normalized_phase_kind TEXT,
    title_source TEXT,
    completeness_status TEXT NOT NULL DEFAULT 'complete'
        CHECK(completeness_status IN ('complete','incomplete','uncertain')),
    UNIQUE(operation_key, sequence_no)
);

CREATE TABLE ravemems_step (
    step_key TEXT PRIMARY KEY,
    phase_key TEXT NOT NULL REFERENCES ravemems_phase(phase_key) ON DELETE CASCADE,
    sequence_no INTEGER NOT NULL CHECK(sequence_no > 0),
    manufacturer_step_no TEXT,
    instruction_source TEXT NOT NULL,
    condition_text TEXT,
    completeness_status TEXT NOT NULL DEFAULT 'complete'
        CHECK(completeness_status IN ('complete','incomplete','uncertain')),
    source_page_start INTEGER,
    source_page_end INTEGER,
    UNIQUE(phase_key, sequence_no),
    CHECK(source_page_start IS NULL OR source_page_start > 0),
    CHECK(source_page_end IS NULL OR source_page_end > 0),
    CHECK(source_page_start IS NULL OR source_page_end IS NULL OR source_page_start <= source_page_end)
);

CREATE TABLE ravemems_notice (
    notice_key TEXT PRIMARY KEY,
    operation_key TEXT NOT NULL REFERENCES ravemems_operation(operation_key) ON DELETE CASCADE,
    sequence_no INTEGER NOT NULL CHECK(sequence_no > 0),
    notice_kind TEXT NOT NULL
        CHECK(notice_kind IN ('warning','caution','note','remark','other')),
    source_text TEXT NOT NULL,
    scope_kind TEXT NOT NULL
        CHECK(scope_kind IN ('operation','phase','step')),
    target_operation_key TEXT REFERENCES ravemems_operation(operation_key) ON DELETE CASCADE,
    target_phase_key TEXT REFERENCES ravemems_phase(phase_key) ON DELETE CASCADE,
    target_step_key TEXT REFERENCES ravemems_step(step_key) ON DELETE CASCADE,
    UNIQUE(operation_key, sequence_no)
);

CREATE TABLE ravemems_requirement (
    requirement_key TEXT PRIMARY KEY,
    operation_key TEXT NOT NULL REFERENCES ravemems_operation(operation_key) ON DELETE CASCADE,
    phase_key TEXT REFERENCES ravemems_phase(phase_key) ON DELETE CASCADE,
    step_key TEXT REFERENCES ravemems_step(step_key) ON DELETE CASCADE,
    sequence_no INTEGER NOT NULL CHECK(sequence_no > 0),
    requirement_type TEXT NOT NULL,
    requirement_source TEXT NOT NULL,
    part_number TEXT,
    quantity REAL,
    unit TEXT,
    before_start INTEGER NOT NULL DEFAULT 0 CHECK(before_start IN (0,1)),
    figure_ref TEXT
);

CREATE TABLE ravemems_specification (
    specification_key TEXT PRIMARY KEY,
    operation_key TEXT REFERENCES ravemems_operation(operation_key) ON DELETE CASCADE,
    phase_key TEXT REFERENCES ravemems_phase(phase_key) ON DELETE CASCADE,
    step_key TEXT REFERENCES ravemems_step(step_key) ON DELETE CASCADE,
    parameter_source TEXT NOT NULL,
    default_unit TEXT,
    condition_text TEXT,
    source_language TEXT NOT NULL
);

CREATE TABLE ravemems_specification_value (
    value_key TEXT PRIMARY KEY,
    specification_key TEXT NOT NULL REFERENCES ravemems_specification(specification_key) ON DELETE CASCADE,
    sequence_no INTEGER NOT NULL CHECK(sequence_no > 0),
    value_numeric REAL,
    value_min REAL,
    value_max REAL,
    angle_deg REAL,
    value_text TEXT,
    unit TEXT,
    condition_text TEXT,
    instruction_text TEXT,
    UNIQUE(specification_key, sequence_no)
);

CREATE TABLE ravemems_table (
    table_key TEXT PRIMARY KEY,
    operation_key TEXT REFERENCES ravemems_operation(operation_key) ON DELETE CASCADE,
    title_source TEXT,
    source_language TEXT NOT NULL,
    row_count INTEGER NOT NULL CHECK(row_count >= 0),
    column_count INTEGER NOT NULL CHECK(column_count >= 0),
    structure_status TEXT NOT NULL DEFAULT 'verified'
        CHECK(structure_status IN ('verified','uncertain','failed'))
);

CREATE TABLE ravemems_table_cell (
    table_key TEXT NOT NULL REFERENCES ravemems_table(table_key) ON DELETE CASCADE,
    row_no INTEGER NOT NULL CHECK(row_no >= 0),
    column_no INTEGER NOT NULL CHECK(column_no >= 0),
    source_text TEXT,
    semantic_role TEXT,
    PRIMARY KEY(table_key, row_no, column_no)
);

CREATE TABLE ravemems_visual (
    visual_key TEXT PRIMARY KEY,
    revision_key TEXT NOT NULL REFERENCES ravemems_document_revision(revision_key) ON DELETE CASCADE,
    page_key TEXT REFERENCES ravemems_page(page_key) ON DELETE SET NULL,
    visual_type TEXT NOT NULL,
    relative_path TEXT NOT NULL,
    sha256 TEXT NOT NULL,
    width INTEGER NOT NULL CHECK(width > 0),
    height INTEGER NOT NULL CHECK(height > 0),
    render_method TEXT NOT NULL,
    source_bbox_json TEXT,
    crop_bbox_json TEXT,
    caption_source TEXT,
    source_language TEXT,
    fidelity_status TEXT NOT NULL DEFAULT 'pending'
        CHECK(fidelity_status IN ('pending','verified','failed')),
    UNIQUE(revision_key, relative_path)
);

CREATE TABLE ravemems_visual_link (
    visual_link_key TEXT PRIMARY KEY,
    visual_key TEXT NOT NULL REFERENCES ravemems_visual(visual_key) ON DELETE CASCADE,
    operation_key TEXT REFERENCES ravemems_operation(operation_key) ON DELETE CASCADE,
    phase_key TEXT REFERENCES ravemems_phase(phase_key) ON DELETE CASCADE,
    step_key TEXT REFERENCES ravemems_step(step_key) ON DELETE CASCADE,
    specification_key TEXT REFERENCES ravemems_specification(specification_key) ON DELETE CASCADE,
    table_key TEXT REFERENCES ravemems_table(table_key) ON DELETE CASCADE,
    relation_type TEXT NOT NULL,
    sequence_no INTEGER NOT NULL DEFAULT 1 CHECK(sequence_no > 0),
    verification_status TEXT NOT NULL DEFAULT 'pending'
        CHECK(verification_status IN ('pending','verified','uncertain','failed'))
);

CREATE TABLE ravemems_operation_relation (
    relation_key TEXT PRIMARY KEY,
    source_operation_key TEXT NOT NULL REFERENCES ravemems_operation(operation_key) ON DELETE CASCADE,
    target_operation_key TEXT NOT NULL REFERENCES ravemems_operation(operation_key) ON DELETE CASCADE,
    relation_type TEXT NOT NULL,
    source_text TEXT,
    sequence_no INTEGER NOT NULL DEFAULT 1 CHECK(sequence_no > 0),
    verification_status TEXT NOT NULL DEFAULT 'verified'
        CHECK(verification_status IN ('verified','uncertain','failed'))
);

CREATE TABLE ravemems_translation (
    translation_key TEXT PRIMARY KEY,
    entity_kind TEXT NOT NULL,
    entity_key TEXT NOT NULL,
    field_name TEXT NOT NULL,
    source_language TEXT NOT NULL,
    target_language TEXT NOT NULL,
    translated_text TEXT NOT NULL,
    translation_status TEXT NOT NULL DEFAULT 'provisional'
        CHECK(translation_status IN ('provisional','validated','rejected')),
    UNIQUE(entity_kind, entity_key, field_name, target_language)
);

CREATE TABLE ravemems_review_flag (
    review_key TEXT PRIMARY KEY,
    entity_kind TEXT NOT NULL,
    entity_key TEXT NOT NULL,
    reason_code TEXT NOT NULL,
    reason_text TEXT NOT NULL,
    severity TEXT NOT NULL DEFAULT 'review'
        CHECK(severity IN ('review','blocking')),
    status TEXT NOT NULL DEFAULT 'open'
        CHECK(status IN ('open','resolved','accepted')),
    created_by TEXT,
    resolution_text TEXT
);

CREATE TABLE ravemems_provenance (
    provenance_key TEXT PRIMARY KEY,
    entity_kind TEXT NOT NULL,
    entity_key TEXT NOT NULL,
    revision_key TEXT NOT NULL REFERENCES ravemems_document_revision(revision_key) ON DELETE CASCADE,
    page_key TEXT REFERENCES ravemems_page(page_key) ON DELETE SET NULL,
    source_element_key TEXT,
    source_bbox_json TEXT,
    source_text_sha256 TEXT
);

CREATE INDEX idx_ravemems_page_revision ON ravemems_page(revision_key, physical_page);
CREATE INDEX idx_ravemems_section_revision ON ravemems_section(revision_key, sequence_no);
CREATE INDEX idx_ravemems_operation_revision ON ravemems_operation(revision_key, sequence_no);
CREATE INDEX idx_ravemems_phase_operation ON ravemems_phase(operation_key, sequence_no);
CREATE INDEX idx_ravemems_step_phase ON ravemems_step(phase_key, sequence_no);
CREATE INDEX idx_ravemems_notice_operation ON ravemems_notice(operation_key, sequence_no);
CREATE INDEX idx_ravemems_requirement_operation ON ravemems_requirement(operation_key, sequence_no);
CREATE INDEX idx_ravemems_spec_operation ON ravemems_specification(operation_key);
CREATE INDEX idx_ravemems_spec_value ON ravemems_specification_value(specification_key, sequence_no);
CREATE INDEX idx_ravemems_visual_revision ON ravemems_visual(revision_key, page_key, visual_type);
CREATE INDEX idx_ravemems_visual_link_visual ON ravemems_visual_link(visual_key, relation_type);
CREATE INDEX idx_ravemems_relation_source ON ravemems_operation_relation(source_operation_key, relation_type);
CREATE INDEX idx_ravemems_translation_entity ON ravemems_translation(entity_kind, entity_key, target_language);
CREATE INDEX idx_ravemems_review_open ON ravemems_review_flag(status, severity, entity_kind, entity_key);
CREATE INDEX idx_ravemems_provenance_entity ON ravemems_provenance(entity_kind, entity_key);
