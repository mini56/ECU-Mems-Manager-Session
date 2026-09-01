#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import sqlite3
import unicodedata
from collections import defaultdict
from pathlib import Path

TARGET_LEGACY_TABLES = [
    'mems_rave_fact', 'mems_rave_illustration', 'mems_rave_illustration_link',
    'mems_knowledge_item', 'mems_procedure', 'mems_procedure_step',
    'mems_specification', 'mems_specification_value'
]
TEXT_HINT = re.compile(r'(text|title|name|description|content|note|procedure|step|warning|source|document|reference|label|summary|detail|value)', re.I)
PUB_CODE = re.compile(r'\b(?:RCL\d{4}[A-Z]{3}|AKM\d+[A-Z]{0,4})\b', re.I)


def sha_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def qident(name: str) -> str:
    return '"' + name.replace('"', '""') + '"'


def enc(v):
    if isinstance(v, bytes):
        return {'__blob__': v.hex()}
    return v


def table_digest(conn: sqlite3.Connection, table: str) -> tuple[int, str, list[dict]]:
    cols = conn.execute(f'PRAGMA table_info({qident(table)})').fetchall()
    names = [r[1] for r in cols]
    order = ','.join(qident(c) for c in names)
    sql = f'SELECT {order} FROM {qident(table)}'
    if names:
        sql += ' ORDER BY ' + order
    h = hashlib.sha256()
    count = 0
    for row in conn.execute(sql):
        h.update((json.dumps([enc(v) for v in row], ensure_ascii=False, sort_keys=True, separators=(',', ':')) + '\n').encode('utf-8'))
        count += 1
    info = [{'cid': r[0], 'name': r[1], 'type': r[2], 'notnull': r[3], 'default': r[4], 'pk': r[5]} for r in cols]
    return count, h.hexdigest(), info


def snapshot(conn: sqlite3.Connection) -> dict:
    objs = conn.execute("SELECT type,name,tbl_name,COALESCE(sql,'') FROM sqlite_master WHERE name NOT LIKE 'sqlite_%' ORDER BY type,name").fetchall()
    out = {
        'user_version': conn.execute('PRAGMA user_version').fetchone()[0],
        'integrity_check': conn.execute('PRAGMA integrity_check').fetchone()[0],
        'foreign_key_check': conn.execute('PRAGMA foreign_key_check').fetchall(),
        'objects': [{'type': r[0], 'name': r[1], 'table': r[2], 'sql': r[3]} for r in objs],
        'tables': {}
    }
    for typ, name, _, sql in objs:
        if typ != 'table':
            continue
        count, digest, cols = table_digest(conn, name)
        out['tables'][name] = {'count': count, 'logical_sha256': digest, 'columns': cols, 'schema_sql': sql}
    return out


def normalize_text(value: str) -> str:
    s = unicodedata.normalize('NFKC', value or '').casefold()
    s = re.sub(r'\s+', ' ', s).strip()
    return s


def first_title(src: sqlite3.Connection, doc_key: str, fallback: str) -> tuple[str, str | None]:
    rows = src.execute('SELECT source_text_exact FROM page WHERE document_key=? ORDER BY physical_page LIMIT 8', (doc_key,)).fetchall()
    text = '\n'.join((r[0] or '') for r in rows)
    lines = [re.sub(r'\s+', ' ', x).strip() for x in text.splitlines() if x.strip()]
    title = lines[0][:500] if lines else fallback
    m = PUB_CODE.search(text + ' ' + fallback)
    return title, (m.group(0).upper() if m else None)


def verify_rave_source(src: sqlite3.Connection, root: Path) -> dict:
    expected = {'document': 47, 'page': 1359, 'line': 54732, 'content': 19039, 'visual_asset': 1070, 'visual_occurrence': 1794, 'ocr_region': 45}
    counts = {t: src.execute(f'SELECT COUNT(*) FROM {t}').fetchone()[0] for t in expected}
    if counts != expected:
        raise RuntimeError(f'RAVEMEMS count mismatch: {counts}')
    if src.execute('PRAGMA integrity_check').fetchone()[0].lower() != 'ok':
        raise RuntimeError('RAVEMEMS integrity_check failed')
    if src.execute('PRAGMA foreign_key_check').fetchall():
        raise RuntimeError('RAVEMEMS foreign_key_check failed')
    checked = 0
    for rel, sha in src.execute('SELECT relative_path,sha256 FROM visual_asset ORDER BY visual_key'):
        p = root / rel
        if not p.is_file():
            raise RuntimeError(f'missing RAVEMEMS asset: {rel}')
        if sha_file(p) != sha:
            raise RuntimeError(f'RAVEMEMS asset SHA mismatch: {rel}')
        checked += 1
    return {'counts': counts, 'registered_assets_verified': checked}


