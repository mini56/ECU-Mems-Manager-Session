#!/usr/bin/env python3
from __future__ import annotations

import json
from typing import Any

import generic_semantic_pass2 as pass2

pe = pass2.pe
_original_render_visuals = pe.render_visuals


def _exact_bbox_json(rect: Any) -> str:
    values = tuple(rect) if isinstance(rect, pe.fitz.Rect) else rect
    return json.dumps([float(value) for value in values], separators=(",", ":"))


def _render_visuals_with_exact_geometry(*args: Any, **kwargs: Any) -> int:
    previous_bbox_json = pe.bbox_json
    pe.bbox_json = _exact_bbox_json
    try:
        return _original_render_visuals(*args, **kwargs)
    finally:
        pe.bbox_json = previous_bbox_json


pe.render_visuals = _render_visuals_with_exact_geometry


if __name__ == "__main__":
    raise SystemExit(pe.main())
