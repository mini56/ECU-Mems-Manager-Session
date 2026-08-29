from __future__ import annotations
import base64,hashlib,json,lzma,re,shutil,sqlite3,subprocess,sys,zlib
from pathlib import Path
EXPECTED_MEMSX64='22dbe75ed14e0a61e694159d505ef72245116b48'
EXPECTED_G4_XZ_SHA='7dd7ae07169fe969e0877de871cc701fdd5f580fedd528527433602986610035'
EXPECTED_G4_TIFF_SHA='93aba01da598a2d9a610b8eee736baccbef0b0849ed5eb11dc5a64808cc2bf0a'
EXPECTED_QZ_SHA='743d27910f60760e35f5efb81232a67a8488fc919f998a86e8d0d521128da0af'
EXPECTED_SQL_SHA='9168045f39f7e53d91b4eed079acc3fb5f79476bde0ecfe2f37ce98b2d0e9a53'
SOURCE_KEY='SRC-RCL0193FRE-USER-PDF'; GENERAL_SCOPE='SCOPE-RCL0193FRE-MINI-VIN134455'
SELECTED=[n for n in range(50,104) if n not in (51,97)]
assert len(SELECTED)==52
TOP={50:'Moteur - table des matières réglages et réparations',52:'Jeu axial pignon primaire vilebrequin - contrôle et réglage',53:'Réglage des culbuteurs',54:'Plaque butée arbre à cames et poulie avant vilebrequin',55:'Poulie avant vilebrequin - suite dépose',56:'Poulie vilebrequin repose et joint avant vilebrequin',57:'Joint avant vilebrequin - dépose/repose',58:"Joints chapeau de palier et plaque avant",59:'Joint de culasse - dépose',60:'Joint de culasse - dépose faisceau admission',61:'Joint de culasse - dépose ventilation carburant',62:'Joint de culasse - dépose alternateur culasse',63:'Joint de culasse - repose et serrages',64:'Joint couvre-culbuteurs et rampe culbuteurs',65:'Rampe des culbuteurs - repose',66:'Soupape - dépose',67:'Soupape - dépose/repose outil spécial',68:'Moteur et boîte de vitesses - dépose',69:'Moteur et boîte - dépose ECM/faisceau',70:'Moteur et boîte - dépose ECT/refroidissement',71:'Moteur et boîte - dépose conduites carburant',72:'Moteur et boîte - dépose faisceau/échappement',73:'Moteur et boîte - dépose suspension',74:'Moteur et boîte - dépose moyeu outil spécial',75:'Moteur et boîte - fin dépose / début repose',76:'Moteur et boîte de vitesses - repose suite',77:'Silentbloc moteur gauche',78:'Silentbloc moteur droit et barre appui supérieure',79:'Barre appui moteur supérieure / inférieure',80:'Barre appui moteur inférieure et joint huile carter volant',81:"Joint d'huile du carter du volant",82:'Joint de carter de volant - dépose',83:'Joint carter volant - dépose suite',84:'Joint carter volant - dépose / contrôles',85:'Joint carter volant - repose et volant',86:'Filtre à huile',87:'Pompe à huile et joint moteur/boîte',88:'Joint moteur/boîte - dépose CKP/carter volant',89:'Joint moteur/boîte - dépose volant outils spéciaux',90:'Joint moteur/boîte - séparation',91:'Joint moteur/boîte - repose et serrages',92:'Manocontact et soupape sûreté pression huile',93:'Soupape sûreté / couvercle distribution / chaîne pignons',94:'Chaîne et pignons distribution - repose réglage alignement',95:'Tendeur chaîne de distribution',96:'Contrôle de dépollution - table des matières',98:'Systèmes antipollution - description et fonctionnement',99:'Gaz de carter et évaporation carburant - schéma composants',100:'Reniflard séparateur huile et boîte contrôle évaporation',101:'Boîte évaporation et soupape de purge',102:'Convertisseur catalytique - dépose/repose',103:'Bouclier thermique convertisseur catalytique'}
def sha(b):return hashlib.sha256(b).hexdigest()
def shaf(p):return sha(p.read_bytes())
def numsuf(p):
 m=re.search(r'_(\d+)\.qz64$',p.name);return int(m.group(1)) if m else 0
def qun(data):
 e=int.from_bytes(data[:4],'big')
 for w in (zlib.MAX_WBITS,-zlib.MAX_WBITS):
  try:
   o=zlib.decompress(data[4:],w)
   if len(o)==e:return o
  except:pass
 raise AssertionError('qz decode')
def dq(p):
 r=p.read_bytes()
 try:return qun(r)
 except:return qun(base64.b64decode(b''.join(r.split())))
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
 h=subprocess.check_output(['git','ls-remote','origin','refs/heads/MEMSX64'],cwd=repo,text=True).split()[0];assert h==EXPECTED_MEMSX64;print('PRODUCTION_PASS',h)
