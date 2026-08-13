#!/usr/bin/env python3
from pathlib import Path
import json
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
TR = ROOT / "translations"
LANGS = ("fr", "en", "es", "it", "pt", "de")


def node_text(el):
    return "" if el is None else "".join(el.itertext())


def load_ts(lang):
    path = TR / f"ECUMemsManager_{lang}.ts"
    tree = ET.parse(path)
    contexts = {}
    count = 0
    for ctx in tree.getroot().findall("context"):
        name = node_text(ctx.find("name")) or "Global"
        entries = []
        for msg in ctx.findall("message"):
            src = node_text(msg.find("source"))
            tr = node_text(msg.find("translation"))
            if not src:
                continue
            entries.append({"source": src, "text": tr if tr else src})
            count += 1
        if entries:
            contexts[name] = entries
    return contexts, count


catalogs = {}
counts = {}
for lang in LANGS:
    catalogs[lang], counts[lang] = load_ts(lang)
    print(f"{lang}: {counts[lang]} messages")

reference_count = counts["fr"]
if reference_count < 623:
    raise SystemExit(f"ERREUR: catalogue FR incomplet: {reference_count} messages")

for lang in LANGS:
    if counts[lang] != reference_count:
        raise SystemExit(f"ERREUR: {lang} contient {counts[lang]} messages, FR={reference_count}")
    out = {
        "language": lang,
        "message_count": counts[lang],
        "contexts": catalogs[lang],
    }
    (TR / f"{lang}.json").write_text(json.dumps(out, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"{lang}.json: {counts[lang]} messages conserves")

print(f"EXPORT OK: {reference_count} messages dans chacune des 6 langues")
