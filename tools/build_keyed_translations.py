#!/usr/bin/env python3
from pathlib import Path
import json
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
TR = ROOT / "translations"

# Stable ranges by UI/source context. 0xxx = common/main window.
RANGES = {
    "MainWindow": 1,
    "CaptureViewer": 100,
    "AnalysisTab": 200,
    "DiagnosticPanel": 300,
    "MainInterface": 1000,
}

def text(el):
    return "" if el is None else "".join(el.itertext())

def read_ts(lang):
    root = ET.parse(TR / f"ECUMemsManager_{lang}.ts").getroot()
    out = {}
    for ctx in root.findall("context"):
        name = text(ctx.find("name")) or "Global"
        vals = []
        for msg in ctx.findall("message"):
            src = text(msg.find("source"))
            val = text(msg.find("translation")) or src
            if src:
                vals.append((src, val))
        out[name] = vals
    return out

en = read_ts("en")
fr = read_ts("fr")
contexts = sorted(set(en) | set(fr))
next_free = 6000
keymap = {}
used = set()

def alloc(ctx):
    global next_free
    base = RANGES.get(ctx)
    if base is not None:
        k = base
        while k in used:
            k += 1
        # keep each named block below the next thousand where possible
        if ctx == "MainInterface" and k >= 2000:
            base = None
        elif ctx == "DiagnosticPanel" and k >= 1000:
            base = None
        elif ctx in ("MainWindow", "CaptureViewer", "AnalysisTab") and k >= 1000:
            base = None
        if base is not None:
            used.add(k)
            RANGES[ctx] = k + 1
            return k
    while next_free in used:
        next_free += 1
    k = next_free
    used.add(k)
    next_free += 1
    return k

out_en = {}
out_fr = {}
for ctx in contexts:
    en_by_src = {s:v for s,v in en.get(ctx, [])}
    fr_by_src = {s:v for s,v in fr.get(ctx, [])}
    sources = []
    for s,_ in en.get(ctx, []):
        if s not in sources: sources.append(s)
    for s,_ in fr.get(ctx, []):
        if s not in sources: sources.append(s)
    for src in sources:
        k = alloc(ctx)
        english = en_by_src.get(src, src)
        french = fr_by_src.get(src, src)
        out_en[str(k)] = english
        out_fr[str(k)] = french
        keymap[f"{ctx}\u241f{src}"] = k

(TR / "en.json").write_text(json.dumps(out_en, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
(TR / "fr.json").write_text(json.dumps(out_fr, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
(ROOT / "translations" / "keymap.json").write_text(json.dumps(keymap, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
print(f"EN={len(out_en)} FR={len(out_fr)} keys={len(keymap)}")
if set(out_en) != set(out_fr):
    raise SystemExit("EN/FR key mismatch")
