#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path
from typing import Any

import fitz

import rcl0193eng_visual_replay_validate as legacy


def replay_visuals(pdf_path: Path, out_dir: Path) -> dict[str, Any]:
    """Run the exact legacy replay and replace only RCL0193-specific metadata.

    Pixel comparison, geometry checks, DB updates and audit validation remain in
    the already-validated replay implementation.  This adapter makes the proof
    metadata truthful for any RAVE source while leaving the RCL0193-specific
    standalone 738/401 success guard untouched.
    """
    legacy.replay_visuals(pdf_path, out_dir)
    evidence_path = out_dir / "visual_replay_validation.json"
    evidence: dict[str, Any] = json.loads(evidence_path.read_text(encoding="utf-8"))
    with fitz.open(pdf_path) as doc:
        page_count = doc.page_count
    evidence["method"] = (
        f"full extraction-order replay page 1..{page_count} with get_text, read_lines, "
        "visual_candidate_rects, expanded_clip, Matrix(1.5,1.5), Pixmap.save"
    )
    evidence_path.write_text(
        json.dumps(evidence, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    return evidence
