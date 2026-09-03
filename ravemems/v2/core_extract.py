#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sqlite3
from collections import defaultdict
from pathlib import Path
from typing import Any

import fitz

from audit import audit_database
from reading_order import geometric_reading_order


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def bbox_json(rect: fitz.Rect | tuple[float, float, float, float]) -> str:
    values = tuple(rect) if isinstance(rect, fitz.Rect) else rect
    return json.dumps([round(float(v), 3) for v in values], separators=(",", ":"))


def clean_label(text: str) -> str:
    return re.sub(r"\s+", " ", text.strip().strip(":.-")).casefold()


def read_lines(page: fitz.Page) -> list[dict[str, Any]]:
    """Read text lines while preserving the PDF span geometry.

    RAVEMEMS must not infer a numbered workshop step from plain text alone when
    the PDF layout can tell us whether the number is a dedicated step marker.
    The old prototype concatenated all spans without spacing, which destroyed
    that distinction. This reader keeps the spans and reconstructs readable
    text only where the PDF has a real horizontal separation.
    """
    data = page.get_text("dict", sort=True)
    lines: list[dict[str, Any]] = []
    for block in data.get("blocks", []):
        if block.get("type") != 0:
            continue
        for line in block.get("lines", []):
            span_items: list[dict[str, Any]] = []
            for span in line.get("spans", []):
                raw_text = str(span.get("text", ""))
                if not raw_text.strip():
                    continue
                span_items.append(
                    {
                        "text": raw_text,
                        "bbox": tuple(float(v) for v in span.get("bbox", (0, 0, 0, 0))),
                        "size": float(span.get("size", 0.0)),
                        "font": str(span.get("font", "")),
                    }
                )
            if not span_items:
                continue

            rebuilt = ""
            previous: dict[str, Any] | None = None
            for span in span_items:
                raw_text = span["text"]
                if previous is not None:
                    gap = float(span["bbox"][0]) - float(previous["bbox"][2])
                    needs_space = (
                        not rebuilt.endswith((" ", "\t", "\n"))
                        and not raw_text.startswith((" ", "\t", "\n"))
                        and gap > 0.65
                    )
                    if needs_space:
                        rebuilt += " "
                rebuilt += raw_text
                previous = span

            text = re.sub(r"\s+", " ", rebuilt).strip()
            if not text:
                continue
            sizes = [float(span["size"]) for span in span_items]
            fonts = [str(span["font"]) for span in span_items]
            lines.append(
                {
                    "text": text,
                    "bbox": tuple(float(v) for v in line.get("bbox", (0, 0, 0, 0))),
                    "max_size": max(sizes) if sizes else 0.0,
                    "bold": any("bold" in font.casefold() for font in fonts),
                    "spans": span_items,
                }
            )
    return geometric_reading_order(lines, float(page.rect.width), float(page.rect.height))


