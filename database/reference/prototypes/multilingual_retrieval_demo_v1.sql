PRAGMA foreign_keys=ON;
BEGIN IMMEDIATE;

-- Future locales are data, not schema. Disabled here because they are not production UI languages yet.
INSERT OR IGNORE INTO mems_doc_locale(locale,display_name,fallback_locale,text_direction,enabled,notes) VALUES
('ja','日本語','en','ltr',0,'Future locale prototype; fallback to English'),
('zh-CN','简体中文','en','ltr',0,'Future locale prototype; fallback to English'),
('hi','हिन्दी','en','ltr',0,'Future locale prototype; fallback to English');

INSERT OR IGNORE INTO mems_doc_document
(document_key,source_key,title_source,publication_code,edition,source_locale,source_kind,file_name,file_sha256,page_count,verification_level,notes)
VALUES
('DEMO-RCL0194ENG','SRC-RCL0194','Rover Mini Electrical Circuit Diagrams RCL0194ENG','RCL0194ENG',NULL,'en','wiring_manual',NULL,NULL,NULL,'verifie_constructeur','Prototype only; reads legacy BUILD #103 data without modifying it.'),
('DEMO-RCL0193FRE','SRC-RCL0193FRE-USER-PDF','Rover Mini Manuel de réparation RCL0193FRE','RCL0193FRE',NULL,'fr','repair_manual',NULL,NULL,NULL,'verifie_constructeur','Prototype only; reads legacy BUILD #103 data without modifying it.');

INSERT OR IGNORE INTO mems_doc_unit
(unit_key,document_key,unit_kind,sequence_no,physical_page,printed_ref,section_ref,source_hash,audit_status,integration_status,audit_reason,notes)
VALUES
('DEMO-RCL0194ENG-20.4','DEMO-RCL0194ENG','diagram',204,NULL,'20.4','Engine Management System (MEMS) SPi (JAPAN)',NULL,'useful','integrated','Representative multilingual visual/knowledge case','Prototype sample from existing RAVE asset.'),
('DEMO-RCL0193FRE-P128','DEMO-RCL0193FRE','page',128,128,'PDF p.128','18.30.10 ECT removal',NULL,'useful','integrated','Representative procedure/source-cleaning case',NULL),
('DEMO-RCL0193FRE-P129','DEMO-RCL0193FRE','page',129,129,'PDF p.129','18.30.10 ECT refit',NULL,'useful','integrated','Related procedure page retained for traceability',NULL),
('DEMO-RCL0193FRE-P159','DEMO-RCL0193FRE','page',159,159,'PDF p.159','Cooling system - thermostat',NULL,'useful','integrated','Representative structured specification/table case',NULL);

-- Knowledge derived from an existing BUILD #103 RAVE fact.
INSERT OR IGNORE INTO mems_doc_entity
(entity_key,document_key,unit_key,entity_kind,parent_entity_key,domain,component_key,verification_level,source_locale,source_locator,legacy_table,legacy_key,sort_order,notes)
SELECT
'DEMO-K-RCL0194-ECT-WIRING','DEMO-RCL0194ENG','DEMO-RCL0194ENG-20.4','knowledge',NULL,
'electrical_engine_management','ect',verification_level,'fr',source_section,'mems_rave_fact',fact_key,10,
'French project knowledge derived from the English constructor diagram; legacy source remains untouched.'
FROM mems_rave_fact WHERE fact_key='RAVE-WIR-SPIJ-008';

INSERT OR IGNORE INTO mems_doc_text(entity_key,field_key,locale,text_value,translation_status,translation_method,source_locale,notes)
SELECT 'DEMO-K-RCL0194-ECT-WIRING','source_exact','fr',statement,'source','legacy_copy','fr',
       'Exact legacy project statement copied for traceability.'
FROM mems_rave_fact WHERE fact_key='RAVE-WIR-SPIJ-008';

