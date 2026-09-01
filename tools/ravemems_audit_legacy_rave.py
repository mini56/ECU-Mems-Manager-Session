#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sqlite3
from collections import Counter, defaultdict
from pathlib import Path

RAVE_RX = re.compile(r"(?:\bRAVE\b|RCL\d{4}(?:ENG|FRE)?|AKM\d{4}(?:ENG|FRE)?|wmxn|cdxn|libxn|rave/)", re.I)
CODE_RX = re.compile(r"\b(?:RCL\d{4}(?:ENG|FRE)?|AKM\d{4}(?:ENG|FRE)?)\b", re.I)
PAGE_RX = re.compile(r"(?:PDF\s*)?[Pp](?:age)?\.?\s*(\d{1,4})|\bPDF[_\s-]*(\d{1,4})\b", re.I)
TOKEN_RX = re.compile(r"[A-Za-z0-9]{3,}")
STOP = {
    'rave','schema','diagramme','diagram','image','images','page','pdf','rover','mini','eng','fre','the','and','for','with',
    'des','les','une','pour','avec','sur','dans','mems','source','physical','voir','ouvrir','section','codes','colour','couleur'
}


def qident(name: str) -> str:
    return '"' + name.replace('"', '""') + '"'


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def safe(v):
    if isinstance(v, bytes):
        return {'__blob_sha256__': hashlib.sha256(v).hexdigest(), 'bytes': len(v)}
    return v


def columns(conn: sqlite3.Connection, table: str):
    return [r[1] for r in conn.execute(f'PRAGMA table_info({qident(table)})')]


def rows_as_dict(conn: sqlite3.Connection, table: str):
    cols = columns(conn, table)
    return [dict(zip(cols, [safe(v) for v in row])) for row in conn.execute(f'SELECT * FROM {qident(table)}')]


def extract_codes(text: str):
    return sorted({m.group(0).upper() for m in CODE_RX.finditer(text or '')})


def extract_pages(row: dict):
    pages = set()
    for k, v in row.items():
        if v is None:
            continue
        s = str(v)
        if 'page' in k.lower() and s.isdigit():
            pages.add(int(s))
        for m in PAGE_RX.finditer(s):
            n = m.group(1) or m.group(2)
            if n:
                pages.add(int(n))
    return sorted(pages)


def tokens(text: str):
    result = []
    for t in TOKEN_RX.findall((text or '').lower()):
        if t in STOP or t.isdigit() or len(t) < 4:
            continue
        if t not in result:
            result.append(t)
    return result


