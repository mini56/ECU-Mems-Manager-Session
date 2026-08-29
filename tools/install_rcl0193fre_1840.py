from __future__ import annotations
import base64,hashlib,json,lzma,re,sqlite3,subprocess,sys,tempfile,zlib
from pathlib import Path
EXPECTED_MEMSX64='22dbe75ed14e0a61e694159d505ef72245116b48'
EXPECTED_G4_XZ_SHA='484c776b67f12b3e922df8ad8cdd4c95a2ddbe74500242b7fcf665e80ecea46b'
EXPECTED_G4_TIFF_SHA='b0d1d952b2521d5efa84f96b7388a1137cb0c820674da873a375f7243f5c9bb8'
EXPECTED_QZ_SHA='7a042d914c76657f0932e2b2f0cf70ba5533ca0e2e6b6dd1433fb167120ffd34'
EXPECTED_SQL_SHA='a6730280d647b68dbdfe46a6b2bc286ea980d600bdc02b22b97f962f1113685a'
GENERAL_SCOPE='SCOPE-RCL0193FRE-MINI-VIN134455'
SELECTED=[256,258,259,260,262,263,264,265,266,267,268,269,270,271,272,274,275,276,277,278]
TOP={256:'SRS - table des matières',258:'SRS - emplacements des composants',259:'SRS - description et fonctionnement du module DCU',260:'SRS - airbag, prétensionneurs, contrôle et diagnostic',262:'SRS - sécurisation et précautions',263:'SRS - manutention et remisage',264:'SRS - câblage et connecteurs',265:"SRS - étiquettes d'avertissement",266:'SRS - étiquettes composants',267:"Déploiement manuel - outil et prétensionneur dans véhicule",268:'Déploiement prétensionneur dans véhicule - suite',269:'Déploiement prétensionneur déposé - début',270:'Déploiement prétensionneur déposé - suite',271:"Déploiement manuel du module d'airbag - début",272:"Déploiement manuel du module d'airbag - suite",274:"Module d'airbag conducteur - dépose/repose",275:'Prétensionneur de ceinture avant - dépose',276:'Prétensionneur - repose / module SRS DCU',277:'Faisceau SRS - précautions',278:'Faisceau SRS - dépose/repose'}
def sha(b):return hashlib.sha256(b).hexdigest()
def shaf(p):return sha(Path(p).read_bytes())
def numsuf(p):
 m=re.search(r'_(\d+)\.qz64$',p.name);return int(m.group(1)) if m else 0
def qun(data):
 e=int.from_bytes(data[:4],'big')
 for w in (zlib.MAX_WBITS,-zlib.MAX_WBITS):
  try:
   o=zlib.decompress(data[4:],w)
   if len(o)==e:return o
  except Exception:pass
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
 if dst.exists():dst.unlink()
 db=sqlite3.connect(dst);seeds=sorted(ref.glob('mems_reference_seed_*.qz64'),key=lambda p:(numsuf(p),p.name));assert seeds
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
 p=repo/'.rcl1840_transport'/'rcl0193fre_1840_g4_150.tif.xz';assert shaf(p)==EXPECTED_G4_XZ_SHA
 b=lzma.decompress(p.read_bytes());assert sha(b)==EXPECTED_G4_TIFF_SHA
 t=Path('/tmp/rcl1840.tif');t.write_bytes(b);print('TRANSPORT_PASS exact hashes');return t
def install_qz(repo,ref):
 p=repo/'.rcl1840_transport'/'research_enrichment_1840.qz64';assert shaf(p)==EXPECTED_QZ_SHA and sha(dq(p))==EXPECTED_SQL_SHA
 d=ref/'research_enrichment_1840.qz64';assert not d.exists();d.write_bytes(p.read_bytes());print('QZ_PASS')
def render(tiff,ref):
 from PIL import Image
 im=Image.open(tiff);assert im.n_frames==20 and im.size==(1240,1755) and im.mode=='1'
 out=ref/'images'/'rave';out.mkdir(parents=True,exist_ok=True);hs={}
 for i,n in enumerate(SELECTED):
  im.seek(i);dst=out/f'RCL0193FRE_PDF_{n:03d}.png';assert not dst.exists();im.copy().save(dst,'PNG',optimize=True);hs[n]=shaf(dst)
 im.close();print('RENDER_PASS 20 factory pages @150dpi');return hs