class SemanticParser:
    def __init__(self, db: sqlite3.Connection, revision_key: str, source_language: str, profile: dict[str, Any]) -> None:
        self.db = db
        self.revision_key = revision_key
        self.source_language = source_language
        self.profile = profile
        self.operation_patterns = [re.compile(x) for x in profile["operation_no_regexes"]]
        self.explicit_step_pattern = re.compile(r"^\s*(\d{1,3})\s*[.)]\s*(.*\S)\s*$")
        self.numeric_candidate_pattern = re.compile(r"^\s*(\d{1,3})(?:\s|[.)])")
        self.ignore_patterns = [re.compile(x, re.IGNORECASE) for x in profile.get("ignore_line_regexes", [])]
        self.requirement_rules = [
            (rule["requirement_type"], re.compile(rule["regex"]))
            for rule in profile.get("requirement_rules", [])
        ]
        self.crossref_patterns = [re.compile(x) for x in profile.get("cross_reference_regexes", [])]
        self.phase_labels = {clean_label(k): v for k, v in profile.get("phase_labels", {}).items()}
        self.notice_labels = {clean_label(k): v for k, v in profile.get("notice_labels", {}).items()}

        self.current_operation_key: str | None = None
        self.current_operation_code: str | None = None
        self.current_phase_key: str | None = None
        self.current_phase_kind: str | None = None
        self.current_step: dict[str, Any] | None = None

        self.operation_sequence = 0
        self.phase_sequence: defaultdict[str, int] = defaultdict(int)
        self.step_sequence: defaultdict[str, int] = defaultdict(int)
        self.notice_sequence: defaultdict[str, int] = defaultdict(int)
        self.requirement_sequence: defaultdict[str, int] = defaultdict(int)
        self.operation_keys_by_code: defaultdict[str, list[str]] = defaultdict(list)
        self.pending_crossrefs: list[tuple[str, str, str]] = []
        self.review_sequence = 0
        self.rejected_numeric_candidates: defaultdict[str, list[dict[str, Any]]] = defaultdict(list)
        self.require_explicit_structure = False

    def _ignored(self, text: str) -> bool:
        return any(pattern.search(text) for pattern in self.ignore_patterns)

    def _operation_code(self, text: str) -> str | None:
        for pattern in self.operation_patterns:
            match = pattern.search(text)
            if match:
                return match.group(1)
        return None

    def _step_match(self, item: dict[str, Any]) -> tuple[str, str, str] | None:
        """Return (manufacturer number, instruction, evidence kind).

        Explicit `1.` / `1)` markers are accepted from text. A plain `1 text`
        form is accepted only when the PDF encodes the marker as its own span to
        the left of one or more instruction spans. That prevents tables, torque
        values and figure labels from becoming workshop steps just because they
        begin with a number.
        """
        text = item["text"].strip()
        explicit = self.explicit_step_pattern.match(text)
        if explicit and explicit.group(2).strip() and int(explicit.group(1)) >= 1:
            return explicit.group(1), explicit.group(2).strip(), "punctuated_text"

        spans = [span for span in item.get("spans", []) if str(span.get("text", "")).strip()]
        if len(spans) < 2:
            return None
        marker_text = str(spans[0]["text"]).strip()
        marker = re.fullmatch(r"(\d{1,3})(?:[.)])?", marker_text)
        if not marker or int(marker.group(1)) < 1:
            return None

        remainder_parts = [str(span["text"]).strip() for span in spans[1:] if str(span["text"]).strip()]
        instruction = re.sub(r"\s+", " ", " ".join(remainder_parts)).strip()
        if not instruction:
            return None

        marker_box = spans[0]["bbox"]
        text_box = spans[1]["bbox"]
        horizontal_gap = float(text_box[0]) - float(marker_box[2])
        marker_mid_y = (float(marker_box[1]) + float(marker_box[3])) / 2.0
        text_mid_y = (float(text_box[1]) + float(text_box[3])) / 2.0
        vertical_tolerance = max(3.0, float(spans[0].get("size", 0.0)) * 0.55)
        aligned = abs(marker_mid_y - text_mid_y) <= vertical_tolerance
        separated = horizontal_gap >= -0.35
        marker_narrower = (float(marker_box[2]) - float(marker_box[0])) < max(
            34.0, (float(text_box[2]) - float(text_box[0])) * 0.65
        )
        if aligned and separated and marker_narrower:
            return marker.group(1), instruction, "dedicated_number_span"
        return None

    def _is_step_line(self, item: dict[str, Any]) -> bool:
        return self._step_match(item) is not None

    def _phase_heading_kind(self, item: dict[str, Any], page_width: float) -> str | None:
        """Recognize a semantic phase only at a real column heading margin.

        Wrapped instruction fragments such as ``assembly.`` or ``remove.`` can
        equal a known phase label textually, but they are indented with the
        instruction body. Genuine workshop phase headings in this manual sit at
        the leading margin of the left or right text column.
        """
        if item.get("reading_region") in {"header", "footer"}:
            return None
        normalized = self.phase_labels.get(clean_label(item["text"]))
        if not normalized:
            return None
        x0 = float(item["bbox"][0])
        midpoint = page_width / 2.0
        if x0 < midpoint:
            aligned = x0 <= page_width * 0.10
        else:
            aligned = x0 <= page_width * 0.56
        return normalized if aligned else None

    def _phase_last_numeric_step(self) -> tuple[int, int] | None:
        if not self.current_phase_key:
            return None
        row = self.db.execute(
            "SELECT manufacturer_step_no,source_page_end FROM ravemems_step "
            "WHERE phase_key=? ORDER BY sequence_no DESC LIMIT 1",
            (self.current_phase_key,),
        ).fetchone()
        if not row:
            return None
        value = str(row[0] or "").strip()
        if not value.isdigit() or int(value) < 1:
            return None
        return int(value), int(row[1])

    def _first_page_step_number(self, lines: list[dict[str, Any]]) -> int | None:
        for item in lines:
            if item.get("reading_region") in {"header", "footer"}:
                continue
            matched = self._step_match(item)
            if matched:
                return int(matched[0])
        return None

    def _guard_page_continuity(self, physical_page: int, lines: list[dict[str, Any]]) -> None:
        """Close a stale phase when a later page restarts numbering at 1.

        A genuine multi-page continuation advances the manufacturer numbering.
        A fresh 1 on a later page, without a new parsed heading yet, is a strong
        structural boundary. Once observed we require explicit structure before
        accepting another implicit numbered procedure, so overview/component
        lists on subsequent pages cannot leak into the previous operation.
        """
        state = self._phase_last_numeric_step()
        if not state:
            return
        _last_number, last_page = state
        if physical_page <= last_page:
            return
        first_number = self._first_page_step_number(lines)
        if first_number != 1:
            return
        self._flush_step()
        self.current_phase_key = None
        self.current_phase_kind = None
        self.require_explicit_structure = True

    def _phase_sequence_analysis(self) -> list[dict[str, Any]]:
        """Validate manufacturer numbering across semantic phase boundaries.

        Numbering may restart at 1 for a new manufacturer sequence, or continue
        across a real semantic phase boundary when the next phase starts exactly
        at the previous phase end + 1. Any gap, duplicate or reordering remains
        a real defect.
        """
        result: list[dict[str, Any]] = []
        operations = self.db.execute(
            "SELECT operation_key FROM ravemems_operation ORDER BY sequence_no"
        ).fetchall()
        for (operation_key,) in operations:
            previous_end: int | None = None
            phases = self.db.execute(
                "SELECT phase_key FROM ravemems_phase WHERE operation_key=? ORDER BY sequence_no",
                (operation_key,),
            ).fetchall()
            for (phase_key,) in phases:
                rows = self.db.execute(
                    "SELECT manufacturer_step_no FROM ravemems_step WHERE phase_key=? ORDER BY sequence_no",
                    (phase_key,),
                ).fetchall()
                if not rows:
                    continue
                values = [str(row[0] or "").strip() for row in rows]
                if not all(value.isdigit() and int(value) >= 1 for value in values):
                    continue
                numbers = [int(value) for value in values]
                start = numbers[0]
                contiguous = numbers == list(range(start, start + len(numbers)))
                start_valid = start == 1 or (previous_end is not None and start == previous_end + 1)
                valid = contiguous and start_valid
                if start == 1:
                    expected = list(range(1, max(numbers) + 1))
                elif previous_end is not None:
                    expected = list(range(previous_end + 1, max(numbers) + 1))
                else:
                    expected = list(range(1, max(numbers) + 1))
                result.append(
                    {
                        "operation_key": operation_key,
                        "phase_key": phase_key,
                        "numbers": numbers,
                        "expected": expected,
                        "valid": valid,
                    }
                )
                previous_end = numbers[-1] if valid else None
        return result

    def _record_rejected_numeric_candidate(self, physical_page: int, item: dict[str, Any]) -> None:
        if not self.current_phase_key:
            return
        text = item["text"].strip()
        if not self.numeric_candidate_pattern.match(text):
            return
        spans = [
            {
                "text": str(span.get("text", "")).strip(),
                "bbox": [round(float(v), 3) for v in span.get("bbox", (0, 0, 0, 0))],
            }
            for span in item.get("spans", [])
            if str(span.get("text", "")).strip()
        ]
        self.rejected_numeric_candidates[self.current_phase_key].append(
            {
                "physical_page": physical_page,
                "operation_key": self.current_operation_key,
                "operation_code": self.current_operation_code,
                "phase_key": self.current_phase_key,
                "phase_kind": self.current_phase_kind,
                "text": text,
                "bbox": [round(float(v), 3) for v in item["bbox"]],
                "spans": spans,
            }
        )

    def _title_candidate(self, lines: list[dict[str, Any]], index: int) -> str:
        def plausible(item: dict[str, Any]) -> bool:
            text = item["text"].strip()
            if len(text) < 3 or self._ignored(text):
                return False
            if self._operation_code(text) or self._is_step_line(item):
                return False
            if clean_label(text) in self.phase_labels:
                return False
            if re.fullmatch(r"\d{1,2}[./-]\d{1,2}[./-]\d{2,4}", text):
                return False
            return True

        before: list[dict[str, Any]] = []
        for candidate in reversed(lines[max(0, index - 8): index]):
            if plausible(candidate):
                before.append(candidate)
                if len(before) == 2:
                    break
        if before:
            before.reverse()
            if len(before) == 2:
                a, b = before
                close = abs(float(b["bbox"][1]) - float(a["bbox"][3])) < 24.0
                emphasized = bool(a["bold"] or b["bold"] or a["text"].isupper() or b["text"].isupper())
                if close and emphasized:
                    return f"{a['text']} {b['text']}".strip()
            return before[-1]["text"].strip()
        for candidate in lines[index + 1: index + 5]:
            if plausible(candidate):
                return candidate["text"].strip()
        return ""

    def _flush_step(self) -> None:
        if not self.current_step:
            return
        text = re.sub(r"\s+", " ", " ".join(self.current_step["parts"])).strip()
        if not text:
            text = self.current_step["manufacturer_step_no"]
        self.db.execute(
            "UPDATE ravemems_step SET instruction_source=?,source_page_end=? WHERE step_key=?",
            (text, self.current_step["last_page"], self.current_step["step_key"]),
        )
        self.current_step = None

    def _add_provenance(self, entity_kind: str, entity_key: str, page_key: str, bbox: tuple[float, float, float, float]) -> None:
        prov_key = f"PROV_{entity_kind}_{entity_key}_{page_key}"
        self.db.execute(
            "INSERT OR IGNORE INTO ravemems_provenance(provenance_key,entity_kind,entity_key,revision_key,page_key,source_bbox_json) VALUES(?,?,?,?,?,?)",
            (prov_key, entity_kind, entity_key, self.revision_key, page_key, bbox_json(bbox)),
        )

    def _new_operation(self, code: str, title: str, page_key: str, bbox: tuple[float, float, float, float]) -> str:
        self._flush_step()
        self.operation_sequence += 1
        safe = re.sub(r"[^A-Za-z0-9]+", "_", code).strip("_")
        operation_key = f"OP_{self.operation_sequence:04d}_{safe}"
        source_title = title.strip() or code
        self.db.execute(
            "INSERT INTO ravemems_operation(operation_key,revision_key,sequence_no,manufacturer_operation_no,operation_kind,title_source,source_language,completeness_status) VALUES(?,?,?,?,?,?,?,?)",
            (operation_key, self.revision_key, self.operation_sequence, code, "manufacturer_operation", source_title, self.source_language, "complete"),
        )
        self.operation_keys_by_code[code].append(operation_key)
        self.current_operation_key = operation_key
        self.current_operation_code = code
        self.current_phase_key = None
        self.current_phase_kind = None
        self.require_explicit_structure = False
        self._add_provenance("operation", operation_key, page_key, bbox)
        return operation_key

    def _new_phase(self, source_label: str, normalized: str, page_key: str, bbox: tuple[float, float, float, float]) -> str | None:
        if not self.current_operation_key:
            return None
        self._flush_step()
        if self.current_phase_key and self.current_phase_kind == normalized:
            return self.current_phase_key
        self.phase_sequence[self.current_operation_key] += 1
        seq = self.phase_sequence[self.current_operation_key]
        phase_key = f"{self.current_operation_key}_PH{seq:02d}"
        self.db.execute(
            "INSERT INTO ravemems_phase(phase_key,operation_key,sequence_no,phase_kind_source,normalized_phase_kind,title_source,completeness_status) VALUES(?,?,?,?,?,?,?)",
            (phase_key, self.current_operation_key, seq, source_label, normalized, source_label, "complete"),
        )
        self.current_phase_key = phase_key
        self.current_phase_kind = normalized
        self.require_explicit_structure = False
        self._add_provenance("phase", phase_key, page_key, bbox)
        return phase_key

    def _new_step(self, manufacturer_no: str, instruction: str, physical_page: int, page_key: str, bbox: tuple[float, float, float, float]) -> str | None:
        if not self.current_phase_key or not self.current_operation_key:
            return None
        self._flush_step()
        self.step_sequence[self.current_phase_key] += 1
        seq = self.step_sequence[self.current_phase_key]
        step_key = f"{self.current_phase_key}_S{seq:03d}"
        self.db.execute(
            "INSERT INTO ravemems_step(step_key,phase_key,sequence_no,manufacturer_step_no,instruction_source,completeness_status,source_page_start,source_page_end) VALUES(?,?,?,?,?,?,?,?)",
            (step_key, self.current_phase_key, seq, manufacturer_no, instruction.strip() or manufacturer_no, "complete", physical_page, physical_page),
        )
        self.current_step = {
            "step_key": step_key,
            "manufacturer_step_no": manufacturer_no,
            "parts": [instruction.strip()] if instruction.strip() else [],
            "last_page": physical_page,
        }
        self._add_provenance("step", step_key, page_key, bbox)
        return step_key

    def _add_notice(self, notice_kind: str, source_text: str, page_key: str, bbox: tuple[float, float, float, float]) -> None:
        if not self.current_operation_key:
            return
        self.notice_sequence[self.current_operation_key] += 1
        seq = self.notice_sequence[self.current_operation_key]
        notice_key = f"{self.current_operation_key}_N{seq:03d}"
        scope_kind = "operation"
        target_operation = self.current_operation_key
        target_phase = None
        target_step = None
        if self.current_step:
            scope_kind = "step"
            target_operation = None
            target_step = self.current_step["step_key"]
        elif self.current_phase_key:
            scope_kind = "phase"
            target_operation = None
            target_phase = self.current_phase_key
        self.db.execute(
            "INSERT INTO ravemems_notice(notice_key,operation_key,sequence_no,notice_kind,source_text,scope_kind,target_operation_key,target_phase_key,target_step_key) VALUES(?,?,?,?,?,?,?,?,?)",
            (notice_key, self.current_operation_key, seq, notice_kind, source_text, scope_kind, target_operation, target_phase, target_step),
        )
        self._add_provenance("notice", notice_key, page_key, bbox)

    def _add_requirement(self, requirement_type: str, source_text: str) -> None:
        if not self.current_operation_key:
            return
        self.requirement_sequence[self.current_operation_key] += 1
        seq = self.requirement_sequence[self.current_operation_key]
        key = f"{self.current_operation_key}_REQ{seq:03d}"
        phase_key = self.current_phase_key
        step_key = self.current_step["step_key"] if self.current_step else None
        before_start = 1 if phase_key and self.step_sequence[phase_key] == 0 else 0
        self.db.execute(
            "INSERT INTO ravemems_requirement(requirement_key,operation_key,phase_key,step_key,sequence_no,requirement_type,requirement_source,before_start) VALUES(?,?,?,?,?,?,?,?)",
            (key, self.current_operation_key, phase_key, step_key, seq, requirement_type, source_text, before_start),
        )

    def parse_page(
        self,
        physical_page: int,
        page_key: str,
        lines: list[dict[str, Any]],
        page_width: float,
        page_height: float,
    ) -> tuple[set[str], set[str]]:
        page_operations: set[str] = set()
        page_phases: set[str] = set()
        self._guard_page_continuity(physical_page, lines)

        for index, item in enumerate(lines):
            text = item["text"].strip()
            if not text:
                continue

            code = self._operation_code(text)
            if code:
                title = self._title_candidate(lines, index)
                if self.current_operation_code != code or not self.current_operation_key:
                    self._new_operation(code, title, page_key, item["bbox"])
                if self.current_operation_key:
                    page_operations.add(self.current_operation_key)
                continue

            normalized_phase = self._phase_heading_kind(item, page_width)
            if normalized_phase:
                phase_key = self._new_phase(text.strip(), normalized_phase, page_key, item["bbox"])
                if phase_key:
                    page_phases.add(phase_key)
                    if self.current_operation_key:
                        page_operations.add(self.current_operation_key)
                continue

            notice_match = re.match(r"^\s*([A-Za-z]+)\s*[:.-]?\s*(.*)$", text)
            if notice_match:
                notice_kind = self.notice_labels.get(clean_label(notice_match.group(1)))
                if notice_kind:
                    self._flush_step()
                    body = notice_match.group(2).strip()
                    self._add_notice(notice_kind, text if body else notice_match.group(1), page_key, item["bbox"])
                    continue

            candidate = self._step_match(item)
            if (
                candidate
                and not self.current_phase_key
                and self.current_operation_key
                and candidate[0] == "1"
                and not self.require_explicit_structure
            ):
                implicit = self._new_phase(
                    "Implicit numbered procedure", "procedure", page_key, item["bbox"]
                )
                if implicit:
                    page_phases.add(implicit)
                    page_operations.add(self.current_operation_key)

            step_match = candidate if self.current_phase_key else None
            if step_match:
                manufacturer_no, instruction, _evidence = step_match
                step_key = self._new_step(manufacturer_no, instruction, physical_page, page_key, item["bbox"])
                if step_key:
                    page_phases.add(self.current_phase_key)
                    if self.current_operation_key:
                        page_operations.add(self.current_operation_key)
            elif self.current_phase_key:
                top = float(item["bbox"][1])
                bottom = float(item["bbox"][3])
                if item.get("reading_region") not in {"header", "footer"} and top >= page_height * 0.055 and bottom <= page_height * 0.955 and not self._ignored(text):
                    self._record_rejected_numeric_candidate(physical_page, item)

            for requirement_type, pattern in self.requirement_rules:
                if pattern.search(text):
                    self._add_requirement(requirement_type, text)

            if self.current_operation_key:
                for pattern in self.crossref_patterns:
                    match = pattern.search(text)
                    if match:
                        self.pending_crossrefs.append((self.current_operation_key, match.group(1), text))

            if self.current_step and step_match is None:
                top = float(item["bbox"][1])
                bottom = float(item["bbox"][3])
                if item.get("reading_region") not in {"header", "footer"} and top >= page_height * 0.055 and bottom <= page_height * 0.955 and not self._ignored(text):
                    self.current_step["parts"].append(text)
                    self.current_step["last_page"] = physical_page
                    if self.current_operation_key:
                        page_operations.add(self.current_operation_key)
                    if self.current_phase_key:
                        page_phases.add(self.current_phase_key)

        self._flush_step()
        return page_operations, page_phases

    def _add_review(self, entity_kind: str, entity_key: str, code: str, text: str, blocking: bool) -> None:
        self.review_sequence += 1
        review_key = f"REVIEW_{self.review_sequence:05d}"
        self.db.execute(
            "INSERT INTO ravemems_review_flag(review_key,entity_kind,entity_key,reason_code,reason_text,severity,status,created_by) VALUES(?,?,?,?,?,?,?,?)",
            (review_key, entity_kind, entity_key, code, text, "blocking" if blocking else "review", "open", "core_extract"),
        )

    def finalize(self) -> None:
        self._flush_step()
        for analysis in self._phase_sequence_analysis():
            if analysis["valid"]:
                continue
            phase_key = analysis["phase_key"]
            operation_key = analysis["operation_key"]
            self.db.execute(
                "UPDATE ravemems_phase SET completeness_status='incomplete' WHERE phase_key=?",
                (phase_key,),
            )
            self.db.execute(
                "UPDATE ravemems_operation SET completeness_status='incomplete' WHERE operation_key=?",
                (operation_key,),
            )
            self._add_review(
                "phase",
                phase_key,
                "manufacturer_step_sequence_incomplete",
                f"Observed manufacturer steps {analysis['numbers']}; expected {analysis['expected']}",
                True,
            )

        seen: set[tuple[str, str, str]] = set()
        relation_sequence: defaultdict[str, int] = defaultdict(int)
        for source_key, target_code, source_text in self.pending_crossrefs:
            token = (source_key, target_code, source_text)
            if token in seen:
                continue
            seen.add(token)
            targets = self.operation_keys_by_code.get(target_code, [])
            if len(targets) == 1 and targets[0] != source_key:
                relation_sequence[source_key] += 1
                seq = relation_sequence[source_key]
                self.db.execute(
                    "INSERT INTO ravemems_operation_relation(relation_key,source_operation_key,target_operation_key,relation_type,source_text,sequence_no,verification_status) VALUES(?,?,?,?,?,?,?)",
                    (f"{source_key}_REL{seq:03d}", source_key, targets[0], "manufacturer_cross_reference", source_text, seq, "verified"),
                )
            elif targets and targets[0] == source_key:
                continue
            else:
                self._add_review(
                    "operation",
                    source_key,
                    "unresolved_cross_reference",
                    f"Target manufacturer operation {target_code} could not be uniquely resolved: {source_text}",
                    False,
                )

    def sequence_diagnostics(self) -> list[dict[str, Any]]:
        diagnostics: list[dict[str, Any]] = []
        metadata = {
            row[0]: row[1:]
            for row in self.db.execute(
                "SELECT p.phase_key,p.normalized_phase_kind,p.completeness_status,"
                "o.operation_key,o.manufacturer_operation_no,o.title_source "
                "FROM ravemems_phase p JOIN ravemems_operation o ON o.operation_key=p.operation_key "
                "ORDER BY o.sequence_no,p.sequence_no"
            ).fetchall()
        }
        for analysis in self._phase_sequence_analysis():
            if analysis["valid"]:
                continue
            phase_key = analysis["phase_key"]
            phase_kind, status, operation_key, operation_no, title = metadata[phase_key]
            steps = self.db.execute(
                "SELECT manufacturer_step_no,source_page_start,source_page_end,instruction_source "
                "FROM ravemems_step WHERE phase_key=? ORDER BY sequence_no",
                (phase_key,),
            ).fetchall()
            diagnostics.append(
                {
                    "operation_key": operation_key,
                    "manufacturer_operation_no": operation_no,
                    "title_source": title,
                    "phase_key": phase_key,
                    "phase_kind": phase_kind,
                    "completeness_status": status,
                    "observed_numbers": analysis["numbers"],
                    "expected_numbers": analysis["expected"],
                    "steps": [
                        {
                            "manufacturer_step_no": step[0],
                            "source_page_start": step[1],
                            "source_page_end": step[2],
                            "instruction_source": step[3],
                        }
                        for step in steps
                    ],
                    "rejected_numeric_candidates": self.rejected_numeric_candidates.get(phase_key, []),
                }
            )
        return diagnostics


