#!/usr/bin/env python3
"""Synthetic-only end-to-end test; no manufacturer PDF is embedded in repository."""
import csv
import hashlib
import json
import os
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path

import fitz

from add_documentary_pdf import append_pdf
from import_rave_fr_archive import import_archive
from pack_mems_documentary import create_pack, inspect_zip


def pdf(path: Path, label: str, pages: int):
    with fitz.open() as document:
        for n in range(pages):
            page=document.new_page()
            page.insert_text((55,85),f"{label} is a synthetic test instruction page number {n+1}")
        document.save(path)


class TestOriginalFrenchRaveArchive(unittest.TestCase):
    def test_full_archive_adds_all_originals_preserving_existing_french(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory)
            source_base=root/"already-present.pdf"
            pdf(source_base,"Previously installed French technical manual",2)
            baseline=root/"base.zip"
            original=append_pdf(None,source_base,"fr",baseline)
            self.assertEqual(original["document_count"],1)
            password="SyntheticCISecret2026LongAndNotPublished"
            old_encrypted=root/"old-fr.encrypted"
            create_pack(baseline,"fr",old_encrypted,password)
            sources=[("new-control-fr.pdf",2),("new-wiring-fr.pdf",3)]
            originals=root/"originals"
            originals.mkdir()
            manifest=root/"inventory.csv"
            sourcezip=root/"rave-fr.zip"
            with zipfile.ZipFile(sourcezip,"w",zipfile.ZIP_DEFLATED) as archive,manifest.open("w",newline="") as file:
                writer=csv.DictWriter(file,fieldnames=["fichier","octets","objets_page_detectes","securite_pdf","sha256"])
                writer.writeheader()
                for filename,pages in sources:
                    manual=originals/filename
                    pdf(manual,filename+" original source",pages)
                    raw=manual.read_bytes()
                    archive.write(manual,"pdf/"+filename)
                    writer.writerow({"fichier":filename,"octets":len(raw),
                        "objets_page_detectes":pages,"securite_pdf":"none",
                        "sha256":hashlib.sha256(raw).hexdigest()})
            output=root/"new-fr.encrypted"
            report=root/"new-fr.audit.json"
            result=import_archive(sourcezip,manifest,old_encrypted,output,password,report)
            self.assertEqual(result["status"],"encrypted_candidate_validated")
            self.assertEqual(result["after"]["documents"],3)
            self.assertEqual(result["after"]["source_pages"],7)
            self.assertEqual(result["after"]["facsimiles"],7)
            self.assertEqual(len(result["source_history"]),2)
            self.assertTrue(report.is_file() and output.is_file())
            self.assertEqual(
                [r["state"] for r in result["sources"]],
                ["indexed_source","indexed_source"])
            self.assertEqual(result["source_history"][0]["source_pages"],2)
            self.assertEqual(result["source_history"][1]["source_pages"],3)

    def test_bad_or_image_only_source_must_not_be_mistaken_for_french_ai_content(self):
        from import_rave_fr_archive import scan_original_archive,load_inventory
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory)
            bad=root/"blank-fr.pdf"
            with fitz.open() as empty:
                empty.new_page()
                empty.save(bad)
            z=root/"blank.zip"
            with zipfile.ZipFile(z,"w") as archive:
                archive.write(bad,bad.name)
            manifest=root/"sources.csv"
            with manifest.open("w",newline="") as f:
                w=csv.DictWriter(f,fieldnames=["fichier","octets","objets_page_detectes","sha256"])
                w.writeheader()
                w.writerow({"fichier":bad.name,"octets":bad.stat().st_size,
                    "objets_page_detectes":1,
                    "sha256":hashlib.sha256(bad.read_bytes()).hexdigest()})
            result=scan_original_archive(z,load_inventory(manifest),root/"extract")
            self.assertEqual(result[0]["state"],"image_only")
            self.assertEqual(result[0]["extracted_characters"],0)


if __name__=="__main__":
    unittest.main()