def best_context_match(key: str, contexts: list[dict], codes: list[str]):
    kt = set(tokens(key))
    if not kt:
        return None
    best = None
    for c in contexts:
        pub = (c.get('publication_code') or '').upper()
        if codes and pub and not any(code == pub or code in pub or pub in code for code in codes):
            continue
        ct = set(tokens(c.get('context_text') or ''))
        if not ct:
            continue
        hit = len(kt & ct)
        ratio = hit / len(kt)
        score = (ratio, hit)
        if best is None or score > best[0]:
            best = (score, c)
    if not best:
        return None
    (ratio, hit), c = best
    return {
        'score': round(ratio, 4),
        'token_hits': hit,
        'occurrence_entity_key': c.get('entity_key'),
        'publication_code': c.get('publication_code'),
        'physical_page': c.get('physical_page'),
        'context_text': (c.get('context_text') or '')[:700],
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--db', required=True)
    ap.add_argument('--old-root', required=True)
    ap.add_argument('--out', required=True)
    args = ap.parse_args()

    db = Path(args.db)
    old_root = Path(args.old_root)
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)

    conn = sqlite3.connect(f'file:{db.as_posix()}?mode=ro', uri=True)
    integrity = conn.execute('PRAGMA integrity_check').fetchone()[0]
    fk = conn.execute('PRAGMA foreign_key_check').fetchall()
    if str(integrity).lower() != 'ok' or fk:
        raise SystemExit('database integrity/FK failed')

    all_tables = [r[0] for r in conn.execute("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%' ORDER BY name")]
    dedicated_names = ['mems_rave_fact', 'mems_rave_illustration', 'mems_rave_illustration_link']
    dedicated = {}
    for table in dedicated_names:
        if table not in all_tables:
            raise SystemExit(f'missing expected legacy table {table}')
        rr = rows_as_dict(conn, table)
        dedicated[table] = {'count': len(rr), 'columns': columns(conn, table), 'rows': rr}

    new_assets = conn.execute('SELECT COUNT(*) FROM mems_doc_visual').fetchone()[0]
    new_occ = conn.execute("SELECT COUNT(*) FROM mems_doc_entity WHERE entity_kind='visual_occurrence'").fetchone()[0]
    new_visual_shas = {r[0] for r in conn.execute('SELECT sha256 FROM mems_doc_visual WHERE sha256 IS NOT NULL')}

    doc_cols = set(columns(conn, 'mems_doc_document'))
    required_doc_cols = {'document_key', 'publication_code', 'title_source'}
    if not required_doc_cols.issubset(doc_cols):
        raise SystemExit(f'unexpected mems_doc_document schema: {sorted(doc_cols)}')
    optional = [c for c in ('source_key', 'file_name', 'notes') if c in doc_cols]
    select_cols = ['document_key', 'publication_code', 'title_source'] + optional
    docs = [dict(zip(select_cols, row)) for row in conn.execute(
        'SELECT ' + ','.join(qident(c) for c in select_cols) + ' FROM mems_doc_document'
    )]
    docs_by_code = defaultdict(list)
    for d in docs:
        pub = (d.get('publication_code') or '').upper()
        if pub:
            docs_by_code[pub].append(d)

    visual_contexts = []
    for row in conn.execute("""
        SELECT e.entity_key,d.publication_code,u.physical_page,t.text_value
        FROM mems_doc_entity e
        JOIN mems_doc_unit u ON u.unit_key=e.unit_key
        JOIN mems_doc_document d ON d.document_key=e.document_key
        LEFT JOIN mems_doc_text t ON t.entity_key=e.entity_key AND t.field_key='visual_context' AND t.locale='en'
        WHERE e.entity_kind='visual_occurrence'
        ORDER BY e.entity_key
    """):
        visual_contexts.append({
            'entity_key': row[0], 'publication_code': row[1], 'physical_page': row[2], 'context_text': row[3] or ''
        })

    visual_pages = defaultdict(int)
    for c in visual_contexts:
        visual_pages[((c.get('publication_code') or '').upper(), int(c.get('physical_page') or 0))] += 1

    def matching_pubs(code: str):
        code = code.upper()
        return [pub for pub in docs_by_code if code == pub or code in pub or pub in code]

    def doc_covered(code: str):
        return bool(matching_pubs(code))

    def page_visual_covered(codes: list[str], pages: list[int]):
        hits = []
        for code in codes:
            for pub in matching_pubs(code):
                for page in pages:
                    count = visual_pages.get((pub, page), 0)
                    if count:
                        hits.append({'publication_code': pub, 'physical_page': page, 'visual_occurrences': count})
        return hits

    fact_audit = []
    for row in dedicated['mems_rave_fact']['rows']:
        text = ' | '.join(str(v) for v in row.values() if v is not None)
        codes = extract_codes(text)
        pages = extract_pages(row)
        fact_audit.append({
            'legacy_row': row,
            'codes': codes,
            'pages': pages,
            'source_document_covered': any(doc_covered(c) for c in codes),
            'source_page_has_ravemems_visual': page_visual_covered(codes, pages),
            'treatment': 'a_migrer_conserver',
            'reason': 'structured historical knowledge is not equivalent to raw/documentary RAVEMEMS text or imagery',
        })

    historical_scan = {}
    for table in all_tables:
        if table.startswith('mems_doc_'):
            continue
        cols = columns(conn, table)
        if not cols:
            continue
        matched = []
        for raw in conn.execute(f'SELECT * FROM {qident(table)}'):
            vals = [safe(v) for v in raw]
            text = ' | '.join(str(v) for v in vals if v is not None)
            if RAVE_RX.search(text):
                matched.append(dict(zip(cols, vals)))
        if matched:
            historical_scan[table] = {'count': len(matched), 'rows': matched}

    manifest_path = old_root / 'database/reference/manifest.json'
    if not manifest_path.is_file():
        raise SystemExit(f'missing old manifest: {manifest_path}')
    manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
    old_diagrams = manifest.get('diagrams', {})
    legacy_diagrams = {k: v for k, v in old_diagrams.items() if isinstance(v, str) and v.replace('\\','/').startswith('images/rave/')}

    image_root = old_root / 'database/reference/images/rave'
    image_files = sorted(p for p in image_root.rglob('*') if p.is_file()) if image_root.is_dir() else []
    image_sha = {p.relative_to(old_root / 'database/reference').as_posix(): sha256_file(p) for p in image_files}
    manifest_paths = {v.replace('\\','/') for v in legacy_diagrams.values()}

    manifest_audit = []
    for key, rel in sorted(legacy_diagrams.items()):
        rel = rel.replace('\\','/')
        abs_path = old_root / 'database/reference' / rel
        sha = sha256_file(abs_path) if abs_path.is_file() else None
        codes = extract_codes(key + ' ' + rel)
        pages = extract_pages({'key': key, 'path': rel})
        exact_sha = bool(sha and sha in new_visual_shas)
        page_hits = page_visual_covered(codes, pages)
        ctx = best_context_match(key, visual_contexts, codes)
        source_ok = any(doc_covered(c) for c in codes)
        if exact_sha:
            cls = 'remplace_par_ravemems_exact_binaire'
        elif page_hits:
            cls = 'remplace_par_ravemems_candidat_page_visuelle'
        elif ctx and ctx['score'] >= 0.45 and ctx['token_hits'] >= 2:
            cls = 'remplace_par_ravemems_candidat_contexte'
        elif source_ok:
            cls = 'a_revoir_source_couverte_sans_equivalence_visuelle_prouvee'
        else:
            cls = 'a_revoir_source_non_resolue'
        manifest_audit.append({
            'key': key, 'relative_path': rel, 'exists': abs_path.is_file(), 'sha256': sha,
            'codes': codes, 'pages': pages, 'exact_ravemems_sha_match': exact_sha,
            'page_visual_hits': page_hits, 'best_context_match': ctx, 'classification': cls,
        })

    illustration_audit = []
    for row in dedicated['mems_rave_illustration']['rows']:
        text = ' | '.join(str(v) for v in row.values() if v is not None)
        codes = extract_codes(text)
        pages = extract_pages(row)
        hits = page_visual_covered(codes, pages)
        illustration_audit.append({
            'legacy_row': row, 'codes': codes, 'pages': pages, 'page_visual_hits': hits,
            'classification': 'remplace_par_ravemems_candidat' if hits else 'a_revoir_ou_conserver'
        })

    manifest_counts = Counter(x['classification'] for x in manifest_audit)
    illustration_counts = Counter(x['classification'] for x in illustration_audit)
    orphan_files = sorted(set(image_sha) - manifest_paths)
    missing_manifest_files = sorted(manifest_paths - set(image_sha))
    exact_old_image_hashes = sum(1 for s in image_sha.values() if s in new_visual_shas)

    summary = {
        'pass': True,
        'read_only': True,
        'integrity_check': integrity,
        'foreign_key_errors': len(fk),
        'legacy_dedicated_counts': {t: dedicated[t]['count'] for t in dedicated_names},
        'legacy_structured_fact_treatment': {'a_migrer_conserver': len(fact_audit)},
        'historical_tables_with_rave_references': {t: v['count'] for t, v in historical_scan.items()},
        'new_ravemems': {'visual_assets': new_assets, 'visual_occurrences': new_occ, 'documents': len(docs)},
        'legacy_manifest': {
            'rave_diagram_entries': len(legacy_diagrams),
            'classification_counts': dict(sorted(manifest_counts.items())),
            'missing_declared_files': len(missing_manifest_files),
        },
        'legacy_image_files': {
            'count': len(image_files),
            'exact_sha_matches_in_ravemems': exact_old_image_hashes,
            'orphan_files_not_in_manifest': len(orphan_files),
        },
        'legacy_illustrations': {
            'count': len(illustration_audit),
            'classification_counts': dict(sorted(illustration_counts.items())),
        },
        'recommended_treatment': [
            'KEEP/MIGRATE all structured legacy RAVE facts and expert mirrors unless a later semantic audit proves exact replacement.',
            'Only the old visual/catalog layer may become removable after each legacy image/illustration has an explicit RAVEMEMS replacement mapping.',
            'Do not delete legacy manifest/image entries still classified a_revoir.',
            'Perform any future removal only on a copy, with before/after historical snapshots and viewer self-tests.',
        ],
    }

    expected = {'mems_rave_fact': 177, 'mems_rave_illustration': 126, 'mems_rave_illustration_link': 329}
    if summary['legacy_dedicated_counts'] != expected:
        summary['pass'] = False
        summary['error'] = f'legacy counts differ from protected BUILD #103 expectation: {summary["legacy_dedicated_counts"]}'
    if new_assets != 1070 or new_occ != 1794:
        summary['pass'] = False
        summary['error_visual'] = {'assets': new_assets, 'occurrences': new_occ}

    files = {
        'legacy_rave_audit_summary.json': summary,
        'legacy_rave_facts.json': fact_audit,
        'legacy_rave_illustrations.json': illustration_audit,
        'legacy_rave_manifest_visuals.json': manifest_audit,
        'historical_rave_reference_scan.json': historical_scan,
        'legacy_image_sha256.json': image_sha,
        'legacy_orphan_images.json': {'orphans': orphan_files, 'missing_manifest_files': missing_manifest_files},
    }
    for name, payload in files.items():
        (out / name).write_text(json.dumps(payload, ensure_ascii=False, indent=2, sort_keys=True), encoding='utf-8')

    md = [
        '# Legacy RAVE #103 vs RAVEMEMS audit', '',
        f"- PASS: **{summary['pass']}**", f"- read-only: **{summary['read_only']}**",
        f"- old facts: **177** -> treatment **KEEP/MIGRATE**",
        f"- old illustrations: **126**", f"- old illustration links: **329**",
        f"- old manifest RAVE diagrams: **{len(legacy_diagrams)}**",
        f"- old local RAVE image files: **{len(image_files)}**",
        f"- RAVEMEMS assets/occurrences: **{new_assets} / {new_occ}**", '',
        '## Manifest classifications'
    ]
    md += [f'- {k}: {v}' for k, v in sorted(manifest_counts.items())]
    md += ['', '## Old illustration classifications']
    md += [f'- {k}: {v}' for k, v in sorted(illustration_counts.items())]
    md += ['', '## Decision gate',
           '- No deletion is performed by this audit.',
           '- Structured legacy RAVE facts/mirrors are preserved because they are curated knowledge, not mere copies of visuals.',
           '- A legacy visual can only be removed after an explicit RAVEMEMS replacement mapping exists.',
           '- Any unresolved visual remains in place until reviewed.']
    (out / 'legacy_rave_audit.md').write_text('\n'.join(md) + '\n', encoding='utf-8')

    print(json.dumps(summary, ensure_ascii=False, sort_keys=True))
    if not summary['pass']:
        raise SystemExit(2)


if __name__ == '__main__':
    main()
