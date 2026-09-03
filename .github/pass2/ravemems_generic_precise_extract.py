#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import sqlite3
import sys
from pathlib import Path
from typing import Any

import fitz

import rcl0193eng_precise_visual_extract as precise
from generic_document_rules import (
    canonical_publication_token,
    document_kind_for_evidence,
    is_bare_calendar_date_identifier,
)


pe = precise.pe


class GenericSemanticParser(precise.pass2.Pass2SemanticParser):
    """PASS2 semantic parser plus corpus-generic document identity guards."""

    def _operation_code(self, text: str) -> str | None:
        code = super()._operation_code(text)
        if code and is_bare_calendar_date_identifier(text, code):
            return None
        return code


pe.SemanticParser = GenericSemanticParser

_REFERENCE_IDENTIFIER = re.compile(r"^[A-Z]{1,4}(?:[-/]?[A-Z0-9]{0,3})?\s*\d{1,4}[A-Z]?$", re.IGNORECASE)
_COLUMN_LABELS = {
    "cav col",
    "cct",
    "cav",
    "col",
}
_TITLE_PATTERN_BY_KIND = {
    "electrical_reference_library": re.compile(r"electrical\s+reference\s+library", re.IGNORECASE),
    "workshop_manual": re.compile(r"workshop\s+manual", re.IGNORECASE),
}


def _argument_value(name: str) -> str:
    try:
        index = sys.argv.index(name)
    except ValueError as exc:
        raise SystemExit(f"missing required argument {name}") from exc
    if index + 1 >= len(sys.argv):
        raise SystemExit(f"missing value for argument {name}")
    return sys.argv[index + 1]


def _early_source_evidence(pdf_path: Path, page_limit: int = 12) -> str:
    parts: list[str] = []
    with fitz.open(pdf_path) as doc:
        for index in range(min(doc.page_count, page_limit)):
            parts.append(doc[index].get_text("text"))
    return "\n".join(parts)


def _document_title_from_evidence(evidence_text: str, document_kind: str) -> str | None:
    """Return only a source title matching the already-selected document family."""
    pattern = _TITLE_PATTERN_BY_KIND.get(document_kind)
    if pattern is None:
        return None
    for raw_line in evidence_text.splitlines():
        line = re.sub(r"\s+", " ", raw_line).strip()
        if line and pattern.search(line):
            return line
    return None


def _rekey_database(db_path: Path, publication_code: str) -> tuple[str, str]:
    """Replace prototype identity keys with publication-derived corpus keys.

    The prototype historically writes RCL0193ENG keys.  The generic layer
    repairs every table carrying document_key or revision_key in one transaction
    with FK checks temporarily disabled, then verifies integrity after re-enable.
    For RCL0193ENG the derived keys are byte-for-byte identical to the historical
    keys, so the validated workshop-manual output remains stable.
    """
    token = canonical_publication_token(publication_code)
    target_document_key = f"DOC_{token}"
    target_revision_key = f"REV_{token}_SOURCE"

    db = sqlite3.connect(db_path)
    db.execute("PRAGMA foreign_keys=OFF")
    old_document_key = db.execute("SELECT document_key FROM ravemems_document").fetchone()[0]
    old_revision_key = db.execute("SELECT revision_key FROM ravemems_document_revision").fetchone()[0]

    table_names = [
        row[0]
        for row in db.execute(
            "SELECT name FROM sqlite_master WHERE type='table' AND name LIKE 'ravemems_%'"
        ).fetchall()
    ]
    for table_name in table_names:
        quoted = '"' + table_name.replace('"', '""') + '"'
        columns = {
            row[1]
            for row in db.execute(f"PRAGMA table_info({quoted})").fetchall()
        }
        if "document_key" in columns and old_document_key != target_document_key:
            db.execute(
                f"UPDATE {quoted} SET document_key=? WHERE document_key=?",
                (target_document_key, old_document_key),
            )
        if "revision_key" in columns and old_revision_key != target_revision_key:
            db.execute(
                f"UPDATE {quoted} SET revision_key=? WHERE revision_key=?",
                (target_revision_key, old_revision_key),
            )
    db.commit()
    db.execute("PRAGMA foreign_keys=ON")
    integrity = db.execute("PRAGMA integrity_check").fetchone()[0]
    foreign_keys = db.execute("PRAGMA foreign_key_check").fetchall()
    db.close()
    if integrity != "ok" or foreign_keys:
        raise RuntimeError(
            f"publication rekey integrity failure: integrity={integrity!r} foreign_keys={foreign_keys!r}"
        )
    return target_document_key, target_revision_key


