from __future__ import annotations
import base64, hashlib, json, lzma, re, sqlite3, subprocess, sys, tempfile, zlib
from pathlib import Path
EXPECTED_MEMSX64='22dbe75ed14e0a61e694159d505ef72245116b48'
EXPECTED_G4_XZ_SHA='2f73a5b6009b8b2607439752bb65290d39744d9b27081e99a496b35a026b50a3'
EXPECTED_G4_TIFF_SHA='82d307a598df5af48ec7ef58ae376633298e9412f413362c170ec0f665ee3f61'
EXPECTED_QZ_SHA='a54409e1e2cf4ddfe5db91e98a94ae5260dad933feb3a8c0c0c870d665dee3a0'
EXPECTED_SQL_SHA='1b282d86f7093b18e153079b102da8c87759d4343bfc7fddf0297675c6b427a8'
GENERAL_SCOPE='SCOPE-RCL0193FRE-MINI-VIN134455'
SELECTED=[176,178,179,180,181,182,183,184,185,186,187,188,189,190,192,194,195,196,197,198,199,200,202,204,205,206,207,208,209,210,212,214,215,216,217,218,219,220,221,222,223,224,225]
TOP={176:'Embrayage - table des matières',178:'Jeu de butée de débrayage - contrôle/réglage',179:'Jeu butée et purge embrayage',180:'Couvercle de carter de volant - dépose',181:'Couvercle carter volant - dépose suite',182:'Couvercle carter volant - dépose suite',183:'Couvercle carter volant - dépose suite',184:'Couvercle carter volant - repose',185:'Embrayage et butée de débrayage',186:'Embrayage - dépose/repose',187:'Butée de débrayage et maître-cylindre',188:'Maître-cylindre embrayage - dépose',189:'Maître-cylindre - repose / cylindre récepteur',190:'Cylindre récepteur embrayage',192:'Boîte de vitesses manuelle - table des matières',194:'Jeu axial du pignon de renvoi',195:'Pignon de renvoi - contrôle du jeu axial',196:'Fixations sélecteur / boîte de vitesses',197:'Vis sans fin compteur de vitesse',198:'Boîte de vitesses - dépose/repose suite',199:'Pignon de commande de compteur',200:'Contacteur des feux de recul',202:'Arbres de transmission - table des matières',204:"Joints d'arbre de roue",205:"Arbre de roue et joints - suite",206:"Arbre de roue - dépose",207:"Arbre de roue - dépose suite",208:"Arbre de roue - dépose suite",209:"Arbre de roue - repose",210:'Direction - table des matières',212:'Parallélisme des roues avant',214:'Crémaillère et pignon de direction',215:'Crémaillère - dépose suite',216:'Crémaillère - dépose suite',217:'Crémaillère - dépose suite',218:'Crémaillère - repose / colonne',219:'Antivol de direction',220:'Colonne de direction - dépose',221:'Colonne de direction - repose',222:'Colonne de direction - dépose suite',223:'Colonne de direction - dépose suite',224:'Colonne de direction - repose / volant',225:'Volant - repose'}
TOC={176,192,202,210}; ADJUST={178,179,194,195,212}
def sha(b): return hashlib.sha256(b).hexdigest()
def shaf(p): return sha(Path(p).read_bytes())
def numsuf(p):
 m=re.search(r'_(\d+)\.qz64$',p.name); return int(m.group(1)) if m else 0
def qun(data):
 expected=int.from_bytes(data[:4],'big')
 for w in (zlib.MAX_WBITS,-zlib.MAX_WBITS):
  try:
   out=zlib.decompress(data[4:],w)
   if len(out)==expected:return out
  except Exception:pass
 raise AssertionError('qz decode')
def dq(p):
 raw=Path(p).read_bytes()
 try:return qun(raw)
 except Exception:return qun(base64.b64decode(b''.join(raw.split())))