def insert_entity(dst, entity_key, document_key, unit_key, kind, source_locator, legacy_table, legacy_key, notes=None, sort_order=0):
    dst.execute('''INSERT OR IGNORE INTO mems_doc_entity
        (entity_key,document_key,unit_key,entity_kind,parent_entity_key,domain,component_key,verification_level,source_locale,source_locator,legacy_table,legacy_key,sort_order,notes)
        VALUES(?,?,?,?,NULL,?,?,?, ?,?,?,?,?,?)''',
        (entity_key, document_key, unit_key, kind, 'RAVEMEMS', None, 'run4_verified', 'en', source_locator, legacy_table, legacy_key, sort_order, notes))


def import_ravemems(dst: sqlite3.Connection, src: sqlite3.Connection) -> dict:
    doc_map = {}
    page_map = {}
    vis_map = {}
    docs = src.execute('SELECT document_key,relative_path,source_sha256,page_count,selection_reason FROM document ORDER BY document_key').fetchall()
    for dk, rel, sha, pages, reason in docs:
        nd = 'RAVEMEMS::DOC::' + dk
        title, pub = first_title(src, dk, Path(rel).name)
        doc_map[dk] = nd
        dst.execute('''INSERT OR IGNORE INTO mems_doc_document
            (document_key,source_key,title_source,publication_code,edition,source_locale,source_kind,file_name,file_sha256,page_count,verification_level,notes)
            VALUES(?,?,?,?,NULL,'en','rave_pdf',?,?,?,?,?)''',
            (nd, 'RAVEMEMS_RUN4', title, pub, rel, sha, pages, 'run4_verified', json.dumps({'original_document_key': dk, 'selection_reason': reason}, ensure_ascii=False)))

    pages = src.execute('''SELECT page_key,document_key,physical_page,page_class,native_text,ocr_used,ocr_source_kind,
                                  source_text_exact,source_text_sha256,image_occurrence_count,vector_drawing_count,needs_review,review_reason
                           FROM page ORDER BY document_key,physical_page''').fetchall()
    for row in pages:
        pk, dk, pn, pclass, native, ocr_used, ocr_kind, text, text_sha, occ_count, vec_count, needs_review, review_reason = row
        unit = 'RAVEMEMS::UNIT::' + pk
        ent = 'RAVEMEMS::PAGE::' + pk
        audit_status = 'needs_review' if needs_review else ('out_of_scope' if pclass == 'blank' else 'useful')
        integration_status = 'backlog' if needs_review else ('not_required' if pclass == 'blank' else 'integrated')
        reason = review_reason or ('visually_or_render_confirmed_blank' if pclass == 'blank' else pclass)
        dst.execute('''INSERT OR IGNORE INTO mems_doc_unit
            (unit_key,document_key,unit_kind,sequence_no,physical_page,printed_ref,section_ref,source_hash,audit_status,integration_status,audit_reason,notes)
            VALUES(?,?,'page',?,?,NULL,NULL,?,?,?,?,?)''',
            (unit, doc_map[dk], pn, pn, text_sha, audit_status, integration_status, reason,
             json.dumps({'page_class': pclass, 'native_text': native, 'ocr_used': ocr_used, 'ocr_source_kind': ocr_kind,
                         'image_occurrence_count': occ_count, 'vector_drawing_count': vec_count}, ensure_ascii=False)))
        locator = f'{dk}:physical_page={pn}'
        insert_entity(dst, ent, doc_map[dk], unit, 'page', locator, 'RAVEMEMS.page', pk, json.dumps({'page_class': pclass}, ensure_ascii=False), pn)
        if text:
            dst.execute('''INSERT OR IGNORE INTO mems_doc_text(entity_key,field_key,locale,text_value,translation_status,translation_method,source_locale,notes)
                           VALUES(?,'source_exact','en',?,'source',NULL,'en',NULL)''', (ent, text))
        page_map[pk] = (doc_map[dk], unit, ent, pn)

    for lk, pk, native_order, block_index, line_in_block, text, bbox, spans in src.execute('''SELECT line_key,page_key,native_order,block_index,line_in_block,source_text,bbox_json,spans_json FROM line ORDER BY page_key,native_order'''):
        doc, unit, page_ent, pn = page_map[pk]
        ent = 'RAVEMEMS::LINE::' + lk
        notes = json.dumps({'bbox_json': bbox, 'spans_json': spans, 'block_index': block_index, 'line_in_block': line_in_block}, ensure_ascii=False)
        insert_entity(dst, ent, doc, unit, 'source_line', f'{pk}:line={native_order}', 'RAVEMEMS.line', lk, notes, native_order)
        dst.execute('''INSERT OR IGNORE INTO mems_doc_text(entity_key,field_key,locale,text_value,translation_status,translation_method,source_locale,notes)
                       VALUES(?,'source_exact','en',?,'source',NULL,'en',NULL)''', (ent, text or ''))
        dst.execute("INSERT OR IGNORE INTO mems_doc_relation(from_entity_key,relation_type,to_entity_key,notes) VALUES(?,'contains_line',?,NULL)", (page_ent, ent))

    for ik, pk, order_index, item_type, text, line_ids, bbox in src.execute('''SELECT item_key,page_key,order_index,item_type,source_text,source_line_ids_json,bbox_json FROM content ORDER BY page_key,order_index'''):
        doc, unit, page_ent, pn = page_map[pk]
        ent = 'RAVEMEMS::CONTENT::' + ik
        notes = json.dumps({'item_type': item_type, 'source_line_ids_json': line_ids, 'bbox_json': bbox}, ensure_ascii=False)
        insert_entity(dst, ent, doc, unit, 'content_block', f'{pk}:content={order_index}', 'RAVEMEMS.content', ik, notes, order_index)
        dst.execute('''INSERT OR IGNORE INTO mems_doc_text(entity_key,field_key,locale,text_value,translation_status,translation_method,source_locale,notes)
                       VALUES(?,'source_exact','en',?,'source',NULL,'en',NULL)''', (ent, text or ''))
        dst.execute("INSERT OR IGNORE INTO mems_doc_relation(from_entity_key,relation_type,to_entity_key,notes) VALUES(?,'contains_content',?,NULL)", (page_ent, ent))

    for vk, dk, xref, ext, rel, sha, width, height in src.execute('SELECT visual_key,document_key,xref,ext,relative_path,sha256,width,height FROM visual_asset ORDER BY visual_key'):
        ent = 'RAVEMEMS::VIS::' + vk
        vis_map[vk] = ent
        insert_entity(dst, ent, doc_map[dk], None, 'visual_asset', f'{dk}:xref={xref}', 'RAVEMEMS.visual_asset', vk,
                      json.dumps({'xref': xref, 'ext': ext}, ensure_ascii=False))
        dst.execute('''INSERT OR IGNORE INTO mems_doc_visual(entity_key,relative_path,sha256,visual_kind,source_width,source_height,original_intact,legacy_illustration_key,notes)
                       VALUES(?,?,?,'rave_original',?,?,1,?,?)''',
                    (ent, 'ravemems_run4/' + rel, sha, width, height, vk, json.dumps({'original_relative_path': rel}, ensure_ascii=False)))

    for ok, pk, vk, bbox in src.execute('SELECT occurrence_key,page_key,visual_key,bbox_json FROM visual_occurrence ORDER BY page_key,occurrence_key'):
        doc, unit, page_ent, pn = page_map[pk]
        ent = 'RAVEMEMS::OCC::' + ok
        insert_entity(dst, ent, doc, unit, 'visual_occurrence', f'{pk}:occurrence={ok}', 'RAVEMEMS.visual_occurrence', ok,
                      json.dumps({'bbox_json': bbox}, ensure_ascii=False))
        dst.execute("INSERT OR IGNORE INTO mems_doc_relation(from_entity_key,relation_type,to_entity_key,notes) VALUES(?,'contains_visual_occurrence',?,NULL)", (page_ent, ent))
        dst.execute("INSERT OR IGNORE INTO mems_doc_relation(from_entity_key,relation_type,to_entity_key,notes) VALUES(?,'uses_visual',?,NULL)", (ent, vis_map[vk]))

    for rk, pk, order_index, text, bbox, conf, words in src.execute('SELECT region_key,page_key,order_index,source_text,bbox_json,mean_confidence,word_count FROM ocr_region ORDER BY page_key,order_index'):
        doc, unit, page_ent, pn = page_map[pk]
        ent = 'RAVEMEMS::OCR::' + rk
        insert_entity(dst, ent, doc, unit, 'ocr_region', f'{pk}:ocr={order_index}', 'RAVEMEMS.ocr_region', rk,
                      json.dumps({'bbox_json': bbox, 'mean_confidence': conf, 'word_count': words}, ensure_ascii=False), order_index)
        dst.execute('''INSERT OR IGNORE INTO mems_doc_text(entity_key,field_key,locale,text_value,translation_status,translation_method,source_locale,notes)
                       VALUES(?,'source_exact','en',?,'source','tesseract','en',?)''',
                    (ent, text or '', json.dumps({'mean_confidence': conf, 'word_count': words}, ensure_ascii=False)))
        dst.execute("INSERT OR IGNORE INTO mems_doc_relation(from_entity_key,relation_type,to_entity_key,notes) VALUES(?,'contains_ocr_region',?,NULL)", (page_ent, ent))

    dst.commit()
    tables = ['mems_doc_locale','mems_doc_document','mems_doc_unit','mems_doc_entity','mems_doc_text','mems_doc_visual','mems_doc_visual_region','mems_doc_table','mems_doc_table_cell','mems_doc_value','mems_doc_relation']
    return {t: dst.execute(f'SELECT COUNT(*) FROM {t}').fetchone()[0] for t in tables}


