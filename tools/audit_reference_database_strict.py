#!/usr/bin/env python3
import pathlib
import re
import sys
import xml.etree.ElementTree as ET

import audit_reference_database as base

LANGUAGES = base.LANGUAGES
LOCALIZED_XML_RE = re.compile(r"^(?P<base>.+?)(?:\.(?P<lang>fr|en|es|it|pt|de))?\.xml\.qz64$", re.I)


def xml_text_slot_count(path: pathlib.Path, expected_language: str, violations: list[str]) -> int:
    try:
        raw = base.unpack_qz64_bytes(path.read_bytes(), path.as_posix())
        root = ET.fromstring(raw.decode("utf-8"))
    except Exception as exc:
        violations.append(f"XML_INVALID|{path.name}|{exc}")
        return -1

    declared = (root.attrib.get("lang") or root.attrib.get("{http://www.w3.org/XML/1998/namespace}lang") or "").lower()
    if expected_language != "fr" and declared != expected_language:
        violations.append(f"XML_LANGUAGE_MARKER|{path.name}|expected={expected_language}|declared={declared or 'none'}")
    if declared and declared not in LANGUAGES:
        violations.append(f"XML_LANGUAGE_MARKER|{path.name}|unsupported={declared}")

    # Compare structural text slots, not vocabulary. A one-word translation such
    # as "Ground" or "Sensormasse" must not be mistaken for a technical token.
    # Counting every populated user-visible slot guarantees that no paragraph,
    # cell, label or selected descriptive attribute disappeared in translation.
    count = 0
    for element in root.iter():
        if element.text and element.text.strip():
            count += 1
        for key, value in element.attrib.items():
            if key.lower() in {"titre", "title", "description", "fonction", "function", "note", "label"} and value.strip():
                count += 1
    return count


def audit_localized_xml(root: pathlib.Path, violations: list[str]) -> tuple[int, int]:
    files = sorted((root / "fiches").glob("*.xml.qz64"))
    if not files:
        violations.append("XML|Aucune fiche XML MEMS")
        return 0, 0

    groups: dict[str, dict[str, pathlib.Path]] = {}
    counts: dict[tuple[str, str], int] = {}
    total_slots = 0

    for path in files:
        match = LOCALIZED_XML_RE.match(path.name)
        if not match:
            violations.append(f"XML_LANGUAGE_FILENAME|{path.name}")
            continue
        stem = match.group("base")
        language = (match.group("lang") or "fr").lower()
        variants = groups.setdefault(stem, {})
        if language in variants:
            violations.append(f"XML_LANGUAGE_DUPLICATE|{stem}|{language}")
            continue
        variants[language] = path
        count = xml_text_slot_count(path, language, violations)
        counts[(stem, language)] = count
        if count > 0:
            total_slots += count

    expected_stems = {"mems_1_2", "mems_1_3", "mems_1_6", "mems_1_9"}
    for stem in sorted(expected_stems - set(groups)):
        violations.append(f"XML_LANGUAGE_MISSING_FICHE|{stem}")

    localized_slots = 0
    for stem in sorted(groups):
        variants = groups[stem]
        missing = sorted(set(LANGUAGES) - set(variants))
        if missing:
            violations.append(f"XML_LANGUAGE_MISSING|{stem}|missing={','.join(missing)}")
            continue

        reference = counts.get((stem, "fr"), -1)
        if reference <= 0:
            violations.append(f"XML_LANGUAGE_EMPTY|{stem}|fr")
            continue

        family_ok = True
        for language in LANGUAGES:
            count = counts.get((stem, language), -1)
            if count <= 0:
                violations.append(f"XML_LANGUAGE_EMPTY|{stem}|{language}")
                family_ok = False
            elif count != reference:
                violations.append(f"XML_LANGUAGE_COVERAGE|{stem}|{language}|slots={count}|fr={reference}")
                family_ok = False
        if family_ok:
            localized_slots += reference * len(LANGUAGES)

    return total_slots, localized_slots


def main() -> int:
    root = pathlib.Path("database/reference")
    violations: list[str] = []
    db_path = None
    try:
        db_path = base.build_database(root)
        stats = base.audit_sqlite(db_path, violations)
        xml_slots, xml_localized = audit_localized_xml(root, violations)
        asset_count, svg_human = base.audit_assets(root, violations)

        language_prefixes = ("LANG_", "XML_LANGUAGE", "SVG_LANGUAGE")
        language_violations = [v for v in violations if v.startswith(language_prefixes)]
        structural_violations = [v for v in violations if not v.startswith(language_prefixes)]

        print("AUDIT MEMS REFERENCE DATABASE — STRICT SIX LANGUAGES")
        print(f"SQLITE tables={stats['tables']} rows={stats['rows']}")
        print(f"LANGUAGE families={stats['language_families']} incomplete_families={stats['incomplete_language_families']}")
        print(f"LANGUAGE legacy_cells={stats['legacy_translatable_cells']} unique_values={stats['legacy_translatable_unique']}")
        print(f"XML text_slots={xml_slots} localized_family_slots={xml_localized}")
        print(f"ASSETS files={asset_count} svg_human_text_nodes={svg_human}")
        print(f"VIOLATIONS structural={len(structural_violations)} language={len(language_violations)} total={len(violations)}")
        for item in violations:
            print("AUDIT_VIOLATION", item)

        if structural_violations:
            return 2
        if language_violations:
            return 3
        return 0
    except Exception as exc:
        print(f"AUDIT_FATAL {exc}", file=sys.stderr)
        return 4
    finally:
        if db_path and db_path.exists():
            db_path.unlink()


if __name__ == "__main__":
    raise SystemExit(main())