def execsql(db,b):
 t=b.decode('utf-8').replace('\ufeff','')
 t=re.sub(r'(?m)^\s*BEGIN(?:\s+TRANSACTION|\s+IMMEDIATE)?\s*;\s*$','',t,flags=re.I)
 t=re.sub(r'(?m)^\s*COMMIT\s*;\s*$','',t,flags=re.I)
 t=re.sub(r'(?m)^\s*PRAGMA\s+user_version\s*=\s*\d+\s*;\s*$','',t,flags=re.I)
 db.executescript(t)
def rebuild(ref,dst):
 if dst.exists():dst.unlink()
 db=sqlite3.connect(dst)
 seeds=sorted(ref.glob('mems_reference_seed_*.qz64'),key=lambda p:(numsuf(p),p.name));assert seeds
 execsql(db,qun(base64.b64decode(b''.join(p.read_bytes().strip() for p in seeds))))
 man=json.loads((ref/'manifest.json').read_text(encoding='utf-8-sig'));seen=set();ordered=[]
 for name in man.get('research_enrichment_batches',[]):
  p=ref/name
  if p.is_file() and p.name!='research_enrichment_1600.qz64':ordered.append(p);seen.add(p.resolve())
 for p in sorted(ref.rglob('research_enrichment*.qz64'),key=lambda p:(numsuf(p),str(p))):
  if p.name=='research_enrichment_1600.qz64' or p.resolve() in seen:continue
  ordered.append(p);seen.add(p.resolve())
 for p in ordered:execsql(db,dq(p))
 db.execute('pragma user_version=20');db.commit();return db
def q1(db,s,a=()):return db.execute(s,a).fetchone()[0]
def verify_prod(repo):
 h=subprocess.check_output(['git','ls-remote','origin','refs/heads/MEMSX64'],cwd=repo,text=True).split()[0]
 assert h==EXPECTED_MEMSX64,(h,EXPECTED_MEMSX64);print('PRODUCTION_PASS MEMSX64 #101 unchanged',h)
def transport(repo):
 p=repo/'.rcl1820_transport'/'rcl0193fre_1820_g4_150.tif.xz';assert shaf(p)==EXPECTED_G4_XZ_SHA
 b=lzma.decompress(p.read_bytes());assert sha(b)==EXPECTED_G4_TIFF_SHA
 t=Path('/tmp/rcl1820.tif');t.write_bytes(b);print('TRANSPORT_PASS G4 xz/tiff hashes exact');return t
def install_qz(repo,ref):
 p=repo/'.rcl1820_transport'/'research_enrichment_1820.qz64';assert shaf(p)==EXPECTED_QZ_SHA and sha(dq(p))==EXPECTED_SQL_SHA
 d=ref/'research_enrichment_1820.qz64';assert not d.exists();d.write_bytes(p.read_bytes());print('QZ_PASS',len(p.read_bytes()),len(dq(p)))
def render(tiff,ref):
 from PIL import Image
 im=Image.open(tiff);assert im.n_frames==43 and im.size==(1240,1755) and im.mode=='1'
 out=ref/'images'/'rave';out.mkdir(parents=True,exist_ok=True);hs={}
 for i,n in enumerate(SELECTED):
  im.seek(i);f=im.copy();dst=out/f'RCL0193FRE_PDF_{n:03d}.png';assert not dst.exists();f.save(dst,'PNG',optimize=True);hs[n]=shaf(dst)
 im.close();print('RENDER_PASS 43 factory pages @150dpi');return hs
def kind(n):
 if n in TOC:return 'toc_page'
 if n in ADJUST:return 'adjustment_page'
 return 'procedure_page'
