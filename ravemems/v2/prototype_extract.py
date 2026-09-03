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
    data = page.get_text("dict", sort=True)
    lines: list[dict[str, Any]] = []
    for block in data.get("blocks", []):
        if block.get("type") != 0:
            continue
        for line in block.get("lines", []):
            spans = line.get("spans", [])
            text = "".join(str(span.get("text", "")) for span in spans).strip()
            if not text:
                continue
            sizes = [float(span.get("size", 0.0)) for span in spans]
            fonts = [str(span.get("font", "")) for span in spans]
            lines.append(
                {
                    "text": text,
                    "bbox": tuple(float(v) for v in line.get("bbox", (0, 0, 0, 0))),
                    "max_size": max(sizes) if sizes else 0.0,
                    "bold": any("bold" in font.casefold() for font in fonts),
                }
            )
    lines.sort(key=lambda item: (item["bbox"][1], item["bbox"][0]))
    return lines


class SemanticParser:
    def __init__(self, db: sqlite3.Connection, revision_key: str, source_language: str, profile: dict[str, Any]) -> None:
        self.db = db
        self.revision_key = revision_key
        self.source_language = source_language
        self.profile = profile
        self.operation_patterns = [re.compile(x) for x in profile["operation_no_regexes"]]
        self.step_pattern = re.compile(r"^\s*(\d{1,3})(?:[.)])\s*(.*)$")
        self.step_fallback_pattern = re.compile(r"^\s*(\d{1,3})\s{2,}(.+)$")
        self.ignore_patterns = [re.compile(x, re.IGNORECASE) for x in profile.get("ignore_line_regexes", [])]
        self.requirement_rules = [(rule["requirement_type"], re.compile(rule["regex"])) for rule in profile.get("requirement_rules", [])]
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

    def _ignored(self, text: str) -> bool:
        return any(pattern.search(text) for pattern in self.ignore_patterns)

    def _operation_code(self, text: str) -> str | None:
        for pattern in self.operation_patterns:
            match = pattern.search(text)
            if match:
                return match.group(1)
        return None

    def _is_step_line(self, text: str) -> bool:
        return bool(self.step_pattern.match(text) or self.step_fallback_pattern.match(text))

    def _title_candidate(self, lines: list[dict[str, Any]], index: int) -> str:
        def plausible(item: dict[str, Any]) -> bool:
            text = item["text"].strip()
            if len(text) < 3 or self._ignored(text):
                return False
            if self._operation_code(text) or self._is_step_line(text):
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
        self.db.execute("UPDATE ravemems_step SET instruction_source=?,source_page_end=? WHERE step_key=?", (text, self.current_step["last_page"], self.current_step["step_key"]))
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
        self.current_step = {"step_key": step_key, "manufacturer_step_no": manufacturer_no, "parts": [instruction.strip()] if instruction.strip() else [], "last_page": physical_page}
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

    def parse_page(self, physical_page: int, page_key: str, lines: list[dict[str, Any]], page_height: float) -> tuple[set[str], set[str]]:
        page_operations: set[str] = set()
        page_phases: set[str] = set()
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
            label = clean_label(text)
            normalized_phase = self.phase_labels.get(label)
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
            if self.current_phase_key:
                match = self.step_pattern.match(text) or self.step_fallback_pattern.match(text)
                if match:
                    step_key = self._new_step(match.group(1), match.group(2), physical_page, page_key, item["bbox"])
                    if step_key:
                        page_phases.add(self.current_phase_key)
                        if self.current_operation_key:
                            page_operations.add(self.current_operation_key)
            for requirement_type, pattern in self.requirement_rules:
                if pattern.search(text):
                    self._add_requirement(requirement_type, text)
            if self.current_operation_key:
                for pattern in self.crossref_patterns:
                    match = pattern.search(text)
                    if match:
                        self.pending_crossrefs.append((self.current_operation_key, match.group(1), text))
            if self.current_step and not self._is_step_line(text):
                top = float(item["bbox"][1])
                bottom = float(item["bbox"][3])
                if top >= page_height * 0.055 and bottom <= page_height * 0.955 and not self._ignored(text):
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
            (review_key, entity_kind, entity_key, code, text, "blocking" if blocking else "review", "open", "prototype_extract"),
        )

    def finalize(self) -> None:
        self._flush_step()
        for phase_key, operation_key in self.db.execute("SELECT phase_key,operation_key FROM ravemems_phase ORDER BY operation_key,sequence_no").fetchall():
            rows = self.db.execute("SELECT manufacturer_step_no FROM ravemems_step WHERE phase_key=? ORDER BY sequence_no", (phase_key,)).fetchall()
            numeric: list[int] = []
            all_numeric = bool(rows)
            for row in rows:
                value = (row[0] or "").strip()
                if not value.isdigit():
                    all_numeric = False
                    break
                numeric.append(int(value))
            if not all_numeric or not numeric:
                continue
            expected = list(range(1, max(numeric) + 1))
            if numeric != expected:
                self.db.execute("UPDATE ravemems_phase SET completeness_status='incomplete' WHERE phase_key=?", (phase_key,))
                self.db.execute("UPDATE ravemems_operation SET completeness_status='incomplete' WHERE operation_key=?", (operation_key,))
                self._add_review("phase", phase_key, "manufacturer_step_sequence_incomplete", f"Observed manufacturer steps {numeric}; expected {expected}", True)
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
                self._add_review("operation", source_key, "unresolved_cross_reference", f"Target manufacturer operation {target_code} could not be uniquely resolved: {source_text}", False)


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