INSERT OR IGNORE INTO mems_doc_text(entity_key,field_key,locale,text_value,translation_status,translation_method,source_locale,notes)
SELECT 'DEMO-K-RCL0194-ECT-WIRING','display','fr',statement,'source','legacy_copy','fr',NULL
FROM mems_rave_fact WHERE fact_key='RAVE-WIR-SPIJ-008';

INSERT OR IGNORE INTO mems_doc_text(entity_key,field_key,locale,text_value,translation_status,translation_method,source_locale,notes)
VALUES
('DEMO-K-RCL0194-ECT-WIRING','display','en',
 'Japanese-market 97MY SPi coolant temperature sensor C165: C165-2 (KG) is wired to MEMS C159-33, while C165-1 (KB) joins the common sensor earth C159-30 through SJ5.',
 'draft','prototype_human_translation','fr','Prototype translation only; not production-approved.');

-- The original constructor visual remains one file, referenced by its legacy SHA-256.
INSERT OR IGNORE INTO mems_doc_entity
(entity_key,document_key,unit_key,entity_kind,parent_entity_key,domain,component_key,verification_level,source_locale,source_locator,legacy_table,legacy_key,sort_order,notes)
SELECT
'DEMO-V-RCL0194ENG-20.4','DEMO-RCL0194ENG','DEMO-RCL0194ENG-20.4','visual','DEMO-K-RCL0194-ECT-WIRING',
'electrical_engine_management',NULL,'verifie_constructeur','en','RCL0194ENG:20.4',
'mems_rave_illustration',illustration_key,20,'Single immutable constructor visual reused by every locale.'
FROM mems_rave_illustration WHERE illustration_key='RAVE:RCL0194ENG:20.4';

INSERT OR IGNORE INTO mems_doc_visual
(entity_key,relative_path,sha256,visual_kind,source_width,source_height,original_intact,legacy_illustration_key,notes)
SELECT
'DEMO-V-RCL0194ENG-20.4',relative_path,sha256,asset_kind,1696,1190,1,illustration_key,
'Dimensions verified from the BUILD #103 PNG; original file is not modified.'
FROM mems_rave_illustration WHERE illustration_key='RAVE:RCL0194ENG:20.4';

-- Region visually verified on the original PNG: text "COOLANT TEMPERATURE SENSOR (094)".
INSERT OR IGNORE INTO mems_doc_entity
(entity_key,document_key,unit_key,entity_kind,parent_entity_key,domain,component_key,verification_level,source_locale,source_locator,legacy_table,legacy_key,sort_order,notes)
VALUES
('DEMO-L-RCL0194ENG-20.4-ECT','DEMO-RCL0194ENG','DEMO-RCL0194ENG-20.4','visual_label','DEMO-V-RCL0194ENG-20.4',
'electrical_engine_management','ect','verifie_constructeur','en','RCL0194ENG:20.4 visual label',NULL,NULL,30,
'Visual overlay label prototype; region coordinates normalized to original PNG dimensions.');

INSERT OR IGNORE INTO mems_doc_visual_region
(region_entity_key,visual_entity_key,region_kind,x_norm,y_norm,width_norm,height_norm,anchor_ref,z_order,notes)
VALUES
('DEMO-L-RCL0194ENG-20.4-ECT','DEMO-V-RCL0194ENG-20.4','label',0.232,0.535,0.122,0.063,'COOLANT TEMPERATURE SENSOR (094)',10,
'Coordinates manually verified on BUILD #103 image for prototype validation.');

INSERT OR IGNORE INTO mems_doc_text(entity_key,field_key,locale,text_value,translation_status,translation_method,source_locale,notes)
VALUES
('DEMO-L-RCL0194ENG-20.4-ECT','label','en','COOLANT TEMPERATURE SENSOR (094)','source','visual_transcription','en','Exact text visible in constructor visual.'),
('DEMO-L-RCL0194ENG-20.4-ECT','label','fr','SONDE DE TEMPÉRATURE DU LIQUIDE DE REFROIDISSEMENT (ECT)','draft','prototype_human_translation','en','Prototype translation only.');

