from __future__ import annotations
import base64, hashlib, json, lzma, re, sqlite3, subprocess, sys, tempfile, zlib
from pathlib import Path

EXPECTED_MEMSX64='22dbe75ed14e0a61e694159d505ef72245116b48'
EXPECTED_XZ_SHA='fbd982995304a41f978fc61280b83fbead120591e898bb66716ad70685da9a08'
EXPECTED_TIFF_SHA='d25fd347ccf9fd795c635fffff5a0dcc2800bbe33e185416c9a188d034b8f2c4'
EXPECTED_QZ_SHA='18e859c236a7e865b3416e0083f024e872bebda540c2dbf15ce233c0900c18be'
EXPECTED_SQL_SHA='1cf7cd58e71d436fc701401d841f0a4b521c9f386ff23a5720d8fb7a5b86fd54'
GENERAL_SCOPE='SCOPE-RCL0193FRE-MINI-VIN134455'
SELECTED=[324,326,328,329,330,331,332,333,334,335,336,337,338,339,340,341,342,343,344,345,346,347,348,349,350,351,352,353,354,355,356,357,358,360,361,362,363,364,365,366,367,368,369,370,371]

def sha(b): return hashlib.sha256(b).hexdigest()
def shaf(p): return sha(Path(p).read_bytes())
def numsuf(p):
    m=re.search(r'_(\d+)\.qz64$',p.name)
    return int(m.group(1)) if m else 0

def qun(data):
    expected=int.from_bytes(data[:4],'big')
    for w in (zlib.MAX_WBITS,-zlib.MAX_WBITS):
        try:
            out=zlib.decompress(data[4:],w)
            if len(out)==expected: return out
        except Exception: pass
    raise AssertionError('qz decode failed')

def dq(p):
    raw=Path(p).read_bytes()
    try: return qun(raw)
    except Exception: return qun(base64.b64decode(b''.join(raw.split())))

def execsql(db,b):
    t=b.decode('utf-8').replace('\ufeff','')
    t=re.sub(r'(?m)^\s*BEGIN(?:\s+TRANSACTION|\s+IMMEDIATE)?\s*;\s*$','',t,flags=re.I)
    t=re.sub(r'(?m)^\s*COMMIT\s*;\s*$','',t,flags=re.I)
    t=re.sub(r'(?m)^\s*PRAGMA\s+user_version\s*=\s*\d+\s*;\s*$','',t,flags=re.I)
    db.executescript(t)

def rebuild(ref,dst):
    if dst.exists(): dst.unlink()
    db=sqlite3.connect(dst)
    seeds=sorted(ref.glob('mems_reference_seed_*.qz64'),key=lambda p:(numsuf(p),p.name)); assert seeds
    execsql(db,qun(base64.b64decode(b''.join(p.read_bytes().strip() for p in seeds))))
    man=json.loads((ref/'manifest.json').read_text(encoding='utf-8-sig')); seen=set(); ordered=[]
    for name in man.get('research_enrichment_batches',[]):
        p=ref/name
        if p.is_file() and p.name!='research_enrichment_1600.qz64': ordered.append(p); seen.add(p.resolve())
    for p in sorted(ref.rglob('research_enrichment*.qz64'),key=lambda p:(numsuf(p),str(p))):
        if p.name=='research_enrichment_1600.qz64' or p.resolve() in seen: continue
        ordered.append(p); seen.add(p.resolve())
    for p in ordered: execsql(db,dq(p))
    db.execute('pragma user_version=20'); db.commit(); return db

def q1(db,s,a=()): return db.execute(s,a).fetchone()[0]

def verify_prod(repo):
    h=subprocess.check_output(['git','ls-remote','origin','refs/heads/MEMSX64'],cwd=repo,text=True).split()[0]
    assert h==EXPECTED_MEMSX64,(h,EXPECTED_MEMSX64)
    print('PRODUCTION_PASS MEMSX64 #101 unchanged',h)

def transport(repo):
    p=repo/'.rcl1860_transport'/'rcl0193fre_1860_g4_150.tif.xz.b64'
    xz=base64.b64decode(b''.join(p.read_bytes().split()))
    assert sha(xz)==EXPECTED_XZ_SHA,(sha(xz),EXPECTED_XZ_SHA)
    tif=lzma.decompress(xz); assert sha(tif)==EXPECTED_TIFF_SHA
    t=Path('/tmp/rcl1860.tif'); t.write_bytes(tif)
    print('TRANSPORT_PASS exact xz/tiff hashes'); return t