def render_visuals(db: sqlite3.Connection, page: fitz.Page, revision_key: str, page_key: str, physical_page: int, page_text: str, page_operations: set[str], page_phases: set[str], assets_dir: Path, profile: dict[str, Any]) -> int:
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
            (visual_key, revision_key, page_key, visual_type, relative_path, sha256_bytes(blob), pix.width, pix.height, "pdf_page_render_crop", bbox_json(source_rect), bbox_json(clip), None, profile["language"], "pending"),
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
    names = ["ravemems_document", "ravemems_document_revision", "ravemems_page", "ravemems_operation", "ravemems_phase", "ravemems_step", "ravemems_notice", "ravemems_requirement", "ravemems_specification", "ravemems_visual", "ravemems_visual_link", "ravemems_operation_relation", "ravemems_translation", "ravemems_review_flag", "ravemems_provenance"]
    return {name: int(db.execute(f"SELECT COUNT(*) FROM {name}").fetchone()[0]) for name in names}


def main() -> int:
    parser = argparse.ArgumentParser(description="RAVEMEMS V2 RCL0193ENG prototype extractor")
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
    db_path = args.out / "ravemems_v2_rcl0193eng.sqlite"
    if db_path.exists():
        db_path.unlink()
    doc = fitz.open(args.pdf)
    head_text = "\n".join(doc[i].get_text("text", sort=True) for i in range(min(20, doc.page_count)))
    publication_match = re.search(profile["publication_code_regex"], head_text, re.IGNORECASE)
    if not publication_match:
        raise SystemExit("Expected publication code RCL0193ENG not found in source PDF")
    publication_code = publication_match.group(0).upper()
    edition_match = re.search(r"(?i)\b(\d+(?:st|nd|rd|th)\s+Edition)\b", head_text)
    edition = edition_match.group(1) if edition_match else None
    db = sqlite3.connect(db_path)
    db.execute("PRAGMA foreign_keys=ON")
    db.executescript(args.schema.read_text(encoding="utf-8"))
    document_key = "DOC_RCL0193ENG"
    revision_key = "REV_RCL0193ENG_SOURCE"
    db.execute("INSERT INTO ravemems_document(document_key,canonical_name,source_language,document_kind,manufacturer,title_source) VALUES(?,?,?,?,?,?)", (document_key, publication_code, profile["language"], "workshop_manual", "Rover", publication_code))
    db.execute(
        "INSERT INTO ravemems_document_revision(revision_key,document_key,edition_label,source_relative_path,source_blob_sha,source_sha256,source_size,page_count,is_current) VALUES(?,?,?,?,?,?,?,?,?)",
        (revision_key, document_key, edition, args.source_relative_path, args.source_blob_sha, sha256_file(args.pdf), args.pdf.stat().st_size, doc.page_count, 1),
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
        db.execute("INSERT INTO ravemems_page(page_key,revision_key,physical_page,source_text_sha256,extraction_status) VALUES(?,?,?,?,?)", (page_key, revision_key, physical_page, sha256_bytes(text.encode("utf-8")), "complete"))
        page_operations, page_phases = semantic.parse_page(physical_page, page_key, lines, float(page.rect.height))
        visual_count += render_visuals(db, page, revision_key, page_key, physical_page, text, page_operations, page_phases, assets_dir, profile)
        if 131 <= physical_page <= 136:
            debug_lines.append(f"===== PHYSICAL PAGE {physical_page} =====")
            debug_lines.extend(item["text"] for item in lines)
            debug_lines.append("")
        if physical_page % 25 == 0:
            db.commit()
    semantic.finalize()
    db.commit()
    audit_issues = audit_database(db)
    counts = table_counts(db)
    integrity = db.execute("PRAGMA integrity_check").fetchone()[0]
    fk = db.execute("PRAGMA foreign_key_check").fetchall()
    manifest = {
        "prototype": "RCL0193ENG",
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
        "audit_issue_count": len(audit_issues),
        "sqlite_integrity": integrity,
        "foreign_key_issue_count": len(fk),
    }
    (args.out / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    (args.out / "audit.json").write_text(json.dumps({"issue_count": len(audit_issues), "issues": audit_issues}, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    (args.out / "debug_pages_131_136.txt").write_text("\n".join(debug_lines), encoding="utf-8")
    operations = []
    for row in db.execute("SELECT operation_key,manufacturer_operation_no,title_source,completeness_status FROM ravemems_operation ORDER BY sequence_no"):
        phases = []
        for phase in db.execute("SELECT phase_key,sequence_no,phase_kind_source,normalized_phase_kind,completeness_status FROM ravemems_phase WHERE operation_key=? ORDER BY sequence_no", (row[0],)):
            steps = db.execute("SELECT manufacturer_step_no,source_page_start,source_page_end,instruction_source FROM ravemems_step WHERE phase_key=? ORDER BY sequence_no", (phase[0],)).fetchall()
            phases.append({"phase_key": phase[0], "sequence_no": phase[1], "phase_kind_source": phase[2], "normalized_phase_kind": phase[3], "completeness_status": phase[4], "steps": [{"manufacturer_step_no": step[0], "source_page_start": step[1], "source_page_end": step[2], "instruction_source": step[3]} for step in steps]})
        operations.append({"operation_key": row[0], "manufacturer_operation_no": row[1], "title_source": row[2], "completeness_status": row[3], "phases": phases})
    (args.out / "operation_summary.json").write_text(json.dumps(operations, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    db.close()
    doc.close()
    print(json.dumps(manifest, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