def compare_legacy(before: dict, after: dict) -> dict:
    changed = []
    for table, b in before['tables'].items():
        a = after['tables'].get(table)
        if not a or b['count'] != a['count'] or b['logical_sha256'] != a['logical_sha256'] or b['schema_sql'] != a['schema_sql']:
            changed.append({'table': table, 'before': b, 'after': a})
    before_objs = {(o['type'], o['name'], o['table'], o['sql']) for o in before['objects']}
    after_objs = {(o['type'], o['name'], o['table'], o['sql']) for o in after['objects']}
    missing_objects = sorted(before_objs - after_objs)
    return {'changed_legacy_tables': changed, 'missing_legacy_objects': missing_objects, 'pass': not changed and not missing_objects}


def legacy_text_rows(conn: sqlite3.Connection, before_tables: set[str]):
    for table in sorted(before_tables):
        cols = conn.execute(f'PRAGMA table_info({qident(table)})').fetchall()
        text_cols = [r[1] for r in cols if ('TEXT' in (r[2] or '').upper() or TEXT_HINT.search(r[1]))]
        if not text_cols:
            continue
        pk_cols = [r[1] for r in cols if r[5]]
        select_cols = []
        for c in pk_cols + text_cols:
            if c not in select_cols:
                select_cols.append(c)
        sql = 'SELECT ' + ','.join(qident(c) for c in select_cols) + ' FROM ' + qident(table)
        for idx, row in enumerate(conn.execute(sql)):
            d = dict(zip(select_cols, row))
            ref = {c: d.get(c) for c in pk_cols} if pk_cols else {'row_index': idx}
            for c in text_cols:
                v = d.get(c)
                if isinstance(v, str) and len(normalize_text(v)) >= 25:
                    yield table, ref, c, v


