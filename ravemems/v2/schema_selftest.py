#!/usr/bin/env python3
from __future__ import annotations

import sqlite3
import tempfile
from pathlib import Path

from audit import audit_database


def expect(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> int:
    here = Path(__file__).resolve().parent
    schema = (here / "schema.sql").read_text(encoding="utf-8")

    with tempfile.TemporaryDirectory(prefix="ravemems-v2-") as td:
        db_path = Path(td) / "foundation.sqlite"
        db = sqlite3.connect(db_path)
        db.execute("PRAGMA foreign_keys=ON")
        db.executescript(schema)

        # 1) A future carburettor/manual-management vehicle with NO engine ECU
        # is a first-class, valid applicability context.
        db.execute(
            "INSERT INTO ravemems_document(document_key,canonical_name,source_language,title_source) "
            "VALUES(?,?,?,?)",
            ("DOC_CARB", "CARB_MANUAL", "en", "Carburettor service manual"),
        )
        db.execute(
            "INSERT INTO ravemems_document_revision("
            "revision_key,document_key,edition_label,source_relative_path,source_sha256,page_count,is_current) "
            "VALUES(?,?,?,?,?,?,?)",
            ("REV_CARB", "DOC_CARB", "1", "manuals/carb.pdf", "a" * 64, 2, 1),
        )
        db.execute(
            "INSERT INTO ravemems_page(page_key,revision_key,physical_page,extraction_status) "
            "VALUES(?,?,?,?)",
            ("CARB_P1", "REV_CARB", 1, "complete"),
        )
        db.execute(
            "INSERT INTO ravemems_applicability_scope("
            "scope_key,make,model,year_from,year_to,market,engine_family,engine_code,"
            "fuel_delivery_kind,engine_management_kind,engine_controller_state,"
            "transmission_kind,air_conditioning_state,source_scope_text) "
            "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
            (
                "SCOPE_CARB",
                "Example",
                "Classic",
                1965,
                1985,
                "EU",
                "A-Series",
                "12A",
                "carburetor",
                "mechanical",
                "absent",
                "manual",
                "absent",
                "Carburettor model without engine ECU",
            ),
        )
        db.execute(
            "INSERT INTO ravemems_revision_scope(revision_key,scope_key) VALUES(?,?)",
            ("REV_CARB", "SCOPE_CARB"),
        )
        db.execute(
            "INSERT INTO ravemems_scope_attribute(scope_key,attribute_key,attribute_value,source_text) "
            "VALUES(?,?,?,?)",
            ("SCOPE_CARB", "ignition_system", "contact_breaker", "Contact breaker ignition"),
        )

        carb = db.execute(
            "SELECT fuel_delivery_kind,engine_management_kind,engine_controller_state,ecu_family "
            "FROM ravemems_applicability_scope WHERE scope_key='SCOPE_CARB'"
        ).fetchone()
        expect(carb == ("carburetor", "mechanical", "absent", None), "carburettor/no-ECU context lost")

        # The schema must actively reject a fake ECU attached to a context that
        # explicitly says no engine controller exists.
        rejected_fake_ecu = False
        try:
            db.execute(
                "INSERT INTO ravemems_applicability_scope("
                "scope_key,fuel_delivery_kind,engine_management_kind,engine_controller_state,ecu_family) "
                "VALUES(?,?,?,?,?)",
                ("SCOPE_INVALID", "carburetor", "mechanical", "absent", "MEMS 1.9"),
            )
        except sqlite3.IntegrityError:
            rejected_fake_ecu = True
        expect(rejected_fake_ecu, "schema accepted an ECU while controller_state=absent")

        # 2) Electronic injection + ECU is independently representable.
        db.execute(
            "INSERT INTO ravemems_document(document_key,canonical_name,source_language,title_source) "
            "VALUES(?,?,?,?)",
            ("DOC_MPI", "MPI_MANUAL", "en", "Electronic injection service manual"),
        )
        db.execute(
            "INSERT INTO ravemems_document_revision("
            "revision_key,document_key,source_relative_path,source_sha256,page_count,is_current) "
            "VALUES(?,?,?,?,?,?)",
            ("REV_MPI", "DOC_MPI", "manuals/mpi.pdf", "b" * 64, 1, 1),
        )
        db.execute(
            "INSERT INTO ravemems_page(page_key,revision_key,physical_page,extraction_status) "
            "VALUES(?,?,?,?)",
            ("MPI_P1", "REV_MPI", 1, "complete"),
        )
        db.execute(
            "INSERT INTO ravemems_applicability_scope("
            "scope_key,make,model,fuel_delivery_kind,engine_management_kind,engine_controller_state,"
            "ecu_family,ecu_reference,ecu_firmware,transmission_kind,source_scope_text) "
            "VALUES(?,?,?,?,?,?,?,?,?,?,?)",
            (
                "SCOPE_MPI",
                "Example",
                "MPI",
                "mpi",
                "electronic",
                "present",
                "MEMS 1.9",
                "MKC",
                "AANMP002",
                "manual",
                "MPI with electronic engine ECU",
            ),
        )
        mpi = db.execute(
            "SELECT engine_controller_state,ecu_family,ecu_firmware "
            "FROM ravemems_applicability_scope WHERE scope_key='SCOPE_MPI'"
        ).fetchone()
        expect(mpi == ("present", "MEMS 1.9", "AANMP002"), "optional ECU context lost")

        # 3) One manufacturer operation spans phases. Remove and Refit each keep
        # their own manufacturer step numbering instead of being flattened.
        db.execute(
            "INSERT INTO ravemems_operation("
            "operation_key,revision_key,sequence_no,manufacturer_operation_no,operation_kind,"
            "title_source,source_language,completeness_status) VALUES(?,?,?,?,?,?,?,?)",
            ("OP1", "REV_CARB", 1, "19.22.61", "service", "Fuel system component", "en", "complete"),
        )
        db.execute(
            "INSERT INTO ravemems_operation_scope(operation_key,scope_key) VALUES(?,?)",
            ("OP1", "SCOPE_CARB"),
        )
        db.execute(
            "INSERT INTO ravemems_phase("
            "phase_key,operation_key,sequence_no,phase_kind_source,normalized_phase_kind,title_source) "
            "VALUES(?,?,?,?,?,?)",
            ("PH_REMOVE", "OP1", 1, "Remove", "remove", "Remove"),
        )
        db.execute(
            "INSERT INTO ravemems_phase("
            "phase_key,operation_key,sequence_no,phase_kind_source,normalized_phase_kind,title_source) "
            "VALUES(?,?,?,?,?,?)",
            ("PH_REFIT", "OP1", 2, "Refit", "refit", "Refit"),
        )
        db.execute(
            "INSERT INTO ravemems_step("
            "step_key,phase_key,sequence_no,manufacturer_step_no,instruction_source,completeness_status,source_page_start) "
            "VALUES(?,?,?,?,?,?,?)",
            ("STEP_RM_1", "PH_REMOVE", 1, "1", "Disconnect component", "complete", 1),
        )
        # Intentional audit defect: sequence 2 is missing and this fragment is incomplete.
        db.execute(
            "INSERT INTO ravemems_step("
            "step_key,phase_key,sequence_no,manufacturer_step_no,instruction_source,condition_text,"
            "completeness_status,source_page_start,source_page_end) VALUES(?,?,?,?,?,?,?,?,?)",
            (
                "STEP_RM_3",
                "PH_REMOVE",
                3,
                "3",
                "Remove component",
                "If fitted",
                "incomplete",
                1,
                2,
            ),
        )
        db.execute(
            "INSERT INTO ravemems_step("
            "step_key,phase_key,sequence_no,manufacturer_step_no,instruction_source,completeness_status,source_page_start) "
            "VALUES(?,?,?,?,?,?,?)",
            ("STEP_RF_1", "PH_REFIT", 1, "1", "Refit component", "complete", 2),
        )
        numbering = db.execute(
            "SELECT p.normalized_phase_kind,s.manufacturer_step_no FROM ravemems_step s "
            "JOIN ravemems_phase p ON p.phase_key=s.phase_key "
            "WHERE s.step_key IN ('STEP_RM_1','STEP_RF_1') ORDER BY p.sequence_no"
        ).fetchall()
        expect(numbering == [("remove", "1"), ("refit", "1")], "manufacturer numbering was flattened")

        # Warning scope and a before-start special tool are structural data.
        db.execute(
            "INSERT INTO ravemems_notice("
            "notice_key,operation_key,sequence_no,notice_kind,source_text,scope_kind,target_phase_key) "
            "VALUES(?,?,?,?,?,?,?)",
            ("NOTICE1", "OP1", 1, "warning", "Protect eyes before this phase", "phase", "PH_REMOVE"),
        )
        db.execute(
            "INSERT INTO ravemems_requirement("
            "requirement_key,operation_key,phase_key,step_key,sequence_no,requirement_type,"
            "requirement_source,part_number,quantity,unit,before_start,figure_ref) "
            "VALUES(?,?,?,?,?,?,?,?,?,?,?,?)",
            (
                "REQ1",
                "OP1",
                "PH_REMOVE",
                None,
                1,
                "special_tool",
                "Use special tool 18G0001",
                "18G0001",
                1.0,
                "item",
                1,
                None,
            ),
        )

        # 4) Sequential/angle tightening is an ordered specification, never a
        # single collapsed torque value.
        db.execute(
            "INSERT INTO ravemems_specification("
            "specification_key,operation_key,parameter_source,default_unit,condition_text,source_language) "
            "VALUES(?,?,?,?,?,?)",
            ("SPEC_TIGHTEN", "OP1", "Fastener tightening sequence", "N.m", "Engine cold", "en"),
        )
        db.execute(
            "INSERT INTO ravemems_specification_value("
            "value_key,specification_key,sequence_no,value_numeric,unit,instruction_text) "
            "VALUES(?,?,?,?,?,?)",
            ("SV1", "SPEC_TIGHTEN", 1, 20.0, "N.m", "Initial torque"),
        )
        db.execute(
            "INSERT INTO ravemems_specification_value("
            "value_key,specification_key,sequence_no,angle_deg,instruction_text) VALUES(?,?,?,?,?)",
            ("SV2", "SPEC_TIGHTEN", 2, 60.0, "First angular pass"),
        )
        db.execute(
            "INSERT INTO ravemems_specification_value("
            "value_key,specification_key,sequence_no,angle_deg,instruction_text) VALUES(?,?,?,?,?)",
            ("SV3", "SPEC_TIGHTEN", 3, 60.0, "Second angular pass"),
        )
        tightening = db.execute(
            "SELECT sequence_no,value_numeric,angle_deg,unit FROM ravemems_specification_value "
            "WHERE specification_key='SPEC_TIGHTEN' ORDER BY sequence_no"
        ).fetchall()
        expect(
            tightening == [(1, 20.0, None, "N.m"), (2, None, 60.0, None), (3, None, 60.0, None)],
            "sequential tightening representation failed",
        )

        # 5) Visuals are typed, rendered/cropped assets with fidelity and link
        # verification gates. They are not raw xref bitmaps implicitly trusted.
        db.execute(
            "INSERT INTO ravemems_visual("
            "visual_key,revision_key,page_key,visual_type,relative_path,sha256,width,height,"
            "render_method,source_bbox_json,crop_bbox_json,caption_source,source_language,fidelity_status) "
            "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
            (
                "VIS1",
                "REV_CARB",
                "CARB_P1",
                "tightening_sequence",
                "visuals/carb_tightening.png",
                "c" * 64,
                800,
                1200,
                "pdf_page_render_crop",
                "[0,0,600,800]",
                "[20,30,580,760]",
                "Tightening order",
                "en",
                "pending",
            ),
        )
        db.execute(
            "INSERT INTO ravemems_visual_link("
            "visual_link_key,visual_key,specification_key,relation_type,verification_status) "
            "VALUES(?,?,?,?,?)",
            ("VL1", "VIS1", "SPEC_TIGHTEN", "illustrates", "pending"),
        )

        # Translation exists but is provisional until the post-extraction audit.
        db.execute(
            "INSERT INTO ravemems_translation("
            "translation_key,entity_kind,entity_key,field_name,source_language,target_language,"
            "translated_text,translation_status) VALUES(?,?,?,?,?,?,?,?)",
            ("TR1", "operation", "OP1", "title", "en", "fr", "Composant du système carburant", "provisional"),
        )
        db.execute(
            "INSERT INTO ravemems_review_flag("
            "review_key,entity_kind,entity_key,reason_code,reason_text,severity,status,created_by) "
            "VALUES(?,?,?,?,?,?,?,?)",
            (
                "REVIEW1",
                "step",
                "STEP_RM_3",
                "sequence_gap",
                "Step sequence requires verification",
                "blocking",
                "open",
                "schema_selftest",
            ),
        )
        db.commit()

        first = audit_database(db)
        codes = {item["code"] for item in first}
        expected_codes = {
            "page_coverage",
            "step_sequence_gap",
            "step_not_complete",
            "visual_fidelity_not_verified",
            "visual_link_not_verified",
            "translation_not_validated",
            "open_review_flag",
        }
        missing = expected_codes - codes
        expect(not missing, f"post-extraction audit missed expected findings: {sorted(missing)}")

        # Repair every intentional defect. The same audit must become fully green.
        db.execute(
            "INSERT INTO ravemems_page(page_key,revision_key,physical_page,extraction_status) "
            "VALUES(?,?,?,?)",
            ("CARB_P2", "REV_CARB", 2, "complete"),
        )
        db.execute(
            "UPDATE ravemems_step SET sequence_no=2,completeness_status='complete' "
            "WHERE step_key='STEP_RM_3'"
        )
        db.execute("UPDATE ravemems_visual SET fidelity_status='verified' WHERE visual_key='VIS1'")
        db.execute(
            "UPDATE ravemems_visual_link SET verification_status='verified' WHERE visual_link_key='VL1'"
        )
        db.execute(
            "UPDATE ravemems_translation SET translation_status='validated' WHERE translation_key='TR1'"
        )
        db.execute(
            "UPDATE ravemems_review_flag SET status='resolved',resolution_text='Sequence checked against source' "
            "WHERE review_key='REVIEW1'"
        )
        db.commit()

        final_issues = audit_database(db)
        expect(final_issues == [], f"repaired V2 database still has findings: {final_issues}")
        expect(db.execute("PRAGMA integrity_check").fetchone()[0].lower() == "ok", "integrity_check failed")
        expect(db.execute("PRAGMA foreign_key_check").fetchall() == [], "foreign key check failed")

        print("RAVEMEMS_V2_SCHEMA_SELFTEST_PASS")
        print("CARBURETOR_WITHOUT_ECU_PASS")
        print("OPTIONAL_ECU_CONTEXT_PASS")
        print("MULTIPHASE_STEP_NUMBERING_PASS")
        print("SEQUENTIAL_TIGHTENING_PASS")
        print("POST_EXTRACTION_AUDIT_PASS")
        db.close()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
