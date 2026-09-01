#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import math
import shutil
import sqlite3
from pathlib import Path

EXPECTED_ASSETS = 1070
EXPECTED_OCCURRENCES = 1794


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def qident(name: str) -> str:
    return '"' + name.replace('"', '""') + '"'


def enc(value):
    if isinstance(value, bytes):
        return {'__blob__': value.hex()}
    return value


def table_digest(conn: sqlite3.Connection, table: str) -> tuple[int, str]:
    cols = [r[1] for r in conn.execute(f'PRAGMA table_info({qident(table)})')]
    sql = f"SELECT {','.join(qident(c) for c in cols)} FROM {qident(table)}"
    if cols:
        sql += ' ORDER BY ' + ','.join(qident(c) for c in cols)
    h = hashlib.sha256()
    count = 0
    for row in conn.execute(sql):
        payload = json.dumps([enc(v) for v in row], ensure_ascii=False, sort_keys=True, separators=(',', ':')) + '\n'
        h.update(payload.encode('utf-8'))
        count += 1
    return count, h.hexdigest()


def historical_snapshot(conn: sqlite3.Connection) -> dict:
    names = [r[0] for r in conn.execute(
        "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%' AND name NOT LIKE 'mems_doc_%' ORDER BY name"
    )]
    out = {}
    for name in names:
        count, digest = table_digest(conn, name)
        out[name] = {'count': count, 'sha256': digest}
    return out


def bbox_from_notes(notes: str | None):
    try:
        obj = json.loads(notes or '{}')
        bbox = obj.get('bbox_json')
        if isinstance(bbox, str):
            bbox = json.loads(bbox)
        if isinstance(bbox, dict):
            vals = [bbox.get(k) for k in ('x0', 'y0', 'x1', 'y1')]
        elif isinstance(bbox, list) and len(bbox) >= 4:
            vals = bbox[:4]
        else:
            return None
        if all(isinstance(v, (int, float)) for v in vals):
            return [float(v) for v in vals]
    except Exception:
        return None
    return None


def center(bbox):
    return ((bbox[0] + bbox[2]) / 2.0, (bbox[1] + bbox[3]) / 2.0)


def context_score(occ_bbox, block_bbox):
    if not occ_bbox or not block_bbox:
        return math.inf
    ocx, ocy = center(occ_bbox)
    bcx, bcy = center(block_bbox)
    vertical_overlap = max(0.0, min(occ_bbox[3], block_bbox[3]) - max(occ_bbox[1], block_bbox[1]))
    score = abs(ocy - bcy) * 4.0 + abs(ocx - bcx)
    if vertical_overlap > 0:
        score *= 0.5
    return score


def clean_text(value: str) -> str:
    return ' '.join((value or '').split())


def build_context(pub: str | None, title: str, page: int, snippets: list[str]) -> str:
    head = ' | '.join(x for x in [pub or '', title or '', f'physical page {page}'] if x)
    body = ' '.join(clean_text(x) for x in snippets if clean_text(x))
    return (head + (' | ' + body if body else '')).strip()