def duplicate_audit(conn: sqlite3.Connection, before_tables: set[str]) -> dict:
    rave_map = defaultdict(list)
    rows = conn.execute('''SELECT t.entity_key,t.text_value,e.legacy_table,e.legacy_key,d.file_name,u.physical_page
                           FROM mems_doc_text t
                           JOIN mems_doc_entity e ON e.entity_key=t.entity_key
                           JOIN mems_doc_document d ON d.document_key=e.document_key
                           LEFT JOIN mems_doc_unit u ON u.unit_key=e.unit_key
                           WHERE t.locale='en' AND t.field_key='source_exact' AND e.entity_key LIKE 'RAVEMEMS::%' ''').fetchall()
    for entity_key, text, legacy_table, legacy_key, file_name, page in rows:
        n = normalize_text(text or '')
        if len(n) >= 25:
            rave_map[n].append({'entity_key': entity_key, 'source_table': legacy_table, 'source_key': legacy_key, 'document': file_name, 'physical_page': page})
    exact = []
    legacy_codes = defaultdict(list)
    for table, ref, col, value in legacy_text_rows(conn, before_tables):
        n = normalize_text(value)
        if n in rave_map:
            exact.append({'classification': 'identical_text_candidate', 'legacy_table': table, 'legacy_ref': ref, 'legacy_column': col,
                          'legacy_text': value, 'ravemems_matches': rave_map[n]})
        for code in set(x.upper() for x in PUB_CODE.findall(value)):
            legacy_codes[code].append({'legacy_table': table, 'legacy_ref': ref, 'legacy_column': col})
    rave_codes = defaultdict(list)
    for doc_key, file_name, pub in conn.execute("SELECT document_key,file_name,publication_code FROM mems_doc_document WHERE document_key LIKE 'RAVEMEMS::DOC::%'"):
        if pub:
            rave_codes[pub.upper()].append({'document_key': doc_key, 'file_name': file_name})
    source_overlap = []
    for code in sorted(set(rave_codes) & set(legacy_codes)):
        source_overlap.append({'classification': 'source_overlap_candidate', 'publication_code': code,
                               'ravemems_documents': rave_codes[code], 'legacy_references': legacy_codes[code]})
    return {
        'exact_normalized_text_candidate_count': len(exact),
        'source_overlap_code_count': len(source_overlap),
        'exact_normalized_text_candidates': exact,
        'source_overlap_candidates': source_overlap,
        'automatic_deletions_performed': 0
    }


