from __future__ import annotations
import base64, hashlib, json, lzma, re, shutil, sqlite3, subprocess, sys, zlib
from pathlib import Path

EXPECTED_MEMSX64='22dbe75ed14e0a61e694159d505ef72245116b48'
EXPECTED_SOURCE_SHA='0c7fef28d0d0f0673ba321d6625a019c005823103caa98afb3258114e1fec713'
EXPECTED_SOURCE_BYTES=67009217
EXPECTED_SOURCE_PAGES=371
EXPECTED_G4_XZ_SHA='90d618af4491938699940b8d019e95c468a626fbf02ccd421567da9b0424f929'
EXPECTED_G4_TIFF_SHA='e47775f522eb19c9034a883cae9e9c059d1ccb2fa47a5833ca526f0097968192'
EXPECTED_SUBSET_PAGES=52
EXPECTED_QZ_SHA='165d3bedb57b38b7fa1550a4360c9f484828209beea51b63b4aab20c5f9885da'
EXPECTED_SQL_SHA='28f0ffc78b957679672608731f0cb0faa3487d1646977304423e23d701465725'
EXPECTED_HIST_RAVE=93
EXPECTED_HIST_EXPERT=105
EXPECTED_USER_VERSION=20
EXPECTED_NEW_KNOWLEDGE=429
EXPECTED_NEW_SCOPES=4
EXPECTED_NEW_SPEC=350
EXPECTED_NEW_VALUES=351
EXPECTED_NEW_PROCS=51
EXPECTED_NEW_STEPS=344
EXPECTED_NEW_REQS=50
EXPECTED_NEW_RELATIONS=379
EXPECTED_NEW_ALIASES=51
SOURCE_KEY='SRC-RCL0193FRE-USER-PDF'
SCOPE_GENERAL='SCOPE-RCL0193FRE-MINI-VIN134455'
SCOPE_SPI_MAN='SCOPE-RCL0193FRE-SPI-MANUAL'
SCOPE_SPI_AUTO='SCOPE-RCL0193FRE-SPI-AUTO'
SCOPE_MPI='SCOPE-RCL0193FRE-MPI-VIN134455'
SELECTED=[34,35,36,37,38,39,40,42,43,44,45,46,48,49,104,106,107,108,109,110,111,112,113,114,115,116,117,118,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,138,140,141,142,143,144,145]
assert len(SELECTED)==52

PAGE_TOPICS={
34:'Caractéristiques générales moteur alimentation refroidissement',35:'Caractéristiques embrayage boîtes manuelle automatique réduction finale',36:'Roues pneus pressions équipement électrique batterie alternateur démarreur',37:'Dimensions poids Mini',38:'Réglages moteur SPi boîte manuelle',39:'Réglages moteur SPi boîte automatique',40:'Réglages moteur MPi',42:'Couples de serrage généraux moteur',43:'Couples de serrage dépollution gestion moteur MEMS',44:'Couples collecteur échappement embrayage boîte manuelle',45:'Couples suspension freins SRS',46:'Couples carrosserie chauffage essuie-glaces équipement électrique instruments',48:'Contenances fluides carburant huile refroidissement lave-glace liquide frein antigel',49:'Graissage lubrifiants moteur boîte direction général',104:'Table des matières système de gestion moteur MEMS',106:'Emplacements composants système gestion moteur MEMS',107:'Stratégie système gestion moteur MEMS ECM allumage',108:'CKP capteur vilebrequin calage allumage',109:'MAP pression absolue collecteur ECT température liquide',110:'Composants système gestion moteur bobine allumage',111:'Système alimentation MEMS entrées sorties ECM',112:'IAT température admission injecteurs carburant',113:'Corps papillon capteur position papillon TP TPS',114:'IACV ralenti module relais purge canister',115:'Pompe carburant régulateur pression rampe',116:'Interrupteur inertiel IFS connecteur diagnostic TestBook',117:'HO2S lambda boucle fermée enrichissement décélération limite régime post-refroidissement',118:'Réglage câble accélérateur',120:'Dépose repose câble accélérateur',121:'Dépose repose pédale accélérateur',122:'Corps papillon dépose',123:'Corps papillon repose capteur TP précautions',124:'Filtre à air élément filtre',125:'Bougies bobine allumage',126:'Bobine allumage ECM module commande moteur',127:'ECM IACV code antivol TestBook',128:'IAT ECT sondes température',129:'ECT CKP capteur vilebrequin',130:'CKP capteur TP',131:'TP MAP capteur pression absolue',132:'Module relais gestion moteur CMP arbre à cames',133:'HO2S lambda collecteur carburant',134:'Collecteur carburant rampe injecteurs dépose',135:'Injecteurs carburant repose joints toriques',136:'Table des matières système alimentation',138:'Essai pression système alimentation outils 18G',140:'Vidange réservoir carburant interrupteur inertiel IFS',141:'Filtre carburant dépose repose',142:'Pompe alimentation réservoir carburant',143:'Réservoir carburant dépose',144:'Réservoir jaugeur pompe clapet ventilation',145:'Clapet deux voies réservoir carburant'}

