#!/usr/bin/env python3
"""Build/validate additive RCL0193 visual backfill candidate batch 1760.

Documentary-only tool. It reconstructs the reference DB through batch 1750,
renders the exact 26 RCL0193 factory pages cited by the 31 historical facts,
creates additive illustration/link rows, and validates all invariants.
"""
from __future__ import annotations

import argparse, base64, hashlib, json, re, shutil, sqlite3, struct, zlib
from pathlib import Path

EXPECTED_RAVE_FACTS=93
EXPECTED_EXPERT_FACTS=105
EXPECTED_RCL0193_FACTS=31
EXPECTED_RCL0193_ASSETS=26
EXPECTED_RCL0193_LINKS=43
EXPECTED_USER_VERSION=20
EXPECTED_PDF_SHA='c050a3eebe50c5a85bf8a69b7722bd2052079944e09d58578a498984ecf06715'
EXPECTED_PAGES=[38,39,40,98,101,107,108,109,112,113,114,117,118,120,121,122,123,125,126,127,128,129,130,131,135,170]

PAGE_INFO={
38:('table_page','Engine Tuning Data SPi manual fuel pressure lost motion throttle',('engine tuning data','fuel pressure')),
39:('table_page','Engine Tuning Data SPi automatic fuel pressure throttle',('engine tuning data','spi')),
40:('table_page','Engine Tuning Data MPi fuel pressure throttle TP',('engine tuning data','mpi')),
98:('technical_description_page','Emission Control EVAP canister purge system',('emission control','evaporative')),
101:('procedure_page','Emission canister purge valve repair',('emission control','canister')),
107:('technical_description_page','MEMS engine management strategy ECM backup',('engine management system','ecm')),
108:('technical_description_page','CKP crankshaft position basic ignition timing',('crankshaft position sensor','ignition')),
109:('technical_description_page','MAP manifold absolute pressure and ECT sensors',('manifold absolute pressure sensor','ect')),
112:('technical_description_page','IAT intake air temperature and MPi injectors',('intake air temperature sensor','injectors')),
113:('technical_description_page','Throttle housing and throttle position TPS',('throttle housing','throttle')),
114:('technical_description_page','IACV idle air control valve throttle TP',('idle air control valve','throttle')),
117:('technical_description_page','Heated oxygen lambda sensor overrun overspeed fuel cut-off',('heated oxygen sensor','fuel cut-off')),
118:('procedure_page','Throttle cable adjustment and throttle stop caution',('throttle cable','adjustments')),
120:('procedure_page','Throttle cable removal and refit',('throttle cable','repairs')),
121:('procedure_page','Throttle pedal removal refit torque',('throttle pedal','repairs')),
122:('procedure_page','Throttle housing removal refit',('throttle housing','repairs')),
123:('procedure_page','Throttle housing TP sensor removal refit',('throttle housing','tp sensor')),
125:('procedure_page','Ignition coil spark plug service',('engine management system','spark plugs')),
126:('procedure_page','Ignition coil refit and ECM service',('ignition coil','refit')),
127:('procedure_page','ECM refit and IACV service',('ecm','refit')),
128:('procedure_page','IAT and ECT sensor service',('intake air temperature','sensor')),
129:('procedure_page','ECT and CKP sensor service',('ect sensor','repairs')),
130:('procedure_page','CKP crankshaft position sensor refit',('ckp sensor','refit')),
131:('procedure_page','MAP and TP sensor repair',('map sensor','repairs')),
135:('procedure_page','Fuel injectors and fuel rail service',('injectors','repairs')),
170:('procedure_page','Inlet manifold injector related service',('inlet manifold','repairs')),
}

def sha256_bytes(b:bytes)->str:return hashlib.sha256(b).hexdigest()
def sha256_file(p:Path)->str:return sha256_bytes(p.read_bytes())
def qt_uncompress(payload:bytes)->bytes:
    if len(payload)<5:raise AssertionError('qCompress payload too short')
    expected=struct.unpack('>I',payload[:4])[0];raw=zlib.decompress(payload[4:])
    if len(raw)!=expected:raise AssertionError((expected,len(raw)))
    return raw
