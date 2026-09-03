#!/usr/bin/env python3
from __future__ import annotations

import re
from typing import Any

import prototype_extract as pe


def _median(values: list[float]) -> float:
    values = sorted(values)
    if not values:
        return 0.0
    mid = len(values) // 2
    return values[mid] if len(values) % 2 else (values[mid - 1] + values[mid]) / 2.0


def _structural_numeric_marker(item: dict[str, Any]) -> bool:
    """Recognize a plausible workshop list marker without accepting decimals."""
    text = str(item.get("text", "")).strip()
    if re.match(r"^[1-9]\d{0,2}(?:\s+|[.)](?!\d)\s*)\S", text):
        return True
    spans = [span for span in item.get("spans", []) if str(span.get("text", "")).strip()]
    if len(spans) < 2:
        return False
    marker_text = str(spans[0].get("text", "")).strip()
    if not re.fullmatch(r"[1-9]\d{0,2}[.)]?", marker_text):
        return False
    marker_box = spans[0].get("bbox", (0, 0, 0, 0))
    text_box = spans[1].get("bbox", (0, 0, 0, 0))
    marker_mid_y = (float(marker_box[1]) + float(marker_box[3])) / 2.0
    text_mid_y = (float(text_box[1]) + float(text_box[3])) / 2.0
    tolerance = max(3.0, float(spans[0].get("size", 0.0)) * 0.55)
    return abs(marker_mid_y - text_mid_y) <= tolerance and float(text_box[0]) - float(marker_box[2]) >= -0.35


def pass2_reading_order(lines: list[dict[str, Any]], page_width: float, page_height: float) -> list[dict[str, Any]]:
    """Column-local reading order for workshop procedures.

    Genuine two-column procedure streams are consumed left then right. Sparse
    columns can still be recognized from their standalone numeric markers.
    """
    if not lines:
        return []

    by_yx = lambda item: (float(item["bbox"][1]), float(item["bbox"][0]))
    header_limit = page_height * 0.070
    footer_limit = page_height * 0.965
    header: list[dict[str, Any]] = []
    body: list[dict[str, Any]] = []
    footer: list[dict[str, Any]] = []
    for item in lines:
        top, bottom = float(item["bbox"][1]), float(item["bbox"][3])
        if bottom <= header_limit:
            item["reading_region"] = "header"
            header.append(item)
        elif top >= footer_limit:
            item["reading_region"] = "footer"
            footer.append(item)
        else:
            body.append(item)

    header.sort(key=by_yx)
    footer.sort(key=by_yx)
    midpoint = page_width / 2.0
    gutter = max(12.0, page_width * 0.025)
    left: list[dict[str, Any]] = []
    right: list[dict[str, Any]] = []
    span: list[dict[str, Any]] = []
    for item in body:
        x0, _, x1, _ = [float(v) for v in item["bbox"]]
        center = (x0 + x1) / 2.0
        if x0 < midpoint - gutter and x1 > midpoint + gutter:
            item["reading_region"] = "body_span"
            span.append(item)
        elif center < midpoint:
            item["reading_region"] = "body_left"
            left.append(item)
        else:
            item["reading_region"] = "body_right"
            right.append(item)

    two_column = False
    if left and right:
        lc = _median([(float(x["bbox"][0]) + float(x["bbox"][2])) / 2.0 for x in left])
        rc = _median([(float(x["bbox"][0]) + float(x["bbox"][2])) / 2.0 for x in right])
        separated = lc < page_width * 0.46 and rc > page_width * 0.54
        if separated:
            lt = min(float(x["bbox"][1]) for x in left)
            lb = max(float(x["bbox"][3]) for x in left)
            rt = min(float(x["bbox"][1]) for x in right)
            rb = max(float(x["bbox"][3]) for x in right)
            overlap = max(0.0, min(lb, rb) - max(lt, rt))
            numbered_left = sum(1 for item in left if _structural_numeric_marker(item))
            numbered_right = sum(1 for item in right if _structural_numeric_marker(item))
            numbered_geometry = numbered_left >= 1 and numbered_right >= 1 and numbered_left + numbered_right >= 3
            broad_geometry = len(left) >= 3 and len(right) >= 3 and overlap >= page_height * 0.01
            two_column = numbered_geometry or broad_geometry

    if not two_column:
        for item in body:
            item["reading_region"] = "body_single"
        return header + sorted(body, key=by_yx) + footer

    left.sort(key=by_yx)
    right.sort(key=by_yx)
    span.sort(key=by_yx)
    return header + left + right + span + footer


