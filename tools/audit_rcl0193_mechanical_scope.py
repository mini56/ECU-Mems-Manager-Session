#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path

import fitz

EXPECTED_SHA256 = "c050a3eebe50c5a85bf8a69b7722bd2052079944e09d58578a498984ecf06715"
EXPECTED_PAGE_COUNT = 372
EXPECTED_CURRENT_PAGES = {
    38, 39, 40, 98, 101, 107, 108, 109, 112, 113, 114, 117, 118,
    120, 121, 122, 123, 125, 126, 127, 128, 129, 130, 131, 135, 170,
}

ENGINE_RELATED_SECTIONS = (
    "INFORMATION",
    "ENGINE",
    "EMISSION CONTROL",
    "ENGINE MANAGEMENT SYSTEM - MEMS",
    "FUEL DELIVERY SYSTEM",
    "COOLING SYSTEM",
    "MANIFOLD & EXHAUST SYSTEMS",
)

SENTINEL_TOKENS = (
    "ROCKER COVER GASKET",
    "FUEL PUMP",
    "COOLING SYSTEM COMPONENTS",
    "FLOW DIAGRAM",
    "COOLANT EXPANSION TANK",
    "EXHAUST SYSTEM COMPONENTS",
    "EXHAUST MANIFOLD COMPONENTS",
    "INLET MANIFOLD COMPONENTS",
)

def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()

def top_lines(text: str, limit: int = 8) -> list[str]:
    return [line.strip().upper() for line in text.splitlines() if line.strip()][:limit]

def page_section(text: str) -> str | None:
    lines = top_lines(text)
    for section in ENGINE_RELATED_SECTIONS:
        if section in lines:
            return section
    return None

def service_repair_numbers(text: str) -> list[str]:
    result = []
    for value in re.findall(r"Service\s+repair\s+no\s*-\s*([^\n\r]+)", text, flags=re.I):
        clean = " ".join(value.split()).strip(" .")
        if clean:
            result.append(clean)
    return result

