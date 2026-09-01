#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import sqlite3
from collections import Counter, defaultdict
from pathlib import Path

CODE_ROOT_RX = re.compile(r'\b(RCL\d{4}|AKM\d{4})', re.I)
PAGE_PATTERNS = [
    re.compile(r'PDF[_\s.-]*(\d{1,4})', re.I),
    re.compile(r'\bP(?:AGE)?[_\s.-]*(\d{1,4})\b', re.I),
]
SECTION_RX = re.compile(r'(?<!\d)(\d{1,3})[._-](\d{1,3})(?!\d)')
CHAPTER_RX = re.compile(r'CH(\d{1,3})[_-](\d{1,3})[_-](\d{1,3})', re.I)


def qident(name: str) -> str:
    return '"' + name.replace('"', '""') + '"'


def norm(s) -> str:
    return re.sub(r'[^a-z0-9]+', '', str(s or '').lower())


def cols(conn, table):
    return [r[1] for r in conn.execute(f'PRAGMA table_info({qident(table)})')]


def pk_cols(conn, table):
    rows = conn.execute(f'PRAGMA table_info({qident(table)})').fetchall()
    return [r[1] for r in sorted(rows, key=lambda r: r[5]) if r[5]]


def rows_dict(conn, table):
    cc = cols(conn, table)
    return [dict(zip(cc, row)) for row in conn.execute(f'SELECT * FROM {qident(table)}')]


def row_text(row: dict) -> str:
    return ' | '.join(str(v) for v in row.values() if v is not None)


def code_roots(text: str):
    return sorted({m.group(1).upper() for m in CODE_ROOT_RX.finditer(text or '')})


def page_candidates(text: str):
    out=[]
    for rx in PAGE_PATTERNS:
        for m in rx.finditer(text or ''):
            n=int(m.group(1))
            if 1 <= n <= 5000 and n not in out:
                out.append(n)
    return out


def locator_candidates(text: str):
    out=[]
    for m in CHAPTER_RX.finditer(text or ''):
        # Old keys such as AKM6348_FR_CH86_86_01 represent printed section/page 86-1.
        for s in (f'{m.group(2)}.{int(m.group(3))}', f'{m.group(2)}-{int(m.group(3))}', f'{m.group(2)}/{int(m.group(3))}'):
            if s not in out: out.append(s)
    for m in SECTION_RX.finditer(text or ''):
        a,b=m.group(1),str(int(m.group(2)))
        for s in (f'{a}.{b}', f'{a}-{b}', f'{a}/{b}'):
            if s not in out: out.append(s)
    return out


def png_size(path: Path):
    try:
        data=path.read_bytes()[:32]
        if data[:8] == b'\x89PNG\r\n\x1a\n' and data[12:16] == b'IHDR':
            return int.from_bytes(data[16:20],'big'), int.from_bytes(data[20:24],'big')
    except Exception:
        pass
    return None, None


def root_match_score(root: str, doc: dict):
    blob=' '.join(str(doc.get(k) or '') for k in ('publication_code','file_name','source_key','title_source','notes'))
    roots=code_roots(blob)
    if root in roots: return 100
    nr=norm(root)
    nb=norm(blob)
    return 70 if nr and nr in nb else 0


def resolve_docs(roots, docs):
    scored=[]
    for d in docs:
        score=max([root_match_score(r,d) for r in roots] or [0])
        if score:
            scored.append((score,d))
    scored.sort(key=lambda x:(-x[0], str(x[1].get('document_key'))))
    if not scored: return []
    top=scored[0][0]
    return [d for s,d in scored if s==top]


def unit_score(unit: dict, pages, locators, text: str):
    score=0
    reasons=[]
    p=unit.get('physical_page')
    if p is not None and int(p) in pages:
        score += 120; reasons.append(f'physical_page={p}')
    printed=str(unit.get('printed_ref') or '')
    section=str(unit.get('section_ref') or '')
    for loc in locators:
        nl=norm(loc)
        if nl and nl in (norm(printed), norm(section)):
            score += 150; reasons.append(f'locator={loc}')
        elif nl and (nl in norm(printed) or nl in norm(section)):
            score += 90; reasons.append(f'locator_partial={loc}')
    nt=norm(text)
    for val,label in ((printed,'printed_ref'),(section,'section_ref')):
        nv=norm(val)
        if len(nv)>=3 and nv in nt:
            score += 35; reasons.append(f'{label}_in_old_key={val}')
    return score,reasons


