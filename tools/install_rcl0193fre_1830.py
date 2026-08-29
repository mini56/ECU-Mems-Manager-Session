from __future__ import annotations
import base64,hashlib,json,lzma,re,sqlite3,subprocess,sys,tempfile,zlib
from pathlib import Path

EXPECTED_MEMSX64='22dbe75ed14e0a61e694159d505ef72245116b48'
EXPECTED_G4_XZ_SHA='0528cb3bf3bdf7c67c8312729de2d822e63c439f5178899d4c0055ffefb21d2f'
EXPECTED_G4_TIFF_SHA='738d77d2041431b85fa4ab474562730d7f152db74189f681ddfd27c00b24b668'
EXPECTED_QZ_SHA='e22d16708d23ac0cedaad6ad7e2811115025a218f6101ce9551a5c19917851c7'
EXPECTED_SQL_SHA='c9e390a0edb9da0e3108449affe1175943a2137972bbec0e1c1a0b0dbacaacd4'
GENERAL_SCOPE='SCOPE-RCL0193FRE-MINI-VIN134455'
SELECTED=[226,228,230,231,232,234,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254]
TOP={226:'Suspension - table des matières',228:"Réglage d'amortisseur",230:'Caoutchoucs de bond et de rebond avant',231:'Amortisseur avant',232:'Amortisseur arrière',234:'Freins - table des matières',236:'Purge du circuit des freins',237:'Contacteur des feux stop - réglage',238:'Tambour de frein arrière',239:'Contacteur de témoin de défaillance de frein',240:'Soupape de tarage - dépose',241:'Soupape de tarage - repose',242:'Contacteur des feux stop / maître-cylindre',243:'Maître-cylindre - repose',244:'Pédale de frein - dépose',245:'Pédale de frein - dépose suite',246:'Pédale de frein - repose / plaquettes avant',247:'Plaquettes avant - repose / segments arrière',248:'Segments arrière - inspection/repose',249:'Ensemble de servocommande - dépose',250:'Servocommande - repose',251:'Étrier de frein avant',252:'Cylindre de frein arrière / câble frein à main',253:'Câble de frein à main avant - dépose',254:'Câble de frein à main avant - repose'}

def sha(b): return hashlib.sha256(b).hexdigest()
def shaf(p): return sha(Path(p).read_bytes())
def numsuf(p):
 m=re.search(r'_(\d+)\.qz64$',p.name); return int(m.group(1)) if m else 0
def qun(data):
 e=int.from_bytes(data[:4],'big')
 for w in (zlib.MAX_WBITS,-zlib.MAX_WBITS):
  try:
   o=zlib.decompress(data[4:],w)
   if len(o)==e:return o
  except Exception: pass
 raise AssertionError('qz decode')
def dq(p):
 r=Path(p).read_bytes()
 try:return qun(r)
 except Exception:return qun(base64.b64decode(b''.join(r.split())))
def execsql(db,b):
 t=b.decode('utf-8').replace('\ufeff','')
 t=re.sub(r'(?m)^\s*BEGIN(?:\s+TRANSACTION|\s+IMMEDIATE)?\s*;\s*$','',t,flags=re.I)
 t=re.sub(r'(?m)^\s*COMMIT\s*;\s*$','',t,flags=re.I)
 t=re.sub(r'(?m)^\s*PRAGMA\s+user_version\s*=\s*\d+\s*;\s*$','',t,flags=re.I)
 db.executescript(t)
def rebuild(ref,dst):
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
 p=repo/'.rcl1830_transport'/'rcl0193fre_1830_g4_150.tif.xz';assert shaf(p)==EXPECTED_G4_XZ_SHA
 b=lzma.decompress(p.read_bytes());assert sha(b)==EXPECTED_G4_TIFF_SHA
 t=Path('/tmp/rcl1830.tif');t.write_bytes(b);print('TRANSPORT_PASS G4 xz/tiff hashes exact');return t
def install_qz(repo,ref):
 p=repo/'.rcl1830_transport'/'research_enrichment_1830.qz64';assert shaf(p)==EXPECTED_QZ_SHA and sha(dq(p))==EXPECTED_SQL_SHA
 d=ref/'research_enrichment_1830.qz64';assert not d.exists();d.write_bytes(p.read_bytes());print('QZ_PASS',len(p.read_bytes()),len(dq(p)))
