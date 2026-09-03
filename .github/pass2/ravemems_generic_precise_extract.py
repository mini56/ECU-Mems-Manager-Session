#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import sqlite3
import sys
from pathlib import Path
from typing import Any

import fitz

import rcl0193eng_precise_visual_extract as precise
from generic_document_rules import canonical_publication_token, document_kind_for_evidence, is_bare_calendar_date_identifier
from source_input import discover_pdf_sources, probe_pdf

pe = precise.pe

class GenericSemanticParser(precise.pass2.Pass2SemanticParser):
    def _operation_code(self, text: str) -> str | None:
        code = super()._operation_code(text)
        if code and is_bare_calendar_date_identifier(text, code): return None
        return code

pe.SemanticParser = GenericSemanticParser
_REFERENCE_IDENTIFIER = re.compile(r"^[A-Z]{1,4}(?:[-/]?[A-Z0-9]{0,3})?\s*\d{1,4}[A-Z]?$", re.IGNORECASE)
_COLUMN_LABELS = {"cav col","cct","cav","col"}
_TITLE_PATTERN_BY_KIND = {
    "electrical_reference_library": re.compile(r"electrical\s+(?:reference\s+)?library", re.IGNORECASE),
    "workshop_manual": re.compile(r"workshop\s+manual", re.IGNORECASE),
    "technical_bulletin": re.compile(r"(?:technical|service)\s+bulletin", re.IGNORECASE),
    "owner_handbook": re.compile(r"owner(?:['’]s|s)?\s+handbook", re.IGNORECASE),
}

def _argument_value(name: str, required: bool = True) -> str | None:
    try: index = sys.argv.index(name)
    except ValueError:
        if required: raise SystemExit(f"missing required argument {name}")
        return None
    if index + 1 >= len(sys.argv): raise SystemExit(f"missing value for argument {name}")
    return sys.argv[index + 1]

def _remove_option(name: str) -> None:
    while name in sys.argv:
        i = sys.argv.index(name); del sys.argv[i:i+2]

def _set_option(name: str, value: str) -> None:
    _remove_option(name); sys.argv.extend([name, value])

def _early_source_evidence(pdf_path: Path, page_limit: int = 20) -> tuple[str, dict[str, Any]]:
    with fitz.open(pdf_path) as doc:
        metadata = dict(doc.metadata or {})
        parts = [doc[i].get_text("text", sort=True) for i in range(min(doc.page_count, page_limit))]
    return "\n".join(parts), metadata

def _document_title_from_evidence(evidence_text: str, metadata: dict[str, Any], document_kind: str, fallback: str) -> str:
    pattern = _TITLE_PATTERN_BY_KIND.get(document_kind)
    if pattern:
        for raw in evidence_text.splitlines():
            line = re.sub(r"\s+", " ", raw).strip()
            if line and pattern.search(line): return line
    metadata_title = re.sub(r"\s+", " ", str(metadata.get("title") or "")).strip()
    if metadata_title: return metadata_title
    for raw in evidence_text.splitlines():
        line = re.sub(r"\s+", " ", raw).strip()
        if 3 <= len(line) <= 180 and any(ch.isalpha() for ch in line): return line
    return fallback

def _publication_match_regex(publication_code: str | None) -> str:
    if not publication_code: return r"(?s)(?=.)"
    m = re.fullmatch(r"([A-Z]+)(\d+)([A-Z]*)", publication_code.upper())
    if m:
        prefix, digits, suffix = m.groups()
        return rf"\b{re.escape(prefix)}\s*{re.escape(digits)}{re.escape(suffix)}\b"
    return re.escape(publication_code)

def _language_profile_path(language: str) -> Path:
    root = Path(__file__).resolve().parents[2] / "ravemems" / "v2" / "profiles"
    exact = root / f"language_{language}.json"
    return exact if exact.exists() else root / "language_generic.json"

