from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
import statistics
from collections import defaultdict
from pathlib import Path

import fitz
import pytesseract
from PIL import Image, ImageDraw, ImageFont
from pytesseract import Output

EXPECTED_PDF_SHA256 = "04f3854038cb48d7a761115ff69b0d5c121661d3a70a35fec6247d70c32db6f6"
EXPECTED_PAGES = 41
EXPECTED_PAGE = 7
EXPECTED_RASTER = (3356, 2320)


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def norm(s: str) -> str:
    return " ".join(re.findall(r"[0-9a-z]+", s.lower()))


def bbox_union(boxes):
    return (
        min(b[0] for b in boxes),
        min(b[1] for b in boxes),
        max(b[2] for b in boxes),
        max(b[3] for b in boxes),
    )


def expand_bbox(b, pad, w, h):
    return (max(0, b[0] - pad), max(0, b[1] - pad), min(w, b[2] + pad), min(h, b[3] + pad))


def overlaps(a, b):
    return max(a[0], b[0]) < min(a[2], b[2]) and max(a[1], b[1]) < min(a[3], b[3])


def load_font(size: int, bold: bool):
    candidates = [
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf" if bold else "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Bold.ttf" if bold else "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
    ]
    for p in candidates:
        if Path(p).exists():
            return ImageFont.truetype(p, size=size)
    return ImageFont.load_default()


def wrap_pixels(draw: ImageDraw.ImageDraw, text: str, font, max_width: int):
    words = text.split()
    lines = []
    cur = ""
    for word in words:
        candidate = word if not cur else cur + " " + word
        width = draw.textbbox((0, 0), candidate, font=font)[2]
        if width <= max_width or not cur:
            cur = candidate
        else:
            lines.append(cur)
            cur = word
    if cur:
        lines.append(cur)
    return lines


def fit_text(draw, text, bbox, start_size, min_ratio, bold):
    w = max(1, bbox[2] - bbox[0])
    h = max(1, bbox[3] - bbox[1])
    min_size = max(10, int(start_size * min_ratio))
    for size in range(start_size, min_size - 1, -1):
        font = load_font(size, bold)
        lines = wrap_pixels(draw, text, font, w)
        line_h = max(1, draw.textbbox((0, 0), "Ag", font=font)[3])
        spacing = max(1, int(line_h * 0.16))
        total_h = len(lines) * line_h + max(0, len(lines) - 1) * spacing
        max_line_w = max((draw.textbbox((0, 0), line, font=font)[2] for line in lines), default=0)
        if total_h <= h and max_line_w <= w:
            return font, lines, line_h, spacing, size, True
    font = load_font(min_size, bold)
    lines = wrap_pixels(draw, text, font, w)
    line_h = max(1, draw.textbbox((0, 0), "Ag", font=font)[3])
    spacing = max(1, int(line_h * 0.16))
    return font, lines, line_h, spacing, min_size, False


def paragraph_entries(ocr):
    groups = defaultdict(list)
    for w in ocr:
        groups[(w["block"], w["par"])].append(w)
    entries = []
    for key, words in groups.items():
        # Preserve Tesseract native token order. Re-sorting by pixel top/left can
        # scramble words on the same visual line when glyph tops differ slightly.
        text = " ".join(w["text"] for w in words)
        b = bbox_union([w["bbox"] for w in words])
        entries.append({"key": key, "text": text, "bbox": b, "words": words})
    entries.sort(key=lambda e: (e["bbox"][1], e["bbox"][0]))
    for i, e in enumerate(entries, 1):
        e["region_id"] = f"CDXN_P007_PIX_R{i:03d}"
    return entries


def find_paragraph(entries, anchor, exact_short=False):
    a = norm(anchor)
    matches = []
    for e in entries:
        t = norm(e["text"])
        if (t == a) if exact_short else (a in t):
            matches.append(e)
    if len(matches) != 1:
        raise RuntimeError(f"paragraph anchor {anchor!r}: expected 1 match, got {len(matches)}: {[m['text'] for m in matches]}")
    return matches[0]