def classify_visual(page_text: str, profile: dict[str, Any]) -> str:
    folded = re.sub(r"\s+", " ", page_text.casefold())
    for rule in profile.get("visual_type_rules", []):
        if any(keyword.casefold() in folded for keyword in rule.get("keywords", [])):
            return rule["visual_type"]
    return profile.get("default_visual_type", "technical_illustration")


def expanded_clip(rect: fitz.Rect, page_rect: fitz.Rect, margin: float = 24.0) -> fitz.Rect:
    clip = fitz.Rect(rect.x0 - margin, rect.y0 - margin, rect.x1 + margin, rect.y1 + margin * 1.35)
    clip &= page_rect
    return clip


def visual_candidate_rects(page: fitz.Page) -> list[fitz.Rect]:
    page_area = max(1.0, float(page.rect.width * page.rect.height))
    candidates: list[fitz.Rect] = []
    seen: set[tuple[int, int, int, int]] = set()

    for image in page.get_images(full=True):
        xref = int(image[0])
        if xref <= 0:
            continue
        try:
            rects = page.get_image_rects(xref)
        except Exception:
            rects = []
        for raw in rects:
            rect = fitz.Rect(raw) & page.rect
            if rect.is_empty or rect.width < 18 or rect.height < 18:
                continue
            if float(rect.width * rect.height) / page_area < 0.003:
                continue
            key = tuple(int(round(v * 2.0)) for v in rect)
            if key in seen:
                continue
            seen.add(key)
            candidates.append(rect)

    if not candidates:
        try:
            drawings = page.get_drawings()
        except Exception:
            drawings = []
        rects: list[fitz.Rect] = []
        for drawing in drawings:
            try:
                rect = fitz.Rect(drawing.get("rect")) & page.rect
            except Exception:
                continue
            if rect.is_empty or rect.width < 2 or rect.height < 2:
                continue
            rects.append(rect)
        if len(rects) >= 4:
            union = fitz.Rect(rects[0])
            for rect in rects[1:]:
                union |= rect
            if float(union.width * union.height) / page_area >= 0.01:
                candidates.append(union)
    return candidates


