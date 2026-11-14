#!/usr/bin/env python3
"""Regression: one PDF import/packing contract for every SESSION language."""
import hashlib
import sqlite3
import tempfile
import unittest
import zipfile
from pathlib import Path

from add_documentary_pdf import append_pdf
from documentary_common import LANGUAGES, validate_source_snapshot
from pack_mems_documentary import create_pack, inspect_zip


class UniformDocumentarySystemTest(unittest.TestCase):
    def test_all_six_languages_and_successive_independent_pdfs(self):
        import fitz
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder)
            first=root/"manual_one.pdf"
            second=root/"other_manual.pdf"
            with fitz.open() as pdf:
                for index in range(3):
                    p=pdf.new_page()
                    p.insert_text((65,90),f"Original technical document page {index+1}")
                pdf.save(first)
            with fitz.open() as pdf:
                for index in range(2):
                    p=pdf.new_page()
                    p.insert_text((65,90),f"Second unrelated document page {index+1}")
                pdf.save(second)
            signatures={}
            for language in sorted(LANGUAGES):
                old=root/f"{language}-one.zip"
                new=root/f"{language}-two.zip"
                report1=append_pdf(None,first,language,old)
                self.assertEqual(report1["document_count"],1)
                self.assertEqual(report1["source_pages"],3)
                self.assertEqual(report1["facsimiles"],3)
                self.assertEqual(inspect_zip(old,language)["documents"],1)
                report2=append_pdf(old,second,language,new)
                self.assertEqual(report2["document_count"],2)
                self.assertEqual(report2["source_pages"],5)
                self.assertEqual(report2["facsimiles"],5)
                self.assertEqual(inspect_zip(new,language)["documents"],2)
                dbname=f"mems_documentary_{language}.sqlite"
                with zipfile.ZipFile(old) as archive:
                    source_snapshot=archive.read(dbname)
                with zipfile.ZipFile(new) as archive:
                    updated_snapshot=archive.read(dbname)
                    self.assertEqual(len([
                        n for n in archive.namelist()
                        if n.startswith("images/")]),5)
                    image_from_old=[
                        (n,hashlib.sha256(archive.read(n)).hexdigest())
                        for n in archive.namelist() if n.startswith("images/")
                        and report1["new_document_id"] in n]
                with tempfile.TemporaryDirectory() as staging:
                    old_path=Path(staging)/"before.sqlite"
                    new_path=Path(staging)/"after.sqlite"
                    old_path.write_bytes(source_snapshot)
                    new_path.write_bytes(updated_snapshot)
                    with sqlite3.connect(old_path) as before,sqlite3.connect(new_path) as after:
                        self.assertEqual(validate_source_snapshot(after,language)["document_count"],2)
                        previous=before.execute(
                            "SELECT document_id,entry_id,source_text,raw_json "
                            "FROM documentary_entries ORDER BY document_id,entry_id").fetchall()
                        unchanged=after.execute(
                            "SELECT document_id,entry_id,source_text,raw_json "
                            "FROM documentary_entries WHERE document_id=? "
                            "ORDER BY document_id,entry_id",
                            (report1["new_document_id"],)).fetchall()
                        self.assertEqual(previous,unchanged)
                        self.assertEqual(
                            after.execute("SELECT COUNT(*) FROM documentary_entries "
                                          "WHERE entry_type='procedure'").fetchone()[0],0)
                        self.assertEqual(
                            after.execute("SELECT COUNT(*) FROM documentary_entries "
                                          "WHERE verification_status='review_required'").fetchone()[0],5)
                        self.assertEqual(
                            after.execute("SELECT COUNT(*) FROM documentary_fts").fetchone()[0],5)
                        signatures[language]={
                            table:tuple(after.execute(f"PRAGMA table_info({table})").fetchall())
                            for table in ("documentary_database","documentary_documents",
                                          "documentary_pages","documentary_images",
                                          "documentary_entries")}
                self.assertEqual(len(image_from_old),3)
                if language in ("fr","en"):
                    encrypted=root/f"{language}.encrypted"
                    pack=create_pack(new,language,encrypted,"test-secret-password-2026")
                    self.assertEqual(pack["documents"],2)
                    self.assertEqual(pack["manufacturer_image_count"],5)
                    self.assertTrue(encrypted.read_bytes().startswith(b"MEMSDOC1"))
                with self.assertRaisesRegex(ValueError,"ORIGINAL_DOCUMENT_ALREADY"):
                    append_pdf(new,first,language,root/f"{language}-duplicate.zip")
                with self.assertRaisesRegex(ValueError,"SOURCE_SNAPSHOT_LIMITS_OR_DATABASE_MISSING"):
                    append_pdf(new,second,
                               "fr" if language!="fr" else "en",
                               root/f"{language}-wrong-language.zip")
            # LANGUAGE has different values but the actual table structure is
            # identical. No hardcoded manual counts, no per-PDF schema.
            table_definitions=[]
            for language in sorted(LANGUAGES):
                columns=signatures[language]
                normalized={
                    name:tuple((row[1],row[2],row[3],row[4],row[5])
                               for row in values)
                    for name,values in columns.items()}
                table_definitions.append(normalized)
            self.assertTrue(all(value==table_definitions[0]
                                for value in table_definitions[1:]))


if __name__=="__main__":
    unittest.main()