class Pass2SemanticParser(pe.SemanticParser):
    def __init__(self, *args: Any, **kwargs: Any) -> None:
        super().__init__(*args, **kwargs)
        self.explicit_step_pattern = re.compile(r"^\s*(\d{1,3})\s*[.)](?!\d)\s*(.*\S)\s*$")
        self.numeric_candidate_pattern = re.compile(r"^\s*([1-9]\d{0,2})(?:\s|[.)](?!\d))")
        self._idle_procedure_pages = 0
        self.boundary_folio_count = 0
        self.boundary_title_count = 0

    def _step_match(self, item: dict[str, Any]) -> tuple[str, str, str] | None:
        match = super()._step_match(item)
        if not match:
            return None
        number, instruction, evidence = match
        if not number.isdigit() or int(number) <= 0:
            return None
        return number, instruction, evidence

    def _clear_active_procedure(self) -> None:
        self._flush_step()
        self.current_operation_key = None
        self.current_operation_code = None
        self.current_phase_key = None
        self.current_phase_kind = None

    @staticmethod
    def _column_margins(lines: list[dict[str, Any]]) -> dict[str, float]:
        margins: dict[str, float] = {}
        for item in lines:
            region = str(item.get("reading_region", ""))
            if region not in {"body_left", "body_right", "body_single"}:
                continue
            x0 = float(item["bbox"][0])
            margins[region] = min(x0, margins.get(region, x0))
        return margins

    def _is_phase_heading(self, item: dict[str, Any], margins: dict[str, float]) -> bool:
        text = str(item.get("text", "")).strip()
        if not text:
            return False
        region = str(item.get("reading_region", ""))
        if region in {"header", "footer", "body_span"}:
            return False
        if item.get("bold"):
            return True
        if text.endswith("."):
            return False
        margin = margins.get(region)
        if margin is None:
            return False
        return float(item["bbox"][0]) <= margin + 10.0

    @staticmethod
    def _is_bottom_folio(item: dict[str, Any], page_height: float) -> bool:
        """Identify the printed page/section folio by text and bottom geometry."""
        text = str(item.get("text", "")).strip()
        if not re.fullmatch(r"\d{1,3}", text):
            return False
        top = float(item["bbox"][1])
        return top >= page_height * 0.94

    def _operation_code_within(self, lines: list[dict[str, Any]], index: int, lookahead: int = 5) -> bool:
        for candidate in lines[index + 1:index + 1 + lookahead]:
            text = str(candidate.get("text", "")).strip()
            if not text:
                continue
            if self._operation_code(text):
                return True
            # A new numbered workshop step means this was ordinary prose, not
            # a pending operation-title block.
            if self._step_match(candidate):
                return False
        return False

    def _is_next_operation_title(self, lines: list[dict[str, Any]], index: int) -> bool:
        """Detect a title line immediately preceding a manufacturer operation id.

        The rule is structural only: emphasized/all-capital text plus a nearby
        operation code. It deliberately knows nothing about component names.
        """
        item = lines[index]
        text = str(item.get("text", "")).strip()
        if not text or self._operation_code(text):
            return False
        letters = [ch for ch in text if ch.isalpha()]
        all_caps = bool(letters) and all(ch.isupper() for ch in letters)
        emphasized = bool(item.get("bold")) or all_caps
        if not emphasized:
            return False
        return self._operation_code_within(lines, index)

    def parse_page(
        self,
        physical_page: int,
        page_key: str,
        lines: list[dict[str, Any]],
        page_width: float,
        page_height: float,
    ) -> tuple[set[str], set[str]]:
        if self._idle_procedure_pages >= 2:
            self._clear_active_procedure()

        page_operations: set[str] = set()
        page_phases: set[str] = set()
        margins = self._column_margins(lines)

        for index, item in enumerate(lines):
            text = item["text"].strip()
            if not text:
                continue

            # A printed folio is never instruction text. Flushing here prevents
            # the last numbered step on a page from acquiring e.g. " 12".
            if self._is_bottom_folio(item, page_height):
                if self.current_step:
                    self._flush_step()
                self.boundary_folio_count += 1
                continue

            # In left-then-right reading order the title of the next procedure
            # can immediately follow the final step of the previous column. A
            # nearby manufacturer operation code proves the boundary.
            if self.current_step and self._is_next_operation_title(lines, index):
                self._flush_step()
                self.boundary_title_count += 1
                continue

            code = self._operation_code(text)
            if code:
                title = self._title_candidate(lines, index)
                if self.current_operation_code != code or not self.current_operation_key:
                    self._new_operation(code, title, page_key, item["bbox"])
                if self.current_operation_key:
                    page_operations.add(self.current_operation_key)
                continue

            label = pe.clean_label(text)
            normalized_phase = self.phase_labels.get(label)
            if normalized_phase and self._is_phase_heading(item, margins):
                phase_key = self._new_phase(text.strip(), normalized_phase, page_key, item["bbox"])
                if phase_key:
                    page_phases.add(phase_key)
                    if self.current_operation_key:
                        page_operations.add(self.current_operation_key)
                continue

            notice_match = re.match(r"^\s*([A-Za-z]+)\s*[:.-]?\s*(.*)$", text)
            if notice_match:
                notice_kind = self.notice_labels.get(pe.clean_label(notice_match.group(1)))
                if notice_kind:
                    self._flush_step()
                    body = notice_match.group(2).strip()
                    self._add_notice(notice_kind, text if body else notice_match.group(1), page_key, item["bbox"])
                    continue

            step_match = self._step_match(item) if self.current_operation_key else None
            if step_match and not self.current_phase_key:
                implicit = self._new_phase("Procedure", "procedure", page_key, item["bbox"])
                if implicit:
                    page_phases.add(implicit)
                    page_operations.add(self.current_operation_key)

            if step_match and self.current_phase_key:
                manufacturer_no, instruction, _evidence = step_match
                step_key = self._new_step(manufacturer_no, instruction, physical_page, page_key, item["bbox"])
                if step_key:
                    page_phases.add(self.current_phase_key)
                    if self.current_operation_key:
                        page_operations.add(self.current_operation_key)
            elif self.current_phase_key:
                top = float(item["bbox"][1])
                bottom = float(item["bbox"][3])
                if item.get("reading_region") not in {"header", "footer"} and top >= page_height * 0.055 and bottom <= page_height * 0.985 and not self._ignored(text):
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
                if item.get("reading_region") not in {"header", "footer"} and top >= page_height * 0.055 and bottom <= page_height * 0.985 and not self._ignored(text):
                    self.current_step["parts"].append(text)
                    self.current_step["last_page"] = physical_page
                    if self.current_operation_key:
                        page_operations.add(self.current_operation_key)
                    if self.current_phase_key:
                        page_phases.add(self.current_phase_key)

        self._flush_step()
        if page_operations or page_phases:
            self._idle_procedure_pages = 0
        else:
            self._idle_procedure_pages += 1
        return page_operations, page_phases

    def finalize(self) -> None:
        super().finalize()

        operations = self.db.execute(
            "SELECT operation_key FROM ravemems_operation ORDER BY sequence_no"
        ).fetchall()
        for (operation_key,) in operations:
            previous_last: int | None = None
            phases = self.db.execute(
                "SELECT phase_key FROM ravemems_phase WHERE operation_key=? ORDER BY sequence_no",
                (operation_key,),
            ).fetchall()
            for (phase_key,) in phases:
                rows = self.db.execute(
                    "SELECT manufacturer_step_no FROM ravemems_step WHERE phase_key=? ORDER BY sequence_no",
                    (phase_key,),
                ).fetchall()
                if not rows or not all(row[0] and str(row[0]).isdigit() for row in rows):
                    previous_last = None
                    continue
                numbers = [int(row[0]) for row in rows]
                contiguous = numbers == list(range(numbers[0], numbers[0] + len(numbers)))
                starts_at_one = contiguous and numbers[0] == 1
                continues_previous = contiguous and previous_last is not None and numbers[0] == previous_last + 1
                if starts_at_one or continues_previous:
                    self.db.execute(
                        "UPDATE ravemems_phase SET completeness_status='complete' WHERE phase_key=?",
                        (phase_key,),
                    )
                    self.db.execute(
                        "DELETE FROM ravemems_review_flag WHERE entity_kind='phase' AND entity_key=? AND reason_code='manufacturer_step_sequence_incomplete'",
                        (phase_key,),
                    )
                    previous_last = numbers[-1]
                else:
                    previous_last = None

            incomplete = self.db.execute(
                "SELECT COUNT(*) FROM ravemems_phase WHERE operation_key=? AND completeness_status='incomplete'",
                (operation_key,),
            ).fetchone()[0]
            if incomplete == 0:
                self.db.execute(
                    "UPDATE ravemems_operation SET completeness_status='complete' WHERE operation_key=?",
                    (operation_key,),
                )

        print("PASS2_BOUNDARY_FOLIOS_IGNORED", self.boundary_folio_count)
        print("PASS2_BOUNDARY_TITLES_FLUSHED", self.boundary_title_count)

    def sequence_diagnostics(self) -> list[dict[str, Any]]:
        diagnostics = super().sequence_diagnostics()
        return [item for item in diagnostics if item.get("completeness_status") == "incomplete"]


pe.geometric_reading_order = pass2_reading_order
pe.SemanticParser = Pass2SemanticParser


if __name__ == "__main__":
    raise SystemExit(pe.main())