INSERT OR IGNORE INTO mems_doc_relation(from_entity_key,relation_type,to_entity_key,notes)
VALUES
('DEMO-K-RCL0194-ECT-WIRING','illustrated_by','DEMO-V-RCL0194ENG-20.4','One original diagram serves every locale.');

-- ECT removal procedure. Source strings are copied exactly from BUILD #103.
INSERT OR IGNORE INTO mems_doc_entity
(entity_key,document_key,unit_key,entity_kind,parent_entity_key,domain,component_key,verification_level,source_locale,source_locator,legacy_table,legacy_key,sort_order,notes)
SELECT
'DEMO-P-ECT-REMOVE','DEMO-RCL0193FRE','DEMO-RCL0193FRE-P128','procedure',NULL,
'engine_management_service',component_key,'verifie_constructeur','fr','PDF p.128; opération 18.30.10',
'mems_procedure',procedure_key,100,'Legacy procedure preserved; clean display text stored separately.'
FROM mems_procedure WHERE procedure_key='PROC-RCL0193FRE-18-30-10-remove-1';

INSERT OR IGNORE INTO mems_doc_text(entity_key,field_key,locale,text_value,translation_status,translation_method,source_locale,notes)
SELECT 'DEMO-P-ECT-REMOVE','title','fr',title_source,'source','legacy_copy','fr',NULL
FROM mems_procedure WHERE procedure_key='PROC-RCL0193FRE-18-30-10-remove-1';

INSERT OR IGNORE INTO mems_doc_text(entity_key,field_key,locale,text_value,translation_status,translation_method,source_locale,notes)
VALUES
('DEMO-P-ECT-REMOVE','title','en','ENGINE COOLANT TEMPERATURE SENSOR (ECT) — removal','draft','prototype_human_translation','fr','Prototype translation only.');

INSERT OR IGNORE INTO mems_doc_entity
(entity_key,document_key,unit_key,entity_kind,parent_entity_key,domain,component_key,verification_level,source_locale,source_locator,legacy_table,legacy_key,sort_order,notes)
SELECT
'DEMO-P-ECT-REMOVE-S' || step_no,'DEMO-RCL0193FRE','DEMO-RCL0193FRE-P128','procedure_step','DEMO-P-ECT-REMOVE',
'engine_management_service','sonde_de_temperature_du_liquide_de_refroidissement_ect','verifie_constructeur','fr',
figure_ref,'mems_procedure_step','PROC-RCL0193FRE-18-30-10-remove-1#' || step_no,100+step_no,
CASE WHEN step_no=3 THEN 'Known legacy page-header residue intentionally retained only in source_exact.' ELSE NULL END
FROM mems_procedure_step
WHERE procedure_key='PROC-RCL0193FRE-18-30-10-remove-1';

INSERT OR IGNORE INTO mems_doc_text(entity_key,field_key,locale,text_value,translation_status,translation_method,source_locale,notes)
SELECT
'DEMO-P-ECT-REMOVE-S' || step_no,'source_exact','fr',instruction_source,'source','legacy_copy','fr',
'Exact BUILD #103 legacy instruction; do not clean this field.'
FROM mems_procedure_step
WHERE procedure_key='PROC-RCL0193FRE-18-30-10-remove-1';

INSERT OR IGNORE INTO mems_doc_text(entity_key,field_key,locale,text_value,translation_status,translation_method,source_locale,notes)
SELECT
'DEMO-P-ECT-REMOVE-S' || step_no,'display','fr',
trim(replace(instruction_source,' SYSTEME DE GESTION MOTEUR - MEMS','')),
CASE WHEN step_no=3 THEN 'draft' ELSE 'source' END,
CASE WHEN step_no=3 THEN 'prototype_normalisation' ELSE 'legacy_copy' END,'fr',
CASE WHEN step_no=3 THEN 'Header residue removed only from display field; source_exact remains unchanged.' ELSE NULL END
FROM mems_procedure_step
WHERE procedure_key='PROC-RCL0193FRE-18-30-10-remove-1';

