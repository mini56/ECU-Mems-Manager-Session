from __future__ import annotations
import re
from typing import Any


def _median(values: list[float]) -> float:
    values = sorted(values)
    n = len(values)
    if not n:
        return 0.0
    m = n // 2
    return values[m] if n % 2 else (values[m - 1] + values[m]) / 2.0


def _dedicated_numeric_marker(item: dict[str, Any]) -> bool:
    """Return True when PDF geometry looks like a real numbered list/step line.

    This is deliberately structural: a marker must be a standalone first span
    aligned with instruction text to its right. It is used only to recognize a
    genuine two-column numbered layout; semantic step acceptance remains the
    parser's responsibility.
    """
    spans = [span for span in item.get('spans', []) if str(span.get('text', '')).strip()]
    if len(spans) < 2:
        return False
    marker_text = str(spans[0].get('text', '')).strip()
    if not re.fullmatch(r'\d{1,3}[.)]?', marker_text):
        return False
    marker_box = spans[0].get('bbox', (0, 0, 0, 0))
    text_box = spans[1].get('bbox', (0, 0, 0, 0))
    horizontal_gap = float(text_box[0]) - float(marker_box[2])
    marker_mid_y = (float(marker_box[1]) + float(marker_box[3])) / 2.0
    text_mid_y = (float(text_box[1]) + float(text_box[3])) / 2.0
    vertical_tolerance = max(3.0, float(spans[0].get('size', 0.0)) * 0.55)
    return abs(marker_mid_y - text_mid_y) <= vertical_tolerance and horizontal_gap >= -0.35


def geometric_reading_order(lines: list[dict[str, Any]], page_width: float, page_height: float) -> list[dict[str, Any]]:
    """Read genuine two-column workshop pages left column then right column.

    Full-width body lines must never force an early jump to the opposite column.
    In addition to broad page geometry, standalone numbered markers are now
    strong evidence for a two-column workshop procedure. This catches sparse
    pages where one column contains only a few numbered steps and the old
    four-lines-per-side threshold incorrectly fell back to global y ordering.
    """
    if not lines:
        return []

    by_yx = lambda item: (float(item['bbox'][1]), float(item['bbox'][0]))
    header_limit = page_height * 0.075
    footer_limit = page_height * 0.90
    header, body, footer = [], [], []
    for item in lines:
        top, bottom = float(item['bbox'][1]), float(item['bbox'][3])
        if bottom <= header_limit:
            item['reading_region'] = 'header'
            header.append(item)
        elif top >= footer_limit:
            item['reading_region'] = 'footer'
            footer.append(item)
        else:
            body.append(item)

    header.sort(key=by_yx)
    footer.sort(key=by_yx)

    midpoint = page_width / 2.0
    gutter = max(12.0, page_width * 0.025)
    left, right, span = [], [], []
    for item in body:
        x0, _, x1, _ = [float(v) for v in item['bbox']]
        center = (x0 + x1) / 2.0
        if x0 < midpoint - gutter and x1 > midpoint + gutter:
            item['reading_region'] = 'body_span'
            span.append(item)
        elif center < midpoint:
            item['reading_region'] = 'body_left'
            left.append(item)
        else:
            item['reading_region'] = 'body_right'
            right.append(item)

    two_column = False
    if left and right:
        lt, lb = min(float(x['bbox'][1]) for x in left), max(float(x['bbox'][3]) for x in left)
        rt, rb = min(float(x['bbox'][1]) for x in right), max(float(x['bbox'][3]) for x in right)
        overlap = max(0.0, min(lb, rb) - max(lt, rt))
        lc = _median([(float(x['bbox'][0]) + float(x['bbox'][2])) / 2.0 for x in left])
        rc = _median([(float(x['bbox'][0]) + float(x['bbox'][2])) / 2.0 for x in right])
        separated_columns = lc < page_width * 0.46 and rc > page_width * 0.54
        broad_geometry = len(left) >= 4 and len(right) >= 4 and overlap >= page_height * 0.02
        numbered_left = sum(1 for item in left if _dedicated_numeric_marker(item))
        numbered_right = sum(1 for item in right if _dedicated_numeric_marker(item))
        numbered_geometry = numbered_left >= 2 and numbered_right >= 2
        two_column = separated_columns and (broad_geometry or numbered_geometry)

    if not two_column:
        for item in body:
            item['reading_region'] = 'body_single'
        return header + sorted(body, key=by_yx) + footer

    left.sort(key=by_yx)
    right.sort(key=by_yx)
    span.sort(key=by_yx)
    return header + left + right + span + footer