def decode_qz64(p:Path)->bytes:
    data=p.read_bytes().strip()
    if data and re.fullmatch(rb'[A-Za-z0-9+/=\r\n\t ]+',data):return qt_uncompress(base64.b64decode(data))
    return qt_uncompress(data)
def encode_qz64(raw:bytes)->bytes:
    return base64.b64encode(struct.pack('>I',len(raw))+zlib.compress(raw,9))+b'\n'
def clean_sql(raw:bytes)->str:
    return '\n'.join(x for x in raw.decode('utf-8').splitlines() if not x.strip().startswith('--'))+'\n'
def execute_project_sql(db:sqlite3.Connection,raw:bytes)->None:db.executescript(clean_sql(raw))
def numeric_suffix(p:Path)->int:
    m=re.search(r'_(\d+)\.qz64$',p.name,re.I);return int(m.group(1)) if m else 0

def rebuild_reference_database(reference:Path,destination:Path)->None:
    if destination.exists():destination.unlink()
    db=sqlite3.connect(str(destination))
    try:
        seeds=sorted(reference.glob('mems_reference_seed_*.qz64'),key=lambda p:(numeric_suffix(p),p.name))
        assert seeds
        encoded=b''.join(p.read_bytes().strip() for p in seeds)
        execute_project_sql(db,qt_uncompress(base64.b64decode(encoded)))
        manifest=json.loads((reference/'manifest.json').read_text(encoding='utf-8-sig'))
        ordered=[];seen=set()
        for name in manifest.get('research_enrichment_batches',[]):
            p=reference/name
            if p.is_file() and p.name!='research_enrichment_1600.qz64' and numeric_suffix(p)<1760:
                ordered.append(p);seen.add(p.resolve())
        for p in sorted(reference.rglob('research_enrichment*.qz64'),key=lambda p:(numeric_suffix(p),str(p))):
            if p.name=='research_enrichment_1600.qz64' or numeric_suffix(p)>=1760:continue
            if p.resolve() not in seen:ordered.append(p);seen.add(p.resolve())
        for p in ordered:execute_project_sql(db,decode_qz64(p))
        db.execute(f'PRAGMA user_version={EXPECTED_USER_VERSION}');db.commit()
    finally:db.close()

def sql_quote(v:str|None)->str:
    return 'NULL' if v is None else "'"+v.replace("'","''")+"'"

def parse_pages(section:str)->list[int]:
    pages=[]
    for m in re.finditer(r'p\.(\d+)(?:-(\d+))?',section,re.I):
        a=int(m.group(1));b=int(m.group(2) or a)
        if b<a:raise AssertionError(f'reverse range {section}')
        pages.extend(range(a,b+1))
    return sorted(set(pages))

def facts_from_db(db_path:Path):
    db=sqlite3.connect(str(db_path));db.row_factory=sqlite3.Row
    try:
        rows=db.execute("SELECT fact_key,variant,topic,statement,source_section,verification_level,image_ref FROM mems_rave_fact WHERE document LIKE '%RCL0193ENG%' ORDER BY fact_key").fetchall()
        assert len(rows)==EXPECTED_RCL0193_FACTS,len(rows)
        out=[]
        for r in rows:
            d=dict(r);d['pages']=parse_pages(str(d['source_section'] or ''))
            assert d['pages'],d['fact_key'];out.append(d)
        by={r['fact_key']:r for r in out}
        assert by['RAVE-CAUTION-THROTTLE-STOP-001']['pages']==[114,118]
        assert by['RAVE-REP-INJECTORS-001']['pages']==[135,170]
        assert sorted({p for r in out for p in r['pages']})==EXPECTED_PAGES
        assert all(r['verification_level']=='verifie_constructeur' and not r['image_ref'] for r in out)
        return out
    finally:db.close()