INSERT OR IGNORE INTO mems_doc_text(entity_key,field_key,locale,text_value,translation_status,translation_method,source_locale,notes)
VALUES
('DEMO-P-ECT-REMOVE-S1','display','en','1. Disconnect the multiplug from the ECT sensor.','draft','prototype_human_translation','fr','Prototype translation only.'),
('DEMO-P-ECT-REMOVE-S2','display','en','2. Position a container to collect any coolant leakage.','draft','prototype_human_translation','fr','Prototype translation only.'),
('DEMO-P-ECT-REMOVE-S3','display','en','3. Remove the ECT sensor and discard the sealing washer.','draft','prototype_human_translation','fr','Prototype translation only.');

INSERT OR IGNORE INTO mems_doc_relation(from_entity_key,relation_type,to_entity_key,notes)
VALUES
('DEMO-P-ECT-REMOVE','has_step','DEMO-P-ECT-REMOVE-S1',NULL),
('DEMO-P-ECT-REMOVE','has_step','DEMO-P-ECT-REMOVE-S2',NULL),
('DEMO-P-ECT-REMOVE','has_step','DEMO-P-ECT-REMOVE-S3',NULL);

-- Thermostat values from RCL0193FRE PDF p.159.
INSERT OR IGNORE INTO mems_doc_entity
(entity_key,document_key,unit_key,entity_kind,parent_entity_key,domain,component_key,verification_level,source_locale,source_locator,legacy_table,legacy_key,sort_order,notes)
SELECT
'DEMO-N-THERMOSTAT-OPEN','DEMO-RCL0193FRE','DEMO-RCL0193FRE-P159','specification_value',NULL,
'cooling_system','thermostat','verifie_constructeur','fr','PDF p.159',
'mems_specification',spec_key,200,'Numeric value copied directly from structured BUILD #103 specification.'
FROM mems_specification WHERE spec_key='SPEC-RCL0193FRE-1810-THERMOSTAT-OPEN';

INSERT OR IGNORE INTO mems_doc_value
(entity_key,value_kind,sequence_no,value_numeric,value_min,value_max,tolerance_minus,tolerance_plus,angle_deg,unit_code,notes)
SELECT
'DEMO-N-THERMOSTAT-OPEN','opening_temperature',1,v.value_numeric,v.value_min,v.value_max,v.tolerance_minus,v.tolerance_plus,v.angle_deg,v.unit,
'Numeric value remains language-independent.'
FROM mems_specification_value v WHERE v.spec_key='SPEC-RCL0193FRE-1810-THERMOSTAT-OPEN' AND v.sequence_no=1;

INSERT OR IGNORE INTO mems_doc_text(entity_key,field_key,locale,text_value,translation_status,translation_method,source_locale,notes)
SELECT
'DEMO-N-THERMOSTAT-OPEN','source_exact','fr',instruction_text,'source','legacy_copy','fr',NULL
FROM mems_specification_value WHERE spec_key='SPEC-RCL0193FRE-1810-THERMOSTAT-OPEN' AND sequence_no=1;

INSERT OR IGNORE INTO mems_doc_text(entity_key,field_key,locale,text_value,translation_status,translation_method,source_locale,notes)
VALUES
('DEMO-N-THERMOSTAT-OPEN','label','fr','Température d''ouverture du thermostat','draft','prototype_normalisation','fr','Display label extracted from source statement.'),
('DEMO-N-THERMOSTAT-OPEN','label','en','Thermostat opening temperature','draft','prototype_human_translation','fr','Prototype translation only.');