def page_kind(n:int)->str:
    if 34<=n<=40 or 42<=n<=46 or n in (48,49): return 'table_page'
    if n in (104,136): return 'toc_page'
    if n in (106,110): return 'component_location_page'
    if 107<=n<=117: return 'technical_description_page'
    if n==118: return 'adjustment_page'
    return 'procedure_page'

def sha256_bytes(b:bytes)->str:return hashlib.sha256(b).hexdigest()
def sha256_file(p:Path)->str:return sha256_bytes(p.read_bytes())
def numeric_suffix(path:Path)->int:
    m=re.search(r'_(\d+)\.qz64$',path.name)
    return int(m.group(1)) if m else 0

def qt_uncompress(data:bytes)->bytes:
    if len(data)<4: raise AssertionError('qCompress payload too short')
    expected=int.from_bytes(data[:4],'big')
    for wb in (zlib.MAX_WBITS,-zlib.MAX_WBITS):
        try:
            out=zlib.decompress(data[4:],wb)
            if len(out)==expected:return out
        except Exception: pass
    raise AssertionError('cannot qUncompress payload')

def decode_qz64(path:Path)->bytes:
    raw=path.read_bytes()
    try:return qt_uncompress(raw)
    except Exception:return qt_uncompress(base64.b64decode(b''.join(raw.split())))

def execute_project_sql(db:sqlite3.Connection,sql_raw:bytes)->None:
    text=sql_raw.decode('utf-8').replace('\ufeff','')
    text=re.sub(r'(?m)^\s*BEGIN(?:\s+TRANSACTION|\s+IMMEDIATE)?\s*;\s*$', '', text, flags=re.I)
    text=re.sub(r'(?m)^\s*COMMIT\s*;\s*$', '', text, flags=re.I)
    text=re.sub(r'(?m)^\s*PRAGMA\s+user_version\s*=\s*\d+\s*;\s*$', '', text, flags=re.I)
    db.executescript(text)

def rebuild_reference_database(reference:Path,destination:Path)->sqlite3.Connection:
    if destination.exists():destination.unlink()
    db=sqlite3.connect(str(destination))
    seeds=sorted(reference.glob('mems_reference_seed_*.qz64'),key=lambda p:(numeric_suffix(p),p.name))
    assert seeds,'no seed files'
    encoded=b''.join(p.read_bytes().strip() for p in seeds)
    execute_project_sql(db,qt_uncompress(base64.b64decode(encoded)))
    manifest=json.loads((reference/'manifest.json').read_text(encoding='utf-8-sig'))
    ordered=[];seen=set()
    for name in manifest.get('research_enrichment_batches',[]):
        p=reference/name
        if p.is_file() and p.name!='research_enrichment_1600.qz64':
            ordered.append(p);seen.add(p.resolve())
    for p in sorted(reference.rglob('research_enrichment*.qz64'),key=lambda p:(numeric_suffix(p),str(p))):
        if p.name=='research_enrichment_1600.qz64':continue
        if p.resolve() not in seen:ordered.append(p);seen.add(p.resolve())
    for p in ordered:execute_project_sql(db,decode_qz64(p))
    db.execute(f'PRAGMA user_version={EXPECTED_USER_VERSION}');db.commit();return db