def render_visuals(
    db: sqlite3.Connection,
    page: fitz.Page,
    revision_key: str,
    page_key: str,
    physical_page: int,
    page_text: str,
    page_operations: set[str],
    page_phases: set[str],
    assets_dir: Path,
    profile: dict[str, Any],
) -> int:
    visual_type = classify_visual(page_text, profile)
    rendered = 0
    for ordinal, source_rect in enumerate(visual_candidate_rects(page), start=1):
        clip = expanded_clip(source_rect, page.rect)
        if clip.is_empty or clip.width < 20 or clip.height < 20:
            continue
        pix = page.get_pixmap(matrix=fitz.Matrix(1.5, 1.5), clip=clip, alpha=False)
        visual_key = f"VIS_P{physical_page:04d}_{ordinal:03d}"
        relative_path = f"assets/{visual_key}.png"
        target = assets_dir / f"{visual_key}.png"
        pix.save(str(target))
        blob = target.read_bytes()
        db.execute(
            "INSERT INTO ravemems_visual(visual_key,revision_key,page_key,visual_type,relative_path,sha256,width,height,render_method,source_bbox_json,crop_bbox_json,caption_source,source_language,fidelity_status) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
            (
                visual_key,
                revision_key,
                page_key,
                visual_type,
                relative_path,
                sha256_bytes(blob),
                pix.width,
                pix.height,
                "pdf_page_render_crop",
                bbox_json(source_rect),
                bbox_json(clip),
                None,
                profile["language"],
                "pending",
            ),
        )
        rendered += 1
        if len(page_phases) == 1:
            phase_key = next(iter(page_phases))
            db.execute(
                "INSERT INTO ravemems_visual_link(visual_link_key,visual_key,phase_key,relation_type,verification_status) VALUES(?,?,?,?,?)",
                (f"{visual_key}_LINK", visual_key, phase_key, "illustrates", "pending"),
            )
        elif len(page_operations) == 1:
            operation_key = next(iter(page_operations))
            db.execute(
                "INSERT INTO ravemems_visual_link(visual_link_key,visual_key,operation_key,relation_type,verification_status) VALUES(?,?,?,?,?)",
                (f"{visual_key}_LINK", visual_key, operation_key, "illustrates", "pending"),
            )
    return rendered