def add_visual_context(conn: sqlite3.Connection) -> dict:
    occurrences = conn.execute(
        "SELECT entity_key,unit_key,notes,legacy_key FROM mems_doc_entity WHERE entity_kind='visual_occurrence' ORDER BY entity_key"
    ).fetchall()
    if len(occurrences) != EXPECTED_OCCURRENCES:
        raise RuntimeError(f'visual occurrence count mismatch: {len(occurrences)}')

    manifest = []
    for occ_entity, unit_key, occ_notes, source_occurrence_key in occurrences:
        page = conn.execute(
            """SELECT p.entity_key,u.physical_page,d.document_key,d.publication_code,d.title_source
               FROM mems_doc_entity p
               JOIN mems_doc_unit u ON u.unit_key=p.unit_key
               JOIN mems_doc_document d ON d.document_key=p.document_key
               WHERE p.entity_kind='page' AND p.unit_key=?""",
            (unit_key,),
        ).fetchone()
        if page is None:
            raise RuntimeError(f'missing page entity for {occ_entity}')
        page_entity, physical_page, document_key, publication_code, title_source = page

        asset = conn.execute(
            """SELECT r.to_entity_key,v.relative_path,v.sha256
               FROM mems_doc_relation r
               JOIN mems_doc_visual v ON v.entity_key=r.to_entity_key
               WHERE r.from_entity_key=? AND r.relation_type='uses_visual'""",
            (occ_entity,),
        ).fetchone()
        if asset is None:
            raise RuntimeError(f'missing visual asset for {occ_entity}')
        asset_entity, stored_path, asset_sha = asset

        blocks = conn.execute(
            """SELECT e.entity_key,e.notes,t.text_value,e.sort_order
               FROM mems_doc_entity e
               LEFT JOIN mems_doc_text t
                 ON t.entity_key=e.entity_key AND t.field_key='source_exact' AND t.locale='en'
               WHERE e.unit_key=? AND e.entity_kind='content_block'
               ORDER BY e.sort_order""",
            (unit_key,),
        ).fetchall()

        occ_bbox = bbox_from_notes(occ_notes)
        scored = []
        fallback = []
        for entity_key, notes, text_value, sort_order in blocks:
            text = clean_text(text_value or '')
            if not text:
                continue
            fallback.append((sort_order, entity_key, text))
            score = context_score(occ_bbox, bbox_from_notes(notes))
            if math.isfinite(score):
                scored.append((score, sort_order, entity_key, text))

        if scored:
            scored.sort(key=lambda x: (x[0], x[1], x[2]))
            selected = [(entity_key, text) for _, _, entity_key, text in scored[:3]]
        else:
            fallback.sort(key=lambda x: (x[0], x[1]))
            selected = [(entity_key, text) for _, entity_key, text in fallback[:3]]

        if selected:
            snippets = [text[:800] for _, text in selected]
        else:
            page_text = conn.execute(
                "SELECT text_value FROM mems_doc_text WHERE entity_key=? AND field_key='source_exact' AND locale='en'",
                (page_entity,),
            ).fetchone()
            snippets = [clean_text(page_text[0])[:1200]] if page_text and clean_text(page_text[0]) else []

        context_text = build_context(publication_code, title_source, physical_page, snippets)
        if not context_text:
            context_text = f'{document_key} | physical page {physical_page}'

        conn.execute(
            """INSERT INTO mems_doc_text
               (entity_key,field_key,locale,text_value,translation_status,translation_method,source_locale,notes)
               VALUES(?,'visual_context','en',?,'draft','derived_visual_context','en',?)
               ON CONFLICT(entity_key,field_key,locale) DO UPDATE SET
                 text_value=excluded.text_value,
                 translation_status=excluded.translation_status,
                 translation_method=excluded.translation_method,
                 source_locale=excluded.source_locale,
                 notes=excluded.notes""",
            (occ_entity, context_text, json.dumps({
                'purpose': 'IA visual retrieval context',
                'ui_label': 'Voir le schéma',
                'pure_text_page_link_ui': False,
            }, ensure_ascii=False, sort_keys=True)),
        )

        for block_entity, _ in selected:
            conn.execute(
                """INSERT OR IGNORE INTO mems_doc_relation(from_entity_key,relation_type,to_entity_key,notes)
                   VALUES(?,'visual_context_for',?,?)""",
                (block_entity, occ_entity, json.dumps({'derived': True, 'scope': 'same_page_nearest_content'}, sort_keys=True)),
            )

        conn.execute(
            """INSERT OR IGNORE INTO mems_doc_relation(from_entity_key,relation_type,to_entity_key,notes)
               VALUES(?,'page_visual_asset',?,?)""",
            (page_entity, asset_entity, json.dumps({'ui_label': 'Voir le schéma', 'all_ravemems_visuals': True}, ensure_ascii=False, sort_keys=True)),
        )

        manifest.append({
            'occurrence_entity_key': occ_entity,
            'source_occurrence_key': source_occurrence_key,
            'asset_entity_key': asset_entity,
            'stored_path': stored_path,
            'sha256': asset_sha,
            'document_key': document_key,
            'publication_code': publication_code,
            'physical_page': physical_page,
            'context_text': context_text,
            'ui_label': 'Voir le schéma',
        })

    conn.commit()
    return {'manifest': manifest}


def copy_assets(conn: sqlite3.Connection, rave_root: Path, bundle_root: Path) -> dict:
    rows = conn.execute('SELECT entity_key,relative_path,sha256,notes FROM mems_doc_visual ORDER BY entity_key').fetchall()
    if len(rows) != EXPECTED_ASSETS:
        raise RuntimeError(f'visual asset count mismatch: {len(rows)}')
    copied = 0
    total_bytes = 0
    for entity_key, relative_path, expected_sha, notes in rows:
        try:
            meta = json.loads(notes or '{}')
        except Exception:
            meta = {}
        original = meta.get('original_relative_path')
        if not original:
            prefix = 'ravemems_run4/'
            original = relative_path[len(prefix):] if relative_path.startswith(prefix) else relative_path
        src = rave_root / original
        if not src.is_file():
            raise RuntimeError(f'missing visual source file: {original}')
        if sha256_file(src) != expected_sha:
            raise RuntimeError(f'visual SHA mismatch: {original}')
        dst = bundle_root / original
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)
        if sha256_file(dst) != expected_sha:
            raise RuntimeError(f'copied visual SHA mismatch: {original}')
        copied += 1
        total_bytes += dst.stat().st_size
    return {'bundled_assets': copied, 'bundle_bytes': total_bytes}