def q1(db,sql,args=()):return db.execute(sql,args).fetchone()[0]

def verify_production(repo:Path):
    out=subprocess.check_output(['git','ls-remote','origin','refs/heads/MEMSX64'],cwd=repo,text=True).strip().split()[0]
    assert out==EXPECTED_MEMSX64,(out,EXPECTED_MEMSX64)
    print('PRODUCTION_PASS MEMSX64 #101 unchanged',out)

def reconstruct_transport(repo:Path)->Path:
    transport=repo/'.rcl1790_transport'
    src=transport/'rcl0193fre_1790_g4.tif.xz'
    assert src.is_file(),src
    raw=src.read_bytes()
    assert sha256_bytes(raw)==EXPECTED_G4_XZ_SHA
    tiff=lzma.decompress(raw)
    assert sha256_bytes(tiff)==EXPECTED_G4_TIFF_SHA
    p=Path('/tmp/rcl0193fre_1790_g4.tif');p.write_bytes(tiff)
    print('TRANSPORT_PASS G4 TIFF xz/tiff hashes exact')
    return p

def install_qz(repo:Path,reference:Path):
    src=repo/'.rcl1790_transport'/'research_enrichment_1790.qz64'
    assert src.is_file(),src
    raw=src.read_bytes();assert sha256_bytes(raw)==EXPECTED_QZ_SHA
    sql=decode_qz64(src);assert sha256_bytes(sql)==EXPECTED_SQL_SHA
    dst=reference/'research_enrichment_1790.qz64';assert not dst.exists();dst.write_bytes(raw)
    print('QZ_PASS',len(raw),len(sql))

def render_assets(tiff_path:Path,reference:Path):
    from PIL import Image
    im=Image.open(tiff_path)
    assert getattr(im,'n_frames',0)==EXPECTED_SUBSET_PAGES, getattr(im,'n_frames',0)
    out=reference/'images'/'rave';out.mkdir(parents=True,exist_ok=True)
    hashes={}
    for idx,original_page in enumerate(SELECTED):
        im.seek(idx)
        frame=im.copy()
        assert frame.size==(992,1403),(original_page,frame.size)
        assert frame.mode=='1', (original_page,frame.mode)
        target=out/f'RCL0193FRE_PDF_{original_page:03d}.png'
        assert not target.exists(),target
        frame.save(target,format='PNG',optimize=True)
        hashes[original_page]=sha256_file(target)
    im.close();assert len(hashes)==52
    print('RENDER_PASS 52 factory pages from lossless G4 120 dpi source')
    return hashes

def patch_manifest(reference:Path,hashes:dict[int,str]):
    mp=reference/'manifest.json';manifest=json.loads(mp.read_text(encoding='utf-8-sig'))
    batches=manifest.setdefault('research_enrichment_batches',[])
    assert 'research_enrichment_1790.qz64' not in batches
    batches.append('research_enrichment_1790.qz64')
    diagrams=manifest.setdefault('diagrams',{})
    visual=manifest.setdefault('visual_assets',{})
    for n in SELECTED:
        path=f'images/rave/RCL0193FRE_PDF_{n:03d}.png'
        diagrams[f'RAVE RCL0193FRE PDF {n:03d} {PAGE_TOPICS[n]}']=path
        key=f'rave:RCL0193FRE:PDF:{n}'
        assert key not in visual
        visual[key]={'path':path,'asset_kind':page_kind(n),'document':'RCL0193FRE','source_page':f'PDF p.{n}','pdf_index':n,'sha256':hashes[n]}
    mp.write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('MANIFEST_PASS 1790 + 52 assets')

