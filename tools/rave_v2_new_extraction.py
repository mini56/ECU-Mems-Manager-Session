import argparse
import hashlib
import json
import re
from pathlib import Path

import fitz
from PIL import Image, ImageDraw, ImageFont

EXPECTED_SHA256 = "c050a3eebe50c5a85bf8a69b7722bd2052079944e09d58578a498984ecf06715"
EXPECTED_PAGES = 372
TECH_ANCHOR_PATTERNS = [
    re.compile(r"^\s*\d+([.,]\d+)?\s*(N\.m|Nm|mm|cm|m|V|A|bar|psi|°C|kPa|MPa|ml|l|kg|g)?\s*$", re.I),
    re.compile(r"^\s*\d+(\.\d+){1,4}\s*$"),
    re.compile(r"^\s*18G\s*\d+[A-Z]?\s*$", re.I),
    re.compile(r"^\s*[A-Z]\d{2,}[A-Z0-9-]*\s*$"),
]

def sha256_file(path):
    h = hashlib.sha256()
    with Path(path).open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()

def parse_ranges(spec):
    out = []
    for part in (spec or "").split(","):
        part = part.strip()
        if not part:
            continue
        if "-" in part:
            a, b = part.split("-", 1)
            out.append((int(a), int(b)))
        else:
            n = int(part)
            out.append((n, n))
    return out

def is_excluded(p, ranges):
    return any(a <= p <= b for a, b in ranges)

def area(r):
    return max(0.0, r.width) * max(0.0, r.height)

def intersects(a, b):
    return area(fitz.Rect(a) & fitz.Rect(b)) > 0

def inflate(r, margin, page_rect):
    return fitz.Rect(r.x0-margin, r.y0-margin, r.x1+margin, r.y1+margin) & page_rect

def normalize(s):
    return re.sub(r"\s+", " ", s).strip()

def technical_anchor(text):
    t = normalize(text)
    if not t or len(t) > 80:
        return False
    if any(p.match(t) for p in TECH_ANCHOR_PATTERNS):
        return True
    letters = sum(c.isalpha() for c in t)
    digits = sum(c.isdigit() for c in t)
    return digits >= 2 and letters <= 2

def metrics(page):
    text = page.get_text("text") or ""
    drawings = page.get_drawings()
    images = page.get_images(full=True)
    words = page.get_text("words") or []
    rects = [fitz.Rect(d["rect"]) for d in drawings if d.get("rect")]
    union = None
    for r in rects:
        union = fitz.Rect(r) if union is None else (union | r)
    ratio = area(union) / (area(page.rect) or 1.0) if union else 0.0
    return {
        "text_chars": len(text.strip()),
        "word_count": len(words),
        "drawing_count": len(drawings),
        "image_count": len(images),
        "visual_area_ratio": ratio,
    }

def choose_mixed_page(doc, excluded_ranges):
    candidates = []
    for idx in range(doc.page_count):
        p = idx + 1
        if is_excluded(p, excluded_ranges):
            continue
        m = metrics(doc[idx])
        mixed = m["text_chars"] >= 250 and m["word_count"] >= 40 and (m["drawing_count"] >= 12 or m["image_count"] >= 1) and m["visual_area_ratio"] >= 0.08
        if not mixed:
            continue
        score = min(m["text_chars"], 2200) / 2200 * 3 + min(m["drawing_count"], 160) / 160 * 4 + min(m["image_count"], 4) * 1.5 + min(m["visual_area_ratio"], 0.65) * 6
        if p < 20:
            score -= 4
        candidates.append((score, p, m))
    if not candidates:
        raise RuntimeError("No mixed text+visual page found outside excluded ranges")
    candidates.sort(reverse=True)
    return candidates[0], candidates[:10]

def text_blocks(page):
    out = []
    for bi, block in enumerate(page.get_text("dict").get("blocks", [])):
        if block.get("type") != 0:
            continue
        spans, parts = [], []
        for line in block.get("lines", []):
            for span in line.get("spans", []):
                txt = span.get("text", "")
                if txt:
                    parts.append(txt)
                    spans.append({"text": txt, "bbox": [round(float(v),3) for v in span["bbox"]], "font": span.get("font"), "size": span.get("size"), "flags": span.get("flags")})
        txt = normalize(" ".join(parts))
        if txt:
            out.append({"block_index": bi, "bbox": [round(float(v),3) for v in block["bbox"]], "text": txt, "spans": spans})
    return out

def visual_metadata(page):
    drawings = []
    for i, d in enumerate(page.get_drawings()):
        if d.get("rect"):
            drawings.append({"index": i, "bbox": [round(float(v),3) for v in d["rect"]], "items": len(d.get("items", [])), "type": d.get("type")})
    images = []
    for i, img in enumerate(page.get_images(full=True)):
        xref = img[0]
        images.append({"index": i, "xref": xref, "width": img[2], "height": img[3], "bboxes": [[round(float(v),3) for v in r] for r in page.get_image_rects(xref)]})
    return drawings, images

