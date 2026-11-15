#!/usr/bin/env python3
"""Import every original French RAVE PDF into the existing French SESSION pack.

Inputs: original ZIP, independent SHA-256 inventory, current encrypted FR pack.
Output: an authenticated FR .encrypted candidate and an auditable JSON report.

Never translates English, guesses technical instructions, removes the original
FR documents, publishes a Release, or touches the diagnostic ECU database.
"""
from __future__ import annotations

import argparse
import csv
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import sqlite3
import struct
import tempfile
import zipfile

from cryptography.hazmat.primitives.ciphers.aead import AESGCM
import fitz

from add_documentary_pdf import append_pdf
from documentary_common import validate_source_snapshot, documentary_import_report
from pack_mems_documentary import create_pack, inspect_zip


def fail(message: str) -> None:
    raise ValueError(message)


def digest_file(path: Path) -> str:
    sha = hashlib.sha256()
    with path.open("rb") as file:
        for block in iter(lambda: file.read(1024 * 1024), b""):
            sha.update(block)
    return sha.hexdigest()


def decrypt_original_pack(path: Path, password: str, output: Path) -> None:
    encrypted=path.read_bytes()
    if len(encrypted)<64 or encrypted[:8]!=b"MEMSDOC1" or encrypted[8]!=1:
        fail("SOURCE_PACK_FORMAT_MISMATCH")
    salt=encrypted[9:25]
    nonce=encrypted[25:37]
    expected_length=struct.unpack("<Q",encrypted[37:45])[0]
    if expected_length<=0 or expected_length>256*1024*1024:
        fail("SOURCE_PACK_UNCOMPRESSED_SIZE_INVALID")
    key=hashlib.pbkdf2_hmac("sha256",password.encode("utf-8"),salt,210000,32)
    data=AESGCM(key).decrypt(nonce,encrypted[45:],encrypted[:45])
    if len(data)!=expected_length:
        fail("SOURCE_PACK_SIZE_AUTHENTICATION_FAILED")
    output.write_bytes(data)
    inspect_zip(output,"fr")


def load_inventory(path: Path) -> dict[str,dict]:
    with path.open(newline="",encoding="utf-8-sig") as file:
        rows=list(csv.DictReader(file))
    if not rows or len(rows)!=len({row["fichier"].casefold() for row in rows}):
        fail("RAVE_FR_SOURCE_MANIFEST_EMPTY_OR_DUPLICATED")
    required={"fichier","octets","objets_page_detectes","sha256"}
    if any(not required.issubset(row) for row in rows):
        fail("RAVE_FR_SOURCE_MANIFEST_COLUMNS_INVALID")
    result={}
    for row in rows:
        filename=row["fichier"]
        if not re.fullmatch(r"[A-Za-z0-9._-]+\.[pP][dD][fF]",filename):
            fail("RAVE_FR_INVALID_ORIGINAL_NAME:"+filename)
        if not re.fullmatch("[0-9a-f]{64}",row["sha256"]):
            fail("RAVE_FR_INVALID_ORIGINAL_SHA:"+filename)
        result[filename.casefold()]={
            "filename":filename,
            "sha256":row["sha256"],
            "size":int(row["octets"]),
            "pages":int(row["objets_page_detectes"])
        }
        if not (1<=result[filename.casefold()]["pages"]<=50000):
            fail("RAVE_FR_INVALID_EXPECTED_PAGE_COUNT:"+filename)
    return result


def scan_original_archive(archive_path:Path, inventory:dict[str,dict],
                          output_dir:Path) -> list[dict]:
    output_dir.mkdir(parents=True,exist_ok=True)
    assessments=[]
    with zipfile.ZipFile(archive_path) as archive:
        if archive.testzip() is not None:
            fail("RAVE_FR_ORIGINAL_ARCHIVE_CRC_FAILURE")
        pdfs=[item for item in archive.infolist()
              if not item.is_dir() and item.filename.lower().endswith(".pdf")]
        if len(pdfs)!=len(inventory):
            fail("RAVE_FR_PDF_COUNT_MISMATCH")
        already=set()
        for item in pdfs:
            parts=PurePosixPath(item.filename).parts
            if (any(x in ("",".","..") for x in parts) or
                    item.filename.startswith("/") or "\\" in item.filename or
                    (item.external_attr >> 16)&0o170000==0o120000):
                fail("RAVE_FR_UNSAFE_SOURCE_ARCHIVE_PATH")
            filename=PurePosixPath(item.filename).name
            key=filename.casefold()
            if key in already or key not in inventory:
                fail("RAVE_FR_UNEXPECTED_OR_DUPLICATE_SOURCE:"+filename)
            already.add(key)
            record=inventory[key]
            if item.file_size!=record["size"]:
                fail("RAVE_FR_SOURCE_SIZE_MISMATCH:"+filename)
            target=output_dir/filename
            with archive.open(item) as source, target.open("wb") as sink:
                for block in iter(lambda:source.read(1024*1024),b""):
                    sink.write(block)
            if digest_file(target)!=record["sha256"]:
                fail("RAVE_FR_ORIGINAL_SHA256_MISMATCH:"+filename)
            assessment={"source_file":filename,"original_sha256":record["sha256"],
                        "expected_pages":record["pages"],"bytes":record["size"],
                        "text_pages":0,"extracted_characters":0,"state":"unverified"}
            try:
                with fitz.open(target) as pdf:
                    if pdf.needs_pass:
                        fail("PROTECTED_SOURCE_REQUIRES_AUTHORIZED_READABLE_PDF")
                    actual=pdf.page_count
                    assessment["actual_pages"]=actual
                    if actual!=record["pages"]:
                        fail("ORIGINAL_PDF_PAGE_COUNT_NOT_MATCHING_INVENTORY")
                    for page in pdf:
                        content=page.get_text("text",sort=True).strip()
                        if content:
                            assessment["text_pages"]+=1
                            assessment["extracted_characters"]+=len(content)
                    assessment["state"]="searchable" if assessment["extracted_characters"]>=20 else "image_only"
            except Exception as exc:
                assessment["state"]="unreadable"
                assessment["error_type"]=type(exc).__name__
                assessment["error"]=str(exc)[:220]
            assessments.append(assessment)
    if len(already)!=len(inventory):
        fail("RAVE_FR_SOURCE_ARCHIVE_INCOMPLETE")
    return sorted(assessments,key=lambda row:row["source_file"].casefold())