def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--db', required=True)
    ap.add_argument('--old-root', required=True)
    ap.add_argument('--out', required=True)
    a=ap.parse_args()
    out=Path(a.out); out.mkdir(parents=True, exist_ok=True)
    old_root=Path(a.old_root)
    conn=sqlite3.connect(f'file:{Path(a.db).as_posix()}?mode=ro', uri=True)
    conn.row_factory=sqlite3.Row
    assert conn.execute('PRAGMA integrity_check').fetchone()[0].lower()=='ok'
    assert not conn.execute('PRAGMA foreign_key_check').fetchall()

    docs=[dict(r) for r in conn.execute('SELECT * FROM mems_doc_document ORDER BY document_key')]
    units=defaultdict(list)
    for r in conn.execute('SELECT * FROM mems_doc_unit ORDER BY document_key,sequence_no,unit_key'):
        units[r['document_key']].append(dict(r))

    occ_by_unit=defaultdict(list)
    for r in conn.execute('''
      SELECT e.entity_key,e.unit_key,v.entity_key asset_entity_key,v.relative_path,v.sha256,v.visual_kind,
             t.text_value visual_context
      FROM mems_doc_entity e
      JOIN mems_doc_relation rel ON rel.from_entity_key=e.entity_key AND rel.relation_type='uses_visual'
      JOIN mems_doc_visual v ON v.entity_key=rel.to_entity_key
      LEFT JOIN mems_doc_text t ON t.entity_key=e.entity_key AND t.field_key='visual_context' AND t.locale='en'
      WHERE e.entity_kind='visual_occurrence'
      ORDER BY e.entity_key
    '''):
        occ_by_unit[r['unit_key']].append(dict(r))

    page_text_units=set(r[0] for r in conn.execute('''
      SELECT DISTINCT e.unit_key FROM mems_doc_entity e
      JOIN mems_doc_text t ON t.entity_key=e.entity_key
      WHERE e.unit_key IS NOT NULL AND t.field_key='source_exact' AND length(trim(t.text_value))>0
    '''))

    illustrations=rows_dict(conn,'mems_rave_illustration')
    links=rows_dict(conn,'mems_rave_illustration_link')
    ill_pk=pk_cols(conn,'mems_rave_illustration')
    link_cols=cols(conn,'mems_rave_illustration_link')

    # Build indexes over old illustration metadata. Exact path/basename/key evidence gets priority.
    ill_index=defaultdict(list)
    for i,row in enumerate(illustrations):
        txt=row_text(row)
        vals=[str(v) for v in row.values() if v is not None]
        for v in vals:
            nv=norm(v)
            if len(nv)>=5:
                ill_index[nv].append(i)
        # Also index whole row normalized for substring fallback.
        ill_index[norm(txt)].append(i)

    manifest=json.loads((old_root/'database/reference/manifest.json').read_text(encoding='utf-8'))
    old_entries={k:v.replace('\\','/') for k,v in manifest.get('diagrams',{}).items()
                 if isinstance(v,str) and v.replace('\\','/').startswith('images/rave/')}
    assert len(old_entries)==427, len(old_entries)

    def attached_illustrations(key,path):
        probes=[norm(key),norm(path),norm(Path(path).name),norm(Path(path).stem)]
        found=set()
        for i,row in enumerate(illustrations):
            rt=norm(row_text(row))
            if any(p and (p==rt or p in rt or rt in p) for p in probes if len(p)>=5):
                found.add(i)
        return sorted(found)

    mapping=[]
    for key,path in sorted(old_entries.items()):
        img=old_root/'database/reference'/path
        w,h=png_size(img)
        attached=attached_illustrations(key,path)
        meta=' | '.join([key,path]+[row_text(illustrations[i]) for i in attached])
        roots=code_roots(meta)
        pages=page_candidates(meta)
        locators=locator_candidates(meta)
        doc_candidates=resolve_docs(roots,docs)
        unit_candidates=[]
        for d in doc_candidates:
            for u in units[d['document_key']]:
                sc,reasons=unit_score(u,pages,locators,meta)
                if sc:
                    unit_candidates.append((sc,reasons,d,u))
        unit_candidates.sort(key=lambda x:(-x[0], str(x[3].get('unit_key'))))
        best_score=unit_candidates[0][0] if unit_candidates else 0
        best=[x for x in unit_candidates if x[0]==best_score]

        occurrence_rows=[]
        for sc,reasons,d,u in best:
            for occ in occ_by_unit.get(u['unit_key'],[]):
                occurrence_rows.append({
                    'document_key':d['document_key'],
                    'publication_code':d.get('publication_code'),
                    'unit_key':u['unit_key'],
                    'physical_page':u.get('physical_page'),
                    'printed_ref':u.get('printed_ref'),
                    'section_ref':u.get('section_ref'),
                    'unit_match_score':sc,
                    'unit_match_reasons':reasons,
                    **occ,
                })
        has_text=any(u['unit_key'] in page_text_units for _,_,_,u in best)
        if occurrence_rows:
            cls='remplacement_ravemems_prouve'
            reason='same source document and exact page/section resolves to one or more RAVEMEMS visual occurrences'
        elif best and has_text:
            cls='ancienne_capture_page_texte_sans_visuel_ravemems'
            reason='same source unit resolved and has source text but no RAVEMEMS visual occurrence; keep text provenance, no UI visual needed'
        elif best:
            cls='ancienne_entree_source_resolue_sans_contenu_visuel'
            reason='same source unit resolved but no RAVEMEMS visual occurrence or source text was found'
        elif doc_candidates:
            cls='a_revoir_document_resolu_page_non_resolue'
            reason='source document resolved but page/section could not be established deterministically'
        else:
            cls='a_revoir_document_non_resolu'
            reason='source document could not be resolved from old key/path/illustration metadata'

        mapping.append({
            'old_key':key,'old_path':path,'old_exists':img.is_file(),'old_width':w,'old_height':h,
            'attached_legacy_illustration_indexes':attached,
            'attached_legacy_illustration_pk':[
                {c:illustrations[i].get(c) for c in ill_pk} for i in attached
            ],
            'source_roots':roots,'page_candidates':pages,'locator_candidates':locators,
            'document_candidates':[{
                'document_key':d['document_key'],'publication_code':d.get('publication_code'),
                'file_name':d.get('file_name'),'source_key':d.get('source_key')
            } for d in doc_candidates],
            'best_units':[{
                'unit_key':u['unit_key'],'physical_page':u.get('physical_page'),'printed_ref':u.get('printed_ref'),
                'section_ref':u.get('section_ref'),'score':sc,'reasons':reasons
            } for sc,reasons,d,u in best],
            'ravemems_visual_occurrences':occurrence_rows,
            'classification':cls,'reason':reason,
        })

    counts=Counter(m['classification'] for m in mapping)

    # Resolve each legacy illustration to the manifest mapping through exact metadata attachment.
    ill_to_manifest=defaultdict(list)
    for m in mapping:
        for idx in m['attached_legacy_illustration_indexes']:
            ill_to_manifest[idx].append(m)
    ill_map=[]
    for i,row in enumerate(illustrations):
        mm=ill_to_manifest.get(i,[])
        classes=Counter(x['classification'] for x in mm)
        if any(x['classification']=='remplacement_ravemems_prouve' for x in mm):
            cls='remplacement_ravemems_prouve'
        elif mm and all(x['classification'].startswith('ancienne_capture_page_texte') for x in mm):
            cls='ancienne_illustration_page_texte_sans_ui'
        elif mm:
            cls='a_revoir_mapping_partiel'
        else:
            cls='a_revoir_non_rattachee_au_manifest'
        ill_map.append({'index':i,'pk':{c:row.get(c) for c in ill_pk},'row':row,
                        'manifest_paths':[x['old_path'] for x in mm],
                        'manifest_classifications':dict(classes),'classification':cls})

    # Coverage of 329 old links: link to an illustration PK when exact value equality is present.
    pk_value_to_idx=defaultdict(set)
    for i,row in enumerate(illustrations):
        for c in ill_pk:
            v=row.get(c)
            if v is not None and str(v): pk_value_to_idx[str(v)].add(i)
    link_map=[]
    for i,row in enumerate(links):
        linked=set()
        for c,v in row.items():
            if v is None: continue
            cv=c.lower()
            if 'illustr' in cv or 'rave' in cv or 'visual' in cv or 'link' in cv:
                linked.update(pk_value_to_idx.get(str(v),set()))
        # fallback exact PK equality across any link column if schema naming is unexpected
        if not linked:
            for v in row.values():
                if v is not None: linked.update(pk_value_to_idx.get(str(v),set()))
        target_classes=Counter(ill_map[j]['classification'] for j in linked)
        if linked and all(c=='remplacement_ravemems_prouve' for c in target_classes):
            cls='lien_migrable_vers_ravemems'
        elif linked:
            cls='lien_a_conserver_jusqua_resolution_illustration'
        else:
            cls='lien_a_revoir_cible_non_resolue'
        link_map.append({'index':i,'row':row,'legacy_illustration_indexes':sorted(linked),
                         'target_classifications':dict(target_classes),'classification':cls})

    ill_counts=Counter(x['classification'] for x in ill_map)
    link_counts=Counter(x['classification'] for x in link_map)
    unresolved=[m for m in mapping if m['classification'].startswith('a_revoir')]
    summary={
        'pass':True,'read_only':True,'old_manifest_entries':len(mapping),'mapping_counts':dict(sorted(counts.items())),
        'mapped_without_review':len(mapping)-len(unresolved),'review_remaining':len(unresolved),
        'legacy_illustrations':len(ill_map),'illustration_counts':dict(sorted(ill_counts.items())),
        'legacy_links':len(link_map),'link_counts':dict(sorted(link_counts.items())),
        'ravemems_assets':conn.execute('SELECT COUNT(*) FROM mems_doc_visual').fetchone()[0],
        'ravemems_occurrences':conn.execute("SELECT COUNT(*) FROM mems_doc_entity WHERE entity_kind='visual_occurrence'").fetchone()[0],
        'safe_to_delete_now':False,
        'rule':'No legacy visual/catalog deletion until all 427 manifest entries and all 126 illustrations/329 links have a deterministic treatment.',
    }
    assert summary['old_manifest_entries']==427
    assert summary['legacy_illustrations']==126
    assert summary['legacy_links']==329
    assert summary['ravemems_assets']==1070
    assert summary['ravemems_occurrences']==1794

    for name,obj in [
        ('legacy_visual_mapping_427.json',mapping),('legacy_illustration_mapping_126.json',ill_map),
        ('legacy_link_mapping_329.json',link_map),('legacy_mapping_summary.json',summary),
        ('legacy_mapping_unresolved.json',unresolved)]:
        (out/name).write_text(json.dumps(obj,ensure_ascii=False,indent=2,sort_keys=True),encoding='utf-8')
    md=['# Exhaustive legacy RAVE visual mapping','',f"- manifest: 427",f"- illustrations: 126",f"- links: 329",
        f"- mapped without review: **{summary['mapped_without_review']}**",f"- review remaining: **{summary['review_remaining']}**",'',
        '## 427 manifest classifications']
    md += [f'- {k}: {v}' for k,v in sorted(counts.items())]
    md += ['', '## 126 illustration classifications']+[f'- {k}: {v}' for k,v in sorted(ill_counts.items())]
    md += ['', '## 329 link classifications']+[f'- {k}: {v}' for k,v in sorted(link_counts.items())]
    md += ['',f"**safe_to_delete_now: {summary['safe_to_delete_now']}**",summary['rule']]
    (out/'legacy_mapping_report.md').write_text('\n'.join(md)+'\n',encoding='utf-8')
    print(json.dumps(summary,ensure_ascii=False,sort_keys=True))

if __name__=='__main__': main()
