#!/usr/bin/env python3
import argparse, json, sqlite3
from pathlib import Path


def qi(s): return '"' + str(s).replace('"','""') + '"'

def trim(v, n=3000):
    if isinstance(v, str) and len(v) > n: return v[:n] + '...'
    return v

def fetch_ref_row(db, table, ref):
    if not ref or 'row_index' in ref:
        return {'unresolved_ref': ref}
    where=[]; vals=[]
    for k,v in ref.items():
        where.append(f'{qi(k)} IS ?'); vals.append(v)
    try:
        cur=db.execute(f'SELECT * FROM {qi(table)} WHERE ' + ' AND '.join(where), vals)
        cols=[x[0] for x in cur.description]
        rows=[]
        for r in cur.fetchall(): rows.append({c:trim(v) for c,v in zip(cols,r)})
        return rows
    except Exception as e:
        return {'error': str(e), 'table': table, 'ref': ref}

def entity_context(db, entity):
    row=db.execute('''SELECT e.entity_key,e.entity_kind,e.legacy_table,e.legacy_key,e.source_locator,
                             d.document_key,d.file_name,d.publication_code,u.physical_page,
                             t.text_value
                      FROM mems_doc_entity e
                      JOIN mems_doc_document d ON d.document_key=e.document_key
                      LEFT JOIN mems_doc_unit u ON u.unit_key=e.unit_key
                      LEFT JOIN mems_doc_text t ON t.entity_key=e.entity_key AND t.locale='en' AND t.field_key='source_exact'
                      WHERE e.entity_key=?''',(entity,)).fetchone()
    if not row: return None
    keys=['entity_key','entity_kind','source_table','source_key','source_locator','document_key','file_name','publication_code','physical_page','source_exact']
    return {k:trim(v) for k,v in zip(keys,row)}

def page_context(db, document_key, page):
    if page is None: return []
    rows=db.execute('''SELECT e.entity_key,e.entity_kind,t.text_value
                       FROM mems_doc_entity e
                       JOIN mems_doc_unit u ON u.unit_key=e.unit_key
                       LEFT JOIN mems_doc_text t ON t.entity_key=e.entity_key AND t.locale='en' AND t.field_key='source_exact'
                       WHERE e.document_key=? AND u.physical_page=? AND e.entity_kind IN ('page','content_block','source_line')
                       ORDER BY e.sort_order LIMIT 30''',(document_key,page)).fetchall()
    return [{'entity':r[0],'kind':r[1],'text':trim(r[2],1500)} for r in rows]

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('--duplicates',type=Path,required=True); ap.add_argument('--db',type=Path,required=True); ap.add_argument('--out',type=Path,required=True); a=ap.parse_args()
    dup=json.loads(a.duplicates.read_text(encoding='utf-8')); db=sqlite3.connect(a.db)
    out={'exact_text_candidates':[],'source_overlap_candidates':[]}
    for i,c in enumerate(dup.get('exact_normalized_text_candidates',[]),1):
        item={'candidate_no':i,'classification_input':c,'legacy_rows':fetch_ref_row(db,c.get('legacy_table'),c.get('legacy_ref')),'ravemems_contexts':[]}
        for m in c.get('ravemems_matches',[]):
            ctx=entity_context(db,m.get('entity_key'))
            if ctx:
                ctx['page_context']=page_context(db,ctx['document_key'],ctx['physical_page'])
            item['ravemems_contexts'].append(ctx)
        out['exact_text_candidates'].append(item)
    for i,c in enumerate(dup.get('source_overlap_candidates',[]),1):
        item={'candidate_no':i,'publication_code':c.get('publication_code'),'ravemems_documents':c.get('ravemems_documents',[]),'legacy_contexts':[],'ravemems_document_samples':[]}
        for r in c.get('legacy_references',[]):
            item['legacy_contexts'].append({'reference':r,'rows':fetch_ref_row(db,r.get('legacy_table'),r.get('legacy_ref'))})
        for d in c.get('ravemems_documents',[]):
            dk=d.get('document_key'); samples=db.execute('''SELECT u.physical_page,t.text_value FROM mems_doc_unit u JOIN mems_doc_entity e ON e.unit_key=u.unit_key LEFT JOIN mems_doc_text t ON t.entity_key=e.entity_key AND t.field_key='source_exact' AND t.locale='en' WHERE u.document_key=? AND e.entity_kind='page' AND t.text_value IS NOT NULL ORDER BY u.physical_page LIMIT 5''',(dk,)).fetchall()
            item['ravemems_document_samples'].append({'document':d,'samples':[{'physical_page':p,'text':trim(t,2000)} for p,t in samples]})
        out['source_overlap_candidates'].append(item)
    a.out.write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('DUPLICATE_CANDIDATE_INSPECTION_BEGIN')
    print(json.dumps(out,ensure_ascii=False,indent=2))
    print('DUPLICATE_CANDIDATE_INSPECTION_END')
    db.close()
if __name__=='__main__': main()
