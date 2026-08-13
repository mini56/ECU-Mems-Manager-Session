#!/usr/bin/env python3
from pathlib import Path
import json
import re
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
UI = ROOT / "mainwindow.ui"
TR = ROOT / "translations"


def node_text(el):
    return "" if el is None else "".join(el.itertext())


def load_ts(lang):
    root = ET.parse(TR / f"ECUMemsManager_{lang}.ts").getroot()
    out = {}
    for ctx in root.findall("context"):
        cname = node_text(ctx.find("name")) or "Global"
        for msg in ctx.findall("message"):
            src = node_text(msg.find("source"))
            val = node_text(msg.find("translation")) or src
            if src:
                out[(cname, src)] = val
                out[("*", src)] = val
    return out


en = load_ts("en")
fr = load_ts("fr")


def read_json(path):
    if not path.exists():
        return {}
    return json.loads(path.read_text(encoding="utf-8"))

out_en = read_json(TR / "en.json")
out_fr = read_json(TR / "fr.json")

raw = UI.read_text(encoding="utf-8")
root = ET.fromstring(raw)

range_by_object = {}
main_tab = root.find(".//widget[@class='QTabWidget'][@name='Tab_main']")
if main_tab is None:
    raise SystemExit("Tab_main introuvable")

for page_index, page in enumerate(main_tab.findall("widget"), start=1):
    base = page_index * 1000
    for obj in page.iter("widget"):
        name = obj.get("name")
        if name:
            range_by_object[name] = base
    page_name = page.get("name")
    if page_name:
        range_by_object[page_name] = base

next_key = {0: 100}
for i in range(1, 20):
    next_key[i * 1000] = i * 1000 + 1

used = {int(k) for k in out_en.keys() if str(k).isdigit()}


def alloc(base):
    k = next_key.setdefault(base, base + 1)
    while k in used:
        k += 1
    if base and k >= base + 1000:
        raise SystemExit(f"Plage {base}xxx pleine")
    used.add(k)
    next_key[base] = k + 1
    return k

parent = {c: p for p in root.iter() for c in p}

def owning_widget(el):
    cur = el
    while cur is not None:
        if cur.tag == "widget":
            return cur
        cur = parent.get(cur)
    return None

count = 0
for string_el in root.iter("string"):
    src = node_text(string_el)
    if not src or src.startswith("@"):
        continue
    owner = owning_widget(string_el)
    objname = owner.get("name") if owner is not None else ""
    base = range_by_object.get(objname, 0)
    key = alloc(base)
    english = en.get(("MainWindow", src), en.get(("*", src), src))
    french = fr.get(("MainWindow", src), fr.get(("*", src), src))
    out_en[str(key)] = english
    out_fr[str(key)] = french
    string_el.set("comment", f"EN: {english}")
    string_el.text = f"@{key:04d}"
    for child in list(string_el):
        string_el.remove(child)
    count += 1

ET.indent(root, space=" ")
UI.write_text("<?xml version='1.0' encoding='utf-8'?>\n" + ET.tostring(root, encoding="unicode") + "\n", encoding="utf-8")
(TR / "en.json").write_text(json.dumps(out_en, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
(TR / "fr.json").write_text(json.dumps(out_fr, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

capture = ROOT / "captureviewer.cpp"
capture_count = 0
if capture.exists():
    source = capture.read_text(encoding="utf-8")
    pattern = re.compile(r'\btr\(\s*"(?:\\.|[^"\\])*"\s*\)')
    matches = list(pattern.finditer(source))
    if matches:
        expected = 11
        if len(matches) != expected:
            raise SystemExit(f"CaptureViewer tr count unexpected: {len(matches)}")
        english_capture = read_json(TR / "en_capture.json")
        nonlocal_index = [0]
        def replace_capture(match):
            nonlocal_index[0] += 1
            key = 6299 + nonlocal_index[0]
            comment = english_capture.get(str(key), "")
            suffix = f" /* EN: {comment} */" if comment else ""
            return f"I18n::text({key}){suffix}"
        source = pattern.sub(replace_capture, source)
        capture.write_text(source, encoding="utf-8")
        capture_count = expected

cmake = ROOT / "CMakeLists.txt"
cmake_changed = False
if cmake.exists():
    cmake_text = cmake.read_text(encoding="utf-8")
    old = '''# Runtime language files. EN/FR numeric dictionaries are the new system.\n# Legacy TS files remain temporarily only while the remaining UI is migrated.\nadd_custom_command(TARGET ${PNAME} POST_BUILD\n    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PNAME}>/translations"\n    COMMAND ${CMAKE_COMMAND} -E copy_if_different\n        "${CMAKE_SOURCE_DIR}/translations/en.json"\n        "${CMAKE_SOURCE_DIR}/translations/fr.json"\n        "${CMAKE_SOURCE_DIR}/translations/ECUMemsManager_fr.ts"\n        "${CMAKE_SOURCE_DIR}/translations/ECUMemsManager_en.ts"\n        "$<TARGET_FILE_DIR:${PNAME}>/translations"\n)'''
    new = '''# Runtime language files. Deploy every keyed JSON module automatically.\nfile(GLOB KEYED_TRANSLATION_JSON "${CMAKE_SOURCE_DIR}/translations/*.json")\nadd_custom_command(TARGET ${PNAME} POST_BUILD\n    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PNAME}>/translations"\n    COMMAND ${CMAKE_COMMAND} -E copy_if_different\n        ${KEYED_TRANSLATION_JSON}\n        "${CMAKE_SOURCE_DIR}/translations/ECUMemsManager_fr.ts"\n        "${CMAKE_SOURCE_DIR}/translations/ECUMemsManager_en.ts"\n        "$<TARGET_FILE_DIR:${PNAME}>/translations"\n)'''
    if old in cmake_text:
        cmake.write_text(cmake_text.replace(old, new, 1), encoding="utf-8")
        cmake_changed = True

print(f"UI migrated: {count} strings")
print(f"CaptureViewer migrated: {capture_count} strings")
print(f"CMake translation deploy patched: {cmake_changed}")
print(f"EN keys: {len(out_en)}")
print(f"FR keys: {len(out_fr)}")
if set(out_en) != set(out_fr):
    raise SystemExit("EN/FR key mismatch")
