#!/usr/bin/env python3
"""Reassemble the checked Rover 482-page documentary SQLite for SESSION.

The Git-managed .part files contain only the compressed *runtime* index.
The original manufacturer PDF is fetched from the pinned Mems-Forge source.
No user ECU database is read, bundled, or modified.
"""
from __future__ import annotations
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import shutil
import sqlite3
import tempfile
from rover_multilingual import create_schema, import_pack, check_localized_schema

DOCUMENT_SHA = 'c8bbb30d7d5a52932e7f92723ba5dc70520012ac3ceac21d19ab0a39b4d4c4e0'
GZIP_SHA = '9e2d30ea5e28461c053a99fdf974e8406cdbb4933f30a3e9b0792f9e65aa7572'
DATABASE_SHA = 'dd37296b5d477c4124133f51e96691858c0ae4746d42c3e4478bf6003a9445bb'
PART_COUNT = 29
PDF_BYTES = 43_906_518
DATABASE_BYTES = 21_508_096


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as stream:
        while data := stream.read(1024 * 1024):
            h.update(data)
    return h.hexdigest()


def ensure(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def check_database(path: Path) -> dict:
    # Read-only access mirrors the C++ documentary reader; never update it.
    con = sqlite3.connect(f'file:{path.as_posix()}?mode=ro', uri=True)
    try:
        ensure(con.execute('PRAGMA integrity_check').fetchone()[0] == 'ok', 'SQLITE_INTEGRITY_FAILED')
        ensure(not con.execute('PRAGMA foreign_key_check').fetchall(), 'SQLITE_FOREIGN_KEYS_FAILED')
        tables = ['source_documents', 'source_page_discovery', 'procedures', 'procedure_steps',
                  'technical_values', 'illustrations', 'search_aliases', 'source_page_fts']
        available = {row[0] for row in con.execute("SELECT name FROM sqlite_master WHERE type IN ('table','view')")}
        ensure(set(tables).issubset(available), 'DOCUMENTARY_SCHEMA_INCOMPLETE')
        counts = {t: con.execute(f'SELECT COUNT(*) FROM {t}').fetchone()[0] for t in tables}
        expected = {'source_documents': 1, 'source_page_discovery': 482, 'procedures': 336,
                    'procedure_steps': 5055, 'technical_values': 630, 'illustrations': 316,
                    'source_page_fts': 482}
        ensure(all(counts[t] == n for t, n in expected.items()), f'SOURCE_COUNTS_MISMATCH: {counts}')
        doc = con.execute('SELECT total_pages,original_pdf_sha256 FROM source_documents').fetchone()
        ensure(doc == (482, DOCUMENT_SHA), 'PDF_PROVENANCE_INVALID')
        pg = con.execute('SELECT COUNT(DISTINCT physical_page),MIN(physical_page),MAX(physical_page) FROM source_page_discovery').fetchone()
        ensure(pg == (482, 1, 482), 'MISSING_PDF_PAGES')
        bad = con.execute('SELECT COUNT(*) FROM procedure_steps WHERE source_page NOT BETWEEN 1 AND 482').fetchone()[0]
        ensure(bad == 0, 'UNLINKED_PROCEDURE_STEPS')
        bad = con.execute('SELECT COUNT(*) FROM procedures WHERE ai_publication_allowed<>0 OR technical_approval<>0').fetchone()[0]
        ensure(bad == 0, 'UNVERIFIED_OCR_MUST_NOT_BE_MARKED_APPROVED')
        ensure(con.execute("SELECT COUNT(*) FROM procedures WHERE manufacturer_repair_no='12.65.12'").fetchone()[0] >= 1,
               'CONSTRUCTOR_PROCEDURE_12_65_12_MISSING')
        ensure(con.execute('SELECT COUNT(*) FROM source_page_fts WHERE source_page_fts MATCH ?', ('timing',)).fetchone()[0] > 0,
               'DOCUMENTARY_FULL_TEXT_SEARCH_FAILED')
        return counts
    finally:
        con.close()


def build(parts: Path, original_pdf: Path, output: Path,
          translation_packs: list[Path] | None = None) -> None:
    ensure(original_pdf.is_file(), 'ORIGINAL_PDF_MISSING')
    ensure(original_pdf.stat().st_size == PDF_BYTES and digest(original_pdf) == DOCUMENT_SHA,
           'ORIGINAL_PDF_MUST_MATCH_482_PAGE_MANUFACTURER_SOURCE')
    pieces = sorted(parts.glob('part-*.gz.part'))
    ensure(len(pieces) == PART_COUNT, f'COMPRESSED_PART_COUNT_INVALID: {len(pieces)}')
    ensure([p.name for p in pieces] == [f'part-{i:03d}.gz.part' for i in range(PART_COUNT)],
           'COMPRESSED_PARTS_NOT_CONTIGUOUS')
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as folder:
        gzip_file = Path(folder) / 'rover.sqlite.gz'
        with gzip_file.open('wb') as dest:
            for piece in pieces:
                with piece.open('rb') as source:
                    shutil.copyfileobj(source, dest)
        ensure(digest(gzip_file) == GZIP_SHA, 'COMPRESSED_DATA_SHA256_MISMATCH')
        temporary_sqlite = Path(folder) / 'rover_482.sqlite'
        with gzip.open(gzip_file, 'rb') as source, temporary_sqlite.open('wb') as dest:
            shutil.copyfileobj(source, dest)
        ensure(temporary_sqlite.stat().st_size == DATABASE_BYTES and digest(temporary_sqlite) == DATABASE_SHA,
               'COMPLETE_SQLITE_SHA256_MISMATCH')
        counts = check_database(temporary_sqlite)
        # The verified English SQLite is never edited in-place. Construct the
        # multilingual *distribution* on this temporary copy, then publish it
        # only after verifying the complete source-to-translation contract.
        with sqlite3.connect(temporary_sqlite) as con:
            create_schema(con, DATABASE_SHA)
            for pack_path in translation_packs or []:
                pack = json.loads(pack_path.read_text(encoding='utf-8'))
                imported = import_pack(con, pack)
                print('ROVER_TRANSLATION_PACK_IMPORTED', pack_path, imported)
            language_report = check_localized_schema(con, DATABASE_SHA)
        distribution_sha = digest(temporary_sqlite)
        shutil.copy2(temporary_sqlite, output / 'rover_482.sqlite')
        ensure(digest(output / 'rover_482.sqlite') == distribution_sha,
               'MULTILINGUAL_DISTRIBUTION_HASH_MISMATCH')
    shutil.copy2(original_pdf, output / 'AKM7169ENG.pdf')
    ensure(digest(output / 'AKM7169ENG.pdf') == DOCUMENT_SHA, 'INSTALLED_PDF_HASH_WRONG')
    report = {'source': 'Rover AKM7169ENG 482-page original', 'pdf_sha256': DOCUMENT_SHA,
              'original_sqlite_sha256': DATABASE_SHA,
              'database_sha256': distribution_sha, 'integrity': 'ok',
              'documentary_counts': counts,
              'documentary_languages': language_report,
              'user_ecu_database_bundled': False,
              'source_certification': 'documentary provenance checked; technical OCR not individually certified'}
    (output / 'rover_482_install_manifest.json').write_text(
        json.dumps(report, indent=2, sort_keys=True), encoding='utf-8')
    print('MEMS_ROVER_482_DOCUMENTARY_RUNTIME_READY',
          json.dumps({'source_counts': counts, 'languages': language_report},
                     sort_keys=True))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--parts', type=Path, default=Path('database/documentary/parts'))
    parser.add_argument('--pdf', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=Path('database/documentary'))
    parser.add_argument('--translation-pack', type=Path, action='append', default=[],
                        help='Validated FR translation pack(s), imported into same SQLite')
    args = parser.parse_args()
    build(args.parts, args.pdf, args.output, args.translation_pack)