def render(tiff,ref):
 from PIL import Image
 im=Image.open(tiff);assert im.n_frames==25 and im.size==(1240,1755) and im.mode=='1'
 out=ref/'images'/'rave';out.mkdir(parents=True,exist_ok=True);hs={}
 for i,n in enumerate(SELECTED):
  im.seek(i);dst=out/f'RCL0193FRE_PDF_{n:03d}.png';assert not dst.exists();im.copy().save(dst,'PNG',optimize=True);hs[n]=shaf(dst)
 im.close();print('RENDER_PASS 25 factory pages @150dpi');return hs
def kind(n):
 if n in (226,234):return 'toc_page'
 if n in (228,236,237):return 'adjustment_page'
 return 'procedure_page'
def patch_manifest(ref,hs):
 p=ref/'manifest.json';m=json.loads(p.read_text(encoding='utf-8-sig'));b=m.setdefault('research_enrichment_batches',[]);assert 'research_enrichment_1830.qz64' not in b;b.append('research_enrichment_1830.qz64')
 d=m.setdefault('diagrams',{});v=m.setdefault('visual_assets',{})
 for n in SELECTED:
  path=f'images/rave/RCL0193FRE_PDF_{n:03d}.png';d[f'RAVE RCL0193FRE PDF {n:03d} {TOP[n]}']=path;k=f'rave:RCL0193FRE:PDF:{n}';assert k not in v;v[k]={'path':path,'asset_kind':kind(n),'document':'RCL0193FRE','source_page':f'PDF p.{n}','pdf_index':n,'sha256':hs[n]}
 p.write_text(json.dumps(m,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('MANIFEST_PASS 1830 +25 assets')
def audit(ref,hs):
 lines=['# RCL0193FRE - audit lot 1830','', '- Source PDF SHA-256: `0c7fef28d0d0f0673ba321d6625a019c005823103caa98afb3258114e1fec713`.','- Périmètre: suspension + freins PDF 226-254.','- 25 pages utiles conservées; pages blanches/intercalaires exclues.','- Captures constructeur monochromes 150 dpi; aucune image générée ni redessinée.','','## Comptages','- 46 connaissances; 29 spécifications/valeurs; 19 opérations constructeur; 37 phases de procédure; 332 étapes; 44 exigences; 21 relations.','','## Points notables','- Purge des freins: schéma constructeur A/B/C/D conservé visuellement; aucune séquence non imprimée n’est inventée.','- Tambour arrière: diamètre neuf 177,75-177,85 mm; limite de service 179 mm; ovalisation maxi 0,012 mm.','- Amortisseur arrière: références gauche 68.15.22 et droite 68.15.23 et particularité réservoir côté gauche conservées.','- Liquide de frein, poussière, remplacement par essieu et pièces neuves conservés comme exigences.','','## Captures']
 for n in SELECTED:lines.append(f'- PDF p.{n}: `images/rave/RCL0193FRE_PDF_{n:03d}.png` - `{hs[n]}`')
 p=ref/'audits'/'rcl0193fre_1830_audit.md';p.write_text('\n'.join(lines)+'\n',encoding='utf-8');print('AUDIT_PASS')
def validate(ref,tmp):
 db=rebuild(ref,tmp/'db.sqlite')
 try:
  assert q1(db,'pragma integrity_check')=='ok' and q1(db,'pragma user_version')==20
  assert q1(db,'select count(*) from mems_rave_fact')==93 and q1(db,'select count(*) from mems_expert_fact_external')==105
  assert q1(db,"select count(*) from mems_knowledge_item where knowledge_key like 'KNOW-RCL0193FRE-1830-%'")==46
  assert q1(db,"select count(*) from mems_knowledge_scope where knowledge_key like 'KNOW-RCL0193FRE-1830-%'")==46
  assert q1(db,"select count(*) from mems_knowledge_scope where knowledge_key like 'KNOW-RCL0193FRE-1830-%' and scope_key!=?",(GENERAL_SCOPE,))==0
  assert q1(db,"select count(*) from mems_specification where spec_key like 'SPEC-RCL0193FRE-1830-%'")==29
  assert q1(db,"select count(*) from mems_specification_value where spec_key like 'SPEC-RCL0193FRE-1830-%'")==29
  assert q1(db,"select count(*) from mems_procedure where procedure_key like 'PROC-RCL0193FRE-1830-%'")==37
  assert q1(db,"select count(*) from mems_procedure_step where procedure_key like 'PROC-RCL0193FRE-1830-%'")==332
  assert q1(db,"select count(*) from mems_procedure_requirement where procedure_key like 'PROC-RCL0193FRE-1830-%'")==44
  assert q1(db,"select count(*) from mems_knowledge_relation where from_key like 'KNOW-RCL0193FRE-1830-%'")==21
  assert q1(db,"select count(*) from mems_specification_value where spec_key='SPEC-RCL0193FRE-1830-REAR-DRUM-NEW-DIAMETER' and abs(value_min-177.75)<1e-9 and abs(value_max-177.85)<1e-9")==1
  assert q1(db,"select count(*) from mems_specification_value where spec_key='SPEC-RCL0193FRE-1830-REAR-DRUM-SERVICE-LIMIT' and abs(value_max-179)<1e-9")==1
  assert q1(db,"select count(*) from mems_specification_value where spec_key='SPEC-RCL0193FRE-1830-REAR-DRUM-OVALITY' and abs(value_max-0.012)<1e-9")==1
  print('DB_PASS integrity=ok uv=20 hist=93/105 k=46 spec=29 values=29 proc=37 steps=332 req=44 rel=21')
 finally:db.close()
def validate_assets(ref,hs):
 assert len(hs)==25
 for n,h in hs.items():assert shaf(ref/'images'/'rave'/f'RCL0193FRE_PDF_{n:03d}.png')==h
 assert shaf(ref/'research_enrichment_1830.qz64')==EXPECTED_QZ_SHA
 print('COMMITTED_ASSET_PASS 25/25 + qz exact')
def allowed_delta(repo):
 out=subprocess.check_output(['git','status','--porcelain','--untracked-files=all'],cwd=repo,text=True).splitlines();bad=[]
 pats=[re.compile(r'database/reference/research_enrichment_1830\.qz64$'),re.compile(r'database/reference/manifest\.json$'),re.compile(r'database/reference/audits/rcl0193fre_1830_audit\.md$'),re.compile(r'database/reference/images/rave/RCL0193FRE_PDF_\d{3}\.png$')]
 for line in out:
  path=line[3:]
  if path.startswith('.rcl1830_transport/') or path in {'.github/workflows/temp-rcl0193fre-1830-install.yml','tools/install_rcl0193fre_1830.py'}:continue
  if not any(p.fullmatch(path) for p in pats):bad.append(line)
 assert not bad,bad
 assert len([l for l in out if 'RCL0193FRE_PDF_' in l])==25
 print('SCOPE_PASS final documentary delta exactly 28 paths')
def install(repo):
 repo=Path(repo);ref=repo/'database'/'reference';verify_prod(repo);t=transport(repo);install_qz(repo,ref);hs=render(t,ref);patch_manifest(ref,hs);audit(ref,hs)
 with tempfile.TemporaryDirectory() as td:validate(ref,Path(td))
 validate_assets(ref,hs);allowed_delta(repo)
def post(repo):
 repo=Path(repo);ref=repo/'database'/'reference';verify_prod(repo)
 with tempfile.TemporaryDirectory() as td:validate(ref,Path(td))
 m=json.loads((ref/'manifest.json').read_text(encoding='utf-8-sig'));assert 'research_enrichment_1830.qz64' in m.get('research_enrichment_batches',[])
 assert sum(1 for k in m.get('visual_assets',{}) if k.startswith('rave:RCL0193FRE:PDF:') and int(k.rsplit(':',1)[1]) in SELECTED)==25
 assert all((ref/'images'/'rave'/f'RCL0193FRE_PDF_{n:03d}.png').exists() for n in SELECTED)
 assert not (repo/'.rcl1830_transport').exists();assert not (repo/'.github/workflows/temp-rcl0193fre-1830-install.yml').exists();assert not (repo/'tools/install_rcl0193fre_1830.py').exists()
 assert not subprocess.check_output(['git','status','--porcelain'],cwd=repo,text=True).strip()
 print('POST_PASS remote 1830 committed, transport/workflow clean')
if __name__=='__main__':
 {'install':install,'post':post}[sys.argv[1]](sys.argv[2])
