#!/usr/bin/env python3
"""Regression checks for IA MEMS unified knowledge selection.

This mirrors the deterministic pre-Qwen filtering contract:
- technical acronyms are token-safe;
- scope qualifiers cannot make an unrelated fact relevant;
- explicit incompatible SPi/MPi and Japan/Europe/UK scopes are rejected;
- packaged documents/images are discoverable without C++ filename switches.
"""

from __future__ import annotations
import argparse
import re
import sqlite3
import sys
import unicodedata
from pathlib import Path

TOKEN_TERMS = {"ect","iat","map","tps","ckp","cmp","iac","iacv","obd","spi","mpi","rpm","pin"}
SCOPE_TERMS = {
    "spi","mpi","monopoint","multipoint","japan","japon","europe","europeen","uk",
    "automatic","automatique","manual","manuelle",
    "1993","1994","1995","1996","1997","1998","1999","2000",
}
PIN_GENERIC = {"broche","broches","pinout","connecteur","connector","cablage","wiring","pin","prise"}
STOP = {
    "que","quoi","sur","les","des","une","dans","pour","avec","est","sais","sait","peux",
    "dire","cette","ce","cet","mon","ma","mes","mems","source","sources","preuve","preuves",
    "document","documentation",
}

def normalize(text: str) -> str:
    decomposed = unicodedata.normalize("NFD", (text or "").lower().strip())
    text = "".join(ch for ch in decomposed if not unicodedata.combining(ch))
    return re.sub(r"\s+", " ", text).strip()

def word(text: str, token: str) -> bool:
    return re.search(r"(^|[^a-z0-9_])" + re.escape(token) + r"([^a-z0-9_]|$)", text) is not None

def term_match(text: str, term: str) -> bool:
    return word(text, term) if term in TOKEN_TERMS or len(term) <= 3 else term in text

def terms(question: str) -> list[str]:
    text = re.sub(r"[^a-z0-9_.-]+", " ", normalize(question))
    result: list[str] = []
    for token in text.split():
        if len(token) < 3 or token in STOP:
            continue
        if token not in result:
            result.append(token)
        if len(result) >= 7:
            break
    original = list(result)
    for token in original:
        extras: tuple[str, ...] = ()
        if token == "bobine": extras = ("dwell","coil","allumage")
        elif token == "papillon": extras = ("tps","throttle")
        elif token == "ralenti": extras = ("iac","iacv","idle")
        elif token in ("liquide","refroidissement"): extras = ("ect","coolant")
        elif token == "temperature": extras = ("ect","iat")
        elif token == "admission": extras = ("iat",)
        elif token == "vilebrequin": extras = ("ckp",)
        elif token == "cames": extras = ("cmp",)
        elif token == "lambda": extras = ("oxygen",)
        elif token in ("broche","connecteur","obd"): extras = ("wiring","pin")
        for extra in extras:
            if extra not in result and len(result) < 12:
                result.append(extra)
    return result

def scope_request(question: str) -> dict[str, str]:
    text = normalize(question)
    out = {"induction":"","market":"","transmission":""}
    if word(text,"spi") or "monopoint" in text: out["induction"] = "spi"
    elif word(text,"mpi") or "multipoint" in text: out["induction"] = "mpi"
    if "japon" in text or "japan" in text: out["market"] = "japan"
    elif word(text,"uk") or "royaume uni" in text or "britannique" in text: out["market"] = "uk"
    elif "europe" in text or "europeen" in text: out["market"] = "europe"
    if "automatique" in text or "automatic" in text: out["transmission"] = "automatic"
    elif "manuelle" in text or word(text,"manual"): out["transmission"] = "manual"
    return out

def scope_note(db: sqlite3.Connection, key: str) -> str:
    rows = db.execute("""
        SELECT s.scope_kind,s.make,s.model,s.engine_family,s.engine_code,s.engine_variant,
               s.induction,s.mems_family,s.transmission,s.year_from,s.year_to,s.vin_from,
               s.vin_to,s.market,s.compression_variant,s.catalyst_state,
               s.air_conditioning_state,s.source_scope_text
        FROM mems_knowledge_scope ks
        JOIN mems_applicability_scope s ON s.scope_key=ks.scope_key
        WHERE ks.knowledge_key=?
        ORDER BY s.scope_key
    """,(key,)).fetchall()
    if not rows:
        return "Portee: non precisee (UNKNOWN)"
    scopes = []
    for row in rows:
        scopes.append(" | ".join(str(x) for x in row if x not in (None,"")))
    return "Portee UNKNOWN: " + " ; ".join(scopes)