def patch_manifest(ref,hs):
 p=ref/'manifest.json';m=json.loads(p.read_text(encoding='utf-8-sig'));b=m.setdefault('research_enrichment_batches',[]);assert 'research_enrichment_1820.qz64' not in b;b.append('research_enrichment_1820.qz64')
 d=m.setdefault('diagrams',{});v=m.setdefault('visual_assets',{})
 for n in SELECTED:
  path=f'images/rave/RCL0193FRE_PDF_{n:03d}.png';d[f'RAVE RCL0193FRE PDF {n:03d} {TOP[n]}']=path;k=f'rave:RCL0193FRE:PDF:{n}';assert k not in v;v[k]={'path':path,'asset_kind':kind(n),'document':'RCL0193FRE','source_page':f'PDF p.{n}','pdf_index':n,'sha256':hs[n]}
 p.write_text(json.dumps(m,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('MANIFEST_PASS 1820 +43 assets')
def audit(ref,hs):
 lines=['# RCL0193FRE - audit lot 1820','', '- Source PDF SHA-256: `0c7fef28d0d0f0673ba321d6625a019c005823103caa98afb3258114e1fec713`.', '- Périmètre: embrayage + boîte manuelle + arbres de transmission + direction, PDF 176-225.', '- 43 pages réellement utiles conservées; intercalaires/pages vides exclus.', '- 43 captures constructeur monochromes CCITT Group 4 150 dpi.', '- Portée documentaire unique: `SCOPE-RCL0193FRE-MINI-VIN134455`; aucune famille MEMS supplémentaire inventée.','', '## Comptages','- 69 connaissances nouvelles.','- 43 spécifications et 43 valeurs.','- 40 phases de procédure, 509 étapes, 51 exigences, 26 relations.','- 21 groupes d’alias français tentés; INSERT OR IGNORE conserve les alias existants sans doublon.','- Intégrité SQLite ok; user_version 20; historiques 93/105 préservés.','', '## Points notables','- Jeu de butée embrayage: 6,5 mm.','- Jeu axial du pignon de renvoi: 0,101 à 0,177 mm.','- Parallélisme avant: écartement de 0° 15\' ± 7,5\' par côté, conservé tel que décodé/imprimé.','- Les tailles de clés, forets et piges restent dans les procédures et ne sont pas transformées en spécifications véhicule.','- La procédure volant conserve le renvoi constructeur au système SRS sans inventer la procédure airbag.','', '## Captures']
 for n in SELECTED:lines.append(f'- PDF p.{n}: `images/rave/RCL0193FRE_PDF_{n:03d}.png` - `{hs[n]}`')
 p=ref/'audits'/'rcl0193fre_1820_audit.md';p.write_text('\n'.join(lines)+'\n',encoding='utf-8');print('AUDIT_PASS')
def validate(ref,tmp):
 db=rebuild(ref,tmp/'db.sqlite')
 try:
  assert q1(db,'pragma integrity_check')=='ok' and q1(db,'pragma user_version')==20
  assert q1(db,'select count(*) from mems_rave_fact')==93 and q1(db,'select count(*) from mems_expert_fact_external')==105
  assert q1(db,"select count(*) from mems_knowledge_item where knowledge_key like 'KNOW-RCL0193FRE-1820-%'")==69
  assert q1(db,"select count(*) from mems_knowledge_scope where knowledge_key like 'KNOW-RCL0193FRE-1820-%'")==69
  assert q1(db,"select count(*) from mems_specification where spec_key like 'SPEC-RCL0193FRE-1820-%'")==43
  assert q1(db,"select count(*) from mems_specification_value where spec_key like 'SPEC-RCL0193FRE-1820-%'")==43
  assert q1(db,"select count(*) from mems_procedure where procedure_key like 'PROC-RCL0193FRE-1820-%'")==40
  assert q1(db,"select count(*) from mems_procedure_step where procedure_key like 'PROC-RCL0193FRE-1820-%'")==509
  assert q1(db,"select count(*) from mems_procedure_requirement where procedure_key like 'PROC-RCL0193FRE-1820-%'")==51
  assert q1(db,"select count(*) from mems_knowledge_relation where from_key like 'KNOW-RCL0193FRE-1820-%'")==26
  assert q1(db,"select count(*) from mems_knowledge_item k left join mems_knowledge_scope s using(knowledge_key) where k.knowledge_key like 'KNOW-RCL0193FRE-1820-%' and s.knowledge_key is null")==0
  assert q1(db,"select count(*) from mems_knowledge_scope where knowledge_key like 'KNOW-RCL0193FRE-1820-%' and scope_key!=?",(GENERAL_SCOPE,))==0
  assert q1(db,"select count(distinct image_ref) from mems_knowledge_item where knowledge_key like 'KNOW-RCL0193FRE-1820-P%'")==43
  assert q1(db,"select count(*) from mems_specification_value where spec_key='SPEC-RCL0193FRE-1820-CLUTCH-RELEASE-CLEARANCE' and abs(value_min-6.5)<1e-9 and abs(value_max-6.5)<1e-9 and unit='mm'")==1
  assert q1(db,"select count(*) from mems_specification_value where spec_key='SPEC-RCL0193FRE-1820-IDLER-END-FLOAT' and abs(value_min-0.101)<1e-9 and abs(value_max-0.177)<1e-9 and unit='mm'")==1
  assert q1(db,"select count(*) from mems_specification_value where spec_key='SPEC-RCL0193FRE-1820-FRONT-TOE' and value_text like 'Ecartement de 0° 15%'")==1
  print('DB_PASS integrity=ok uv=20 hist=93/105 k=69 spec=43 values=43 proc=40 steps=509 req=51 rel=26')
 finally:db.close()
def validate_assets(ref,hs):
 assert len(hs)==43
 for n,h in hs.items():assert shaf(ref/'images'/'rave'/f'RCL0193FRE_PDF_{n:03d}.png')==h
 assert shaf(ref/'research_enrichment_1820.qz64')==EXPECTED_QZ_SHA
 print('COMMITTED_ASSET_PASS 43/43 + qz exact')
def allowed_delta(repo):
 out=subprocess.check_output(['git','status','--porcelain','--untracked-files=all'],cwd=repo,text=True).splitlines();bad=[]
 pats=[re.compile(r'database/reference/research_enrichment_1820\.qz64$'),re.compile(r'database/reference/manifest\.json$'),re.compile(r'database/reference/audits/rcl0193fre_1820_audit\.md$'),re.compile(r'database/reference/images/rave/RCL0193FRE_PDF_\d{3}\.png$')]
 for line in out:
  path=line[3:]
  if path.startswith('.rcl1820_transport/') or path in {'.github/workflows/temp-rcl0193fre-1820-install.yml','tools/install_rcl0193fre_1820.py'}:continue
  if not any(p.fullmatch(path) for p in pats):bad.append(line)
 assert not bad,bad;assert len([l for l in out if 'RCL0193FRE_PDF_' in l])==43
 print('SCOPE_PASS final documentary delta exactly 46 paths')
def install(repo):
 repo=Path(repo);ref=repo/'database'/'reference';verify_prod(repo);t=transport(repo);install_qz(repo,ref);hs=render(t,ref);patch_manifest(ref,hs);audit(ref,hs)
 with tempfile.TemporaryDirectory() as td:validate(ref,Path(td))
 validate_assets(ref,hs);allowed_delta(repo)
def post(repo):
 repo=Path(repo);ref=repo/'database'/'reference';verify_prod(repo)
 with tempfile.TemporaryDirectory() as td:validate(ref,Path(td))
 m=json.loads((ref/'manifest.json').read_text(encoding='utf-8-sig'));assert 'research_enrichment_1820.qz64' in m.get('research_enrichment_batches',[])
 assert sum(1 for k in m.get('visual_assets',{}) if k.startswith('rave:RCL0193FRE:PDF:') and int(k.rsplit(':',1)[1]) in SELECTED)==43
 assert all((ref/'images'/'rave'/f'RCL0193FRE_PDF_{n:03d}.png').exists() for n in SELECTED)
 assert not (repo/'.rcl1820_transport').exists();assert not (repo/'.github/workflows/temp-rcl0193fre-1820-install.yml').exists();assert not (repo/'tools/install_rcl0193fre_1820.py').exists();assert not subprocess.check_output(['git','status','--porcelain'],cwd=repo,text=True).strip()
 print('POST_PASS remote 1820 committed, transport/workflow clean')
if __name__=='__main__':
 mode=sys.argv[1];repo=sys.argv[2];{'install':install,'post':post}[mode](repo)