def visual_regions(page):
    rects = []
    for d in page.get_drawings():
        if not d.get("rect"):
            continue
        r = fitz.Rect(d["rect"])
        if area(r) >= 8:
            rects.append(r)
    clusters = []
    for r in sorted(rects, key=lambda q: (q.y0, q.x0)):
        for c in clusters:
            if intersects(inflate(c["rect"], 8, page.rect), r):
                c["rect"] |= r
                c["count"] += 1
                break
        else:
            clusters.append({"rect": fitz.Rect(r), "count": 1})
    changed = True
    while changed:
        changed = False
        merged = []
        while clusters:
            c = clusters.pop(0)
            j = 0
            while j < len(clusters):
                if intersects(inflate(c["rect"], 10, page.rect), clusters[j]["rect"]):
                    c["rect"] |= clusters[j]["rect"]
                    c["count"] += clusters[j]["count"]
                    clusters.pop(j)
                    changed = True
                else:
                    j += 1
            merged.append(c)
        clusters = merged
    page_area = area(page.rect) or 1
    useful = [c for c in clusters if c["count"] >= 3 and area(c["rect"]) / page_area >= 0.025]
    useful.sort(key=lambda c: area(c["rect"]), reverse=True)
    return useful[:8]

def translation_zones(page, blocks, regions, doc_key, pnum):
    vr = [inflate(c["rect"], 16, page.rect) for c in regions]
    zones, seen = [], set()
    for block in blocks:
        if not any(intersects(block["bbox"], r) for r in vr):
            continue
        for span in block["spans"]:
            txt = normalize(span["text"])
            if len(txt) < 2 or len(txt) > 90 or not any(c.isalpha() for c in txt) or technical_anchor(txt):
                continue
            if not any(intersects(span["bbox"], r) for r in vr):
                continue
            key = tuple(round(v,1) for v in span["bbox"]) + (txt,)
            if key in seen:
                continue
            seen.add(key)
            zones.append({"key": None, "source_text": txt, "bbox": span["bbox"], "status": "translation_required"})
    zones.sort(key=lambda z: (z["bbox"][1], z["bbox"][0], z["source_text"]))
    for i, z in enumerate(zones, 1):
        z["key"] = f"{doc_key}_P{pnum:03d}_T{i:02d}"
    return zones

def font(size, bold=False):
    candidates = [
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf" if bold else "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Bold.ttf" if bold else "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
    ]
    for p in candidates:
        if Path(p).exists():
            return ImageFont.truetype(p, size=size)
    return ImageFont.load_default()

def wrap(draw, text, fnt, width):
    words = text.split()
    if not words:
        return [""]
    lines, cur = [], words[0]
    for word in words[1:]:
        test = cur + " " + word
        if draw.textlength(test, font=fnt) <= width:
            cur = test
        else:
            lines.append(cur)
            cur = word
    lines.append(cur)
    return lines

def render_original(page, dpi=160):
    scale = dpi / 72.0
    pix = page.get_pixmap(matrix=fitz.Matrix(scale, scale), alpha=False)
    img = Image.frombytes("RGB" if pix.n == 3 else "RGBA", [pix.width, pix.height], pix.samples)
    if img.mode == "RGBA":
        bg = Image.new("RGB", img.size, "white")
        bg.paste(img, mask=img.getchannel("A"))
        img = bg
    return img, scale