def table_counts(db: sqlite3.Connection) -> dict[str, int]:
    names = [
        "ravemems_document",
        "ravemems_document_revision",
        "ravemems_page",
        "ravemems_operation",
        "ravemems_phase",
        "ravemems_step",
        "ravemems_notice",
        "ravemems_requirement",
        "ravemems_specification",
        "ravemems_visual",
        "ravemems_visual_link",
        "ravemems_operation_relation",
        "ravemems_translation",
        "ravemems_review_flag",
        "ravemems_provenance",
    ]
    return {name: int(db.execute(f"SELECT COUNT(*) FROM {name}").fetchone()[0]) for name in names}


def semantic_window(db: sqlite3.Connection, first_page: int, last_page: int) -> list[dict[str, Any]]:
    rows = db.execute(
        "SELECT DISTINCT o.operation_key,o.manufacturer_operation_no,o.title_source,"
        "p.phase_key,p.normalized_phase_kind,p.completeness_status "
        "FROM ravemems_step s "
        "JOIN ravemems_phase p ON p.phase_key=s.phase_key "
        "JOIN ravemems_operation o ON o.operation_key=p.operation_key "
        "WHERE s.source_page_start<=? AND s.source_page_end>=? "
        "ORDER BY o.sequence_no,p.sequence_no",
        (last_page, first_page),
    ).fetchall()
    result: list[dict[str, Any]] = []
    for operation_key, operation_no, title, phase_key, phase_kind, status in rows:
        steps = db.execute(
            "SELECT manufacturer_step_no,source_page_start,source_page_end,instruction_source "
            "FROM ravemems_step WHERE phase_key=? AND source_page_start<=? AND source_page_end>=? ORDER BY sequence_no",
            (phase_key, last_page, first_page),
        ).fetchall()
        result.append(
            {
                "operation_key": operation_key,
                "manufacturer_operation_no": operation_no,
                "title_source": title,
                "phase_key": phase_key,
                "phase_kind": phase_kind,
                "completeness_status": status,
                "steps": [
                    {
                        "manufacturer_step_no": step[0],
                        "source_page_start": step[1],
                        "source_page_end": step[2],
                        "instruction_source": step[3],
                    }
                    for step in steps
                ],
            }
        )
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description="RAVEMEMS V2 document extractor")
    parser.add_argument("--pdf", type=Path, required=True)
    parser.add_argument("--schema", type=Path, required=True)
    parser.add_argument("--profile", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--source-relative-path", required=True)
    parser.add_argument("--source-commit", required=True)
    parser.add_argument("--source-blob-sha", required=True)
    args = parser.parse_args()

    profile = json.loads(args.profile.read_text(encoding="utf-8"))
    args.out.mkdir(parents=True, exist_ok=True)
    assets_dir = args.out / "assets"
    assets_dir.mkdir(parents=True, exist_ok=True)
    db_path = args.out / str(profile.get("db_filename") or "ravemems_v2.sqlite")
    if db_path.exists():
        db_path.unlink()

    doc = fitz.open(args.pdf)
    head_text = "\n".join(doc[i].get_text("text", sort=True) for i in range(min(20, doc.page_count)))
    publication_match = re.search(profile["publication_code_regex"], head_text, re.IGNORECASE)
    publication_code = publication_match.group(0).upper() if publication_match else None
    edition_match = re.search(r"(?i)\b(\d+(?:st|nd|rd|th)\s+Edition)\b", head_text)
    edition = edition_match.group(1) if edition_match else None

    db = sqlite3.connect(db_path)
    db.execute("PRAGMA foreign_keys=ON")
    db.executescript(args.schema.read_text(encoding="utf-8"))
    identity_token = str(profile.get("document_identity_token") or publication_code or sha256_file(args.pdf)[:16].upper())
    identity_token = re.sub(r"[^A-Z0-9]+", "", identity_token.upper()) or sha256_file(args.pdf)[:16].upper()
    document_key = str(profile.get("document_key") or f"DOC_{identity_token}")
    revision_key = str(profile.get("revision_key") or f"REV_{identity_token}_SOURCE")
    db.execute(
        "INSERT INTO ravemems_document(document_key,canonical_name,source_language,document_kind,manufacturer,title_source) VALUES(?,?,?,?,?,?)",
        (document_key, publication_code or identity_token, profile["language"], profile.get("initial_document_kind", "document"), profile.get("manufacturer"), publication_code or identity_token),
    )
    db.execute(
        "INSERT INTO ravemems_document_revision(revision_key,document_key,edition_label,source_relative_path,source_blob_sha,source_sha256,source_size,page_count,is_current) VALUES(?,?,?,?,?,?,?,?,?)",
        (
            revision_key,
            document_key,
            edition,
            args.source_relative_path,
            args.source_blob_sha,
            sha256_file(args.pdf),
            args.pdf.stat().st_size,
            doc.page_count,
            1,
        ),
    )

    semantic = SemanticParser(db, revision_key, profile["language"], profile)
    debug_lines: list[str] = []
    visual_count = 0

    for index in range(doc.page_count):
        page = doc[index]
        physical_page = index + 1
        page_key = f"PAGE_{physical_page:04d}"
        text = page.get_text("text", sort=True)
        lines = read_lines(page)
        db.execute(
            "INSERT INTO ravemems_page(page_key,revision_key,physical_page,source_text_sha256,extraction_status) VALUES(?,?,?,?,?)",
            (page_key, revision_key, physical_page, sha256_bytes(text.encode("utf-8")), "complete"),
        )
        page_operations, page_phases = semantic.parse_page(
            physical_page, page_key, lines, float(page.rect.width), float(page.rect.height)
        )
        visual_count += render_visuals(
            db,
            page,
            revision_key,
            page_key,
            physical_page,
            text,
            page_operations,
            page_phases,
            assets_dir,
            profile,
        )
        if 131 <= physical_page <= 136:
            debug_lines.append(f"===== PHYSICAL PAGE {physical_page} =====")
            for item in lines:
                span_text = " | ".join(str(span["text"]).strip() for span in item.get("spans", []))
                debug_lines.append(f"{item['text']}\tREGION={item.get('reading_region')}\tBBOX={bbox_json(item['bbox'])}\tSPANS=[{span_text}]")
            debug_lines.append("")
        if physical_page % 25 == 0:
            db.commit()

    semantic.finalize()
    db.commit()

    sequence_diagnostics = semantic.sequence_diagnostics()
    semantic_131_136 = semantic_window(db, 131, 136)
    audit_issues = audit_database(db)
    counts = table_counts(db)
    integrity = db.execute("PRAGMA integrity_check").fetchone()[0]
    fk = db.execute("PRAGMA foreign_key_check").fetchall()

    manifest = {
        "engine_id": str(profile.get("engine_id") or "ravemems_v2_generic"),
        "source_relative_path": args.source_relative_path,
        "source_commit": args.source_commit,
        "source_blob_sha": args.source_blob_sha,
        "source_sha256": sha256_file(args.pdf),
        "source_size": args.pdf.stat().st_size,
        "publication_code": publication_code,
        "edition": edition,
        "page_count": doc.page_count,
        "profile_id": profile["profile_id"],
        "visual_render_method": "pdf_page_render_crop",
        "raw_extract_image_used_for_user_visual": False,
        "counts": counts,
        "visual_files": visual_count,
        "numeric_phase_defect_count": len(sequence_diagnostics),
        "rejected_numeric_candidate_count": sum(
            len(items) for items in semantic.rejected_numeric_candidates.values()
        ),
        "audit_issue_count": len(audit_issues),
        "sqlite_integrity": integrity,
        "foreign_key_issue_count": len(fk),
    }
    (args.out / "manifest.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    (args.out / "audit.json").write_text(
        json.dumps({"issue_count": len(audit_issues), "issues": audit_issues}, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    (args.out / "step_sequence_diagnostics.json").write_text(
        json.dumps(sequence_diagnostics, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    (args.out / "pages_131_136_semantic.json").write_text(
        json.dumps(semantic_131_136, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    (args.out / "debug_pages_131_136.txt").write_text("\n".join(debug_lines), encoding="utf-8")

    operations = []
    for row in db.execute(
        "SELECT operation_key,manufacturer_operation_no,title_source,completeness_status "
        "FROM ravemems_operation ORDER BY sequence_no"
    ):
        phases = []
        for phase in db.execute(
            "SELECT phase_key,sequence_no,phase_kind_source,normalized_phase_kind,completeness_status "
            "FROM ravemems_phase WHERE operation_key=? ORDER BY sequence_no",
            (row[0],),
        ):
            steps = db.execute(
                "SELECT manufacturer_step_no,source_page_start,source_page_end,instruction_source "
                "FROM ravemems_step WHERE phase_key=? ORDER BY sequence_no",
                (phase[0],),
            ).fetchall()
            phases.append(
                {
                    "phase_key": phase[0],
                    "sequence_no": phase[1],
                    "phase_kind_source": phase[2],
                    "normalized_phase_kind": phase[3],
                    "completeness_status": phase[4],
                    "steps": [
                        {
                            "manufacturer_step_no": step[0],
                            "source_page_start": step[1],
                            "source_page_end": step[2],
                            "instruction_source": step[3],
                        }
                        for step in steps
                    ],
                }
            )
        operations.append(
            {
                "operation_key": row[0],
                "manufacturer_operation_no": row[1],
                "title_source": row[2],
                "completeness_status": row[3],
                "phases": phases,
            }
        )
    (args.out / "operation_summary.json").write_text(
        json.dumps(operations, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )

    db.close()
    doc.close()
    print(json.dumps(manifest, ensure_ascii=False, indent=2))
    print("GENERIC_SEQUENCE_DIAGNOSTICS_BEGIN")
    print(json.dumps(sequence_diagnostics[:15], ensure_ascii=False, indent=2))
    print("GENERIC_SEQUENCE_DIAGNOSTICS_END")
    print("GENERIC_PAGES_131_136_SEMANTIC_BEGIN")
    print(json.dumps(semantic_131_136, ensure_ascii=False, indent=2))
    print("GENERIC_PAGES_131_136_SEMANTIC_END")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