def transport(repo):
 p=repo/'.rcl1800_transport'/'rcl0193fre_1800_g4_150.tif.xz';assert shaf(p)==EXPECTED_G4_XZ_SHA
 b=lzma.decompress(p.read_bytes());assert sha(b)==EXPECTED_G4_TIFF_SHA
 t=Path('/tmp/rcl1800.tif');t.write_bytes(b);return t
def install_qz(repo,ref):
 p=repo/'.rcl1800_transport'/'research_enrichment_1800.qz64';assert shaf(p)==EXPECTED_QZ_SHA and sha(dq(p))==EXPECTED_SQL_SHA
 d=ref/'research_enrichment_1800.qz64';assert not d.exists();d.write_bytes(p.read_bytes());print('QZ_PASS')
def render(tiff,ref):
 from PIL import Image
 im=Image.open(tiff);assert im.n_frames==52 and im.size==(1240,1754) and im.mode=='1'
 out=ref/'images'/'rave';hs={}
 for i,n in enumerate(SELECTED):
  im.seek(i);f=im.copy();dst=out/f'RCL0193FRE_PDF_{n:03d}.png';assert not dst.exists();f.save(dst,'PNG',optimize=True);hs[n]=shaf(dst)
 im.close();print('RENDER_PASS 52 @150dpi');return hs
def kind(n):
 if n in (50,96):return 'toc_page'
 if n in (98,99):return 'technical_description_page'
 if n in (52,53):return 'adjustment_page'
 return 'procedure_page'