def validate(conn: sqlite3.Connection) -> dict:
    if conn.execute('PRAGMA integrity_check').fetchone()[0].lower() != 'ok':
        raise RuntimeError('integrity_check failed')
    fk = conn.execute('PRAGMA foreign_key_check').fetchall()
    if fk:
        raise RuntimeError(f'foreign_key_check failed: {fk[:5]}')
    assets = conn.execute('SELECT COUNT(*) FROM mems_doc_visual').fetchone()[0]
    occurrences = conn.execute("SELECT COUNT(*) FROM mems_doc_entity WHERE entity_kind='visual_occurrence'").fetchone()[0]
    uses_visual = conn.execute("SELECT COUNT(*) FROM mems_doc_relation WHERE relation_type='uses_visual'").fetchone()[0]
    contains_occ = conn.execute("SELECT COUNT(*) FROM mems_doc_relation WHERE relation_type='contains_visual_occurrence'").fetchone()[0]
    contexts = conn.execute("SELECT COUNT(*) FROM mems_doc_text WHERE field_key='visual_context' AND locale='en'").fetchone()[0]
    page_asset = conn.execute("SELECT COUNT(*) FROM mems_doc_relation WHERE relation_type='page_visual_asset'").fetchone()[0]
    visual_context_for = conn.execute("SELECT COUNT(*) FROM mems_doc_relation WHERE relation_type='visual_context_for'").fetchone()[0]
    if assets != EXPECTED_ASSETS or occurrences != EXPECTED_OCCURRENCES or uses_visual != EXPECTED_OCCURRENCES or contains_occ != EXPECTED_OCCURRENCES or contexts != EXPECTED_OCCURRENCES:
        raise RuntimeError(f'visual validation mismatch assets={assets} occurrences={occurrences} uses={uses_visual} contains={contains_occ} contexts={contexts}')
    return {
        'assets': assets,
        'occurrences': occurrences,
        'uses_visual_relations': uses_visual,
        'contains_visual_occurrence_relations': contains_occ,
        'visual_context_rows': contexts,
        'page_visual_asset_relations': page_asset,
        'visual_context_for_relations': visual_context_for,
    }


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument('--merged-db', required=True)
    ap.add_argument('--rave-root', required=True)
    ap.add_argument('--out', required=True)
    args = ap.parse_args()

    src_db = Path(args.merged_db)
    rave_root = Path(args.rave_root)
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    out_db = out / 'ia_mems_reference_r20_plus_ravemems_VISUAL.sqlite'
    if out_db.exists():
        out_db.unlink()
    shutil.copy2(src_db, out_db)

    conn = sqlite3.connect(out_db)
    before = historical_snapshot(conn)
    first = add_visual_context(conn)
    first_validation = validate(conn)
    counts_after_first = {
        'texts': conn.execute('SELECT COUNT(*) FROM mems_doc_text').fetchone()[0],
        'relations': conn.execute('SELECT COUNT(*) FROM mems_doc_relation').fetchone()[0],
    }

    second = add_visual_context(conn)
    second_validation = validate(conn)
    counts_after_second = {
        'texts': conn.execute('SELECT COUNT(*) FROM mems_doc_text').fetchone()[0],
        'relations': conn.execute('SELECT COUNT(*) FROM mems_doc_relation').fetchone()[0],
    }
    idempotent = counts_after_first == counts_after_second and first_validation == second_validation

    after = historical_snapshot(conn)
    historical_preserved = before == after
    if not historical_preserved:
        raise RuntimeError('historical tables changed')

    bundle_dir = out / 'visuals'
    if bundle_dir.exists():
        shutil.rmtree(bundle_dir)
    bundle = copy_assets(conn, rave_root, bundle_dir)

    manifest = first['manifest']
    if len(manifest) != EXPECTED_OCCURRENCES:
        raise RuntimeError('manifest occurrence count mismatch')
    unique_assets = {x['asset_entity_key'] for x in manifest}
    if len(unique_assets) != EXPECTED_ASSETS:
        raise RuntimeError(f'manifest unique asset count mismatch: {len(unique_assets)}')
    if not idempotent:
        raise RuntimeError('visual context linking is not idempotent')

    (out / 'visual_bundle_manifest.json').write_text(json.dumps({
        'ui_label': 'Voir le schéma',
        'pure_text_page_link_ui': False,
        'asset_count': EXPECTED_ASSETS,
        'occurrence_count': EXPECTED_OCCURRENCES,
        'entries': manifest,
    }, ensure_ascii=False, indent=2, sort_keys=True), encoding='utf-8')

    summary = {
        'pass': True,
        'historical_tables_preserved': historical_preserved,
        'historical_table_count': len(before),
        'idempotent_second_link_pass': idempotent,
        'first_validation': first_validation,
        'second_validation': second_validation,
        'bundle': bundle,
        'manifest_occurrences': len(manifest),
        'manifest_unique_assets': len(unique_assets),
        'ui_label': 'Voir le schéma',
        'pure_text_page_link_ui': False,
        'database_sha256': sha256_file(out_db),
    }
    (out / 'visual_bundle_summary.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2, sort_keys=True), encoding='utf-8')
    print(json.dumps(summary, ensure_ascii=False, sort_keys=True))
    conn.close()
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