def render_assets(pdf_path:Path,reference:Path)->list[dict]:
    import fitz
    assert sha256_file(pdf_path)==EXPECTED_PDF_SHA
    out=reference/'images'/'rave';out.mkdir(parents=True,exist_ok=True)
    doc=fitz.open(str(pdf_path));assert doc.page_count==372
    assets=[]
    try:
        for p in EXPECTED_PAGES:
            kind,title,tokens=PAGE_INFO[p];page=doc[p]
            text=' '.join(page.get_text('text').lower().split())
            for token in tokens:
                if token not in text:raise AssertionError(f'page {p} token missing: {token}')
            filename=f'RCL0193ENG_PDF_{p:03d}.png';target=out/filename
            pix=page.get_pixmap(matrix=fitz.Matrix(2.0,2.0),alpha=False);pix.save(target)
            digest=sha256_file(target)
            assets.append({'page':p,'source_page':f'PDF p.{p}','kind':kind,'title':title,'filename':filename,'sha256':digest,'width':pix.width,'height':pix.height})
            print(f'PNG_PASS p={p} sha256={digest} {pix.width}x{pix.height}')
    finally:doc.close()
    return assets

def build_batch_sql(baseline:Path,assets:list[dict])->bytes:
    facts=facts_from_db(baseline);asset_by_page={a['page']:a for a in assets}
    lines=['-- RCL0193 visual backfill batch 1760',
      "CREATE TABLE IF NOT EXISTS mems_rave_illustration(illustration_key TEXT PRIMARY KEY,document TEXT NOT NULL,source_page TEXT NOT NULL,pdf_index INTEGER,asset_kind TEXT NOT NULL,relative_path TEXT NOT NULL UNIQUE,sha256 TEXT NOT NULL,notes TEXT);",
      "CREATE TABLE IF NOT EXISTS mems_rave_illustration_link(fact_key TEXT NOT NULL,illustration_key TEXT NOT NULL,link_role TEXT NOT NULL DEFAULT 'source',notes TEXT,PRIMARY KEY(fact_key,illustration_key,link_role));",
      "CREATE INDEX IF NOT EXISTS idx_rave_illustration_document_page ON mems_rave_illustration(document,source_page);",
      "CREATE INDEX IF NOT EXISTS idx_rave_illustration_link_fact ON mems_rave_illustration_link(fact_key);",
      "CREATE INDEX IF NOT EXISTS idx_rave_illustration_link_asset ON mems_rave_illustration_link(illustration_key);"]
    for a in assets:
        key=f"RAVE:RCL0193ENG:PDF:{a['page']}"
        lines.append("INSERT OR REPLACE INTO mems_rave_illustration(illustration_key,document,source_page,pdf_index,asset_kind,relative_path,sha256,notes) VALUES("+
          ','.join([sql_quote(key),sql_quote('RCL0193ENG'),sql_quote(a['source_page']),str(a['page']),sql_quote(a['kind']),sql_quote('images/rave/'+a['filename']),sql_quote(a['sha256']),sql_quote('Original Rover RCL0193ENG factory page retained additively; documentary only.')])+');')
    link_count=0
    for r in facts:
        for p in r['pages']:
            assert p in asset_by_page
            lines.append("INSERT OR IGNORE INTO mems_rave_illustration_link(fact_key,illustration_key,link_role,notes) VALUES("+
              ','.join([sql_quote(r['fact_key']),sql_quote(f'RAVE:RCL0193ENG:PDF:{p}'),sql_quote('source'),sql_quote('Exact factory source page derived from the historical source_section; historical fact preserved unchanged.')])+');')
            link_count+=1
    assert link_count==EXPECTED_RCL0193_LINKS,link_count
    return ('\n'.join(lines)+'\n').encode('utf-8')