def write_audit(reference:Path,hashes:dict[int,str]):
    lines=[
      '# RCL0193FRE — audit lot 1790','',
      'Source : manuel atelier Mini français `RCL0193FRE`, fichier utilisateur `Manuel Rover MPI.pdf`.','',
      f'- PDF source original vérifié localement avant pousse : {EXPECTED_SOURCE_PAGES} pages, {EXPECTED_SOURCE_BYTES} octets.',
      f'- SHA-256 PDF source : `{EXPECTED_SOURCE_SHA}`.',
      f'- Transport visuel dérivé uniquement des 52 pages retenues : TIFF monochrome CCITT Group 4 120 dpi SHA-256 `{EXPECTED_G4_TIFF_SHA}` ; archive XZ SHA-256 `{EXPECTED_G4_XZ_SHA}`.',
      '- Aucun OCR approximatif : contenu structuré issu de la couche texte décodée déterministement et contrôlé contre les pages constructeur.','',
      '## Périmètre 1790','',
      '- Information/réglages/couples/fluides : PDF 34-49 (pages utiles seulement).',
      '- Gestion moteur MEMS : PDF 104-135 (pages utiles seulement).',
      '- Alimentation carburant : PDF 136-145 (pages utiles seulement).',
      '- SPi manuelle, SPi automatique et MPi restent séparés ; aucune génération MEMS n’est inventée.','',
      '## Comptages structurés','',
      '- 429 connaissances ; 350 spécifications ; 351 valeurs ; 51 procédures/phases pour 27 opérations ; 344 étapes ; 50 exigences ; 379 relations ; 51 alias.','',
      '## Validation','',
      '- `PRAGMA integrity_check = ok` ; `PRAGMA user_version = 20`.',
      '- Historiques préservés : 93 `mems_rave_fact`, 105 `mems_expert_fact_external`.',
      '- Culasse : séquence 34 N.m puis 34 N.m de plus conservée.',
      '- PDF 45 : conflit imprimé 32/30 N.m pour le même libellé de ceinture conservé comme conflit à vérifier.','',
      '## Captures constructeur conservées','']
    for n in SELECTED: lines.append(f'- PDF p.{n}: `images/rave/RCL0193FRE_PDF_{n:03d}.png` — `{hashes[n]}`')
    lines += ['', 'Aucune table historique n’a été modifiée et aucune capture n’a été générée artificiellement : les PNG sont des rendus monochromes CCITT Group 4 à 120 dpi des pages du manuel source, sans image générée ni contenu redessiné.','']
    p=reference/'audits'/'rcl0193fre_1790_audit.md';p.parent.mkdir(parents=True,exist_ok=True);p.write_text('\n'.join(lines),encoding='utf-8')
    print('AUDIT_PASS')

