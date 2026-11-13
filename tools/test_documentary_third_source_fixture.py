#!/usr/bin/env python3
"""Synthetic third manual regression fixture; NEVER manufacturer content."""
import argparse
import hashlib
import json
from pathlib import Path
import sqlite3
from test_documentary_fixture import DUMMY_PNG


def extend(directory: Path) -> None:
    dbfile = directory / "mems_documentary_fr.sqlite"
    assert dbfile.is_file()
    document_id = "SYNTHETIC_THIRD_MANUAL"
    revision = "revision-1"
    digest = hashlib.sha256(DUMMY_PNG).hexdigest()
    with sqlite3.connect(dbfile) as db:
        db.execute(
            "INSERT INTO documentary_documents VALUES(?,?,?,?,?,?,?,?,?)",
            (document_id, revision, "fr", "Synthetic test publisher",
             document_id, "THIRD_SOURCE_CI_ONLY.pdf", "a"*64, 4, "CI_ONLY"))
        for page in range(1, 5):
            image_path = f"images/{document_id}_PDF_{page:03d}.png"
            (directory / image_path).write_bytes(DUMMY_PNG)
            db.execute("INSERT INTO documentary_pages VALUES(?,?,?,?,?)",
                       (document_id, revision, page, 3, "source_linked"))
            db.execute("INSERT INTO documentary_images VALUES(?,?,?,?,?,?)",
                       (document_id, revision, page, image_path,
                        digest, len(DUMMY_PNG)))
            for entry in range(3):
                db.execute(
                    "INSERT INTO documentary_entries VALUES(?,?,?,?,?,?,?,?,?,?)",
                    (document_id, revision, f"third:{page}:{entry}", "notice",
                     page, "Document synthétique",
                     "Test de mise à jour : nouvel ouvrage indépendant",
                     "source_checked", "ci_third_manual",
                     json.dumps({"ci_only": True})))
        db.commit()
        assert db.execute("PRAGMA integrity_check").fetchone()[0] == "ok"
        assert not db.execute("PRAGMA foreign_key_check").fetchall()
        assert db.execute("SELECT COUNT(*) FROM documentary_documents").fetchone()[0] == 3
        assert db.execute("SELECT COUNT(*) FROM documentary_pages").fetchone()[0] == 395
        assert db.execute("SELECT COUNT(*) FROM documentary_images").fetchone()[0] == 349
        assert db.execute("SELECT COUNT(*) FROM documentary_entries").fetchone()[0] == 6399
        assert db.execute("SELECT COUNT(*) FROM documentary_fts").fetchone()[0] == 6399
    print("THIRD_INDEPENDENT_FRENCH_MANUAL_ADDED_TO_SNAPSHOT")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True, type=Path)
    extend(parser.parse_args().source)