def validate_after_apply(db_path:Path)->dict:
    db=sqlite3.connect(str(db_path))
    try:
        integrity=db.execute('PRAGMA integrity_check').fetchone()[0];uv=db.execute('PRAGMA user_version').fetchone()[0]
        rave=db.execute('SELECT COUNT(*) FROM mems_rave_fact').fetchone()[0]
        expert=db.execute('SELECT COUNT(*) FROM mems_expert_fact_external').fetchone()[0]
        assets=db.execute("SELECT COUNT(*) FROM mems_rave_illustration WHERE document='RCL0193ENG'").fetchone()[0]
        facts=db.execute("SELECT COUNT(DISTINCT r.fact_key) FROM mems_rave_fact r JOIN mems_rave_illustration_link l ON l.fact_key=r.fact_key JOIN mems_rave_illustration i ON i.illustration_key=l.illustration_key WHERE r.document LIKE '%RCL0193ENG%' AND i.document='RCL0193ENG' AND l.link_role='source'").fetchone()[0]
        links=db.execute("SELECT COUNT(*) FROM mems_rave_illustration_link l JOIN mems_rave_illustration i ON i.illustration_key=l.illustration_key WHERE i.document='RCL0193ENG' AND l.link_role='source'").fetchone()[0]
        hist_nonempty=db.execute("SELECT COUNT(*) FROM mems_rave_fact WHERE document LIKE '%RCL0193ENG%' AND COALESCE(image_ref,'')<>''").fetchone()[0]
        rcl0194=db.execute("SELECT COUNT(*) FROM mems_rave_illustration WHERE document='RCL0194ENG'").fetchone()[0]
        assert str(integrity).lower()=='ok';assert uv==20;assert rave==93;assert expert==105
        assert assets==26;assert facts==31;assert links==43;assert hist_nonempty==0;assert rcl0194==8
        return {'integrity':str(integrity),'user_version':uv,'rave_facts':rave,'expert_facts':expert,'rcl0193_assets':assets,'rcl0193_fact_coverage':facts,'rcl0193_links':links,'rcl0194_assets_preserved':rcl0194}
    finally:db.close()

def update_manifest(reference:Path,assets:list[dict])->None:
    p=reference/'manifest.json';data=json.loads(p.read_text(encoding='utf-8-sig'));assert data.get('database_revision')==20
    batches=data.setdefault('research_enrichment_batches',[])
    if 'research_enrichment_1760.qz64' not in batches:batches.append('research_enrichment_1760.qz64')
    diagrams=data.setdefault('diagrams',{});visual=data.setdefault('visual_assets',{})
    for a in assets:
        name=f"RAVE RCL0193ENG PDF {a['page']} {a['title']}"
        path=f"images/rave/{a['filename']}";diagrams[name]=path
        visual[f"rave:RCL0193ENG:PDF:{a['page']}"]={'path':path,'asset_kind':a['kind'],'document':'RCL0193ENG','source_page':a['source_page'],'pdf_index':a['page'],'sha256':a['sha256']}
    p.write_text(json.dumps(data,indent=2,ensure_ascii=False)+'\n',encoding='utf-8',newline='\n')

