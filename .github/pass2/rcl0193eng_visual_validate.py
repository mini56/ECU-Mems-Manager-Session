#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import sqlite3
from pathlib import Path
from typing import Any

import fitz

from audit import audit_database


def _bbox(value: str | None) -> fitz.Rect:
    raw = json.loads(value or "null")
    if not isinstance(raw, list) or len(raw) != 4:
        raise ValueError(f"invalid bbox: {value!r}")
    return fitz.Rect(*(float(v) for v in raw))


def _rect_contains(outer: fitz.Rect, inner: fitz.Rect, tolerance: float = 0.05) -> bool:
    return (
        inner.x0 >= outer.x0 - tolerance
        and inner.y0 >= outer.y0 - tolerance
        and inner.x1 <= outer.x1 + tolerance
        and inner.y1 <= outer.y1 + tolerance
    )


def _sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def validate_visual_fidelity(
    db: sqlite3.Connection,
    doc: fitz.Document,
    out_dir: Path,
) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    verified: list[dict[str, Any]] = []
    failed: list[dict[str, Any]] = []
    rows = db.execute(
        "SELECT v.visual_key,v.page_key,v.relative_path,v.sha256,v.width,v.height,"
        "v.render_method,v.source_bbox_json,v.crop_bbox_json,p.physical_page "
        "FROM ravemems_visual v JOIN ravemems_page p ON p.page_key=v.page_key "
        "ORDER BY v.visual_key"
    ).fetchall()

    for row in rows:
        visual_key, page_key, relative_path, stored_sha, width, height, render_method, source_json, crop_json, physical_page = row
        evidence: dict[str, Any] = {
            "visual_key": visual_key,
            "page_key": page_key,
            "physical_page": int(physical_page),
            "render_method": render_method,
        }
        try:
            if render_method != "pdf_page_render_crop":
                raise ValueError(f"unexpected render_method={render_method}")
            page = doc[int(physical_page) - 1]
            source = _bbox(source_json)
            crop = _bbox(crop_json)
            if source.is_empty or crop.is_empty:
                raise ValueError("empty source/crop bbox")
            if not _rect_contains(page.rect, crop):
                raise ValueError("crop bbox outside source page")
            if not _rect_contains(crop, source):
                raise ValueError("source bbox not contained by crop bbox")

            asset_path = out_dir / str(relative_path)
            blob = asset_path.read_bytes()
            file_sha = _sha256(blob)
            if file_sha != stored_sha:
                raise ValueError(f"asset sha mismatch db={stored_sha} file={file_sha}")

            expected = page.get_pixmap(matrix=fitz.Matrix(1.5, 1.5), clip=crop, alpha=False)
            actual = fitz.Pixmap(str(asset_path))
            if (actual.width, actual.height) != (int(width), int(height)):
                raise ValueError(
                    f"stored dimensions mismatch db={(width, height)} png={(actual.width, actual.height)}"
                )
            if (expected.width, expected.height, expected.n) != (actual.width, actual.height, actual.n):
                raise ValueError(
                    "rerender dimensions/channels mismatch "
                    f"expected={(expected.width, expected.height, expected.n)} "
                    f"actual={(actual.width, actual.height, actual.n)}"
                )
            expected_samples = bytes(expected.samples)
            actual_samples = bytes(actual.samples)
            expected_pixel_sha = _sha256(expected_samples)
            actual_pixel_sha = _sha256(actual_samples)
            if expected_samples != actual_samples:
                raise ValueError(
                    f"pixel mismatch expected={expected_pixel_sha} actual={actual_pixel_sha}"
                )

            db.execute(
                "UPDATE ravemems_visual SET fidelity_status='verified' WHERE visual_key=?",
                (visual_key,),
            )
            evidence.update(
                {
                    "status": "verified",
                    "asset_sha256": file_sha,
                    "pixel_sha256": actual_pixel_sha,
                    "width": actual.width,
                    "height": actual.height,
                    "source_bbox": list(source),
                    "crop_bbox": list(crop),
                }
            )
            verified.append(evidence)
        except Exception as exc:
            db.execute(
                "UPDATE ravemems_visual SET fidelity_status='failed' WHERE visual_key=?",
                (visual_key,),
            )
            evidence.update({"status": "failed", "reason": str(exc)})
            failed.append(evidence)
    return verified, failed