def kind(n):
 if n==256:return 'toc_page'
 if n in (258,259,260):return 'technical_description_page'
 if n in (262,263,264,265,266):return 'warning_page'
 return 'procedure_page'
def patch_manifest(ref,hs):
 p=ref/'manifest.json';m=json.loads(p.read_text(encoding='utf-8-sig'));b=m.setdefault('research_enrichment_batches',[]);assert 'research_enrichment_1840.qz64' not in b;b.append('research_enrichment_1840.qz64')
 d=m.setdefault('diagrams',{});v=m.setdefault('visual_assets',{})
 for n in SELECTED:
  path=f'images/rave/RCL0193FRE_PDF_{n:03d}.png';d[f'RAVE RCL0193FRE PDF {n:03d} {TOP[n]}']=path;k=f'rave:RCL0193FRE:PDF:{n}';assert k not in v;v[k]={'path':path,'asset_kind':kind(n),'document':'RCL0193FRE','source_page':f'PDF p.{n}','pdf_index':n,'sha256':hs[n]}
 p.write_text(json.dumps(m,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('MANIFEST_PASS 1840 +20 assets')
def audit(ref,hs):
 lines=['# RCL0193FRE - audit lot 1840','', '- Source PDF SHA-256: `0c7fef28d0d0f0673ba321d6625a019c005823103caa98afb3258114e1fec713`.','- Périmètre: SRS / airbag / prétensionneurs PDF 256-278.','- 20 pages utiles conservées; intercalaires/pages blanches exclus.','- Captures constructeur monochromes 150 dpi; aucune image générée ni redessinée.','','## Comptages','- 34 connaissances; 19 spécifications/valeurs; 4 opérations de réparation; 8 phases structurées; 74 étapes; 9 exigences; 15 relations.','- 4 procédures de déploiement manuel conservées textuellement avec ordre constructeur et pages exactes.','','## Points importants','- DCU non réparable; faisceau SRS jaune non réparable/épissable.','- Appareil digital uniquement; le manuel avertit qu’un appareil analogique peut provoquer un déploiement accidentel.','- Airbag: environ 30 ms pour le gonflage et environ 0,1 s jusqu’au dégonflement complet.','- Batterie déconnectée et attente 10 min avant intervention SRS.','- Déploiement manuel: distance personnel >= 15 m; attente 5 min prétensionneur et 30 min airbag.','- Prétensionneur: remplacement tous les 15 ans.','- Divergence constructeur conservée: témoin SRS ~5 s en Description/Fonctionnement, 3 s dans les contrôles après réparation; relation conflicts_with, aucun arbitrage.','','## Captures']
 for n in SELECTED:lines.append(f'- PDF p.{n}: `images/rave/RCL0193FRE_PDF_{n:03d}.png` - `{hs[n]}`')
 p=ref/'audits'/'rcl0193fre_1840_audit.md';p.write_text('\n'.join(lines)+'\n',encoding='utf-8');print('AUDIT_PASS')
def validate(ref,tmp):
 db=rebuild(ref,tmp/'db.sqlite')
 try:
  assert q1(db,'pragma integrity_check')=='ok' and q1(db,'pragma user_version')==20
  assert q1(db,'select count(*) from mems_rave_fact')==93 and q1(db,'select count(*) from mems_expert_fact_external')==105
  assert q1(db,"select count(*) from mems_knowledge_item where knowledge_key like 'KNOW-RCL0193FRE-1840-%'")==34
  assert q1(db,"select count(*) from mems_knowledge_scope where knowledge_key like 'KNOW-RCL0193FRE-1840-%'")==34
  assert q1(db,"select count(*) from mems_knowledge_scope where knowledge_key like 'KNOW-RCL0193FRE-1840-%' and scope_key!=?",(GENERAL_SCOPE,))==0
  assert q1(db,"select count(*) from mems_specification where spec_key like 'SPEC-RCL0193FRE-1840-%'")==19
  assert q1(db,"select count(*) from mems_specification_value where spec_key like 'SPEC-RCL0193FRE-1840-%'")==19
  assert q1(db,"select count(*) from mems_procedure where procedure_key like 'PROC-RCL0193FRE-1840-%'")==8
  assert q1(db,"select count(*) from mems_procedure_step where procedure_key like 'PROC-RCL0193FRE-1840-%'")==74
  assert q1(db,"select count(*) from mems_procedure_requirement where procedure_key like 'PROC-RCL0193FRE-1840-%'")==9
  assert q1(db,"select count(*) from mems_knowledge_relation where from_key like 'KNOW-RCL0193FRE-1840-%'")==15
  assert q1(db,"select count(*) from mems_knowledge_relation where from_key='KNOW-RCL0193FRE-1840-P260' and relation_type='conflicts_with' and to_key='KNOW-RCL0193FRE-1840-P274'")==1
  print('DB_PASS integrity=ok uv=20 hist=93/105 k=34 spec=19 proc=8 steps=74 req=9 rel=15')
 finally:db.close()
def validate_assets(ref,hs):
 assert len(hs)==20
 for n,h in hs.items():assert shaf(ref/'images'/'rave'/f'RCL0193FRE_PDF_{n:03d}.png')==h
 assert shaf(ref/'research_enrichment_1840.qz64')==EXPECTED_QZ_SHA
 print('ASSET_PASS 20/20 + qz exact')
def allowed_delta(repo):
 out=subprocess.check_output(['git','status','--porcelain','--untracked-files=all'],cwd=repo,text=True).splitlines();bad=[]
 pats=[re.compile(r'database/reference/research_enrichment_1840\.qz64$'),re.compile(r'database/reference/manifest\.json$'),re.compile(r'database/reference/audits/rcl0193fre_1840_audit\.md$'),re.compile(r'database/reference/images/rave/RCL0193FRE_PDF_\d{3}\.png$')]
 for line in out:
  path=line[3:]
  if path.startswith('.rcl1840_transport/') or path in {'.github/workflows/temp-rcl0193fre-1840-install.yml','tools/install_rcl0193fre_1840.py'}:continue
  if not any(p.fullmatch(path) for p in pats):bad.append(line)
 assert not bad,bad
 assert len([l for l in out if 'RCL0193FRE_PDF_' in l])==20
 print('SCOPE_PASS final documentary delta exactly 23 paths')
def install(repo):
 repo=Path(repo);ref=repo/'database'/'reference';verify_prod(repo);t=transport(repo);install_qz(repo,ref);hs=render(t,ref);patch_manifest(ref,hs);audit(ref,hs)
 with tempfile.TemporaryDirectory() as td:validate(ref,Path(td))
 validate_assets(ref,hs);allowed_delta(repo)
def post(repo):
 repo=Path(repo);ref=repo/'database'/'reference';verify_prod(repo)
 with tempfile.TemporaryDirectory() as td:validate(ref,Path(td))
 m=json.loads((ref/'manifest.json').read_text(encoding='utf-8-sig'));assert 'research_enrichment_1840.qz64' in m.get('research_enrichment_batches',[])
 assert sum(1 for k in m.get('visual_assets',{}) if k.startswith('rave:RCL0193FRE:PDF:') and int(k.rsplit(':',1)[1]) in SELECTED)==20
 assert all((ref/'images'/'rave'/f'RCL0193FRE_PDF_{n:03d}.png').exists() for n in SELECTED)
 assert not (repo/'.rcl1840_transport').exists();assert not (repo/'.github/workflows/temp-rcl0193fre-1840-install.yml').exists();assert not (repo/'tools/install_rcl0193fre_1840.py').exists()
 assert not subprocess.check_output(['git','status','--porcelain'],cwd=repo,text=True).strip()
 print('POST_PASS remote 1840 committed, transport/workflow clean')
if __name__=='__main__':{'install':install,'post':post}[sys.argv[1]](sys.argv[2])