def write_audit(reference:Path,qz:Path,sql_raw:bytes,assets:list[dict],validation:dict)->None:
    a=reference/'audits'/'rcl0193_visual_backfill_1760_audit.md';a.parent.mkdir(parents=True,exist_ok=True)
    rows='\n'.join(f"- PDF p.{x['page']}: `{x['kind']}`, `{x['filename']}`, SHA-256 `{x['sha256']}`" for x in assets)
    text=f'''# RCL0193 visual backfill — batch 1760 candidate audit

Date: 2026-08-29

Production remains `MEMSX64` BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`. Documentary-only candidate: no ECU communication, protocol, acquisition, RAM, write/reset, Qwen/ONNX, UI or 32-bit code is modified. Base/IA/RAVE remains consultative only.

Primary source: Rover `RCL0193ENG`, 372-page factory PDF, SHA-256 `{EXPECTED_PDF_SHA}`.

Historical scope: 31 RCL0193 RAVE facts from batch 1660. Historical facts remain 31/31 unchanged and keep empty historical `image_ref`. V3 mapping retains every page reference, including non-contiguous 114+118 and 135+170.

This candidate retains 26 unique factory pages once and creates 43 fact→page source links. A fact may therefore expose more than one original page without duplicating image files.

Validation on reconstructed BUILD #101 reference DB with batch 1750 already applied:
- integrity: `{validation['integrity']}`
- user_version: `{validation['user_version']}`
- historical RAVE facts: `{validation['rave_facts']}`
- historical expert facts: `{validation['expert_facts']}`
- RCL0193 assets: `{validation['rcl0193_assets']}`
- RCL0193 fact coverage: `{validation['rcl0193_fact_coverage']}/31`
- RCL0193 source links: `{validation['rcl0193_links']}`
- RCL0194 assets preserved: `{validation['rcl0194_assets_preserved']}`

Generated `research_enrichment_1760.qz64`: {qz.stat().st_size} bytes, SHA-256 `{sha256_file(qz)}`; decompressed SQL {len(sql_raw)} bytes, SHA-256 `{sha256_bytes(sql_raw)}`.

## Retained factory pages
{rows}
'''
    a.write_text(text,encoding='utf-8',newline='\n')

def main()->int:
    ap=argparse.ArgumentParser();ap.add_argument('--root',type=Path,default=Path('.'));ap.add_argument('--pdf',type=Path,required=True);ap.add_argument('--work',type=Path,default=Path('.tmp-rcl0193-1760'));args=ap.parse_args()
    root=args.root.resolve();reference=root/'database'/'reference';work=(root/args.work).resolve() if not args.work.is_absolute() else args.work;work.mkdir(parents=True,exist_ok=True)
    baseline=work/'baseline.sqlite';candidate=work/'candidate.sqlite'
    rebuild_reference_database(reference,baseline)
    # Verify that 1750 is present before building 1760.
    db=sqlite3.connect(str(baseline))
    try:
        assert db.execute("SELECT COUNT(*) FROM mems_rave_illustration WHERE document='RCL0194ENG'").fetchone()[0]==8
    finally:db.close()
    facts_from_db(baseline)
    assets=render_assets(args.pdf.resolve(),reference)
    sql_raw=build_batch_sql(baseline,assets);qz=reference/'research_enrichment_1760.qz64';qz.write_bytes(encode_qz64(sql_raw));assert decode_qz64(qz)==sql_raw
    shutil.copy2(baseline,candidate);db=sqlite3.connect(str(candidate))
    try:execute_project_sql(db,decode_qz64(qz));db.execute('PRAGMA user_version=20');db.commit()
    finally:db.close()
    validation=validate_after_apply(candidate);update_manifest(reference,assets);write_audit(reference,qz,sql_raw,assets,validation)
    manifest=json.loads((reference/'manifest.json').read_text(encoding='utf-8'));assert 'research_enrichment_1760.qz64' in manifest['research_enrichment_batches']
    vals={k:v for k,v in manifest.get('visual_assets',{}).items() if k.startswith('rave:RCL0193ENG:PDF:')};assert len(vals)==26
    for v in vals.values():
        f=reference/v['path'];assert f.is_file() and sha256_file(f)==v['sha256']
    print('RCL0193_1760_CANDIDATE_PASS')
    for k,v in validation.items():print(f'{k}={v}')
    print(f'qz64_sha256={sha256_file(qz)}');print(f'sql_sha256={sha256_bytes(sql_raw)}');print(f'manifest_sha256={sha256_file(reference/"manifest.json")}');print(f'audit_sha256={sha256_file(reference/"audits"/"rcl0193_visual_backfill_1760_audit.md")}')
    return 0
if __name__=='__main__':raise SystemExit(main())