def procedure_titles(text: str) -> list[str]:
    lines = [line.strip() for line in text.splitlines() if line.strip()]
    titles: list[str] = []
    for index, line in enumerate(lines):
        if not re.search(r"Service\s+repair\s+no\s*-", line, flags=re.I):
            continue
        for previous in range(index - 1, max(-1, index - 6), -1):
            candidate = lines[previous].strip()
            upper = candidate.upper()
            if (
                candidate
                and len(candidate) <= 100
                and upper not in {
                    "REPAIRS", "ADJUSTMENTS", "DESCRIPTION AND OPERATION",
                    "CONTENTS PAGE", "INFORMATION", "ENGINE", "EMISSION CONTROL",
                    "ENGINE MANAGEMENT SYSTEM - MEMS", "FUEL DELIVERY SYSTEM",
                    "COOLING SYSTEM", "MANIFOLD & EXHAUST SYSTEMS",
                }
            ):
                titles.append(candidate)
                break
    return titles

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--pdf", required=True)
    parser.add_argument("--reference", default="database/reference")
    parser.add_argument("--out-json", default="database/reference/audits/rcl0193_mechanical_scope_audit.json")
    parser.add_argument("--out-md", default="database/reference/audits/rcl0193_mechanical_scope_audit.md")
    args = parser.parse_args()

    pdf_path = Path(args.pdf)
    reference = Path(args.reference)
    digest = sha256_file(pdf_path)
    if digest != EXPECTED_SHA256:
        raise SystemExit(f"RCL0193 SHA-256 mismatch: {digest}")

    manifest = json.loads((reference / "manifest.json").read_text(encoding="utf-8-sig"))
    covered = {
        int(entry["pdf_index"])
        for entry in manifest.get("visual_assets", {}).values()
        if entry.get("document") == "RCL0193ENG" and isinstance(entry.get("pdf_index"), int)
    }
    if covered != EXPECTED_CURRENT_PAGES:
        raise SystemExit(f"Unexpected current RCL0193 visual set: {sorted(covered)}")

    doc = fitz.open(str(pdf_path))
    try:
        if doc.page_count != EXPECTED_PAGE_COUNT:
            raise SystemExit(f"Unexpected RCL0193 page count: {doc.page_count}")

        rows: list[dict] = []
        all_repairs: set[str] = set()
        all_titles: set[str] = set()
        sentinel_hits: dict[str, list[int]] = {token: [] for token in SENTINEL_TOKENS}

        for index in range(doc.page_count):
            page = doc[index]
            text = page.get_text("text")
            upper_text = text.upper()
            section = page_section(text)
            for token in SENTINEL_TOKENS:
                if token in upper_text:
                    sentinel_hits[token].append(index)
            if section is None:
                continue

            repairs = service_repair_numbers(text)
            titles = procedure_titles(text)
            all_repairs.update(repairs)
            all_titles.update(titles)
            image_count = len(page.get_images(full=True))
            vector_count = len(page.get_drawings())
            torque_count = len(re.findall(r"\b\d+(?:\.\d+)?\s*Nm\b", text, flags=re.I))
            graphic = image_count > 0 or vector_count >= 10

            rows.append({
                "pdf_index": index,
                "viewer_page": index + 1,
                "section": section,
                "captured": index in covered,
                "service_repair_numbers": repairs,
                "procedure_titles": titles,
                "torque_occurrences": torque_count,
                "embedded_images": image_count,
                "vector_drawings": vector_count,
                "graphic_detected": graphic,
            })

        scope_pages = {row["pdf_index"] for row in rows}
        captured_scope = scope_pages & covered
        uncaptured_scope = scope_pages - covered
        graphic_pages = {row["pdf_index"] for row in rows if row["graphic_detected"]}
        uncaptured_graphic = graphic_pages - covered

        by_section: dict[str, dict] = {}
        for section in ENGINE_RELATED_SECTIONS:
            items = [row for row in rows if row["section"] == section]
            if not items:
                continue
            pages = {row["pdf_index"] for row in items}
            repairs = {value for row in items for value in row["service_repair_numbers"]}
            by_section[section] = {
                "pages": len(pages),
                "first_pdf_index": min(pages),
                "last_pdf_index": max(pages),
                "captured": len(pages & covered),
                "uncaptured": len(pages - covered),
                "unique_service_repair_numbers": len(repairs),
                "torque_occurrences": sum(row["torque_occurrences"] for row in items),
                "graphic_pages": sum(1 for row in items if row["graphic_detected"]),
                "uncaptured_graphic_pages": sum(1 for row in items if row["graphic_detected"] and not row["captured"]),
            }

        sentinel_rows = []
        for token, hits in sentinel_hits.items():
            if not hits:
                continue
            preferred = next((value for value in hits if value not in covered), hits[0])
            page = doc[preferred]
            sentinel_rows.append({
                "token": token,
                "pdf_index": preferred,
                "viewer_page": preferred + 1,
                "already_captured": preferred in covered,
                "embedded_images": len(page.get_images(full=True)),
                "vector_drawings": len(page.get_drawings()),
            })

        result = {
            "document": "RCL0193ENG",
            "sha256": digest,
            "pdf_pages": doc.page_count,
            "historical_rcl0193_facts": 31,
            "current_rcl0193_visual_pages": len(covered),
            "current_visual_page_indices": sorted(covered),
            "audit_sections": list(ENGINE_RELATED_SECTIONS),
            "scope_pages": len(scope_pages),
            "scope_currently_captured_pages": len(captured_scope),
            "scope_currently_uncaptured_pages": len(uncaptured_scope),
            "unique_service_repair_numbers_in_scope": len(all_repairs),
            "unique_detected_procedure_titles_in_scope": len(all_titles),
            "torque_occurrences_in_scope": sum(row["torque_occurrences"] for row in rows),
            "graphic_pages_in_scope": len(graphic_pages),
            "uncaptured_pages_with_detected_graphics": len(uncaptured_graphic),
            "sections": by_section,
            "sentinel_examples": sentinel_rows,
        }

        out_json = Path(args.out_json)
        out_json.parent.mkdir(parents=True, exist_ok=True)
        out_json.write_text(json.dumps(result, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

        section_table = "\n".join(
            f"| {name} | {data['first_pdf_index']}-{data['last_pdf_index']} | {data['pages']} | {data['captured']} | {data['uncaptured']} | {data['unique_service_repair_numbers']} | {data['torque_occurrences']} | {data['graphic_pages']} | {data['uncaptured_graphic_pages']} |"
            for name, data in by_section.items()
        )
        sentinels = "\n".join(
            f"- PDF p.{row['pdf_index']} / viewer {row['viewer_page']} — `{row['token']}` — captured={row['already_captured']} — images={row['embedded_images']} — vectors={row['vector_drawings']}"
            for row in sentinel_rows
        )
        coverage = 100.0 * len(captured_scope) / len(scope_pages) if scope_pages else 0.0

        markdown = f"""# RCL0193ENG — audit mécanique / visuel hors extraction historique

Date: 2026-08-29

Source exacte: `RCL0193ENG`, SHA-256 `{digest}`, **{doc.page_count} pages**.

Cet audit ne modifie aucune donnée. Il mesure ce qui existe dans le manuel mais n'a pas encore été structuré dans la base.

## Conclusion

Les **31 faits RCL0193 historiques** ont bien leurs **26 pages visuelles actuelles**. La lacune n'est donc pas une perte de capture sur ces 31 faits.

Le manque est en amont: le manuel contient beaucoup d'informations mécaniques, procédures, couples et vues qui n'ont jamais été transformés en connaissances structurées.

Périmètre moteur/MEMS audité par en-têtes constructeur: `{", ".join(ENGINE_RELATED_SECTIONS)}`.

- pages détectées dans ce périmètre: **{len(scope_pages)}**;
- pages déjà couvertes par les 26 assets RCL0193 actuels: **{len(captured_scope)}**;
- pages non couvertes: **{len(uncaptured_scope)}**;
- couverture physique actuelle du périmètre: **{coverage:.2f}%**;
- numéros uniques `Service repair no` détectés: **{len(all_repairs)}**;
- titres de procédures détectés: **{len(all_titles)}**;
- occurrences de couples `Nm`: **{result['torque_occurrences_in_scope']}**;
- pages graphiques détectées: **{len(graphic_pages)}**;
- pages graphiques non couvertes: **{len(uncaptured_graphic)}**.

## Répartition par section

| Section | PDF indices | Pages | Capturées | Non capturées | Repair no | Couples Nm | Pages graphiques | Graphiques non capturées |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
{section_table}

## Exemples sentinelles retrouvés dans le manuel

{sentinels}

## Règle d'enrichissement

Ne pas transformer automatiquement chaque page en un fait. Extraire et structurer les composants, spécifications, valeurs, procédures, étapes, exigences, couples, avertissements et relations; conserver chaque vue/page originale une seule fois et la relier à ce qu'elle prouve.

Si un type de donnée mécanique ne peut pas être représenté correctement par le socle actuel, créer d'abord une structure additive adaptée. La base reste consultative et ne prend jamais la main sur la communication ECU.
"""
        Path(args.out_md).write_text(markdown, encoding="utf-8", newline="\n")
        print(json.dumps(result, ensure_ascii=False))
        return 0
    finally:
        doc.close()

if __name__ == "__main__":
    raise SystemExit(main())