def _prepare_runtime_profile(pdf_path: Path, out: Path) -> tuple[Path, dict[str, Any]]:
    probe = probe_pdf(pdf_path)
    profile = json.loads(_language_profile_path(str(probe["language"])).read_text(encoding="utf-8"))
    profile["profile_id"] = f"runtime_{profile.get('profile_id','generic')}"
    profile["language"] = str(probe["language"])
    profile["publication_code_regex"] = _publication_match_regex(probe.get("publication_code"))
    path = out / "_runtime_profile.json"
    path.write_text(json.dumps(profile, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return path, probe

def _resolve_single_pdf(source: Path, out: Path) -> Path:
    pdfs = discover_pdf_sources(source, out / "_resolved_source")
    if len(pdfs) != 1: raise SystemExit(f"single-document extractor received {len(pdfs)} PDFs; use ravemems_extract_source.py")
    return pdfs[0]

def _prepare_legacy_arguments() -> tuple[Path, Path, dict[str, Any], str]:
    out = Path(str(_argument_value("--out"))).resolve(); out.mkdir(parents=True, exist_ok=True)
    source_arg = _argument_value("--source", False); pdf_arg = _argument_value("--pdf", False)
    if source_arg and pdf_arg: raise SystemExit("use either --source or --pdf, not both")
    if source_arg:
        pdf_path = _resolve_single_pdf(Path(source_arg), out); _remove_option("--source")
    elif pdf_arg: pdf_path = Path(pdf_arg).expanduser().resolve()
    else: raise SystemExit("missing required argument --source (or compatibility --pdf)")
    runtime_profile, probe = _prepare_runtime_profile(pdf_path, out)
    _set_option("--pdf", str(pdf_path)); _set_option("--profile", str(runtime_profile))
    source_relative = _argument_value("--source-relative-path", False)
    if not source_relative: source_relative = pdf_path.name; _set_option("--source-relative-path", source_relative)
    if not _argument_value("--source-commit", False): _set_option("--source-commit", "external-source")
    if not _argument_value("--source-blob-sha", False): _set_option("--source-blob-sha", str(probe["source_sha256"]))
    return out, pdf_path, probe, source_relative

def _rekey_database(db_path: Path, identity: str) -> tuple[str,str]:
    token = canonical_publication_token(identity); dk=f"DOC_{token}"; rk=f"REV_{token}_SOURCE"
    db=sqlite3.connect(db_path); db.execute("PRAGMA foreign_keys=OFF")
    old_d=db.execute("SELECT document_key FROM ravemems_document").fetchone()[0]
    old_r=db.execute("SELECT revision_key FROM ravemems_document_revision").fetchone()[0]
    tables=[r[0] for r in db.execute("SELECT name FROM sqlite_master WHERE type='table' AND name LIKE 'ravemems_%'")]
    for table in tables:
        q='"'+table.replace('"','""')+'"'; cols={r[1] for r in db.execute(f"PRAGMA table_info({q})")}
        if "document_key" in cols and old_d != dk: db.execute(f"UPDATE {q} SET document_key=? WHERE document_key=?",(dk,old_d))
        if "revision_key" in cols and old_r != rk: db.execute(f"UPDATE {q} SET revision_key=? WHERE revision_key=?",(rk,old_r))
    db.commit(); db.execute("PRAGMA foreign_keys=ON")
    integrity=db.execute("PRAGMA integrity_check").fetchone()[0]; fk=db.execute("PRAGMA foreign_key_check").fetchall(); db.close()
    if integrity!="ok" or fk: raise RuntimeError(f"document rekey integrity failure: {integrity!r} {fk!r}")
    return dk,rk

def _meaningful_reference_line(text: str) -> bool:
    compact=re.sub(r"\s+"," ",text).strip()
    if len(compact)<3 or len(compact)>180 or compact.casefold() in _COLUMN_LABELS or not any(c.isalpha() for c in compact): return False
    if re.fullmatch(r"MINI\s+\d{2}MY",compact,re.I): return False
    return True

def _reference_entry_from_page(page: fitz.Page) -> tuple[str|None,str|None,str|None,str|None]:
    lines=pe.read_lines(page)
    if not lines: return None,None,None,None
    identifier=None; category=None; title_item=None
    for item in lines:
        text=re.sub(r"\s+"," ",str(item.get("text",""))).strip(); bbox=item.get("bbox",(0,0,0,0))
        if not text: continue
        in_header=item.get("reading_region")=="header" or float(bbox[1])<=float(page.rect.height)*0.12
        if in_header and identifier is None and _REFERENCE_IDENTIFIER.fullmatch(text): identifier=text; continue
        if in_header and category is None and _meaningful_reference_line(text) and not _REFERENCE_IDENTIFIER.fullmatch(text): category=text
    for item in lines:
        if item.get("reading_region") in {"header","footer"}: continue
        text=re.sub(r"\s+"," ",str(item.get("text",""))).strip()
        if _meaningful_reference_line(text) and not (identifier and text.casefold()==identifier.casefold()): title_item=item; break
    if title_item is not None:
        title=re.sub(r"\s+"," ",str(title_item["text"])).strip(); return identifier,title,pe.bbox_json(title_item["bbox"]),category
    if category:
        for item in lines:
            text=re.sub(r"\s+"," ",str(item.get("text",""))).strip()
            if text==category: return identifier,category,pe.bbox_json(item["bbox"]),category
    return identifier,None,None,category

def _build_page_grounded_sections(pdf_path: Path, db_path: Path, revision_key: str, token: str, language: str, kind: str) -> dict[str,int]:
    db=sqlite3.connect(db_path); existing=int(db.execute("SELECT COUNT(*) FROM ravemems_section").fetchone()[0])
    if existing: db.close(); return {"section_count":existing,"page_coverage_count":0,"identified_entry_count":0,"titled_entry_count":0}
    page_keys={int(p):k for k,p in db.execute("SELECT page_key,physical_page FROM ravemems_page ORDER BY physical_page")}
    count=identified=titled=coverage=0
    with fitz.open(pdf_path) as doc:
        for physical in range(1,doc.page_count+1):
            page_key=page_keys.get(physical)
            if not page_key: continue
            identifier,title,bbox,category=_reference_entry_from_page(doc[physical-1]); source_title=title or category
            if not source_title: continue
            count+=1; coverage+=1; identified+=bool(identifier); titled+=bool(title)
            sk=f"SEC_{token}_P{physical:04d}"; section_kind={"electrical_reference_library":"electrical_reference_entry","owner_handbook":"owner_handbook_page","technical_bulletin":"technical_bulletin_page"}.get(kind,"document_page")
            db.execute("INSERT INTO ravemems_section(section_key,revision_key,parent_section_key,sequence_no,section_kind,manufacturer_identifier,title_source,source_language) VALUES(?,?,?,?,?,?,?,?)",(sk,revision_key,None,physical,section_kind,identifier,source_title,language))
            db.execute("INSERT INTO ravemems_provenance(provenance_key,entity_kind,entity_key,revision_key,page_key,source_element_key,source_bbox_json,source_text_sha256) VALUES(?,?,?,?,?,?,?,?)",(f"PROV_{sk}","section",sk,revision_key,page_key,f"page:{physical}:document-entry",bbox,pe.sha256_bytes(source_title.encode("utf-8"))))
    db.commit(); integrity=db.execute("PRAGMA integrity_check").fetchone()[0]; fk=db.execute("PRAGMA foreign_key_check").fetchall(); db.close()
    if integrity!="ok" or fk: raise RuntimeError(f"section integrity failure: {integrity!r} {fk!r}")
    return {"section_count":count,"page_coverage_count":coverage,"identified_entry_count":identified,"titled_entry_count":titled}

def main() -> int:
    out,pdf_path,probe,source_relative=_prepare_legacy_arguments(); result=pe.main()
    if result!=0: return result
    db_path=out/"ravemems_v2_rcl0193eng.sqlite"; manifest_path=out/"manifest.json"; manifest=json.loads(manifest_path.read_text(encoding="utf-8"))
    identity=str(probe["identity"]); publication=probe.get("publication_code"); token=canonical_publication_token(identity); dk,rk=_rekey_database(db_path,identity)
    evidence,metadata=_early_source_evidence(pdf_path); kind=document_kind_for_evidence(source_relative,evidence,str(metadata.get("title") or "")); title=_document_title_from_evidence(evidence,metadata,kind,publication or identity); language=str(probe["language"])
    db=sqlite3.connect(db_path); db.execute("UPDATE ravemems_document SET canonical_name=?,source_language=?,document_kind=?,title_source=?",(title,language,kind,title)); db.execute("UPDATE ravemems_document_revision SET source_relative_path=?,source_blob_sha=?,source_sha256=?,page_count=?",(source_relative,str(probe["source_sha256"]),str(probe["source_sha256"]),int(probe["page_count"]))); db.commit(); db.close()
    structure=None
    if kind!="workshop_manual": structure=_build_page_grounded_sections(pdf_path,db_path,rk,token,language,kind)
    db=sqlite3.connect(db_path); manifest["publication_code"]=publication; manifest["publication_detected"]=bool(probe["publication_detected"]); manifest["document_identity"]=identity; manifest["document_key"]=dk; manifest["revision_key"]=rk; manifest["source_language"]=language; manifest["document_kind"]=kind; manifest["document_title_source"]=title; manifest["source_sha256"]=str(probe["source_sha256"]); manifest["source_input"]={"classification_uses_source_name":False,"physical_name":pdf_path.name,"publication_detection":"content" if publication else "sha256_fallback","language_detection":language}
    if structure is not None: manifest["reference_structure"]=structure
    manifest.setdefault("counts",{})["ravemems_section"]=db.execute("SELECT COUNT(*) FROM ravemems_section").fetchone()[0]; manifest["counts"]["ravemems_provenance"]=db.execute("SELECT COUNT(*) FROM ravemems_provenance").fetchone()[0]; manifest["sqlite_integrity"]=db.execute("PRAGMA integrity_check").fetchone()[0]; manifest["foreign_key_issue_count"]=len(db.execute("PRAGMA foreign_key_check").fetchall()); db.close(); manifest_path.write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
    print("DOCUMENT_IDENTITY",identity); print("PUBLICATION_CODE",publication or "UNRESOLVED"); print("SOURCE_LANGUAGE",language); print("DOCUMENT_KEY",dk); print("REVISION_KEY",rk); print("DOCUMENT_KIND",kind)
    if structure is not None: print("DOCUMENT_REFERENCE_STRUCTURE",json.dumps(structure,sort_keys=True))
    return 0

if __name__=="__main__": raise SystemExit(main())