def _active_phases(db: sqlite3.Connection, page_key: str, physical_page: int) -> set[str]:
    phases = {
        str(row[0])
        for row in db.execute(
            "SELECT DISTINCT phase_key FROM ravemems_step "
            "WHERE source_page_start<=? AND source_page_end>=?",
            (physical_page, physical_page),
        )
    }
    phases.update(
        str(row[0])
        for row in db.execute(
            "SELECT DISTINCT entity_key FROM ravemems_provenance "
            "WHERE entity_kind='phase' AND page_key=?",
            (page_key,),
        )
    )
    return phases


def _active_operations(
    db: sqlite3.Connection,
    page_key: str,
    phases: set[str],
) -> set[str]:
    operations = {
        str(row[0])
        for row in db.execute(
            "SELECT DISTINCT entity_key FROM ravemems_provenance "
            "WHERE entity_kind='operation' AND page_key=?",
            (page_key,),
        )
    }
    if phases:
        placeholders = ",".join("?" for _ in phases)
        operations.update(
            str(row[0])
            for row in db.execute(
                f"SELECT DISTINCT operation_key FROM ravemems_phase WHERE phase_key IN ({placeholders})",
                tuple(sorted(phases)),
            )
        )
    return operations


def validate_visual_links(db: sqlite3.Connection) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    verified: list[dict[str, Any]] = []
    failed: list[dict[str, Any]] = []
    rows = db.execute(
        "SELECT l.visual_link_key,l.visual_key,l.relation_type,l.operation_key,l.phase_key,"
        "l.step_key,l.specification_key,l.table_key,v.page_key,p.physical_page "
        "FROM ravemems_visual_link l JOIN ravemems_visual v ON v.visual_key=l.visual_key "
        "JOIN ravemems_page p ON p.page_key=v.page_key ORDER BY l.visual_link_key"
    ).fetchall()

    for row in rows:
        (
            link_key,
            visual_key,
            relation_type,
            operation_key,
            phase_key,
            step_key,
            specification_key,
            table_key,
            page_key,
            physical_page,
        ) = row
        evidence: dict[str, Any] = {
            "visual_link_key": link_key,
            "visual_key": visual_key,
            "page_key": page_key,
            "physical_page": int(physical_page),
            "relation_type": relation_type,
        }
        try:
            targets = [operation_key, phase_key, step_key, specification_key, table_key]
            if sum(value is not None for value in targets) != 1:
                raise ValueError("visual link must have exactly one target")
            if relation_type != "illustrates":
                raise ValueError(f"unexpected relation_type={relation_type}")

            phases = _active_phases(db, str(page_key), int(physical_page))
            operations = _active_operations(db, str(page_key), phases)
            evidence["active_phase_keys"] = sorted(phases)
            evidence["active_operation_keys"] = sorted(operations)

            if phase_key is not None:
                if phases != {str(phase_key)}:
                    raise ValueError(
                        f"phase link is ambiguous on page: linked={phase_key} active={sorted(phases)}"
                    )
                evidence["target_kind"] = "phase"
                evidence["target_key"] = phase_key
            elif operation_key is not None:
                if operations != {str(operation_key)}:
                    raise ValueError(
                        f"operation link is ambiguous on page: linked={operation_key} active={sorted(operations)}"
                    )
                evidence["target_kind"] = "operation"
                evidence["target_key"] = operation_key
            elif step_key is not None:
                valid = db.execute(
                    "SELECT 1 FROM ravemems_step WHERE step_key=? AND source_page_start<=? AND source_page_end>=?",
                    (step_key, int(physical_page), int(physical_page)),
                ).fetchone()
                if valid is None:
                    raise ValueError(f"step target does not cover visual page: {step_key}")
                evidence["target_kind"] = "step"
                evidence["target_key"] = step_key
            else:
                raise ValueError("unsupported specification/table visual link in this prototype")

            db.execute(
                "UPDATE ravemems_visual_link SET verification_status='verified' WHERE visual_link_key=?",
                (link_key,),
            )
            evidence["status"] = "verified"
            verified.append(evidence)
        except Exception as exc:
            db.execute(
                "UPDATE ravemems_visual_link SET verification_status='failed' WHERE visual_link_key=?",
                (link_key,),
            )
            evidence.update({"status": "failed", "reason": str(exc)})
            failed.append(evidence)
    return verified, failed


