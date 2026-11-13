#!/usr/bin/env python3
"""CI ONLY: Synthetic AKM7169FRE SPi pages for two-document installer tests.

Never use these dummy images or rows in a delivered manufacturer database.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import sqlite3
from pathlib import Path
from test_documentary_fixture import DUMMY_PNG

SPI="AKM7169FRE"
REV="MEMS-SPI-20P-ORIGINAL-FR-1993"
SHA="37cd0434347fe4009963d075e4b91b0b21ebc905bae5657519a8080c3026d4f5"

def add_spi(folder:Path) -> None:
    database=folder/"mems_documentary_fr.sqlite"
    assert database.exists(),"RCL fixture must already exist"
    folder.joinpath("images").mkdir(exist_ok=True)
    with sqlite3.connect(database) as conn:
        assert conn.execute("SELECT COUNT(*) FROM documentary_entries").fetchone()[0]==5958
        conn.execute("INSERT INTO documentary_documents VALUES (?,?,?,?,?,?,?,?,?)",
                     (SPI,REV,"fr","Rover Group","AKM7169FRE",
                      "SYNTHETIC_CI_NO_MANUFACTURER_CONTENT",SHA,20,"CI_ONLY"))
        digest=hashlib.sha256(DUMMY_PNG).hexdigest()
        for page in range(1,21):
            rel=f"images/AKM7169FRE_PDF_{page:03d}.png"
            (folder/rel).write_bytes(DUMMY_PNG)
            conn.execute("INSERT INTO documentary_pages VALUES (?,?,?,?,?)",
                         (SPI,REV,page,0,"source_linked"))
            conn.execute("INSERT INTO documentary_images VALUES (?,?,?,?,?,?)",
                         (SPI,REV,page,rel,digest,len(DUMMY_PNG)))
        for table,kind,n in (
            ("ci_figure","figure",20),("ci_components","reference",19),
            ("ci_locations","reference",18),("ci_index","reference",20),
            ("ci_facsimiles","other",20),("ci_procedures","procedure",20),
            ("ci_steps","step",267),("ci_technical","technical_value",22),
            ("akm7169fre_description_relie","other",19),
            ("ci_notices","notice",4)):
            for i in range(n):
                conn.execute("INSERT INTO documentary_entries VALUES (?,?,?,?,?,?,?,?,?,?)",
                             (SPI,REV,f"{table}-{i:03d}",kind,i%20+1,
                              "SYNTHETIC_CI_ONLY",
                              "SYNTHETIC_CI_NO_MANUFACTURER_CONTENT",
                              "source_checked",table,json.dumps({"test":True})))
        conn.execute(
            "UPDATE documentary_pages SET structured_records="
            "(SELECT COUNT(*) FROM documentary_entries e "
            " WHERE e.document_id=documentary_pages.document_id "
            " AND e.source_page=documentary_pages.physical_page) "
            "WHERE document_id='AKM7169FRE'")
        conn.commit()
        assert conn.execute("PRAGMA integrity_check").fetchone()[0]=="ok"
        assert not conn.execute("PRAGMA foreign_key_check").fetchall()
        assert conn.execute("SELECT COUNT(*) FROM documentary_entries").fetchone()[0]==6387
        assert conn.execute("SELECT COUNT(*) FROM documentary_fts").fetchone()[0]==6387
        assert conn.execute("SELECT COUNT(*) FROM documentary_images").fetchone()[0]==345
        assert conn.execute("SELECT COUNT(*) FROM documentary_pages").fetchone()[0]==391
        print("SYNTHETIC_FRENCH_TWO_DOCUMENTS_391P_6387E_345IMG_PASS")

if __name__=="__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("--source",type=Path,required=True)
    add_spi(parser.parse_args().source)