def render_localized(original, scale, zones, translations, out_path):
    legend_width = max(560, int(original.width * 0.42))
    canvas = Image.new("RGB", (original.width + legend_width, original.height), "white")
    canvas.paste(original, (0,0))
    draw = ImageDraw.Draw(canvas)
    num_font = font(max(16, int(16*scale)), True)
    legend_font = font(max(16, int(13*scale)))
    header_font = font(max(20, int(17*scale)), True)
    for i, z in enumerate(zones, 1):
        x0,y0,x1,y1 = [int(v*scale) for v in z["bbox"]]
        pad = max(2, int(1.8*scale))
        draw.rectangle((x0-pad,y0-pad,x1+pad,y1+pad), fill="white")
        cx, cy = x0 + max(10,int(7*scale)), y0 + max(10,int(7*scale))
        radius = max(10,int(8*scale))
        draw.ellipse((cx-radius,cy-radius,cx+radius,cy+radius), outline="black", width=max(1,int(scale)))
        label = str(i)
        bb = draw.textbbox((0,0), label, font=num_font)
        draw.text((cx-(bb[2]-bb[0])/2, cy-(bb[3]-bb[1])/2-1), label, fill="black", font=num_font)
    lx, y = original.width + 24, 24
    draw.text((lx,y), "LÉGENDE TRADUITE — FR", fill="black", font=header_font)
    y += int(34*scale)
    body_width = legend_width - 48
    for i, z in enumerate(zones, 1):
        translated = translations.get(z["key"], z["source_text"])
        for line in wrap(draw, f"{i}. {translated}", legend_font, body_width):
            draw.text((lx,y), line, fill="black", font=legend_font)
            y += int(18*scale)
        y += int(5*scale)
        if y > original.height - int(60*scale):
            draw.text((lx, original.height-int(40*scale)), "[Légende trop longue — needs_review]", fill="black", font=legend_font)
            break
    canvas.save(out_path)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--pdf", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--exclude", default="193-202")
    ap.add_argument("--translations", default="")
    ap.add_argument("--page", default="auto")
    args = ap.parse_args()
    pdf, out = Path(args.pdf), Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    actual_sha = sha256_file(pdf)
    if actual_sha != EXPECTED_SHA256:
        raise SystemExit(f"RAVE source SHA mismatch: {actual_sha}")
    doc = fitz.open(pdf)
    if doc.page_count != EXPECTED_PAGES:
        raise SystemExit(f"RAVE page count mismatch: {doc.page_count}")
    excludes = parse_ranges(args.exclude)
    if args.page == "auto":
        (score,pnum,m), top = choose_mixed_page(doc, excludes)
    else:
        pnum = int(args.page)
        if is_excluded(pnum, excludes):
            raise SystemExit(f"Requested page {pnum} is excluded")
        m, score, top = metrics(doc[pnum-1]), 0, [(0,pnum,metrics(doc[pnum-1]))]
    page = doc[pnum-1]
    source_text = page.get_text("text") or ""
    blocks = text_blocks(page)
    drawings, images = visual_metadata(page)
    regions = visual_regions(page)
    zones = translation_zones(page, blocks, regions, "RCL0193ENG", pnum)
    original, scale = render_original(page)
    original_path = out / f"RCL0193ENG_P{pnum:03d}_ORIGINAL.png"
    original.save(original_path)
    translations = {}
    if args.translations and Path(args.translations).exists():
        loaded = json.loads(Path(args.translations).read_text(encoding="utf-8"))
        translations = loaded.get("translations", loaded) if isinstance(loaded, dict) else {}
    localized_path = out / f"RCL0193ENG_P{pnum:03d}_FR_NUMBERED_LEGEND.png"
    render_localized(original.copy(), scale, zones, translations, localized_path)
    (out/"source_text.txt").write_text(source_text, encoding="utf-8")
    (out/"text_blocks.json").write_text(json.dumps(blocks, ensure_ascii=False, indent=2), encoding="utf-8")
    (out/"visuals.json").write_text(json.dumps({"drawings":drawings,"images":images,"drawing_regions":[{"bbox":[round(float(v),3) for v in c["rect"]],"drawing_count":c["count"]} for c in regions]}, ensure_ascii=False, indent=2), encoding="utf-8")
    (out/"translation_zones.json").write_text(json.dumps(zones, ensure_ascii=False, indent=2), encoding="utf-8")
    template = {"locale":"fr","page":pnum,"translations":{z["key"]:z["source_text"] for z in zones},"note":"Replace source strings with French; technical anchors are intentionally excluded."}
    (out/"translation_template_fr.json").write_text(json.dumps(template, ensure_ascii=False, indent=2), encoding="utf-8")
    manifest = {
        "method":"RAVE_V2_NEW_EXTRACTION",
        "source":{"path":str(pdf).replace("\\","/"),"sha256":actual_sha,"page_count":doc.page_count,"document_key":"RCL0193ENG","edition":"5th Edition"},
        "page":{"physical_page":pnum,"classification":"mixed_text_visual","selection_score":score,**m},
        "preservation":{"source_text_full":bool(source_text.strip()),"original_page_raster_intact":original_path.exists(),"original_page_sha256":sha256_file(original_path),"localized_derivative_separate":localized_path.exists()},
        "extraction":{"text_blocks":len(blocks),"drawing_objects":len(drawings),"embedded_images":len(images),"visual_regions":len(regions),"translation_zones":len(zones),"translation_keys_stable":all(z["key"] for z in zones),"technical_anchors_preserved_by_rule":True},
        "translation":{"locale":"fr","provided_translation_count":sum(1 for z in zones if translations.get(z["key"])),"required_count":len(zones),"legend_inside_same_displayed_image":True,"status":"translated" if zones and all(translations.get(z["key"]) for z in zones) else "needs_translation"},
        "provenance":{"pdf_page_1based":pnum,"source_sha256":actual_sha},
        "gate":{"source_verified":actual_sha==EXPECTED_SHA256 and doc.page_count==EXPECTED_PAGES,"mixed_page":m["text_chars"]>=250 and (m["drawing_count"]>=12 or m["image_count"]>=1),"source_text_present":bool(source_text.strip()),"visual_evidence_present":original_path.exists(),"zones_present":len(zones)>0},
        "candidate_ranking":[{"page":p,"score":round(s,4),**mm} for s,p,mm in top],
    }
    manifest["gate"]["pilot_extraction_pass"] = all(manifest["gate"].values())
    (out/"manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")
    if not manifest["gate"]["pilot_extraction_pass"]:
        raise SystemExit("Pilot extraction gate failed; inspect output artifact")
    print(json.dumps({"selected_page":pnum,"text_chars":m["text_chars"],"drawing_count":m["drawing_count"],"image_count":m["image_count"],"visual_regions":len(regions),"translation_zones":len(zones),"translation_status":manifest["translation"]["status"]}, ensure_ascii=False))

if __name__ == "__main__":
    main()
