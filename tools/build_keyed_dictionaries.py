#!/usr/bin/env python3
from pathlib import Path
import ast
import json
import re
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
TR = ROOT / "translations"
LANGS = ("fr", "en", "es", "it", "pt", "de")
MIN_KEYS = 650


def node_text(el):
    return "" if el is None else "".join(el.itertext())


def load_ts(lang):
    path = TR / f"ECUMemsManager_{lang}.ts"
    tree = ET.parse(path)
    values = {}
    order = []
    for ctx in tree.getroot().findall("context"):
        for msg in ctx.findall("message"):
            src = node_text(msg.find("source"))
            tr = node_text(msg.find("translation"))
            if not src:
                continue
            if src not in values:
                order.append(src)
            values[src] = tr if tr else src
    return values, order


catalogs = {}
orders = {}
for lang in LANGS:
    catalogs[lang], orders[lang] = load_ts(lang)
    print(f"{lang}: {len(catalogs[lang])} chaines uniques")
    if len(catalogs[lang]) < MIN_KEYS:
        raise SystemExit(f"ERREUR: {lang} ne contient que {len(catalogs[lang])} chaines (< {MIN_KEYS})")

# Union stable: ordre français puis éventuelles chaînes absentes du français.
all_sources = list(orders["fr"])
seen = set(all_sources)
for lang in LANGS:
    for src in orders[lang]:
        if src not in seen:
            seen.add(src)
            all_sources.append(src)

# Chaque langue doit posséder toutes les clés. Une absence est une erreur de catalogue,
# pas une raison d'inventer silencieusement une traduction.
missing = {}
for lang in LANGS:
    miss = [s for s in all_sources if s not in catalogs[lang]]
    if miss:
        missing[lang] = miss
if missing:
    for lang, miss in missing.items():
        print(f"ERREUR {lang}: {len(miss)} chaines manquantes")
        for s in miss[:20]:
            print("  -", repr(s))
    raise SystemExit("Catalogues incomplets: migration annulee")

keys = {src: f"T{i:04d}" for i, src in enumerate(all_sources, 1)}

# Fichier de référence lisible pour maintenir les clés sans dépendre de Qt.
reference = {key: src for src, key in keys.items()}
(TR / "keys.json").write_text(json.dumps(reference, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

for lang in LANGS:
    out = {keys[src]: catalogs[lang][src] for src in all_sources}
    if len(out) != len(all_sources):
        raise SystemExit(f"ERREUR interne: dictionnaire {lang} incomplet")
    (TR / f"{lang}.json").write_text(json.dumps(out, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"{lang}.json: {len(out)} cles")

# ---------------------------------------------------------------------------
# Migration C++ : I18n::text("texte") / tr("texte") -> I18n::text("Txxxx")
# Prend aussi en charge les littéraux C++ concaténés sur plusieurs lignes.
# ---------------------------------------------------------------------------
CALL = re.compile(
    r"(?P<fn>\b(?:I18n::text|tr))\(\s*(?P<lits>(?:\"(?:\\.|[^\"\\])*\"\s*)+)\)",
    re.S,
)
STRING = re.compile(r'\"(?:\\.|[^\"\\])*\"')


def decode_cpp_literals(block):
    parts = []
    for m in STRING.finditer(block):
        lit = m.group(0)
        try:
            parts.append(ast.literal_eval(lit))
        except Exception:
            return None
    return "".join(parts)


def migrate_cpp(path):
    original = path.read_text(encoding="utf-8")

    def repl(m):
        value = decode_cpp_literals(m.group("lits"))
        key = keys.get(value) if value is not None else None
        if not key:
            return m.group(0)
        return f'I18n::text("{key}")'

    updated, count = CALL.subn(repl, original)
    if updated != original:
        path.write_text(updated, encoding="utf-8")
    return count


cpp_count = 0
for path in ROOT.rglob("*.cpp"):
    if any(part in {"translations", "prebuilt-librosco", "build", ".git"} for part in path.parts):
        continue
    cpp_count += migrate_cpp(path)
for path in ROOT.rglob("*.h"):
    if any(part in {"translations", "prebuilt-librosco", "build", ".git"} for part in path.parts):
        continue
    cpp_count += migrate_cpp(path)
print(f"C++: {cpp_count} appel(s) migre(s) vers des cles")

# ---------------------------------------------------------------------------
# Migration .ui : tout <string> correspondant au catalogue devient Txxxx.
# Le chargeur runtime remplace ensuite la clé selon la langue choisie.
# ---------------------------------------------------------------------------
ui_count = 0
for path in ROOT.rglob("*.ui"):
    tree = ET.parse(path)
    changed = 0
    for el in tree.getroot().iter("string"):
        value = el.text or ""
        key = keys.get(value)
        if key:
            el.text = key
            changed += 1
    if changed:
        ET.indent(tree, space=" ")
        tree.write(path, encoding="utf-8", xml_declaration=True)
        ui_count += changed
print(f"UI: {ui_count} chaine(s) migree(s) vers des cles")

# Contrôle final: mêmes clés dans les 6 JSON et nombre suffisant.
expected = set(reference)
for lang in LANGS:
    data = json.loads((TR / f"{lang}.json").read_text(encoding="utf-8"))
    if set(data) != expected:
        raise SystemExit(f"ERREUR: jeu de cles different dans {lang}.json")
    if len(data) < MIN_KEYS:
        raise SystemExit(f"ERREUR: {lang}.json contient moins de {MIN_KEYS} cles")

print(f"MIGRATION OK: {len(expected)} cles identiques dans les 6 langues")
