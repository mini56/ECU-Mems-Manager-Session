#!/usr/bin/env python3
"""Synthetic-only two independent FR SQLite packs for SESSION multi-base CI."""
from __future__ import annotations
import hashlib
import json
import sqlite3
import sys
from pathlib import Path
from documentary_common import schema, validate_source_snapshot


def prepare(folder: Path, docs: tuple[tuple[str, str], ...]) -> None:
    folder.mkdir(parents=True, exist_ok=True)
    (folder / "images").mkdir(exist_ok=True)
    dbfile=folder / "mems_documentary_fr.sqlite"
    with sqlite3.connect(dbfile) as db:
        db.executescript(schema("fr"))
        db.execute(
            "INSERT INTO documentary_database VALUES(1,1,'fr',1,'validated')"
        )
        for number,(identifier,title) in enumerate(docs):
            revision="ci-synthetic-1"
            digest=hashlib.sha256(identifier.encode()).hexdigest()
            db.execute(
                "INSERT INTO documentary_documents VALUES(?,?,?,?,?,?,?,?,?)",
                (identifier,revision,"fr","Synthetic CI","TEST",identifier+".pdf",
                 digest,1,"CI_SYNTHETIC_ONLY")
            )
            db.execute(
                "INSERT INTO documentary_pages VALUES(?,?,1,1,'source_linked')",
                (identifier,revision)
            )
            filename=(f"images/akm7169_{identifier.lower()}_p0001.jpg"
                      if "AKM7169" in identifier else f"images/{identifier}_PDF_001.png")
            raw=(b"TEST-SYNTHETIC-IMAGE-" + identifier.encode())
            (folder/filename).write_bytes(raw)
            db.execute(
                "INSERT INTO documentary_images VALUES(?,?,1,?,?,?)",
                (identifier,revision,filename,hashlib.sha256(raw).hexdigest(),len(raw))
            )
            db.execute(
                "INSERT INTO documentary_entries VALUES(?,?,?,?,?,?,?,?,?,?)",
                (identifier,revision,"REFERENCE-1","reference",1,title,
                 "Description francophone synthetique de "+title,
                 "source_checked","synthetic_ci",json.dumps({"ci":True},ensure_ascii=False))
            )
            db.execute(
                "INSERT INTO documentary_import_history("
                "document_id,source_revision,source_sha256,source_pages,"
                "import_kind,imported_utc,previous_snapshot_sha256,"
                "entries_at_import,procedures_at_import,steps_at_import,images_at_import)"
                " VALUES(?,?,?,?,?,?,?,?,?,?,?)",
                (identifier,revision,digest,1,"source_build",None,None,1,0,0,1)
            )
        db.commit()
        result=validate_source_snapshot(db,"fr")
        assert result["document_count"]==len(docs)
        assert db.execute("PRAGMA integrity_check").fetchone()[0]=="ok"


if __name__=="__main__":
    base=Path(sys.argv[1])
    prepare(base/"original",(
        ("RCL0193FRE","Culasse moteur historique"),
        ("DOC_CDXN010F_TEST","Entretien transmission historique")
    ))
    prepare(base/"addition",(
        ("AKM7169_VOLUME1_TEST","Freinage Mini"),
        ("AKM7169_VOLUME2_TEST","Direction suspension Mini"),
        ("AKM7169_VOLUME3_TEST","Injection carburateur Mini")
    ))
    print("SYNTHETIC_MULTIBASE_FIXTURES_READY")