-- The legacy row contains the exact 11 N.m statement but not a parsed numeric value.
-- The prototype demonstrates parsing it into a language-independent number while retaining exact source text.
INSERT OR IGNORE INTO mems_doc_entity
(entity_key,document_key,unit_key,entity_kind,parent_entity_key,domain,component_key,verification_level,source_locale,source_locator,legacy_table,legacy_key,sort_order,notes)
SELECT
'DEMO-N-THERMOSTAT-HOUSING-TORQUE','DEMO-RCL0193FRE','DEMO-RCL0193FRE-P159','specification_value',NULL,
'cooling_system','thermostat_housing','verifie_constructeur','fr','PDF p.159',
'mems_specification',spec_key,210,'11 N.m parsed from the exact manufacturer statement retained in source_exact.'
FROM mems_specification WHERE spec_key='SPEC-RCL0193FRE-1810-0009';

INSERT OR IGNORE INTO mems_doc_value
(entity_key,value_kind,sequence_no,value_numeric,unit_code,notes)
SELECT 'DEMO-N-THERMOSTAT-HOUSING-TORQUE','tightening_torque',1,11.0,'Nm',
       'Prototype parser result validated against exact source statement containing 11 N.m.'
WHERE EXISTS (
  SELECT 1 FROM mems_specification_value
  WHERE spec_key='SPEC-RCL0193FRE-1810-0009' AND instruction_text LIKE '%11 N.m.%'
);

INSERT OR IGNORE INTO mems_doc_text(entity_key,field_key,locale,text_value,translation_status,translation_method,source_locale,notes)
SELECT
'DEMO-N-THERMOSTAT-HOUSING-TORQUE','source_exact','fr',instruction_text,'source','legacy_copy','fr',NULL
FROM mems_specification_value WHERE spec_key='SPEC-RCL0193FRE-1810-0009' AND sequence_no=1;

INSERT OR IGNORE INTO mems_doc_text(entity_key,field_key,locale,text_value,translation_status,translation_method,source_locale,notes)
VALUES
('DEMO-N-THERMOSTAT-HOUSING-TORQUE','label','fr','Couple de serrage des boulons du boîtier supérieur de thermostat','draft','prototype_normalisation','fr','Prototype display label.'),
('DEMO-N-THERMOSTAT-HOUSING-TORQUE','label','en','Upper thermostat housing bolt tightening torque','draft','prototype_human_translation','fr','Prototype translation only.');

-- Structured two-value table. Text labels and numbers are separate.
INSERT OR IGNORE INTO mems_doc_entity
(entity_key,document_key,unit_key,entity_kind,parent_entity_key,domain,component_key,verification_level,source_locale,source_locator,legacy_table,legacy_key,sort_order,notes)
VALUES
('DEMO-T-THERMOSTAT','DEMO-RCL0193FRE','DEMO-RCL0193FRE-P159','table',NULL,
'cooling_system','thermostat','verifie_constructeur','fr','PDF p.159',NULL,NULL,220,
'Prototype structured table assembled from two source specifications.');

INSERT OR IGNORE INTO mems_doc_table(entity_key,row_count,column_count,header_rows,header_columns,notes)
VALUES('DEMO-T-THERMOSTAT',3,2,1,1,'Rows: header, opening temperature, housing torque.');