def install_qz(repo,ref):
    p=repo/'.rcl1860_transport'/'research_enrichment_1860.qz64'
    assert shaf(p)==EXPECTED_QZ_SHA,(shaf(p),EXPECTED_QZ_SHA)
    assert sha(dq(p))==EXPECTED_SQL_SHA
    d=ref/'research_enrichment_1860.qz64'; assert not d.exists(); d.write_bytes(p.read_bytes())
    print('QZ_PASS exact qz/sql hashes')

def render(tiff,ref):
    from PIL import Image
    im=Image.open(tiff); assert im.n_frames==45 and im.size==(1240,1755) and im.mode=='1',(im.n_frames,im.size,im.mode)
    out=ref/'images'/'rave'; out.mkdir(parents=True,exist_ok=True); hs={}
    for i,n in enumerate(SELECTED):
        im.seek(i); dst=out/f'RCL0193FRE_PDF_{n:03d}.png'; assert not dst.exists(); im.copy().save(dst,'PNG',optimize=True); hs[n]=shaf(dst)
    im.close(); print('RENDER_PASS 45 factory pages @150dpi'); return hs

def patch_manifest(ref,hs):
    p=ref/'manifest.json'; m=json.loads(p.read_text(encoding='utf-8-sig'))
    batches=m.setdefault('research_enrichment_batches',[]); assert 'research_enrichment_1860.qz64' not in batches; batches.append('research_enrichment_1860.qz64')
    d=m.setdefault('diagrams',{}); v=m.setdefault('visual_assets',{})
    for n in SELECTED:
        path=f'images/rave/RCL0193FRE_PDF_{n:03d}.png'
        d[f'RAVE RCL0193FRE PDF {n:03d} lot 1860']=path
        k=f'rave:RCL0193FRE:PDF:{n}'; assert k not in v
        v[k]={'path':path,'asset_kind':'manual_page','document':'RCL0193FRE','source_page':f'PDF p.{n}','pdf_index':n,'sha256':hs[n]}
    p.write_text(json.dumps(m,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('MANIFEST_PASS 1860 +45 assets')

def audit(ref,hs):
    lines=['# RCL0193FRE - audit lot 1860','',
      '- Source PDF SHA-256: `0c7fef28d0d0f0673ba321d6625a019c005823103caa98afb3258114e1fec713`.',
      '- Périmètre: équipement électrique + instruments PDF 324-371.',
      '- 45 pages utiles conservées; pages 325, 327 et 359 exclues comme blanches/intercalaires.',
      '- Captures constructeur monochromes 150 dpi; aucune image générée ni redessinée.','',
      '## Comptages',
      '- 112 connaissances; 18 spécifications/valeurs; 50 opérations constructeur; 100 phases structurées; 516 étapes; 29 exigences; 79 relations; 64 alias effectifs.','',
      '## Points importants',
      '- Courroie auxiliaire: charge 10 kg, flèche 6-8 mm, poulie de tension 25 N·m.',
      '- Réglage phares: commutateur position 0 et faisceau 1,4 % sous l’horizontale.',
      '- Alternateur: poulie 25 N·m; outil Rover 18G 1653.',
      '- Démarreur: fixation 37 N·m; borne solénoïde 4 N·m.',
      '- Accouplement tournant SRS: batterie déconnectée masse en premier, attente 10 min, ensemble non démontable.',
      '- Sonde de température d’huile: 60 N·m.',
      '- Révisions constructeur 11/98 et 06/99 conservées avec leur provenance exacte.','',
      '## Captures']
    for n in SELECTED: lines.append(f'- PDF p.{n}: `images/rave/RCL0193FRE_PDF_{n:03d}.png` - `{hs[n]}`')
    p=ref/'audits'/'rcl0193fre_1860_audit.md'; p.parent.mkdir(parents=True,exist_ok=True); p.write_text('\n'.join(lines)+'\n',encoding='utf-8'); print('AUDIT_PASS')

def validate(ref,tmp):
    db=rebuild(ref,tmp/'db.sqlite')
    try:
        assert q1(db,'pragma integrity_check')=='ok' and q1(db,'pragma user_version')==20
        assert q1(db,'select count(*) from mems_rave_fact')==93
        assert q1(db,'select count(*) from mems_expert_fact_external')==105
        assert q1(db,"select count(*) from mems_knowledge_item where knowledge_key like 'KNOW-RCL0193FRE-1860-%'")==112
        assert q1(db,"select count(*) from mems_knowledge_scope where knowledge_key like 'KNOW-RCL0193FRE-1860-%'")==112
        assert q1(db,"select count(*) from mems_knowledge_scope where knowledge_key like 'KNOW-RCL0193FRE-1860-%' and scope_key!=?",(GENERAL_SCOPE,))==0
        assert q1(db,"select count(*) from mems_specification where spec_key like 'SPEC-RCL0193FRE-1860-%'")==18
        assert q1(db,"select count(*) from mems_specification_value where spec_key like 'SPEC-RCL0193FRE-1860-%'")==18
        assert q1(db,"select count(*) from mems_procedure where procedure_key like 'PROC-RCL0193FRE-1860-%'")==100
        assert q1(db,"select count(*) from mems_procedure_step where procedure_key like 'PROC-RCL0193FRE-1860-%'")==516
        assert q1(db,"select count(*) from mems_procedure_requirement where procedure_key like 'PROC-RCL0193FRE-1860-%'")==29
        assert q1(db,"select count(*) from mems_knowledge_relation where from_key like 'KNOW-RCL0193FRE-1860-%'")==79
        assert q1(db,"select count(*) from mems_knowledge_relation r left join mems_knowledge_item k on k.knowledge_key=r.to_key where r.from_key like 'KNOW-RCL0193FRE-1860-%' and k.knowledge_key is null")==0
        assert q1(db,"select count(*) from (select procedure_key,count(*) c,min(step_no) mi,max(step_no) ma from mems_procedure_step where procedure_key like 'PROC-RCL0193FRE-1860-%' group by procedure_key having mi!=1 or ma!=c)")==0
        print('DB_PASS integrity=ok uv=20 hist=93/105 k=112 spec=18 proc=100 steps=516 req=29 rel=79')
    finally: db.close()

def validate_assets(ref,hs):
    assert len(hs)==45
    for n,h in hs.items(): assert shaf(ref/'images'/'rave'/f'RCL0193FRE_PDF_{n:03d}.png')==h
    assert shaf(ref/'research_enrichment_1860.qz64')==EXPECTED_QZ_SHA
    print('ASSET_PASS 45/45 + qz exact')

def allowed_delta(repo):
    out=subprocess.check_output(['git','status','--porcelain','--untracked-files=all'],cwd=repo,text=True).splitlines(); bad=[]
    pats=[re.compile(r'database/reference/research_enrichment_1860\.qz64$'),re.compile(r'database/reference/manifest\.json$'),re.compile(r'database/reference/audits/rcl0193fre_1860_audit\.md$'),re.compile(r'database/reference/images/rave/RCL0193FRE_PDF_\d{3}\.png$')]
    for line in out:
        path=line[3:]
        if path.startswith('.rcl1860_transport/') or path in {'.github/workflows/temp-rcl0193fre-1860-install.yml','tools/install_rcl0193fre_1860.py'}: continue
        if not any(p.fullmatch(path) for p in pats): bad.append(line)
    assert not bad,bad
    assert len([l for l in out if 'RCL0193FRE_PDF_' in l])==45
    print('SCOPE_PASS final documentary delta exactly 48 paths')

def install(repo):
    repo=Path(repo); ref=repo/'database'/'reference'; verify_prod(repo); t=transport(repo); install_qz(repo,ref); hs=render(t,ref); patch_manifest(ref,hs); audit(ref,hs)
    with tempfile.TemporaryDirectory() as td: validate(ref,Path(td))
    validate_assets(ref,hs); allowed_delta(repo)

def post(repo):
    repo=Path(repo); ref=repo/'database'/'reference'; verify_prod(repo)
    with tempfile.TemporaryDirectory() as td: validate(ref,Path(td))
    m=json.loads((ref/'manifest.json').read_text(encoding='utf-8-sig'))
    assert 'research_enrichment_1860.qz64' in m.get('research_enrichment_batches',[])
    assert sum(1 for k in m.get('visual_assets',{}) if k.startswith('rave:RCL0193FRE:PDF:') and int(k.rsplit(':',1)[1]) in SELECTED)==45
    assert all((ref/'images'/'rave'/f'RCL0193FRE_PDF_{n:03d}.png').exists() for n in SELECTED)
    assert not (repo/'.rcl1860_transport').exists(); assert not (repo/'.github/workflows/temp-rcl0193fre-1860-install.yml').exists(); assert not (repo/'tools/install_rcl0193fre_1860.py').exists()
    assert not subprocess.check_output(['git','status','--porcelain'],cwd=repo,text=True).strip()
    print('POST_PASS remote 1860 committed, transport/workflow clean')

if __name__=='__main__': {'install':install,'post':post}[sys.argv[1]](sys.argv[2])