def main() -> int:
    parser = argparse.ArgumentParser(description="Evidence-based RCL0193ENG visual validation")
    parser.add_argument("--pdf", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()

    db_path = args.out / "ravemems_v2_rcl0193eng.sqlite"
    manifest_path = args.out / "manifest.json"
    audit_path = args.out / "audit.json"
    evidence_path = args.out / "visual_validation.json"

    doc = fitz.open(args.pdf)
    db = sqlite3.connect(db_path)
    db.execute("PRAGMA foreign_keys=ON")
    try:
        fidelity_ok, fidelity_failed = validate_visual_fidelity(db, doc, args.out)
        links_ok, links_failed = validate_visual_links(db)
        db.commit()

        integrity = db.execute("PRAGMA integrity_check").fetchone()[0]
        foreign_keys = db.execute("PRAGMA foreign_key_check").fetchall()
        issues = audit_database(db)

        evidence = {
            "method": {
                "visual_fidelity": "independent source-page rerender at Matrix(1.5,1.5) with exact raw-pixel comparison, asset SHA-256 check, bbox containment and dimension checks",
                "visual_links": "independent persisted semantic-page reconstruction; link verified only when the linked phase/operation is the unique active semantic target on the visual page",
            },
            "visual_fidelity_verified_count": len(fidelity_ok),
            "visual_fidelity_failed_count": len(fidelity_failed),
            "visual_link_verified_count": len(links_ok),
            "visual_link_failed_count": len(links_failed),
            "sqlite_integrity": integrity,
            "foreign_key_issue_count": len(foreign_keys),
            "audit_issue_count_after_validation": len(issues),
            "fidelity": fidelity_ok + fidelity_failed,
            "links": links_ok + links_failed,
        }
        evidence_path.write_text(json.dumps(evidence, ensure_ascii=False, indent=2), encoding="utf-8")
        audit_path.write_text(json.dumps({"issue_count": len(issues), "issues": issues}, ensure_ascii=False, indent=2), encoding="utf-8")

        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        manifest["audit_issue_count"] = len(issues)
        manifest["visual_validation"] = {
            "evidence_file": evidence_path.name,
            "fidelity_verified": len(fidelity_ok),
            "fidelity_failed": len(fidelity_failed),
            "links_verified": len(links_ok),
            "links_failed": len(links_failed),
        }
        manifest_path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")

        print("RCL0193ENG_VISUAL_FIDELITY_VERIFIED", len(fidelity_ok))
        print("RCL0193ENG_VISUAL_FIDELITY_FAILED", len(fidelity_failed))
        print("RCL0193ENG_VISUAL_LINKS_VERIFIED", len(links_ok))
        print("RCL0193ENG_VISUAL_LINKS_FAILED", len(links_failed))
        print("RCL0193ENG_POST_VISUAL_AUDIT_ISSUES", len(issues))
        print("RCL0193ENG_POST_VISUAL_SQLITE_INTEGRITY", integrity)
        print("RCL0193ENG_POST_VISUAL_FOREIGN_KEYS", len(foreign_keys))

        if fidelity_failed or links_failed or str(integrity).lower() != "ok" or foreign_keys or issues:
            return 1
        print("RAVEMEMS_V2_RCL0193ENG_VISUAL_VALIDATION_PASS")
        return 0
    finally:
        db.close()
        doc.close()


if __name__ == "__main__":
    raise SystemExit(main())