def _meaningful_reference_line(text: str) -> bool:
    compact = re.sub(r"\s+", " ", text).strip()
    if len(compact) < 3 or len(compact) > 180:
        return False
    if compact.casefold() in _COLUMN_LABELS:
        return False
    if not any(ch.isalpha() for ch in compact):
        return False
    if re.fullmatch(r"MINI\s+\d{2}MY", compact, re.IGNORECASE):
        return False
    return True


def _reference_entry_from_page(page: fitz.Page) -> tuple[str | None, str | None, str | None, str | None]:
    """Return identifier, title, title bbox JSON and header category for a page."""
    lines = pe.read_lines(page)
    if not lines:
        return None, None, None, None

    identifier: str | None = None
    category: str | None = None
    title_item: dict[str, Any] | None = None

    for item in lines:
        text = re.sub(r"\s+", " ", str(item.get("text", ""))).strip()
        if not text:
            continue
        bbox = item.get("bbox", (0, 0, 0, 0))
        top_band = float(bbox[1]) <= float(page.rect.height) * 0.12
        in_header = item.get("reading_region") == "header" or top_band
        if in_header and identifier is None and _REFERENCE_IDENTIFIER.fullmatch(text):
            identifier = text
            continue
        if in_header and category is None and _meaningful_reference_line(text):
            if not _REFERENCE_IDENTIFIER.fullmatch(text):
                category = text

    for item in lines:
        if item.get("reading_region") in {"header", "footer"}:
            continue
        text = re.sub(r"\s+", " ", str(item.get("text", ""))).strip()
        if not _meaningful_reference_line(text):
            continue
        if identifier and text.casefold() == identifier.casefold():
            continue
        title_item = item
        break

    if title_item is not None:
        title = re.sub(r"\s+", " ", str(title_item["text"])).strip()
        bbox_json = pe.bbox_json(title_item["bbox"])
        return identifier, title, bbox_json, category
    if category:
        for item in lines:
            text = re.sub(r"\s+", " ", str(item.get("text", ""))).strip()
            if text == category:
                return identifier, category, pe.bbox_json(item["bbox"]), category
    return identifier, None, None, category