def target_legacy_schema(conn: sqlite3.Connection) -> dict:
    out = {}
    names = {r[0] for r in conn.execute("SELECT name FROM sqlite_master WHERE type='table'")}
    for t in TARGET_LEGACY_TABLES:
        if t in names:
            out[t] = {
                'count': conn.execute(f'SELECT COUNT(*) FROM {qident(t)}').fetchone()[0],
                'columns': [{'name': r[1], 'type': r[2], 'pk': r[5]} for r in conn.execute(f'PRAGMA table_info({qident(t)})')]
            }
    return out


def write_report(path: Path, base_sha: str, rave_sha: str, before: dict, after: dict, import_counts: dict, legacy_cmp: dict, dup: dict, rave_verify: dict, legacy_schema: dict, merged_sha: str):
    lines = [
        '# RAVEMEMS - AUDIT DE FUSION ADDITIVE DANS LA BASE BUILD #103', '',
        '## Regle de finalite',
        'RAVEMEMS est ajoute a une copie de la base BUILD #103. Il ne remplace aucune table ni aucune ligne historique. Aucun doublon nest supprime pendant cet audit.', '',
        '## Identite des bases',
        f'- Base BUILD #103 SHA-256 : `{base_sha}`',
        f'- Source RAVEMEMS SQLite SHA-256 : `{rave_sha}`',
        f'- Base fusionnee de test SHA-256 : `{merged_sha}`', '',
        '## Etat AVANT',
        f"- user_version : {before['user_version']}",
        f"- tables historiques : {len(before['tables'])}",
        f"- integrity_check : {before['integrity_check']}",
        f"- foreign_key_check : {len(before['foreign_key_check'])}", '',
        '## Source RAVEMEMS verifiee',
        f"- compteurs : `{json.dumps(rave_verify['counts'], sort_keys=True)}`",
        f"- assets enregistres verifies par SHA : {rave_verify['registered_assets_verified']}", '',
        '## Etat APRES AJOUT',
        f"- user_version : {after['user_version']}",
        f"- tables totales : {len(after['tables'])}",
        f"- integrity_check : {after['integrity_check']}",
        f"- foreign_key_check : {len(after['foreign_key_check'])}",
        f"- compteurs mems_doc_* : `{json.dumps(import_counts, sort_keys=True)}`", '',
        '## Preservation historique',
        f"- tables historiques modifiees : {len(legacy_cmp['changed_legacy_tables'])}",
        f"- objets historiques disparus : {len(legacy_cmp['missing_legacy_objects'])}",
        f"- RESULTAT : {'PASS - historique strictement preserve' if legacy_cmp['pass'] else 'FAIL - historique modifie'}", '',
        '## Candidats doublons / recouvrements',
        f"- correspondances de texte normalise exact : {dup['exact_normalized_text_candidate_count']}",
        f"- codes publication presents a la fois dans historique et RAVEMEMS : {dup['source_overlap_code_count']}",
        '- suppressions automatiques : 0',
        '- Ces lignes sont uniquement des candidats. La classification identique/complementaire/conflit/a revoir doit preceder toute fusion ou suppression.', '',
        '## Tables historiques RAVE/connaissance ciblees',
    ]
    for t, meta in legacy_schema.items():
        cols = ', '.join(c['name'] for c in meta['columns'])
        lines.append(f"- `{t}` : {meta['count']} lignes ; colonnes : {cols}")
    lines += ['', '## Verdict audit',
              'La fusion de test est acceptable uniquement si lintegrite/FK sont vertes, si les compteurs RAVEMEMS sont complets et si aucune table historique nest modifiee.',
              f"VERDICT : {'PASS' if legacy_cmp['pass'] and after['integrity_check'].lower() == 'ok' and not after['foreign_key_check'] else 'FAIL'}", '']
    path.write_text('\n'.join(lines), encoding='utf-8')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--base', type=Path, required=True)
    ap.add_argument('--rave-root', type=Path, required=True)
    ap.add_argument('--foundation-sql', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    base = args.base.resolve()
    rave_root = args.rave_root.resolve()
    rave_db = rave_root / 'ravemems_full_corpus.sqlite'
    merged = args.out / 'ia_mems_reference_r20_plus_ravemems_AUDIT.sqlite'
    shutil.copy2(base, merged)
    base_sha = sha_file(base)
    rave_sha = sha_file(rave_db)

    src = sqlite3.connect(rave_db)
    rave_verify = verify_rave_source(src, rave_root)
    dst = sqlite3.connect(merged)
    dst.execute('PRAGMA foreign_keys=ON')
    before = snapshot(dst)
    if before['user_version'] != 20:
        raise RuntimeError(f'BUILD #103 base user_version expected 20, got {before["user_version"]}')
    if before['integrity_check'].lower() != 'ok' or before['foreign_key_check']:
        raise RuntimeError('BUILD #103 base integrity/FK failed before import')
    before_tables = set(before['tables'])
    legacy_schema = target_legacy_schema(dst)

    foundation = args.foundation_sql.read_text(encoding='utf-8')
    dst.executescript(foundation)
    first_counts = import_ravemems(dst, src)
    first_doc_hash = table_digest(dst, 'mems_doc_entity')[1]
    second_counts = import_ravemems(dst, src)
    second_doc_hash = table_digest(dst, 'mems_doc_entity')[1]
    if first_counts != second_counts or first_doc_hash != second_doc_hash:
        raise RuntimeError('RAVEMEMS additive import is not idempotent')

    after = snapshot(dst)
    legacy_cmp = compare_legacy(before, after)
    dup = duplicate_audit(dst, before_tables)
    if after['integrity_check'].lower() != 'ok' or after['foreign_key_check']:
        raise RuntimeError('Merged database integrity/FK failed')
    if not legacy_cmp['pass']:
        raise RuntimeError('Historical BUILD #103 content changed during additive import')
    if first_counts.get('mems_doc_document') != 47 or first_counts.get('mems_doc_unit') != 1359 or first_counts.get('mems_doc_visual') != 1070:
        raise RuntimeError(f'Unexpected additive counts: {first_counts}')

    merged_sha = sha_file(merged)
    (args.out / 'audit_before.json').write_text(json.dumps(before, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    (args.out / 'audit_after.json').write_text(json.dumps(after, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    (args.out / 'duplicate_candidates.json').write_text(json.dumps(dup, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    summary = {
        'pass': True,
        'base_sha256': base_sha,
        'rave_sqlite_sha256': rave_sha,
        'merged_sha256': merged_sha,
        'before_user_version': before['user_version'],
        'after_user_version': after['user_version'],
        'historical_tables_preserved': legacy_cmp['pass'],
        'changed_historical_tables': len(legacy_cmp['changed_legacy_tables']),
        'missing_historical_objects': len(legacy_cmp['missing_legacy_objects']),
        'rave_source': rave_verify,
        'additive_counts': first_counts,
        'idempotent_second_import': True,
        'duplicate_candidate_counts': {
            'exact_normalized_text': dup['exact_normalized_text_candidate_count'],
            'source_overlap_codes': dup['source_overlap_code_count']
        },
        'automatic_deletions_performed': 0
    }
    (args.out / 'audit_summary.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    write_report(args.out / 'RAVEMEMS_ADDITIVE_MERGE_AUDIT.md', base_sha, rave_sha, before, after, first_counts, legacy_cmp, dup, rave_verify, legacy_schema, merged_sha)
    src.close()
    dst.close()
    print('RAVEMEMS_ADDITIVE_MERGE_AUDIT_PASS')
    print(json.dumps(summary, sort_keys=True))


if __name__ == '__main__':
    main()