def compatible(note: str, request: dict[str,str]) -> bool:
    scope = normalize(note)
    spi, mpi = word(scope,"spi"), word(scope,"mpi")
    if request["induction"] == "spi" and mpi and not spi: return False
    if request["induction"] == "mpi" and spi and not mpi: return False
    japan = "japan" in scope or "japon" in scope
    europe = "europe" in scope
    uk = word(scope,"uk") or "royaume uni" in scope
    if request["market"] == "japan" and (europe or uk) and not japan: return False
    if request["market"] in ("europe","uk") and japan and not europe and not uk: return False
    automatic = "automatic" in scope or "automatique" in scope
    manual = word(scope,"manual") or "manuelle" in scope
    if request["transmission"] == "automatic" and manual and not automatic: return False
    if request["transmission"] == "manual" and automatic and not manual: return False
    return True

def scope_bonus(note: str, request: dict[str,str]) -> int:
    scope = normalize(note)
    score = 0
    if request["induction"] and word(scope, request["induction"]): score += 4
    if request["market"] == "japan" and ("japan" in scope or "japon" in scope): score += 4
    if request["transmission"] == "automatic" and ("automatic" in scope or "automatique" in scope): score += 3
    if request["transmission"] == "manual" and (word(scope,"manual") or "manuelle" in scope): score += 3
    return score

def rank(db: sqlite3.Connection, question: str, pinout: bool=False) -> list[tuple[int,str]]:
    qterms = terms(question)
    specific = [t for t in qterms if t not in SCOPE_TERMS and not (pinout and t in PIN_GENERIC)]
    if not specific: specific = qterms
    request = scope_request(question)
    rows = db.execute("""
        SELECT knowledge_key,domain,knowledge_type,topic,component_key,source_text,verification_level,notes
        FROM mems_knowledge_item ORDER BY knowledge_key
    """).fetchall()
    ranked: list[tuple[int,str]] = []
    for key,domain,kind,topic,component,statement,level,notes in rows:
        note = scope_note(db,key) + (" | " + notes if notes else "")
        if not compatible(note,request): continue
        identity = normalize(f"{key} {topic or ''} {component or ''} {domain or ''} {kind or ''}")
        body = normalize(statement or "")
        searchable = identity + " " + body
        if pinout and not any(x in searchable for x in ("wiring","broche","pinout","connector","connecteur","socket","diagnostic_socket","obd","c159","c549","prise")):
            continue
        score = 0
        matches = 0
        for term in specific:
            if term_match(identity,term):
                score += 5; matches += 1
            elif term_match(body,term):
                score += 2; matches += 1
        if not matches: continue
        score += min(matches,4)*3
        if level == "verifie_constructeur": score += 8
        score += scope_bonus(note,request)
        if pinout: score += 16
        ranked.append((score,key))
    ranked.sort(key=lambda x:(-x[0],x[1]))
    return ranked

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("database", type=Path)
    ap.add_argument("reference_root", type=Path)
    args = ap.parse_args()
    db = sqlite3.connect(str(args.database))
    try:
        assert db.execute("PRAGMA integrity_check").fetchone()[0].lower() == "ok"
        assert db.execute("PRAGMA user_version").fetchone()[0] == 20
        assert db.execute("SELECT COUNT(*) FROM mems_knowledge_item").fetchone()[0] == 93

        assert not term_match(normalize("injector wiring"), "ect")

        ect = rank(db, "couple serrage ECT SPi Japan 1997")
        assert ect and ect[0][1] == "KNOW-RAVE-REP-ECT-001", ect[:5]

        mpi_map = rank(db, "broche MAP MPi 1997", pinout=True)
        assert mpi_map and mpi_map[0][1] == "KNOW-RAVE-WIR-MPI-009", mpi_map[:5]

        spi_map = rank(db, "broche MAP SPi Japan 1997", pinout=True)
        assert not spi_map, spi_map[:5]

        europe = rank(db, "broche MAP Europe 1997", pinout=True)
        assert all("SPIJ" not in key for _,key in europe[:10]), europe[:10]

        image_ext = {"svg","png","jpg","jpeg","webp","gif"}
        doc_ext = {"pdf","html","htm","md","txt","csv","xml"}
        assets = [p for p in args.reference_root.rglob("*")
                  if p.is_file() and p.name != "manifest.json"
                  and p.suffix.lower().lstrip(".") in image_ext | doc_ext]
        assert any(p.suffix.lower().lstrip(".") in image_ext for p in assets)
        assert any(p.suffix.lower().lstrip(".") in doc_ext for p in assets)

        print("IA_UNIFIED_KNOWLEDGE_PASS")
        print(f"KNOWLEDGE_ITEMS={len(db.execute('SELECT knowledge_key FROM mems_knowledge_item').fetchall())}")
        print(f"PACKAGED_ASSETS={len(assets)}")
        print(f"ECT_TOP={ect[0][1]}")
        print(f"MPI_MAP_TOP={mpi_map[0][1]}")
        print("SPI_MAP_RESULT=NONE")
        return 0
    except AssertionError as exc:
        print(f"IA_UNIFIED_KNOWLEDGE_FAIL: {exc}", file=sys.stderr)
        return 1
    finally:
        db.close()

if __name__ == "__main__":
    raise SystemExit(main())
