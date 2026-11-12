#!/usr/bin/env python3
"""CI-ONLY synthetic installer fixture. Never ship this as manufacturer data."""
from __future__ import annotations
import argparse
import hashlib
import sqlite3
from pathlib import Path

# Valid 1x1 PNG - only to test that installer hashes and copies image files.
DUMMY_PNG = bytes.fromhex(
    "89504e470d0a1a0a0000000d494844520000000100000001"
    "08060000001f15c4890000000b49444154789c63600002"
    "0000050001a5f645400000000049454e44ae426082"
)


def make_fixture(source: Path) -> None:
    sqlite = source / "mems_documentary_fr.sqlite"
    images = source / "images"
    images.mkdir(parents=True, exist_ok=True)
    with sqlite3.connect(sqlite) as db:
        assert db.execute("select count(*) from documentary_pages").fetchone()[0] == 371
        assert db.execute("select count(*) from documentary_images").fetchone()[0] == 301
        source_revision = db.execute(
            "select source_revision from documentary_documents "
            "where document_id='RCL0193FRE'"
        ).fetchone()[0]
        current = {r[0] for r in db.execute(
            "select physical_page from documentary_images")}
        extra_pages = [p for p in range(1, 34) if p not in current][:24]
        assert len(extra_pages) == 24
        digest = hashlib.sha256(DUMMY_PNG).hexdigest()
        for page in extra_pages:
            relative = f"images/RCL0193FRE_PDF_{page:03}.png"
            (source / relative).write_bytes(DUMMY_PNG)
            db.execute(
                "insert into documentary_images values (?,?,?,?,?,?)",
                ("RCL0193FRE", source_revision, page, relative, digest,
                 len(DUMMY_PNG)),
            )
            db.execute(
                "insert into documentary_entries values (?,?,?,?,?,?,?,?,?,?)",
                ("RCL0193FRE", source_revision,
                 f"test-only-preliminary-page:{page}", "figure", page,
                 f"SYNTHETIC TEST PAGE {page}", "SYNTHETIC TEST DATA",
                 "source_checked", "test_fixture", "{}"),
            )
        # This copy is deliberately synthetic, to test installer mechanics only.
        db.execute(
            "update documentary_entries set verification_status='source_checked'")
        db.execute(
            "create table if not exists documentary_page_visual_status("
            "document_id text not null, source_revision text not null,"
            "physical_page integer not null, is_blank integer not null,"
            "visual_source text not null, technical_section integer not null,"
            "visual_ink_ratio real not null,"
            "primary key(document_id,source_revision,physical_page))"
        )
        db.execute("delete from documentary_page_visual_status")
        visual = {p for p, in db.execute(
            "select physical_page from documentary_images")}
        db.executemany(
            "insert into documentary_page_visual_status values (?,?,?,?,?,?,?)",
            (("RCL0193FRE", source_revision, p, 0 if p in visual else 1,
              "SYNTHETIC_CI_ONLY", 0, 0.5 if p in visual else 0.0)
             for p in range(1, 372)),
        )
        db.commit()
        assert db.execute("pragma integrity_check").fetchone()[0] == "ok"
        assert not db.execute("pragma foreign_key_check").fetchall()
        assert db.execute("select count(*) from documentary_entries").fetchone()[0] == 5958
        assert db.execute("select count(*) from documentary_fts").fetchone()[0] == 5958
        assert db.execute("select count(*) from documentary_images").fetchone()[0] == 325
        print("SYNTHETIC_CI_ONLY_DOCUMENTARY_FIXTURE_READY")


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", type=Path, required=True)
    make_fixture(ap.parse_args().source)
