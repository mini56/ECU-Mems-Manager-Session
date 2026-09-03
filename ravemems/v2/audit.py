#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sqlite3
from pathlib import Path
from typing import Any


def _table_exists(db: sqlite3.Connection, name: str) -> bool:
    return db.execute(
        "SELECT 1 FROM sqlite_master WHERE type='table' AND name=? LIMIT 1", (name,)
    ).fetchone() is not None


def _issue(issues: list[dict[str, Any]], code: str, **details: Any) -> None:
    issues.append({"code": code, **details})


def audit_database(db: sqlite3.Connection) -> list[dict[str, Any]]:
    """Return all blocking/review findings for a RAVEMEMS V2 database.

    The audit is deliberately language-neutral. It verifies structural facts,
    not French/English keywords. An empty result means that the V2 structural
    gates implemented here are satisfied; it is not a substitute for the later
    document-specific human validation of the prototype.
    """
    db.row_factory = sqlite3.Row
    issues: list[dict[str, Any]] = []

    integrity = db.execute("PRAGMA integrity_check").fetchone()[0]
    if str(integrity).lower() != "ok":
        _issue(issues, "sqlite_integrity", result=integrity)

    for row in db.execute("PRAGMA foreign_key_check").fetchall():
        _issue(
            issues,
            "foreign_key",
            table=row[0],
            rowid=row[1],
            parent=row[2],
            fk_index=row[3],
        )

    required_tables = {
        "ravemems_document_revision",
        "ravemems_page",
        "ravemems_operation",
        "ravemems_phase",
        "ravemems_step",
        "ravemems_visual",
        "ravemems_visual_link",
        "ravemems_translation",
        "ravemems_review_flag",
    }
    missing = sorted(name for name in required_tables if not _table_exists(db, name))
    for name in missing:
        _issue(issues, "missing_table", table=name)
    if missing:
        return issues

    # Every revision must account for every physical PDF page exactly once.
    for revision in db.execute(
        "SELECT revision_key,page_count FROM ravemems_document_revision ORDER BY revision_key"
    ):
        revision_key = revision["revision_key"]
        expected_count = int(revision["page_count"])
        pages = [
            int(row[0])
            for row in db.execute(
                "SELECT physical_page FROM ravemems_page WHERE revision_key=? ORDER BY physical_page",
                (revision_key,),
            )
        ]
        expected = list(range(1, expected_count + 1))
        if pages != expected:
            _issue(
                issues,
                "page_coverage",
                revision_key=revision_key,
                expected=expected,
                observed=pages,
            )

    for row in db.execute(
        "SELECT page_key,extraction_status FROM ravemems_page "
        "WHERE extraction_status<>'complete' ORDER BY page_key"
    ):
        _issue(
            issues,
            "page_not_complete",
            page_key=row["page_key"],
            status=row["extraction_status"],
        )

    # Internal sequence_no is RAVEMEMS ordering. Manufacturer numbering remains
    # free text and can legitimately restart at 1 for Refit after Remove.
    for phase in db.execute("SELECT phase_key FROM ravemems_phase ORDER BY phase_key"):
        phase_key = phase["phase_key"]
        sequence = [
            int(row[0])
            for row in db.execute(
                "SELECT sequence_no FROM ravemems_step WHERE phase_key=? ORDER BY sequence_no",
                (phase_key,),
            )
        ]
        expected = list(range(1, len(sequence) + 1))
        if sequence != expected:
            _issue(
                issues,
                "step_sequence_gap",
                phase_key=phase_key,
                expected=expected,
                observed=sequence,
            )

    for table, key_column, code in (
        ("ravemems_operation", "operation_key", "operation_not_complete"),
        ("ravemems_phase", "phase_key", "phase_not_complete"),
        ("ravemems_step", "step_key", "step_not_complete"),
    ):
        for row in db.execute(
            f"SELECT {key_column},completeness_status FROM {table} "
            "WHERE completeness_status<>'complete' ORDER BY 1"
        ):
            _issue(
                issues,
                code,
                entity_key=row[0],
                status=row[1],
            )

    for row in db.execute(
        "SELECT visual_key,fidelity_status FROM ravemems_visual "
        "WHERE fidelity_status<>'verified' ORDER BY visual_key"
    ):
        _issue(
            issues,
            "visual_fidelity_not_verified",
            visual_key=row["visual_key"],
            status=row["fidelity_status"],
        )

    for row in db.execute(
        "SELECT visual_link_key,verification_status,operation_key,phase_key,step_key,"
        "specification_key,table_key FROM ravemems_visual_link ORDER BY visual_link_key"
    ):
        if row["verification_status"] != "verified":
            _issue(
                issues,
                "visual_link_not_verified",
                visual_link_key=row["visual_link_key"],
                status=row["verification_status"],
            )
        targets = [
            row["operation_key"],
            row["phase_key"],
            row["step_key"],
            row["specification_key"],
            row["table_key"],
        ]
        if sum(value is not None for value in targets) != 1:
            _issue(
                issues,
                "visual_link_target_count",
                visual_link_key=row["visual_link_key"],
                target_count=sum(value is not None for value in targets),
            )

    if _table_exists(db, "ravemems_table"):
        for row in db.execute(
            "SELECT table_key,structure_status FROM ravemems_table "
            "WHERE structure_status<>'verified' ORDER BY table_key"
        ):
            _issue(
                issues,
                "table_structure_not_verified",
                table_key=row["table_key"],
                status=row["structure_status"],
            )

    if _table_exists(db, "ravemems_operation_relation"):
        for row in db.execute(
            "SELECT relation_key,verification_status FROM ravemems_operation_relation "
            "WHERE verification_status<>'verified' ORDER BY relation_key"
        ):
            _issue(
                issues,
                "operation_relation_not_verified",
                relation_key=row["relation_key"],
                status=row["verification_status"],
            )

    # A provisional/rejected translation is not silently accepted. The audit
    # must surface it before corpus validation.
    for row in db.execute(
        "SELECT translation_key,translation_status,target_language FROM ravemems_translation "
        "WHERE translation_status<>'validated' ORDER BY translation_key"
    ):
        _issue(
            issues,
            "translation_not_validated",
            translation_key=row["translation_key"],
            target_language=row["target_language"],
            status=row["translation_status"],
        )

    for row in db.execute(
        "SELECT review_key,entity_kind,entity_key,reason_code,severity FROM ravemems_review_flag "
        "WHERE status='open' ORDER BY review_key"
    ):
        _issue(
            issues,
            "open_review_flag",
            review_key=row["review_key"],
            entity_kind=row["entity_kind"],
            entity_key=row["entity_key"],
            reason_code=row["reason_code"],
            severity=row["severity"],
        )

    # Warnings/notes have a real structural scope. Verify that the selected
    # target belongs to the operation carrying the notice and that exactly the
    # target appropriate to scope_kind is populated.
    if _table_exists(db, "ravemems_notice"):
        for row in db.execute(
            "SELECT notice_key,operation_key,scope_kind,target_operation_key,target_phase_key,"
            "target_step_key FROM ravemems_notice ORDER BY notice_key"
        ):
            valid = False
            if row["scope_kind"] == "operation":
                valid = (
                    row["target_operation_key"] == row["operation_key"]
                    and row["target_phase_key"] is None
                    and row["target_step_key"] is None
                )
            elif row["scope_kind"] == "phase":
                valid = (
                    row["target_operation_key"] is None
                    and row["target_phase_key"] is not None
                    and row["target_step_key"] is None
                    and db.execute(
                        "SELECT 1 FROM ravemems_phase WHERE phase_key=? AND operation_key=?",
                        (row["target_phase_key"], row["operation_key"]),
                    ).fetchone()
                    is not None
                )
            elif row["scope_kind"] == "step":
                valid = (
                    row["target_operation_key"] is None
                    and row["target_phase_key"] is None
                    and row["target_step_key"] is not None
                    and db.execute(
                        "SELECT 1 FROM ravemems_step s JOIN ravemems_phase p ON p.phase_key=s.phase_key "
                        "WHERE s.step_key=? AND p.operation_key=?",
                        (row["target_step_key"], row["operation_key"]),
                    ).fetchone()
                    is not None
                )
            if not valid:
                _issue(
                    issues,
                    "notice_scope_invalid",
                    notice_key=row["notice_key"],
                    scope_kind=row["scope_kind"],
                )

    return issues


def main() -> int:
    parser = argparse.ArgumentParser(description="Audit a RAVEMEMS V2 SQLite database")
    parser.add_argument("database", type=Path)
    parser.add_argument(
        "--allow-issues",
        action="store_true",
        help="print findings but return success (inspection mode only)",
    )
    args = parser.parse_args()

    db = sqlite3.connect(args.database)
    try:
        issues = audit_database(db)
    finally:
        db.close()

    print(json.dumps({"issue_count": len(issues), "issues": issues}, ensure_ascii=False, indent=2))
    return 0 if (not issues or args.allow_issues) else 1


if __name__ == "__main__":
    raise SystemExit(main())