INSERT OR IGNORE INTO mems_doc_entity
(entity_key,document_key,unit_key,entity_kind,parent_entity_key,domain,component_key,verification_level,source_locale,source_locator,legacy_table,legacy_key,sort_order,notes)
VALUES
('DEMO-T-THERMOSTAT-C00','DEMO-RCL0193FRE','DEMO-RCL0193FRE-P159','table_cell','DEMO-T-THERMOSTAT','cooling_system','thermostat','verifie_constructeur','fr','PDF p.159',NULL,NULL,221,NULL),
('DEMO-T-THERMOSTAT-C01','DEMO-RCL0193FRE','DEMO-RCL0193FRE-P159','table_cell','DEMO-T-THERMOSTAT','cooling_system','thermostat','verifie_constructeur','fr','PDF p.159',NULL,NULL,222,NULL),
('DEMO-T-THERMOSTAT-C10','DEMO-RCL0193FRE','DEMO-RCL0193FRE-P159','table_cell','DEMO-T-THERMOSTAT','cooling_system','thermostat','verifie_constructeur','fr','PDF p.159',NULL,NULL,223,NULL),
('DEMO-T-THERMOSTAT-C11','DEMO-RCL0193FRE','DEMO-RCL0193FRE-P159','table_cell','DEMO-T-THERMOSTAT','cooling_system','thermostat','verifie_constructeur','fr','PDF p.159',NULL,NULL,224,NULL),
('DEMO-T-THERMOSTAT-C20','DEMO-RCL0193FRE','DEMO-RCL0193FRE-P159','table_cell','DEMO-T-THERMOSTAT','cooling_system','thermostat_housing','verifie_constructeur','fr','PDF p.159',NULL,NULL,225,NULL),
('DEMO-T-THERMOSTAT-C21','DEMO-RCL0193FRE','DEMO-RCL0193FRE-P159','table_cell','DEMO-T-THERMOSTAT','cooling_system','thermostat_housing','verifie_constructeur','fr','PDF p.159',NULL,NULL,226,NULL);

INSERT OR IGNORE INTO mems_doc_table_cell
(cell_entity_key,table_entity_key,row_no,column_no,row_span,column_span,value_numeric,value_min,value_max,unit_code,notes)
VALUES
('DEMO-T-THERMOSTAT-C00','DEMO-T-THERMOSTAT',0,0,1,1,NULL,NULL,NULL,NULL,'Header label'),
('DEMO-T-THERMOSTAT-C01','DEMO-T-THERMOSTAT',0,1,1,1,NULL,NULL,NULL,NULL,'Header label'),
('DEMO-T-THERMOSTAT-C10','DEMO-T-THERMOSTAT',1,0,1,1,NULL,NULL,NULL,NULL,'Parameter label'),
('DEMO-T-THERMOSTAT-C11','DEMO-T-THERMOSTAT',1,1,1,1,88.0,NULL,NULL,'°C','Language-independent numeric value'),
('DEMO-T-THERMOSTAT-C20','DEMO-T-THERMOSTAT',2,0,1,1,NULL,NULL,NULL,NULL,'Parameter label'),
('DEMO-T-THERMOSTAT-C21','DEMO-T-THERMOSTAT',2,1,1,1,11.0,NULL,NULL,'Nm','Language-independent numeric value');

INSERT OR IGNORE INTO mems_doc_text(entity_key,field_key,locale,text_value,translation_status,translation_method,source_locale,notes)
VALUES
('DEMO-T-THERMOSTAT-C00','display','fr','Paramètre','draft','prototype_ui_label','fr',NULL),
('DEMO-T-THERMOSTAT-C00','display','en','Parameter','draft','prototype_human_translation','fr',NULL),
('DEMO-T-THERMOSTAT-C01','display','fr','Valeur','draft','prototype_ui_label','fr',NULL),
('DEMO-T-THERMOSTAT-C01','display','en','Value','draft','prototype_human_translation','fr',NULL),
('DEMO-T-THERMOSTAT-C10','display','fr','Température d''ouverture','draft','prototype_normalisation','fr',NULL),
('DEMO-T-THERMOSTAT-C10','display','en','Opening temperature','draft','prototype_human_translation','fr',NULL),
('DEMO-T-THERMOSTAT-C20','display','fr','Couple boîtier supérieur','draft','prototype_normalisation','fr',NULL),
('DEMO-T-THERMOSTAT-C20','display','en','Upper housing torque','draft','prototype_human_translation','fr',NULL);

INSERT OR IGNORE INTO mems_doc_relation(from_entity_key,relation_type,to_entity_key,notes)
VALUES
('DEMO-T-THERMOSTAT','summarizes','DEMO-N-THERMOSTAT-OPEN',NULL),
('DEMO-T-THERMOSTAT','summarizes','DEMO-N-THERMOSTAT-HOUSING-TORQUE',NULL);

COMMIT;
