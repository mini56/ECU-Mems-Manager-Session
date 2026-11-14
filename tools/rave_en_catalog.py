#!/usr/bin/env python3
"""Add ALL remaining original English RAVE PDFs to one native EN SQLite.

The main RCL0193ENG workshop manual is imported separately; this module
adds the other 46 PDFs without translating, substituting or inventing text.
All extracted text is review_required; original pages are preserved as images.
"""
from __future__ import annotations
from collections import Counter
import hashlib
import json
from pathlib import Path
import sqlite3

import fitz


def add_remaining_rave_pdfs(db: sqlite3.Connection, main_rave: Path,
                            image_dir: Path) -> dict:
    root = main_rave.parent.parent
    files = sorted(p for p in root.rglob("*")
                   if p.is_file() and p.suffix.lower() == ".pdf")
    if len(files) != 47 or main_rave.resolve() not in {p.resolve() for p in files}:
        raise ValueError("RAVE_ORIGINAL_47_PDF_CORPUS_INCOMPLETE")
    others = [p for p in files if p.resolve() != main_rave.resolve()]
    counter = Counter()
    ids: set[str] = set()
    manifest: list[dict] = []
    image_dir.mkdir(parents=True, exist_ok=True)
    for source in others:
        rel = source.relative_to(root).as_posix()
        sha = hashlib.sha256(source.read_bytes()).hexdigest()
        doc = "RAVE_" + source.stem.upper()
        if doc in ids:
            raise ValueError("RAVE_DOCUMENT_ID_COLLISION:" + doc)
        ids.add(doc)
        revision = "original-" + sha[:16]
        with fitz.open(source) as pdf:
            count = pdf.page_count
            if not 1 <= count <= 999:
                raise ValueError("INVALID_RAVE_PDF_PAGE_COUNT:" + rel)
            db.execute("INSERT INTO documentary_documents VALUES(?,?,?,?,?,?,?,?,?)",
                       (doc, revision, "en", "Rover", source.stem.upper(),
                        "rave/" + rel, sha, count,
                        "Original RAVE PDF, native text + page facsimiles, review required"))
            manifest.append({"document_id": doc, "pdf_path": "rave/" + rel,
                             "pages": count, "source_sha256": sha})
            counter["documents"] += 1
            counter["pages"] += count
            for index in range(count):
                number = index + 1
                page = pdf.load_page(index)
                text = page.get_text("text", sort=True).strip()
                db.execute("INSERT INTO documentary_pages VALUES(?,?,?,?,?)",
                           (doc, revision, number, 0,
                            "source_linked" if text else "not_yet_structured"))
                counter["pages_with_digital_text" if text else
                        "pages_without_digital_text"] += 1
                raw = json.dumps({"physical_page": number,
                                  "source_pdf_sha256": sha,
                                  "extraction": "native_pdf_text",
                                  "review": "pending"}, ensure_ascii=False)
                db.execute("INSERT INTO documentary_entries VALUES(?,?,?,?,?,?,?,?,?,?)",
                           (doc, revision, f"page:{number:03d}", "reference", number,
                            f"{doc} — original RAVE page {number}", text,
                            "review_required", "rave_original_pdf_text", raw))
                filename = f"{doc}_PDF_{number:03d}.png"
                png = page.get_pixmap(matrix=fitz.Matrix(1.15, 1.15),
                                      colorspace=fitz.csGRAY,
                                      alpha=False).tobytes("png")
                (image_dir / filename).write_bytes(png)
                db.execute("INSERT INTO documentary_images VALUES(?,?,?,?,?,?)",
                           (doc, revision, number, "images/" + filename,
                            hashlib.sha256(png).hexdigest(), len(png)))
                counter["facsimiles"] += 1
    if counter["documents"] != 46 or counter["pages"] < 100:
        raise ValueError("RAVE_46_ADDITIONAL_MANUALS_INCOMPLETE")
    return {**counter, "source_documents": manifest}