def patch_manifest(ref,hs):
 p=ref/'manifest.json';m=json.loads(p.read_text(encoding='utf-8-sig'));b=m.setdefault('research_enrichment_batches',[]);assert 'research_enrichment_1800.qz64' not in b;b.append('research_enrichment_1800.qz64')
 d=m.setdefault('diagrams',{});v=m.setdefault('visual_assets',{})
 for n in SELECTED:
  path=f'images/rave/RCL0193FRE_PDF_{n:03d}.png';d[f'RAVE RCL0193FRE PDF {n:03d} {TOP[n]}']=path;k=f'rave:RCL0193FRE:PDF:{n}';assert k not in v;v[k]={'path':path,'asset_kind':kind(n),'document':'RCL0193FRE','source_page':f'PDF p.{n}','pdf_index':n,'sha256':hs[n]}
 p.write_text(json.dumps(m,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('MANIFEST_PASS')
def audit(ref,hs):
 lines=['# RCL0193FRE - audit lot 1800','',f'- Source PDF SHA-256: `0c7fef28d0d0f0673ba321d6625a019c005823103caa98afb3258114e1fec713`.','- Périmètre: moteur PDF 50-95 et dépollution PDF 96-103.','- Pages blanches 51 et 97 exclues.','- 52 captures constructeur monochromes CCITT Group 4 150 dpi.','- Aucune famille MEMS 1.6/1.9 inventée; portée générale RCL0193FRE depuis VIN 134455.','','## Comptages','- 88 connaissances nouvelles: 52 pages + 32 opérations + 4 sous-thèmes dépollution.','- 71 spécifications et 71 valeurs.','- 60 phases de procédure, 699 étapes, 81 exigences, 36 relations, 22 alias nouveaux effectifs.','- Intégrité SQLite ok; user_version 20; historiques 93/105 préservés.','','## Points notables','- Jeu axial pignon primaire: 0,089 à 0,165 mm.','- Jeu culbuteurs: 0.30 mm.','- Culasse: 34 N.m puis serrage final 68 N.m dans l’ordre constructeur.','- EVAP: soupape de purge commandée par ECM et fermée jusqu’à température moteur > 70°C.','- Avertissement carburant plombé / convertisseur catalytique conservé.','','## Captures']
 for n in SELECTED:lines.append(f'- PDF p.{n}: `images/rave/RCL0193FRE_PDF_{n:03d}.png` - `{hs[n]}`')
 p=ref/'audits'/'rcl0193fre_1800_audit.md';p.write_text('\n'.join(lines)+'\n',encoding='utf-8');print('AUDIT_PASS')
def validate(ref,tmp):
 db=rebuild(ref,tmp/'db.sqlite')
 try:
  assert q1(db,'pragma integrity_check')=='ok' and q1(db,'pragma user_version')==20
  assert q1(db,'select count(*) from mems_rave_fact')==93 and q1(db,'select count(*) from mems_expert_fact_external')==105
  assert q1(db,"select count(*) from mems_knowledge_item where knowledge_key like 'KNOW-RCL0193FRE-1800-%'")==88
  assert q1(db,"select count(*) from mems_knowledge_scope where knowledge_key like 'KNOW-RCL0193FRE-1800-%'")==88
  assert q1(db,"select count(*) from mems_specification where spec_key like 'SPEC-RCL0193FRE-1800-%'")==71
  assert q1(db,"select count(*) from mems_specification_value where spec_key like 'SPEC-RCL0193FRE-1800-%'")==71
  assert q1(db,"select count(*) from mems_procedure where procedure_key like 'PROC-RCL0193FRE-1800-%'")==60
  assert q1(db,"select count(*) from mems_procedure_step where procedure_key like 'PROC-RCL0193FRE-1800-%'")==699
  assert q1(db,"select count(*) from mems_procedure_requirement where procedure_key like 'PROC-RCL0193FRE-1800-%'")==81
  assert q1(db,"select count(*) from mems_knowledge_relation where from_key like 'KNOW-RCL0193FRE-1800-%'")==36
  assert q1(db,"select count(*) from mems_knowledge_item k left join mems_knowledge_scope s using(knowledge_key) where k.knowledge_key like 'KNOW-RCL0193FRE-1800-%' and s.knowledge_key is null")==0
  assert q1(db,"select count(*) from mems_knowledge_scope where knowledge_key like 'KNOW-RCL0193FRE-1800-%' and scope_key!=?",(GENERAL_SCOPE,))==0
  assert q1(db,"select count(distinct image_ref) from mems_knowledge_item where knowledge_key like 'KNOW-RCL0193FRE-1800-P%'")==52
  assert q1(db,"select count(*) from mems_specification_value where spec_key='SPEC-RCL0193FRE-1800-P052-PRIMARY-ENDPLAY' and abs(value_min-0.089)<1e-9 and abs(value_max-0.165)<1e-9")==1
  assert q1(db,"select count(*) from mems_knowledge_item where knowledge_key='KNOW-RCL0193FRE-1800-P098' and source_text like '%70°C%'")==1
  print('DB_PASS uv20 hist93/105 k88 spec71 proc60 steps699 req81 rel36')
 finally:db.close()
def assetcheck(ref):
 m=json.loads((ref/'manifest.json').read_text(encoding='utf-8-sig'));assert m['research_enrichment_batches'].count('research_enrichment_1800.qz64')==1
 for n in SELECTED:
  a=m['visual_assets'][f'rave:RCL0193FRE:PDF:{n}'];p=ref/a['path'];assert p.is_file() and shaf(p)==a['sha256']
 assert shaf(ref/'research_enrichment_1800.qz64')==EXPECTED_QZ_SHA;print('ASSET_PASS')
def guard(repo):
 allowed={'database/reference/research_enrichment_1800.qz64','database/reference/manifest.json','database/reference/audits/rcl0193fre_1800_audit.md'}|{f'database/reference/images/rave/RCL0193FRE_PDF_{n:03d}.png' for n in SELECTED}
 bad=[]
 for l in subprocess.check_output(['git','status','--porcelain','--untracked-files=all'],cwd=repo,text=True).splitlines():
  p=l[3:]
  if p.startswith('.rcl1800_transport/') or p in {'.github/workflows/temp-rcl0193fre-1800-install.yml','tools/install_rcl0193fre_1800.py'}:continue
  if p not in allowed:bad.append(l)
 assert not bad,bad
 actual={l[3:] for l in subprocess.check_output(['git','status','--porcelain','--untracked-files=all'],cwd=repo,text=True).splitlines() if l[3:] in allowed};assert actual==allowed,(len(actual),len(allowed));print('SCOPE_PASS 55')
def install(repo):
 verify_prod(repo);ref=repo/'database'/'reference';t=transport(repo);install_qz(repo,ref);hs=render(t,ref);patch_manifest(ref,hs);audit(ref,hs);tmp=Path('/tmp/rcl1800val');shutil.rmtree(tmp,ignore_errors=True);tmp.mkdir();validate(ref,tmp);assetcheck(ref);guard(repo);shutil.rmtree(tmp)
def post(repo):
 verify_prod(repo);ref=repo/'database'/'reference';assetcheck(ref);tmp=Path('/tmp/rcl1800post');shutil.rmtree(tmp,ignore_errors=True);tmp.mkdir();validate(ref,tmp);shutil.rmtree(tmp)
 for p in ['.github/workflows/temp-rcl0193fre-1800-install.yml','tools/install_rcl0193fre_1800.py','.rcl1800_transport']:assert not (repo/p).exists(),p
 assert not subprocess.check_output(['git','status','--porcelain'],cwd=repo,text=True).strip();print('POST_PASS')
if __name__=='__main__':
 repo=Path(sys.argv[2] if len(sys.argv)>2 else '.').resolve();install(repo) if sys.argv[1]=='install' else post(repo)
