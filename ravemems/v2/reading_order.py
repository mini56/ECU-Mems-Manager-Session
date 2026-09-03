from __future__ import annotations
from typing import Any

def _median(values: list[float]) -> float:
    values = sorted(values)
    n = len(values)
    if not n:
        return 0.0
    m = n // 2
    return values[m] if n % 2 else (values[m - 1] + values[m]) / 2.0

def geometric_reading_order(lines: list[dict[str, Any]], page_width: float, page_height: float) -> list[dict[str, Any]]:
    """Read genuine two-column workshop pages left column then right column."""
    if not lines:
        return []
    by_yx = lambda item: (float(item['bbox'][1]), float(item['bbox'][0]))
    header_limit = page_height * 0.075
    footer_limit = page_height * 0.90
    header, body, footer = [], [], []
    for item in lines:
        top, bottom = float(item['bbox'][1]), float(item['bbox'][3])
        if bottom <= header_limit:
            item['reading_region'] = 'header'; header.append(item)
        elif top >= footer_limit:
            item['reading_region'] = 'footer'; footer.append(item)
        else:
            body.append(item)
    header.sort(key=by_yx); footer.sort(key=by_yx)
    midpoint = page_width / 2.0
    gutter = max(12.0, page_width * 0.025)
    left, right, span = [], [], []
    for item in body:
        x0, _, x1, _ = [float(v) for v in item['bbox']]
        center = (x0 + x1) / 2.0
        if x0 < midpoint - gutter and x1 > midpoint + gutter:
            item['reading_region'] = 'body_span'; span.append(item)
        elif center < midpoint:
            item['reading_region'] = 'body_left'; left.append(item)
        else:
            item['reading_region'] = 'body_right'; right.append(item)
    two_column = False
    if len(left) >= 4 and len(right) >= 4:
        lt, lb = min(float(x['bbox'][1]) for x in left), max(float(x['bbox'][3]) for x in left)
        rt, rb = min(float(x['bbox'][1]) for x in right), max(float(x['bbox'][3]) for x in right)
        overlap = max(0.0, min(lb, rb) - max(lt, rt))
        lc = _median([(float(x['bbox'][0]) + float(x['bbox'][2])) / 2.0 for x in left])
        rc = _median([(float(x['bbox'][0]) + float(x['bbox'][2])) / 2.0 for x in right])
        two_column = overlap >= page_height * 0.12 and lc < page_width * 0.46 and rc > page_width * 0.54
    if not two_column:
        for item in body:
            item['reading_region'] = 'body_single'
        return header + sorted(body, key=by_yx) + footer
    left.sort(key=by_yx); right.sort(key=by_yx); span.sort(key=by_yx)
    if not span:
        return header + left + right + footer
    ordered = []
    for separator in span:
        sy = float(separator['bbox'][1])
        a = [x for x in left if float(x['bbox'][1]) < sy]
        b = [x for x in right if float(x['bbox'][1]) < sy]
        ordered += a + b + [separator]
        aset, bset = {id(x) for x in a}, {id(x) for x in b}
        left = [x for x in left if id(x) not in aset]
        right = [x for x in right if id(x) not in bset]
    return header + ordered + left + right + footer