def import_archive(source_archive:Path, inventory_file:Path,
                   current_fr_encrypted:Path, output_encrypted:Path,
                   publisher_password:str, report_path:Path) -> dict:
    if len(publisher_password)<12:
        fail("DOCUMENTARY_ADMIN_SECRET_UNAVAILABLE")
    if not source_archive.is_file() or not current_fr_encrypted.is_file():
        fail("ORIGINAL_FRENCH_RAVE_OR_INSTALLED_PACK_MISSING")
    inventory=load_inventory(inventory_file)
    report={"language":"fr",
            "origin_archive_sha256":digest_file(source_archive),
            "initial_encrypted_sha256":digest_file(current_fr_encrypted),
            "source_inventory_sha256":digest_file(inventory_file),
            "expected_manuals":len(inventory),
            "expected_source_pages":sum(row["pages"] for row in inventory.values()),
            "status":"source_verification_pending",
            "sources":[],"published":False}
    report_path.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="rave-fr-documentary-") as directory:
        root=Path(directory)
        originals=root/"pdf"
        try:
            report["sources"]=scan_original_archive(source_archive,inventory,originals)
            unsearchable=[row["source_file"] for row in report["sources"]
                          if row["state"]!="searchable"]
            if unsearchable:
                fail("RAVE_FR_EXTRACTION_UNREADABLE_OR_OCR_REQUIRED:"+",".join(unsearchable))
            old=root/"before.zip"
            decrypt_original_pack(current_fr_encrypted,publisher_password,old)
            before=inspect_zip(old,"fr")
            report["before"]=before
            current=old
            for index,assessment in enumerate(report["sources"],1):
                candidate=root/f"added-{index:02d}.zip"
                result=append_pdf(current,originals/assessment["source_file"],
                                  "fr",candidate,publisher="RAVE français original")
                assessment["indexed_document_id"]=result["new_document_id"]
                assessment["import_sequence"]=result["import_sequence"]
                assessment["state"]="indexed_source"
                current=candidate
            after=inspect_zip(current,"fr")
            if after["documents"]!=before["documents"]+len(inventory):
                fail("RAVE_FR_DOCUMENT_COUNT_NOT_ADDITIVE")
            if after["source_pages"]!=before["source_pages"]+report["expected_source_pages"]:
                fail("RAVE_FR_SOURCE_PAGES_NOT_ADDITIVE")
            report["after"]=after
            if report["after"]["facsimiles"]<before["facsimiles"]+report["expected_source_pages"]:
                fail("RAVE_FR_FACSIMILES_INCOMPLETE")
            with zipfile.ZipFile(current) as z:
                with tempfile.TemporaryDirectory() as dbdir:
                    dbpath=Path(dbdir)/"fr.sqlite"
                    dbpath.write_bytes(z.read("mems_documentary_fr.sqlite"))
                    with sqlite3.connect(dbpath) as database:
                        validate_source_snapshot(database,"fr")
                        history=documentary_import_report(database,"fr")
                        source_events=history["history"][-len(inventory):]
                        if any(event["import_kind"]!="added_pdf"
                               for event in source_events):
                            fail("DOCUMENTARY_APPEND_HISTORY_INVALID")
                        report["source_history"]=source_events
            asset=create_pack(current,"fr",output_encrypted,publisher_password)
            report["encrypted_pack"]={
                "name":output_encrypted.name,
                "sha256":asset["encrypted_sha256"],
                "bytes":asset["encrypted_size"]
            }
            report["status"]="encrypted_candidate_validated"
        except Exception as exc:
            report["status"]="blocked"
            report["error"]=str(exc)[:700]
            raise
        finally:
            report_path.write_text(json.dumps(report,ensure_ascii=False,
                               sort_keys=True,indent=2)+"\n",encoding="utf-8")
    return report


def main() -> None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rave-fr-archive",type=Path,required=True)
    parser.add_argument("--source-inventory",type=Path,required=True)
    parser.add_argument("--current-fr-encrypted",type=Path,required=True)
    parser.add_argument("--output-encrypted",type=Path,required=True)
    parser.add_argument("--audit",type=Path,required=True)
    parser.add_argument("--password-env",default="MEMS_DOCUMENTARY_PUBLISH_PASSWORD")
    args=parser.parse_args()
    pwd=os.environ.get(args.password_env,"")
    result=import_archive(args.rave_fr_archive,args.source_inventory,
                          args.current_fr_encrypted,args.output_encrypted,
                          pwd,args.audit)
    print(json.dumps({
        "status":result["status"],
        "language":"fr",
        "manuals_added":len(result["sources"]),
        "source_pages_added":result["expected_source_pages"],
        "documents_after":result["after"]["documents"],
        "encrypted_file":result["encrypted_pack"]["name"]
    },ensure_ascii=False))


if __name__=="__main__":
    main()
