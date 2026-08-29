from __future__ import annotations
import base64,hashlib,json,lzma,re,sqlite3,subprocess,sys,zlib,tempfile
from pathlib import Path
EXPECTED_MEMSX64='22dbe75ed14e0a61e694159d505ef72245116b48'
EXPECTED_G4_XZ_SHA='dd4c5171b38f39b68f4b18e5002ae23c9234bc7f5ff9e27f963ae3388174c329'
EXPECTED_G4_TIFF_SHA='8edf3246dbf0d0dd7ce8a65c94c66768790241fc0e972d9e4ac070385d5166c1'
EXPECTED_QZ_SHA='eb9e28913d2815b8a270ec4b3f4dd5fdfd69c23acf3a0cdbd3b07a1ab9365226'
EXPECTED_SQL_SHA='acc515ffc12b680339f0319aa1c10d335108590120008cc2b777746d959a1ecb'
SOURCE_KEY='SRC-RCL0193FRE-USER-PDF'; GENERAL_SCOPE='SCOPE-RCL0193FRE-MINI-VIN134455'
SELECTED=[146,148,149,150,152,154,155,156,157,158,159,160,162,163,164,165,166,167,168,169,170,171,172,173,174]
TOP={146:'Refroidissement - table des matières',148:'Composants circuit refroidissement - implantation',149:'Fonctionnement refroidissement - schéma écoulement froid/chaud',150:'Fonctionnement circuit refroidissement thermostat chauffage ventilateur',152:'Vidange et remplissage circuit refroidissement',154:"Vase d'expansion",155:'Poulie pompe à eau et ventilateur radiateur',156:'Ventilateur radiateur et relais ventilateur',157:'Radiateur - dépose/repose',158:'Thermostat - dépose',159:'Thermostat contrôle/repose et pompe à eau',160:'Collecteur et échappement - table des matières',162:"Composants système d'échappement",163:"Composants collecteur d'échappement",164:"Composants collecteur d'admission",165:"Collecteur d'admission - description MAP IACV IAT rampe injecteurs",166:'Tuyaux échappement avant et arrière',167:'Joint tuyau avant / collecteur',168:'Bouclier thermique tuyau intermédiaire',169:"Collecteur d'admission - dépose",170:"Collecteurs admission/échappement - repose et HO2S",171:'Joint collecteurs admission et échappement - dépose',172:'Joint collecteurs - dépose faisceau carburant',173:'Joint collecteurs - fin dépose',174:'Joint collecteurs - repose et serrages'}
def sha(b): return hashlib.sha256(b).hexdigest()
def shaf(p): return sha(p.read_bytes())
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
 r=p.read_bytes()
 try:return qun(r)
 except Exception:return qun(base64.b64decode(b''.join(r.split())))
def execsql(db,b):
 t=b.decode('utf-8').replace('\ufeff','')
 t=re.sub(r'(?m)^\s*BEGIN(?:\s+TRANSACTION|\s+IMMEDIATE)?\s*;\s*$','',t,flags=re.I)
 t=re.sub(r'(?m)^\s*COMMIT\s*;\s*$','',t,flags=re.I)
 t=re.sub(r'(?m)^\s*PRAGMA\s+user_version\s*=\s*\d+\s*;\s*$','',t,flags=re.I)
 db.executescript(t)
def rebuild(ref,dst):
 if dst.exists():dst.unlink()
 db=sqlite3.connect(dst)
 seeds=sorted(ref.glob('mems_reference_seed_*.qz64'),key=lambda p:(numsuf(p),p.name)); assert seeds
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
 p=repo/'.rcl1810_transport'/'rcl0193fre_1810_g4_150.tif.xz';assert shaf(p)==EXPECTED_G4_XZ_SHA
 b=lzma.decompress(p.read_bytes());assert sha(b)==EXPECTED_G4_TIFF_SHA
 t=Path('/tmp/rcl1810.tif');t.write_bytes(b);print('TRANSPORT_PASS G4 xz/tiff hashes exact');return t
def install_qz(repo,ref):
 p=repo/'.rcl1810_transport'/'research_enrichment_1810.qz64';assert shaf(p)==EXPECTED_QZ_SHA and sha(dq(p))==EXPECTED_SQL_SHA
 d=ref/'research_enrichment_1810.qz64';assert not d.exists();d.write_bytes(p.read_bytes());print('QZ_PASS',len(p.read_bytes()),len(dq(p)))