def validate_database(reference:Path,tmp:Path):
    db=rebuild_reference_database(reference,tmp/'candidate.sqlite')
    try:
        assert q1(db,'PRAGMA integrity_check')=='ok'
        assert q1(db,'PRAGMA user_version')==EXPECTED_USER_VERSION
        assert q1(db,'SELECT count(*) FROM mems_rave_fact')==EXPECTED_HIST_RAVE
        assert q1(db,'SELECT count(*) FROM mems_expert_fact_external')==EXPECTED_HIST_EXPERT
        assert q1(db,'SELECT count(*) FROM mems_knowledge_item WHERE source_key=?',(SOURCE_KEY,))==EXPECTED_NEW_KNOWLEDGE
        assert q1(db,"SELECT count(*) FROM mems_applicability_scope WHERE scope_key LIKE 'SCOPE-RCL0193FRE-%'")==EXPECTED_NEW_SCOPES
        assert q1(db,'SELECT count(*) FROM mems_knowledge_scope s JOIN mems_knowledge_item k USING(knowledge_key) WHERE k.source_key=?',(SOURCE_KEY,))==EXPECTED_NEW_KNOWLEDGE
        assert q1(db,'SELECT count(*) FROM mems_specification s JOIN mems_knowledge_item k USING(knowledge_key) WHERE k.source_key=?',(SOURCE_KEY,))==EXPECTED_NEW_SPEC
        assert q1(db,'SELECT count(*) FROM mems_specification_value v JOIN mems_specification s USING(spec_key) JOIN mems_knowledge_item k USING(knowledge_key) WHERE k.source_key=?',(SOURCE_KEY,))==EXPECTED_NEW_VALUES
        assert q1(db,'SELECT count(*) FROM mems_procedure p JOIN mems_knowledge_item k USING(knowledge_key) WHERE k.source_key=?',(SOURCE_KEY,))==EXPECTED_NEW_PROCS
        assert q1(db,'SELECT count(*) FROM mems_procedure_step st JOIN mems_procedure p USING(procedure_key) JOIN mems_knowledge_item k USING(knowledge_key) WHERE k.source_key=?',(SOURCE_KEY,))==EXPECTED_NEW_STEPS
        assert q1(db,'SELECT count(*) FROM mems_procedure_requirement r JOIN mems_procedure p USING(procedure_key) JOIN mems_knowledge_item k USING(knowledge_key) WHERE k.source_key=?',(SOURCE_KEY,))==EXPECTED_NEW_REQS
        assert q1(db,'SELECT count(*) FROM mems_knowledge_relation r JOIN mems_knowledge_item k ON k.knowledge_key=r.from_key WHERE k.source_key=?',(SOURCE_KEY,))==EXPECTED_NEW_RELATIONS
        alias_keys=('ckp_sensor','cmp_sensor','ecm','ect_sensor','fuel_injector','fuel_pressure_regulator','fuel_pump','fuel_rail','ho2s_sensor','iacv','iat_sensor','ifs','map_sensor','relay_module','tp_sensor')
        ph=','.join('?' for _ in alias_keys)
        assert q1(db,f"SELECT count(*) FROM mems_term_alias WHERE entity_type='component' AND language='fr' AND entity_key IN ({ph})",alias_keys)==EXPECTED_NEW_ALIASES
        def scope_for(k):return db.execute('SELECT scope_key FROM mems_knowledge_scope WHERE knowledge_key=?',(k,)).fetchone()[0]
        assert scope_for('KNOW-RCL0193FRE-P038')==SCOPE_SPI_MAN
        assert scope_for('KNOW-RCL0193FRE-P039')==SCOPE_SPI_AUTO
        assert scope_for('KNOW-RCL0193FRE-P040')==SCOPE_MPI
        assert scope_for('KNOW-RCL0193FRE-P043')==SCOPE_GENERAL
        assert scope_for('KNOW-RCL0193FRE-P104')==SCOPE_MPI
        assert scope_for('KNOW-RCL0193FRE-P145')==SCOPE_MPI
        assert q1(db,"SELECT count(*) FROM mems_applicability_scope WHERE scope_key LIKE 'SCOPE-RCL0193FRE-%' AND mems_family IS NOT NULL")==0
        rows=db.execute("SELECT sequence_no,value_numeric,instruction_text FROM mems_specification_value WHERE spec_key='SPEC-RCL0193FRE-350' ORDER BY sequence_no").fetchall()
        assert rows==[(1,34.0,'Premier serrage: 34 N.m'),(2,34.0,'Puis 34 N.m de plus')],rows
        assert q1(db,"SELECT count(*) FROM mems_knowledge_item WHERE source_key=? AND source_section='PDF p.45' AND source_text LIKE '%Boulon d''ancrage supérieur de ceinture avant%' AND verification_level='conflit_a_verifier'",(SOURCE_KEY,))==2
        assert q1(db,"SELECT count(*) FROM mems_knowledge_relation r JOIN mems_knowledge_item k ON k.knowledge_key=r.from_key WHERE k.source_key=? AND r.relation_type='conflicts_with'",(SOURCE_KEY,))==2
        assert q1(db,'SELECT count(*) FROM mems_knowledge_item k LEFT JOIN mems_knowledge_scope s USING(knowledge_key) WHERE k.source_key=? AND s.knowledge_key IS NULL',(SOURCE_KEY,))==0
        assert q1(db,'SELECT count(DISTINCT image_ref) FROM mems_knowledge_item WHERE source_key=? AND image_ref IS NOT NULL',(SOURCE_KEY,))==52
        print('DB_PASS integrity=ok uv=20 hist=93/105 k=429 spec=350 values=351 proc=51 steps=344 req=50 rel=379 alias=51')
    finally:db.close()