def _build_electrical_reference_sections(
    pdf_path: Path,
    db_path: Path,
    revision_key: str,
    publication_token: str,
    source_language: str,
) -> dict[str, int]:
    """Create page-grounded reference entries without inventing procedures."""
    db = sqlite3.connect(db_path)
    page_keys = {
        int(physical_page): page_key
        for page_key, physical_page in db.execute(
            "SELECT page_key,physical_page FROM ravemems_page ORDER BY physical_page"
        ).fetchall()
    }
    section_count = 0
    identified_count = 0
    titled_count = 0
    page_coverage_count = 0

    with fitz.open(pdf_path) as doc:
        for physical_page in range(1, doc.page_count + 1):
            page_key = page_keys.get(physical_page)
            if not page_key:
                continue
            identifier, title, title_bbox, category = _reference_entry_from_page(doc[physical_page - 1])
            source_title = title or category
            if not source_title:
                continue

            section_count += 1
            page_coverage_count += 1
            if identifier:
                identified_count += 1
            if title:
                titled_count += 1
            section_key = f"SEC_{publication_token}_P{physical_page:04d}"
            db.execute(
                "INSERT INTO ravemems_section("
                "section_key,revision_key,parent_section_key,sequence_no,section_kind,"
                "manufacturer_identifier,title_source,source_language"
                ") VALUES(?,?,?,?,?,?,?,?)",
                (
                    section_key,
                    revision_key,
                    None,
                    physical_page,
                    "electrical_reference_entry",
                    identifier,
                    source_title,
                    source_language,
                ),
            )
            db.execute(
                "INSERT INTO ravemems_provenance("
                "provenance_key,entity_kind,entity_key,revision_key,page_key,"
                "source_element_key,source_bbox_json,source_text_sha256"
                ") VALUES(?,?,?,?,?,?,?,?)",
                (
                    f"PROV_{section_key}",
                    "section",
                    section_key,
                    revision_key,
                    page_key,
                    f"page:{physical_page}:reference-entry",
                    title_bbox,
                    pe.sha256_bytes(source_title.encode("utf-8")),
                ),
            )
    db.commit()
    integrity = db.execute("PRAGMA integrity_check").fetchone()[0]
    foreign_keys = db.execute("PRAGMA foreign_key_check").fetchall()
    db.close()
    if integrity != "ok" or foreign_keys:
        raise RuntimeError(
            f"electrical section integrity failure: integrity={integrity!r} foreign_keys={foreign_keys!r}"
        )
    return {
        "section_count": section_count,
        "page_coverage_count": page_coverage_count,
        "identified_entry_count": identified_count,
        "titled_entry_count": titled_count,
    }


def main() -> int:
    out = Path(_argument_value("--out"))
    pdf_path = Path(_argument_value("--pdf"))
    source_relative_path = _argument_value("--source-relative-path")
    result = pe.main()
    if result != 0:
        return result

    db_path = out / "ravemems_v2_rcl0193eng.sqlite"
    manifest_path = out / "manifest.json"
    manifest: dict[str, Any] = json.loads(manifest_path.read_text(encoding="utf-8"))
    publication_code = str(manifest["publication_code"])
    publication_token = canonical_publication_token(publication_code)
    document_key, revision_key = _rekey_database(db_path, publication_code)

    evidence_text = _early_source_evidence(pdf_path)
    document_kind = document_kind_for_evidence(source_relative_path, evidence_text)
    document_title = _document_title_from_evidence(evidence_text, document_kind)

    db = sqlite3.connect(db_path)
    db.execute(
        "UPDATE ravemems_document SET document_kind=?,title_source=?",
        (document_kind, document_title or publication_code),
    )
    db.commit()
    source_language = db.execute("SELECT source_language FROM ravemems_document").fetchone()[0]
    db.close()

    reference_structure: dict[str, int] | None = None
    if document_kind == "electrical_reference_library":
        reference_structure = _build_electrical_reference_sections(
            pdf_path,
            db_path,
            revision_key,
            publication_token,
            source_language,
        )

    db = sqlite3.connect(db_path)
    manifest["document_key"] = document_key
    manifest["revision_key"] = revision_key
    manifest["document_kind"] = document_kind
    manifest["document_title_source"] = document_title or publication_code
    if reference_structure is not None:
        manifest["reference_structure"] = reference_structure
    manifest.setdefault("counts", {})["ravemems_section"] = db.execute(
        "SELECT COUNT(*) FROM ravemems_section"
    ).fetchone()[0]
    manifest["counts"]["ravemems_provenance"] = db.execute(
        "SELECT COUNT(*) FROM ravemems_provenance"
    ).fetchone()[0]
    manifest["sqlite_integrity"] = db.execute("PRAGMA integrity_check").fetchone()[0]
    manifest["foreign_key_issue_count"] = len(db.execute("PRAGMA foreign_key_check").fetchall())
    db.close()

    manifest_path.write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    print("RAVEMEMS_DOCUMENT_KEY", document_key)
    print("RAVEMEMS_REVISION_KEY", revision_key)
    print("RAVEMEMS_DOCUMENT_KIND", document_kind)
    if reference_structure is not None:
        print("RAVEMEMS_REFERENCE_STRUCTURE", json.dumps(reference_structure, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
