#!/usr/bin/env python3
from pathlib import Path
import ast
import re
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parent
LANGS = ("es", "it", "pt", "de")


def literal_from(path: Path, name: str, default):
    tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
    for node in tree.body:
        if isinstance(node, ast.Assign):
            for target in node.targets:
                if isinstance(target, ast.Name) and target.id == name:
                    try:
                        return ast.literal_eval(node.value)
                    except Exception:
                        return default
    return default


base_file = ROOT / "complete_catalogs.py"
remaining_file = ROOT / "translate_remaining.py"
final_file = ROOT / "final_cleanup.py"

BASE_EXACT = literal_from(base_file, "T", {})
BASE_HTML = literal_from(base_file, "HTML_T", {})
BASE_PHRASES = literal_from(base_file, "PHRASES", {})
SECOND_EXACT = literal_from(remaining_file, "EXACT", {})
EN_EXACT = literal_from(remaining_file, "EN_EXACT", {})
SECOND_PHRASES = literal_from(remaining_file, "PHRASES", {})
FINAL_EXACT = literal_from(final_file, "EXACT", {})
FINAL_FRAGMENTS = literal_from(final_file, "FRAGMENTS", {})
UNDOC_PREFIX = literal_from(final_file, "UNDOC_PREFIX", {
    "es": "No documentado", "it": "Non documentato",
    "pt": "Não documentado", "de": "Nicht dokumentiert"
})

# Compléments pour les bulles d'aide et phrases longues observées à l'écran.
FULL_EN = {
    "Idle switch state. It should be ON with the throttle closed during idle.": {
        "es": "Estado del interruptor de ralentí. Debe estar ACTIVADO con el acelerador cerrado durante el ralentí.",
        "it": "Stato dell'interruttore del minimo. Deve essere ATTIVO con la farfalla chiusa durante il minimo.",
        "pt": "Estado do interruptor de ralenti. Deve estar ATIVO com a borboleta fechada durante o ralenti.",
        "de": "Status des Leerlaufschalters. Er sollte bei geschlossener Drosselklappe im Leerlauf AKTIV sein."
    },
    "Closed-loop state. When active, the ECU uses the lambda sensor to correct the mixture.": {
        "es": "Estado de bucle cerrado. Cuando está activo, la ECU utiliza la sonda lambda para corregir la mezcla.",
        "it": "Stato ad anello chiuso. Quando è attivo, la ECU usa la sonda lambda per correggere la miscela.",
        "pt": "Estado de malha fechada. Quando ativo, a ECU utiliza a sonda lambda para corrigir a mistura.",
        "de": "Status des geschlossenen Regelkreises. Wenn aktiv, verwendet die ECU die Lambdasonde zur Gemischkorrektur."
    },
    "0-200 mV: lean mixture\n700-900 mV: rich mixture": {
        "es": "0-200 mV: mezcla pobre\n700-900 mV: mezcla rica",
        "it": "0-200 mV: miscela magra\n700-900 mV: miscela ricca",
        "pt": "0-200 mV: mistura pobre\n700-900 mV: mistura rica",
        "de": "0-200 mV: mageres Gemisch\n700-900 mV: fettes Gemisch"
    },
}

EN_WORDS = re.compile(r"\b(the|this|that|with|without|from|into|only|before|after|select|check|value|status|sensor|engine|fuel|file|report|warning|error|unknown|received|calculated|enabled|disabled|open|closed|reset|capture|available|stored|maximum|minimum|position|temperature|pressure|voltage|signal|control|time|idle|lambda|battery|throttle|coil|boost|pump|circuit|should|during|uses|mixture|state)\b", re.I)


def text(el):
    return "" if el is None else "".join(el.itertext())


def set_text(el, value):
    el.clear()
    el.text = value
    el.attrib.pop("type", None)


def replace_all(value: str, pairs):
    out = value
    for a, b in sorted(pairs, key=lambda x: len(x[0]), reverse=True):
        out = re.sub(re.escape(a), b, out, flags=re.I)
    return out


def translate_one(src: str, old: str, en: str, lang: str):
    # Une seule décision de traduction par entrée, avec priorité aux traductions exactes.
    if src in FINAL_EXACT:
        return FINAL_EXACT[src].get(lang, old)
    if src in SECOND_EXACT:
        return SECOND_EXACT[src].get(lang, old)
    if src in BASE_HTML:
        return BASE_HTML[src].get(lang, old)
    if src in BASE_EXACT:
        return BASE_EXACT[src].get(lang, old)

    m = re.fullmatch(r"Non documenté\s*([0-9A-Fa-f]+)", src.strip())
    if m:
        return f"{UNDOC_PREFIX[lang]} {m.group(1)}"

    candidate = old
    if candidate in EN_EXACT:
        candidate = EN_EXACT[candidate].get(lang, candidate)
    if en in FULL_EN and (candidate.strip() == en.strip() or EN_WORDS.search(candidate)):
        candidate = FULL_EN[en][lang]
    elif candidate.strip() == en.strip() and en:
        candidate = en

    # Traduction de secours en un seul passage logique.
    pairs = []
    pairs.extend(BASE_PHRASES.get(lang, []))
    pairs.extend(SECOND_PHRASES.get(lang, []))
    candidate = replace_all(candidate, pairs)

    # Nettoyage final des fragments résiduels, toujours dans ce même passage.
    for a, b in sorted(FINAL_FRAGMENTS.get(lang, {}).items(), key=lambda kv: len(kv[0]), reverse=True):
        candidate = candidate.replace(a, b)

    return candidate


def load_english():
    tree = ET.parse(ROOT / "ECUMemsManager_en.ts")
    out = {}
    for ctx in tree.getroot().findall("context"):
        cname = text(ctx.find("name"))
        for msg in ctx.findall("message"):
            src = text(msg.find("source"))
            tr = text(msg.find("translation"))
            out[(cname, src)] = tr
    return out


def main():
    english = load_english()
    total_changed = 0
    residuals = []

    for lang in LANGS:
        path = ROOT / f"ECUMemsManager_{lang}.ts"
        tree = ET.parse(path)
        changed = 0
        for ctx in tree.getroot().findall("context"):
            cname = text(ctx.find("name"))
            for msg in ctx.findall("message"):
                src_el = msg.find("source")
                tr_el = msg.find("translation")
                if src_el is None or tr_el is None:
                    continue
                src = text(src_el)
                old = text(tr_el)
                en = english.get((cname, src), "")
                new = translate_one(src, old, en, lang)
                if new != old:
                    set_text(tr_el, new)
                    changed += 1
                if len(new) > 18 and EN_WORDS.search(new):
                    residuals.append((lang, cname, src[:70], new[:120]))

        ET.indent(tree, space=" ")
        tree.write(path, encoding="utf-8", xml_declaration=True)
        total_changed += changed
        print(f"{lang}: {changed} entrée(s) corrigée(s)")

    # Le rapport permet de voir les vrais résidus sans produire silencieusement un faux succès.
    report = ROOT / "translation_residuals.txt"
    report.write_text("\n".join(f"{a}\t{b}\t{c}\t{d}" for a,b,c,d in residuals), encoding="utf-8")
    print(f"Total corrigé: {total_changed}; résidus anglais potentiels: {len(residuals)}")


if __name__ == "__main__":
    main()