def validate_assets(reference:Path):
    manifest=json.loads((reference/'manifest.json').read_text(encoding='utf-8-sig'))
    assert manifest['research_enrichment_batches'].count('research_enrichment_1790.qz64')==1
    visual=manifest['visual_assets']
    for n in SELECTED:
        key=f'rave:RCL0193FRE:PDF:{n}';assert key in visual
        p=reference/visual[key]['path'];assert p.is_file();assert sha256_file(p)==visual[key]['sha256']
    qz=reference/'research_enrichment_1790.qz64';assert sha256_file(qz)==EXPECTED_QZ_SHA;assert sha256_bytes(decode_qz64(qz))==EXPECTED_SQL_SHA
    print('COMMITTED_ASSET_PASS 52/52 + qz exact')

def scope_guard(repo:Path):
    allowed={'database/reference/research_enrichment_1790.qz64','database/reference/audits/rcl0193fre_1790_audit.md','database/reference/manifest.json'}
    allowed|={f'database/reference/images/rave/RCL0193FRE_PDF_{n:03d}.png' for n in SELECTED}
    temps={'.github/workflows/temp-rcl0193fre-1790-render-install.yml','tools/install_rcl0193fre_1790_render.py'}
    temps|={p.as_posix() for p in Path('.rcl1790_transport').glob('*')}
    status=subprocess.check_output(['git','status','--porcelain','--untracked-files=all'],cwd=repo,text=True)
    actual=set()
    for line in status.splitlines():
        p=line[3:]
        if p in temps or p.startswith('.rcl1790_transport/'):continue
        actual.add(p)
    assert actual==allowed,(sorted(actual-allowed),sorted(allowed-actual))
    print('SCOPE_PASS final documentary delta exactly 55 paths')

def install(repo:Path):
    verify_production(repo)
    reference=repo/'database'/'reference'
    subset=reconstruct_transport(repo)
    install_qz(repo,reference)
    hashes=render_assets(subset,reference)
    patch_manifest(reference,hashes)
    write_audit(reference,hashes)
    tmp=Path('/tmp/rcl0193fre1790validate');shutil.rmtree(tmp,ignore_errors=True);tmp.mkdir()
    validate_database(reference,tmp);validate_assets(reference);scope_guard(repo)
    shutil.rmtree(tmp)

def post(repo:Path):
    verify_production(repo);reference=repo/'database'/'reference';validate_assets(reference)
    tmp=Path('/tmp/rcl0193fre1790post');shutil.rmtree(tmp,ignore_errors=True);tmp.mkdir();validate_database(reference,tmp);shutil.rmtree(tmp)
    for p in ['.github/workflows/temp-rcl0193fre-1790-render-install.yml','tools/install_rcl0193fre_1790_render.py','.rcl1790_transport']:assert not (repo/p).exists(),p
    assert subprocess.check_output(['git','status','--porcelain'],cwd=repo,text=True).strip()==''
    print('POST_PUSH_PASS remote committed bytes reproduce all 1790 invariants and temporary transport is absent')

if __name__=='__main__':
    mode=sys.argv[1];repo=Path(sys.argv[2] if len(sys.argv)>2 else '.').resolve()
    install(repo) if mode=='install' else post(repo)