def find_word_sequence(entry, wanted):
    # Ignore numeric/punctuation OCR tokens while matching human-language words.
    alpha = [w for w in entry["words"] if re.search(r"[A-Za-z]", w["text"])]
    targets = [norm(x) for x in wanted]
    for i in range(len(alpha)):
        got = []
        chosen = []
        for w in alpha[i:]:
            n = norm(w["text"])
            if not n:
                continue
            got.append(n)
            chosen.append(w)
            if got == targets:
                return chosen
            if len(got) >= len(targets) or got != targets[: len(got)]:
                break
    raise RuntimeError(f"word sequence {wanted!r} not found in {entry['text']!r}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--pdf", required=True)
    ap.add_argument("--page", type=int, default=EXPECTED_PAGE)
    ap.add_argument("--translations", required=True)
    ap.add_argument("--out", required=True)
    args = ap.parse_args()

    pdf = Path(args.pdf)
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    cfg = json.loads(Path(args.translations).read_text(encoding="utf-8"))

    pdf_sha = sha256(pdf)
    if pdf_sha != EXPECTED_PDF_SHA256:
        raise SystemExit(f"wrong canonical cdxn990e.pdf SHA-256: {pdf_sha}")

    doc = fitz.open(pdf)
    if doc.page_count != EXPECTED_PAGES:
        raise SystemExit(f"wrong page count: {doc.page_count}")
    if args.page != EXPECTED_PAGE:
        raise SystemExit("TEST2 is intentionally locked to physical page 7")

    page = doc[args.page - 1]
    native_text = page.get_text("text").strip()
    if native_text:
        raise SystemExit(f"TEST2 requires zero native PDF text; got {len(native_text)} chars")

    images = page.get_images(full=True)
    if not images:
        raise SystemExit("no raster image found on TEST2 page")
    imginfo = max(images, key=lambda x: int(x[2]) * int(x[3]))
    xref = int(imginfo[0])
    pix = fitz.Pixmap(doc, xref)
    if (pix.width, pix.height) != EXPECTED_RASTER:
        raise SystemExit(f"unexpected embedded raster: {(pix.width, pix.height)}")

    original_path = out / "CDXN990E_P007_RASTER_ORIGINAL.png"
    pix.save(str(original_path))
    original_sha = sha256(original_path)
    image = Image.open(original_path).convert("RGB")
    W, H = image.size

    # OCR is used only because this page has zero native text and all language is burned into pixels.
    data = pytesseract.image_to_data(image, lang="eng", config="--psm 3", output_type=Output.DICT)
    words = []
    for i, txt in enumerate(data["text"]):
        txt = (txt or "").strip()
        if not txt:
            continue
        try:
            conf = float(data["conf"][i])
        except Exception:
            conf = -1
        if conf < 25:
            continue
        x, y, w, h = (int(data[k][i]) for k in ("left", "top", "width", "height"))
        words.append({
            "text": txt,
            "confidence": conf,
            "left": x,
            "top": y,
            "width": w,
            "height": h,
            "bbox": (x, y, x + w, y + h),
            "block": int(data["block_num"][i]),
            "par": int(data["par_num"][i]),
            "line": int(data["line_num"][i]),
        })

    if len(words) < 120:
        raise SystemExit(f"OCR fallback recovered too few words: {len(words)}")
    all_ocr = " ".join(w["text"] for w in words)
    for required in ("HOW TO USE THE CIRCUIT DIAGRAMS", "Power Distribution", "AUTOMATIC", "86M3825", "86M3831"):
        if norm(required) not in norm(all_ocr):
            raise SystemExit(f"OCR missing expected evidence: {required}")

    entries = paragraph_entries(words)
    regions_json = []
    for e in entries:
        regions_json.append({
            "region_id": e["region_id"],
            "source_text": e["text"],
            "bbox": list(e["bbox"]),
            "source": "ocr_fallback_raster_only",
        })
    (out / "CDXN990E_P007_OCR_REGIONS.json").write_text(json.dumps(regions_json, ensure_ascii=False, indent=2), encoding="utf-8")

    rendered = image.copy()
    draw = ImageDraw.Draw(rendered)
    original = image.copy()
    replacement_masks = []
    op_results = []

    for idx, op in enumerate(cfg["operations"], 1):
        mode = op["mode"]
        bold = bool(op.get("bold", False))
        if mode == "paragraph":
            entry = find_paragraph(entries, op["anchor"], bool(op.get("exact_short", False)))
            chosen_words = entry["words"]
            source_text = entry["text"]
            bbox = entry["bbox"]
            region_id = entry["region_id"]
        elif mode == "words":
            entry = find_paragraph(entries, op["paragraph_anchor"], False)
            chosen_words = find_word_sequence(entry, op["words"])
            source_text = " ".join(w["text"] for w in chosen_words)
            bbox = bbox_union([w["bbox"] for w in chosen_words])
            region_id = entry["region_id"]
        else:
            raise RuntimeError(f"unknown operation mode {mode}")

        b = expand_bbox(bbox, 5, W, H)
        # Word/paragraph bounding boxes on this raster page sit on white paper.
        # Never modify pixels outside the extracted text zone.
        crop = original.crop(b).convert("L")
        white_ratio = sum(1 for px in crop.getdata() if px >= 240) / max(1, crop.width * crop.height)
        if white_ratio < 0.60:
            raise RuntimeError(f"text zone contains too much non-paper artwork: op {idx}, ratio={white_ratio:.3f}, source={source_text!r}")

        median_h = statistics.median([max(1, w["height"]) for w in chosen_words])
        start_size = max(12, int(median_h * 1.12))
        font, lines, line_h, spacing, used_size, fitted = fit_text(draw, op["text"], b, start_size, 0.62, bold)
        if not fitted:
            raise RuntimeError(f"translation would be clipped even at allowed minimum size: {op['text']!r} in {b}")

        draw.rectangle(b, fill=(255, 255, 255))
        y = b[1]
        for line in lines:
            draw.text((b[0], y), line, font=font, fill=(0, 0, 0))
            y += line_h + spacing
        if y - spacing > b[3] + 1:
            raise RuntimeError(f"post-render overflow: {op['text']!r}")

        replacement_masks.append(b)
        op_results.append({
            "operation": idx,
            "mode": mode,
            "region_id": region_id,
            "source_text": source_text,
            "localized_text": op["text"],
            "bbox": list(b),
            "source_font_estimate": start_size,
            "render_font_size": used_size,
            "font_ratio": round(used_size / start_size, 3),
            "fitted": True,
            "source_white_ratio": round(white_ratio, 4),
        })

    # Technical tokens must not be touched by any replacement mask and their pixel crops must remain byte-identical.
    immutable_results = []
    for token in cfg["immutable_tokens"]:
        candidates = [w for w in words if norm(w["text"]) == norm(token)]
        if not candidates:
            raise RuntimeError(f"immutable technical token not found by OCR: {token}")
        w = candidates[0]
        tb = expand_bbox(w["bbox"], 2, W, H)
        if any(overlaps(tb, m) for m in replacement_masks):
            raise RuntimeError(f"replacement mask overlaps immutable technical token {token}: {tb}")
        a = original.crop(tb).tobytes()
        z = rendered.crop(tb).tobytes()
        same = a == z
        if not same:
            raise RuntimeError(f"technical token pixels changed: {token}")
        immutable_results.append({"token": token, "bbox": list(tb), "pixels_identical": True})

    # Pixel-level geometry guard: outside all translation masks, the complete raster must be identical.
    # Compare by zeroing the allowed masks in both images, then hashing the resulting byte arrays.
    guard_a = original.copy()
    guard_b = rendered.copy()
    ga = ImageDraw.Draw(guard_a)
    gb = ImageDraw.Draw(guard_b)
    for m in replacement_masks:
        ga.rectangle(m, fill=(255, 255, 255))
        gb.rectangle(m, fill=(255, 255, 255))
    outside_identical = guard_a.tobytes() == guard_b.tobytes()
    if not outside_identical:
        raise RuntimeError("pixels outside declared translation zones changed")

    localized_path = out / "CDXN990E_P007_MEMS_SIM_FR.png"
    rendered.save(localized_path)

    manifest = {
        "method": "ravemems",
        "test": "TEST2_raster_only_embedded_human_text",
        "source": {
            "path": str(pdf).replace("\\", "/"),
            "sha256": pdf_sha,
            "page_count": doc.page_count,
            "physical_page": args.page,
            "native_pdf_text_chars": len(native_text),
            "embedded_raster": {"xref": xref, "width": W, "height": H},
            "original_png_sha256": original_sha,
        },
        "extraction": {
            "ocr_used": True,
            "ocr_reason": "zero native PDF text; human language is rasterized in image pixels",
            "ocr_words": len(words),
            "ocr_regions": len(entries),
            "translation_data_canonical": False,
            "translation_fixture_role": "MEMS Manager display simulation only",
        },
        "render": {
            "locale": cfg.get("locale"),
            "replacement_operations": len(op_results),
            "operations": op_results,
            "immutable_tokens": immutable_results,
            "outside_translation_masks_pixel_identical": outside_identical,
            "artificial_visible_ids": False,
            "output_width": rendered.width,
            "output_height": rendered.height,
            "localized_png_sha256": sha256(localized_path),
        },
        "gate": {
            "canonical_source_verified": pdf_sha == EXPECTED_PDF_SHA256,
            "raster_only_case_verified": len(native_text) == 0 and (W, H) == EXPECTED_RASTER,
            "ocr_fallback_recovered_text": len(words) >= 120,
            "all_requested_replacements_fitted": all(x["fitted"] for x in op_results),
            "technical_tokens_pixel_identical": all(x["pixels_identical"] for x in immutable_results),
            "geometry_unchanged_outside_text_zones": outside_identical,
            "no_visible_internal_numbering": True,
        },
    }
    manifest["pass"] = all(manifest["gate"].values())
    (out / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps({
        "pass": manifest["pass"],
        "ocr_words": len(words),
        "ocr_regions": len(entries),
        "ops": len(op_results),
        "immutable": len(immutable_results),
        "outside_masks_identical": outside_identical,
        "original": str(original_path),
        "localized": str(localized_path),
    }, indent=2))
    if not manifest["pass"]:
        raise SystemExit(2)


if __name__ == "__main__":
    main()