def render(tiff,ref):
 from PIL import Image
 im=Image.open(tiff);assert im.n_frames==25 and im.size==(1240,1754) and im.mode=='1'
 out=ref/'images'/'rave';out.mkdir(parents=True,exist_ok=True);hs={}
 for i,n in enumerate(SELECTED):
  im.seek(i);f=im.copy();dst=out/f'RCL0193FRE_PDF_{n:03d}.png';assert not dst.exists();f.save(dst,'PNG',optimize=True);hs[n]=shaf(dst)
 im.close();print('RENDER_PASS 25 factory pages @150dpi');return hs
def kind(n):
 if n in (146,160):return 'toc_page'
 if n in (148,149,150,162,163,164,165):return 'technical_description_page'
 if n==152:return 'adjustment_page'
 return 'procedure_page'
def patch_manifest(ref,hs):
 p=ref/'manifest.json';m=json.loads(p.read_text(encoding='utf-8-sig'));b=m.setdefault('research_enrichment_batches',[]);assert 'research_enrichment_1810.qz64' not in b;b.append('research_enrichment_1810.qz64')
 d=m.setdefault('diagrams',{});v=m.setdefault('visual_assets',{})
 for n in SELECTED:
  path=f'images/rave/RCL0193FRE_PDF_{n:03d}.png';d[f'RAVE RCL0193FRE PDF {n:03d} {TOP[n]}']=path;k=f'rave:RCL0193FRE:PDF:{n}';assert k not in v;v[k]={'path':path,'asset_kind':kind(n),'document':'RCL0193FRE','source_page':f'PDF p.{n}','pdf_index':n,'sha256':hs[n]}
 p.write_text(json.dumps(m,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('MANIFEST_PASS 1810 +25 assets')
def audit(ref,hs):
 lines=['# RCL0193FRE - audit lot 1810','', '- Source PDF SHA-256: `0c7fef28d0d0f0673ba321d6625a019c005823103caa98afb3258114e1fec713`.', '- Périmètre: refroidissement + collecteurs/admission/échappement PDF 146-175.', '- 25 pages réellement utiles conservées; intercalaires/pages vides exclus.', '- 25 captures constructeur monochromes CCITT Group 4 150 dpi.', '- Aucune famille MEMS supplémentaire inventée; portée générale RCL0193FRE depuis VIN 134455.','','## Comptages','- 44 connaissances nouvelles.','- 23 spécifications et 23 valeurs.','- 30 phases de procédure, 271 étapes, 19 exigences, 19 relations.','- 10 groupes d’alias français tentés; INSERT OR IGNORE conserve les alias existants sans doublon.','- Intégrité SQLite ok; user_version 20; historiques 93/105 préservés.','','## Points notables','- Thermostat: ouverture 88 °C, conservée comme spécification typée.','- Corps de papillon / IACV / IAT / MAP / rampe / injecteurs conservés dans le bloc admission tel qu’imprimé.','- La clé 22 mm de la sonde HO2S reste un élément de procédure/outillage et n’est pas transformée en spécification véhicule.','','## Captures']
 for n in SELECTED:lines.append(f'- PDF p.{n}: `images/rave/RCL0193FRE_PDF_{n:03d}.png` - `{hs[n]}`')
 p=ref/'audits'/'rcl0193fre_1810_audit.md';p.write_text('\n'.join(lines)+'\n',encoding='utf-8');print('AUDIT_PASS')
def validate(ref,tmp):
 db=rebuild(ref,tmp/'db.sqlite')
 try:
  assert q1(db,'pragma integrity_check')=='ok' and q1(db,'pragma user_version')==20
  assert q1(db,'select count(*) from mems_rave_fact')==93 and q1(db,'select count(*) from mems_expert_fact_external')==105
  assert q1(db,"select count(*) from mems_knowledge_item where knowledge_key like 'KNOW-RCL0193FRE-1810-%'")==44
  assert q1(db,"select count(*) from mems_knowledge_scope where knowledge_key like 'KNOW-RCL0193FRE-1810-%'")==44
  assert q1(db,"select count(*) from mems_specification where spec_key like 'SPEC-RCL0193FRE-1810-%'")==23
  assert q1(db,"select count(*) from mems_specification_value where spec_key like 'SPEC-RCL0193FRE-1810-%'")==23
  assert q1(db,"select count(*) from mems_procedure where procedure_key like 'PROC-RCL0193FRE-1810-%'")==30
  assert q1(db,"select count(*) from mems_procedure_step where procedure_key like 'PROC-RCL0193FRE-1810-%'")==271
  assert q1(db,"select count(*) from mems_procedure_requirement where procedure_key like 'PROC-RCL0193FRE-1810-%'")==19
  assert q1(db,"select count(*) from mems_knowledge_relation where from_key like 'KNOW-RCL0193FRE-1810-%'")==19
  assert q1(db,"select count(*) from mems_knowledge_item k left join mems_knowledge_scope s using(knowledge_key) where k.knowledge_key like 'KNOW-RCL0193FRE-1810-%' and s.knowledge_key is null")==0
  assert q1(db,"select count(*) from mems_knowledge_scope where knowledge_key like 'KNOW-RCL0193FRE-1810-%' and scope_key!=?",(GENERAL_SCOPE,))==0
  assert q1(db,"select count(distinct image_ref) from mems_knowledge_item where knowledge_key like 'KNOW-RCL0193FRE-1810-P%'")==25
  assert q1(db,"select count(*) from mems_specification_value where spec_key='SPEC-RCL0193FRE-1810-THERMOSTAT-OPEN' and abs(value_numeric-88.0)<1e-9 and unit='°C'")==1
  assert q1(db,"select count(*) from mems_specification_value where spec_key like 'SPEC-RCL0193FRE-1810-%' and lower(instruction_text) like '%22 mm%' and (lower(instruction_text) like '%clé%' or lower(instruction_text) like '%clef%')")==0
  print('DB_PASS integrity=ok uv=20 hist=93/105 k=44 spec=23 values=23 proc=30 steps=271 req=19 rel=19')
 finally: db.close()
def validate_assets(ref,hs):
 assert len(hs)==25
 for n,h in hs.items():assert shaf(ref/'images'/'rave'/f'RCL0193FRE_PDF_{n:03d}.png')==h
 assert shaf(ref/'research_enrichment_1810.qz64')==EXPECTED_QZ_SHA
 print('COMMITTED_ASSET_PASS 25/25 + qz exact')
def allowed_delta(repo):
 out=subprocess.check_output(['git','status','--porcelain','--untracked-files=all'],cwd=repo,text=True).splitlines();bad=[]
 pats=[re.compile(r'database/reference/research_enrichment_1810\.qz64$'),re.compile(r'database/reference/manifest\.json$'),re.compile(r'database/reference/audits/rcl0193fre_1810_audit\.md$'),re.compile(r'database/reference/images/rave/RCL0193FRE_PDF_\d{3}\.png$')]
 for line in out:
  path=line[3:]
  if path.startswith('.rcl1810_transport/') or path in {'.github/workflows/temp-rcl0193fre-1810-install.yml','tools/install_rcl0193fre_1810.py'}:continue
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
 m=json.loads((ref/'manifest.json').read_text(encoding='utf-8-sig'));assert 'research_enrichment_1810.qz64' in m.get('research_enrichment_batches',[])
 assert sum(1 for k in m.get('visual_assets',{}) if k.startswith('rave:RCL0193FRE:PDF:') and int(k.rsplit(':',1)[1]) in SELECTED)==25
 assert all((ref/'images'/'rave'/f'RCL0193FRE_PDF_{n:03d}.png').exists() for n in SELECTED)
 assert not (repo/'.rcl1810_transport').exists();assert not (repo/'.github/workflows/temp-rcl0193fre-1810-install.yml').exists();assert not (repo/'tools/install_rcl0193fre_1810.py').exists()
 assert not subprocess.check_output(['git','status','--porcelain'],cwd=repo,text=True).strip()
 print('POST_PASS remote 1810 committed, transport/workflow clean')
if __name__=='__main__':
 mode=sys.argv[1];repo=sys.argv[2]
 {'install':install,'post':post}[mode](repo)
