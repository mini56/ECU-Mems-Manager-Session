from __future__ import annotations
import argparse, base64, hashlib, json, re, shutil, sqlite3, zlib
from pathlib import Path

EXPECTED_SOURCE_SHA='14fcb88e3f450cab47129b2acc8df948e1d4002418deef63a95ce45157be0594'
EXPECTED_SOURCE_PAGES=53
EXPECTED_FACTORY_PAGES=49
EXPECTED_ASSETS=52
EXPECTED_HIST_RAVE=93
EXPECTED_HIST_EXPERT=105
EXPECTED_USER_VERSION=20
GENERAL_SCOPE='SCOPE-AKM6348-FR-MINI-1976-03-PLUS'
DOCUMENT='AKM6348-FR-CH86'
SOURCE_KEY='SRC-AKM6348-FR-WEB-20260829'


def sha256_bytes(data:bytes)->str:return hashlib.sha256(data).hexdigest()
def sha256_file(path:Path)->str:return sha256_bytes(path.read_bytes())
def numeric_suffix(path:Path)->int:
    m=re.search(r'_(\d+)\.qz64$',path.name)
    return int(m.group(1)) if m else 0

def qt_uncompress(data:bytes)->bytes:
    assert len(data)>=4
    expected=int.from_bytes(data[:4],'big')
    for wbits in (zlib.MAX_WBITS,-zlib.MAX_WBITS):
        try:
            out=zlib.decompress(data[4:],wbits)
            if len(out)==expected:return out
        except Exception:pass
    raise AssertionError('cannot qUncompress payload')

def decode_qz64(path:Path)->bytes:
    raw=path.read_bytes()
    # Historical enrichment files use both raw Qt qCompress bytes and
    # Base64-wrapped qCompress. Preserve both historical encodings.
    try:
        return qt_uncompress(raw)
    except Exception:
        compact=b''.join(raw.split())
        return qt_uncompress(base64.b64decode(compact))

def encode_qz64(sql_raw:bytes)->bytes:
    packed=len(sql_raw).to_bytes(4,'big')+zlib.compress(sql_raw,9)
    return base64.b64encode(packed)+b'\n'

def execute_project_sql(db:sqlite3.Connection,sql_raw:bytes)->None:
    text=sql_raw.decode('utf-8').replace('\ufeff','')
    text=re.sub(r'(?m)^\s*BEGIN(?:\s+TRANSACTION|\s+IMMEDIATE)?\s*;\s*$', '', text, flags=re.I)
    text=re.sub(r'(?m)^\s*COMMIT\s*;\s*$', '', text, flags=re.I)
    text=re.sub(r'(?m)^\s*PRAGMA\s+user_version\s*=\s*\d+\s*;\s*$', '', text, flags=re.I)
    db.executescript(text)

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
            if p.is_file() and p.name!='research_enrichment_1600.qz64' and numeric_suffix(p)<1780:
                ordered.append(p);seen.add(p.resolve())
        for p in sorted(reference.rglob('research_enrichment*.qz64'),key=lambda p:(numeric_suffix(p),str(p))):
            if p.name=='research_enrichment_1600.qz64' or numeric_suffix(p)>=1780:continue
            if p.resolve() not in seen:ordered.append(p);seen.add(p.resolve())
        for p in ordered:execute_project_sql(db,decode_qz64(p))
        db.execute(f'PRAGMA user_version={EXPECTED_USER_VERSION}');db.commit()
    finally:db.close()

def sql_quote(v):
    if v is None:return 'NULL'
    if isinstance(v,(int,float)):return str(v)
    return "'"+str(v).replace("'","''")+"'"

def asset_rows():
    rows=[]
    for pdf in range(1,45):rows.append((pdf,f'86-{pdf}',None))
    for seg,pdf in enumerate(range(45,49),1):rows.append((pdf,'86-45',seg))
    for pdf,factory in [(50,'86-46'),(51,'86-47'),(52,'86-48'),(53,'86-49')]:rows.append((pdf,factory,None))
    assert len(rows)==EXPECTED_ASSETS
    return rows

PAGE_KIND={**{i:'wiring' for i in range(2,14)},27:'diagram',29:'diagram',30:'wiring',31:'wiring',32:'wiring',33:'wiring',34:'diagram',35:'table',36:'service_page',42:'table',43:'table',44:'table',45:'wiring',46:'wiring',47:'wiring',48:'wiring'}

def render_assets(pdf_path:Path,reference:Path):
    import fitz
    assert sha256_file(pdf_path)==EXPECTED_SOURCE_SHA
    doc=fitz.open(str(pdf_path));assert doc.page_count==EXPECTED_SOURCE_PAGES
    out=reference/'images'/'rave';out.mkdir(parents=True,exist_ok=True)
    assets=[]
    try:
        for pdf_no,factory,seg in asset_rows():
            page=doc[pdf_no-1]
            kind=PAGE_KIND.get(int(factory.split('-')[1]),'service_page')
            suffix=f'_SEG{seg}' if seg else ''
            factory_num=int(factory.split('-')[1])
            filename=f'AKM6348_FR_CH86_86_{factory_num:02d}{suffix}.png'
            target=out/filename
            pix=page.get_pixmap(matrix=fitz.Matrix(1.5,1.5),alpha=False);pix.save(target)
            assets.append(dict(pdf=pdf_no,factory=factory,seg=seg,kind=kind,filename=filename,path='images/rave/'+filename,sha256=sha256_file(target),width=pix.width,height=pix.height))
    finally:doc.close()
    assert len({a['factory'] for a in assets})==EXPECTED_FACTORY_PAGES
    assert sum(1 for a in assets if a['factory']=='86-45')==4
    assert not any(a['pdf']==49 for a in assets)
    return assets

SCOPES=[
('SCOPE-AKM6348-MINI850-1976-ON','Mini 850',1976,None,None,'Mini 850 à partir de 1976'),('SCOPE-AKM6348-MINI1000-1976-ON','Mini 1000',1976,None,None,'Mini 1000, trois instruments, à partir de 1976'),('SCOPE-AKM6348-MINI-SPECIAL-1976-1977','Mini Special',1976,1977,None,'Mini Special 1976-1977'),('SCOPE-AKM6348-CLUBMAN-1976-ON','Clubman',1976,None,None,'Clubman à partir de 1976'),('SCOPE-AKM6348-ESTATE-1976-ON','Estate',1976,None,None,'Estate à partir de 1976'),('SCOPE-AKM6348-1275GT-1976-ON','1275GT',1976,None,None,'1275GT à partir de 1976'),('SCOPE-AKM6348-MINI-SPECIAL-1977-1978','Mini Special',1977,1978,None,'Mini Special 1977-1978'),('SCOPE-AKM6348-MINI1000-CANADA-1977-ON','Mini 1000',1977,None,'Canada','Mini 1000 Canada à partir de 1977'),('SCOPE-AKM6348-MINI-SPECIAL-1979','Mini Special',1979,1979,None,'Mini Special 1979'),('SCOPE-AKM6348-SINGLE-INSTRUMENT-1984-ON',None,1984,None,None,'Modèles à un instrument à partir de 1984'),('SCOPE-AKM6348-MULTI-INSTRUMENT-1984-ON',None,1984,None,None,'Modèles à plusieurs instruments à partir de 1984'),('SCOPE-AKM6348-1986-ON',None,1986,None,None,'Faisceaux et points de masse à partir de 1986'),('SCOPE-AKM6348-1988-ON',None,1988,None,None,'Schémas électriques à partir de 1988'),('SCOPE-AKM6348-1989-ON',None,1989,None,None,'Radio à partir de 1989'),('SCOPE-AKM6348-COOPER','Cooper',None,None,None,'Cooper; année non précisée sur les pages exploitées')]

K=[
('PRECAUTIONS','service','warning','Précautions de service électrique','electrical_system',[1],"La page 86-1 regroupe les précautions de service électrique concernant notamment batterie, alternateur et composants à semi-conducteurs.",[GENERAL_SCOPE]),('ALT-REMOVE','service','procedure','Alternateur — dépose et repose','alternator',[1],"La page 86-1 contient la procédure de dépose/repose de l'alternateur, tous types.",[GENERAL_SCOPE]),('RADIO-1989','service','procedure','Radio — dépose et repose à partir de 1989','radio',[1],"La page 86-1 contient la procédure de dépose/repose de la radio pour les véhicules à partir de 1989.",['SCOPE-AKM6348-1989-ON']),('WIRING-850-1976','electrical','wiring','Schéma électrique Mini 850 à partir de 1976','wiring',[2,3],"Les pages 86-2 et 86-3 forment le schéma électrique et sa légende pour Mini 850 berline/fourgonnette/pick-up à partir de 1976.",['SCOPE-AKM6348-MINI850-1976-ON']),('WIRING-1000-1976','electrical','wiring','Schéma électrique Mini 1000 trois instruments à partir de 1976','wiring',[4,5],"Les pages 86-4 et 86-5 donnent le schéma électrique et sa légende pour Mini 1000 à trois instruments à partir de 1976; les marchés GB/Europe/Suède sont explicitement mentionnés sur la page.",['SCOPE-AKM6348-MINI1000-1976-ON']),('WIRING-SPECIAL-7677','electrical','wiring','Schéma électrique Mini Special 1976-1977','wiring',[4,5],"Les pages 86-4 et 86-5 couvrent également la Mini Special 1976-1977.",['SCOPE-AKM6348-MINI-SPECIAL-1976-1977']),('WIRING-CLUBMAN-1976','electrical','wiring','Schéma électrique Clubman, Estate et 1275GT à partir de 1976','wiring',[6,7],"Les pages 86-6 et 86-7 forment le schéma électrique et sa légende pour Clubman, Estate et 1275GT à partir de 1976.",['SCOPE-AKM6348-CLUBMAN-1976-ON','SCOPE-AKM6348-ESTATE-1976-ON','SCOPE-AKM6348-1275GT-1976-ON']),('WIRING-SPECIAL-7778','electrical','wiring','Schéma électrique Mini Special 1977-1978','wiring',[8,9],"Les pages 86-8 et 86-9 forment le schéma électrique et sa légende pour Mini Special 1977-1978.",['SCOPE-AKM6348-MINI-SPECIAL-1977-1978']),('WIRING-1000-CANADA','electrical','wiring','Schéma électrique Mini 1000 Canada à partir de 1977','wiring',[10,11],"Les pages 86-10 et 86-11 forment le schéma électrique et sa légende pour Mini 1000 Canada à partir de 1977.",['SCOPE-AKM6348-MINI1000-CANADA-1977-ON']),('WIRING-SPECIAL-1979','electrical','wiring','Schéma électrique Mini Special 1979','wiring',[12,13],"Les pages 86-12 et 86-13 forment le schéma électrique et sa légende pour Mini Special 1979.",['SCOPE-AKM6348-MINI-SPECIAL-1979']),('ALT-16ACR','service','fact','Alternateur Lucas 16ACR — révision et caractéristiques','alternator',[14],"La page 86-14 couvre la révision de l'alternateur Lucas 16ACR et présente ses caractéristiques constructeur.",[GENERAL_SCOPE]),('DIST-LUCAS','service','fact','Allumeur Lucas 45D4 / 59D4','distributor',[15,16],"Les pages 86-15 et 86-16 couvrent la dépose/repose puis la révision des allumeurs Lucas 45D4/59D4, avec leurs caractéristiques.",[GENERAL_SCOPE]),('DIST-DUCELLIER','service','fact','Allumeur Ducellier — révision','distributor',[17,18],"Les pages 86-17 et 86-18 couvrent la révision de l'allumeur Ducellier et les données associées.",[GENERAL_SCOPE]),('LIGHTING-SERVICE','service','procedure','Éclairage — opérations de service','lighting',[18,19,20],"Les pages 86-18 à 86-20 regroupent des opérations de service sur l'éclairage avant, arrière, plaque d'immatriculation et éclairage intérieur.",[GENERAL_SCOPE]),('STARTER-M35J','service','fact','Démarreur Lucas M35J — dépose, repose et révision','starter',[20,21,22],"Les pages 86-20 à 86-22 couvrent la dépose/repose, la révision et les caractéristiques du démarreur Lucas M35J.",[GENERAL_SCOPE]),('SWITCH-FUSE','electrical','fact','Contacteurs, commutateurs et boîte à fusibles','switches',[22,23],"Les pages 86-22 et 86-23 couvrent notamment le contacteur d'allumage/démarrage, des commutateurs, le contacteur de stop hydraulique, le commutateur phare/avertisseur et la boîte à fusibles.",[GENERAL_SCOPE]),('HARNESS-SERVICE','service','procedure','Faisceaux électriques — dépose et repose','harness',[24,25,26],"Les pages 86-24 à 86-26 couvrent les opérations de dépose/repose de plusieurs faisceaux avant, tableau de bord et arrière selon les variantes indiquées.",[GENERAL_SCOPE]),('IGN-ADVANCE-TRANSMISSION','electrical','diagram','Système de correction d’avance commandé par transmission','ignition_advance',[27],"La page 86-27 décrit et illustre le système de correction d'avance à l'allumage commandé par la transmission, ainsi que ses organes de commande.",[GENERAL_SCOPE]),('STARTER-M79-REMOVE','service','procedure','Démarreur Lucas M79 pré-engagé — dépose/repose et solénoïde','starter',[28],"La page 86-28 couvre le démarreur Lucas M79 pré-engagé, sa dépose/repose et son solénoïde.",[GENERAL_SCOPE]),('HEADLAMP-DIM','electrical','diagram','Système de réduction d’intensité des phares','headlamp_dimming',[29],"La page 86-29 décrit le système de réduction d'intensité des phares, avec boîtier de commande et résistance.",[GENERAL_SCOPE]),('WIRING-1INST-1984','electrical','wiring','Schéma électrique modèles à un instrument à partir de 1984','wiring',[30,31],"Les pages 86-30 et 86-31 forment le schéma électrique et sa légende pour les modèles à un instrument à partir de 1984.",['SCOPE-AKM6348-SINGLE-INSTRUMENT-1984-ON']),('WIRING-MULTI-1984','electrical','wiring','Schéma électrique modèles à plusieurs instruments à partir de 1984','wiring',[32,33],"Les pages 86-32 et 86-33 forment le schéma électrique et sa légende pour les modèles à plusieurs instruments à partir de 1984.",['SCOPE-AKM6348-MULTI-INSTRUMENT-1984-ON']),('HARNESS-1986','electrical','diagram','Implantation des faisceaux et connexions à partir de 1986','harness',[34,35],"Les pages 86-34 et 86-35 montrent l'implantation des faisceaux moteur, tableau de bord et carrosserie à partir de 1986, avec les listes de connexions associées.",['SCOPE-AKM6348-1986-ON']),('GROUNDS-1986','electrical','diagram','Points de masse à partir de 1986','grounds',[35,36],"Les pages 86-35 et 86-36 donnent les listes et positions des points de masse à partir de 1986.",['SCOPE-AKM6348-1986-ON']),('BATTERY','service','fact','Batterie — contrôles et charge','battery',[36,37],"Les pages 86-36 et 86-37 couvrent les contrôles de batterie et les indications constructeur de charge.",[GENERAL_SCOPE]),('ALT-A115','service','fact','Alternateur A115 — révision','alternator',[37],"La page 86-37 débute la révision de l'alternateur A115.",[GENERAL_SCOPE]),('ALT-A127','service','fact','Alternateur A127 — révision et contrôle','alternator',[38,39],"Les pages 86-38 et 86-39 couvrent la révision et le contrôle de l'alternateur A127.",[GENERAL_SCOPE]),('STARTER-M79-OVERHAUL','service','fact','Démarreur Lucas M79 — révision et caractéristiques','starter',[39,40,41],"Les pages 86-39 à 86-41 couvrent la révision du démarreur Lucas M79 et ses caractéristiques constructeur.",[GENERAL_SCOPE]),('WIRING-1988-CODES','electrical','wiring','Schémas électriques à partir de 1988 — conventions, couleurs et symboles','wiring',[42],"La page 86-42 donne les conventions de lecture, codes de couleur des fils et symboles utilisés par les schémas électriques à partir de 1988.",['SCOPE-AKM6348-1988-ON']),('WIRING-1988-TABLES','electrical','wiring','Schémas électriques à partir de 1988 — connexions et légende composants','wiring',[43,44],"Les pages 86-43 et 86-44 donnent des tableaux de connexion, la légende des composants et leurs références de grille pour les schémas à partir de 1988.",['SCOPE-AKM6348-1988-ON']),('WIRING-1988-FOLDOUT','electrical','wiring','Schéma électrique principal à partir de 1988','wiring',[45],"La page constructeur 86-45 est le grand schéma électrique principal à partir de 1988; le scan source est réparti sur quatre pages PDF conservées comme quatre segments d'une seule page constructeur.",['SCOPE-AKM6348-1988-ON']),('WIRING-1988-INSTRUMENTS','electrical','wiring','Circuits instruments et tableau de bord à partir de 1988','instruments',[46],"La page 86-46 présente les circuits des instruments et du tableau de bord dans l'ensemble de schémas à partir de 1988.",['SCOPE-AKM6348-1988-ON']),('COOPER-FAN-LAMPS','electrical','wiring','Cooper — ventilateur auxiliaire et phares additionnels','cooling_fan',[47,48],"Les pages 86-47 et 86-48 présentent des circuits de ventilateur auxiliaire de refroidissement et de phares additionnels, avec application Cooper explicitement indiquée sur les vues concernées.",['SCOPE-AKM6348-COOPER']),('COOPER-DRIVING-LAMPS','service','procedure','Cooper — phares additionnels dépose/repose','driving_lamps',[49],"La page 86-49 contient la procédure de dépose/repose des phares additionnels Cooper.",['SCOPE-AKM6348-COOPER'])]

ALIASES={'alternator':['alternateur'],'radio':['radio'],'wiring':['schéma électrique','schema electrique','câblage','cablage'],'distributor':['allumeur'],'lighting':['éclairage','eclairage'],'starter':['démarreur','demarreur'],'switches':['contacteur','commutateur'],'harness':['faisceau','faisceau électrique','faisceau electrique'],'ignition_advance':['avance à l’allumage','avance allumage'],'headlamp_dimming':['réduction intensité phares','reduction intensite phares'],'grounds':['point de masse','points de masse'],'battery':['batterie'],'instruments':['tableau de bord','instruments'],'cooling_fan':['ventilateur auxiliaire','ventilateur refroidissement'],'driving_lamps':['phares additionnels']}

def normalize_alias(s:str)->str:
    import unicodedata
    s=''.join(c for c in unicodedata.normalize('NFKD',s) if not unicodedata.combining(c)).lower();return re.sub(r'[^a-z0-9]+',' ',s).strip()
def primary_asset(factory_page:int,assets):
    choices=[a for a in assets if a['factory']==f'86-{factory_page}'];assert choices;return choices[0]

def build_sql(assets):
    lines=['-- AKM6348 French CH86 complete visual/knowledge batch 1780','BEGIN IMMEDIATE;']
    for key,model,y0,y1,market,source_text in SCOPES:
        vals=[key,'vehicle','Mini',model,None,None,None,None,None,None,None,y0,y1,None,None,market,None,None,None,source_text,'AKM6348 CH86 source-explicit scope; unspecified dimensions remain NULL.'];cols='scope_key,scope_kind,make,model,engine_family,engine_code,displacement_cc,engine_variant,induction,mems_family,transmission,year_from,year_to,vin_from,vin_to,market,compression_variant,catalyst_state,air_conditioning_state,source_scope_text,notes';lines.append(f"INSERT OR IGNORE INTO mems_applicability_scope({cols}) VALUES("+','.join(sql_quote(v) for v in vals)+');')
    for a in assets:
        segnote=f"; segment {a['seg']}/4 de la page constructeur 86-45" if a['seg'] else '';key=f"RAVE:{DOCUMENT}:PDF:{a['pdf']}";notes=f"Scan original AKM6348 français CH86; PDF p.{a['pdf']} = page constructeur {a['factory']}{segnote}; source SHA-256 {EXPECTED_SOURCE_SHA}.";vals=[key,DOCUMENT,a['factory'],a['pdf'],a['kind'],a['path'],a['sha256'],notes];lines.append("INSERT OR REPLACE INTO mems_rave_illustration(illustration_key,document,source_page,pdf_index,asset_kind,relative_path,sha256,notes) VALUES("+','.join(sql_quote(v) for v in vals)+');')
    link_count=0
    for suffix,domain,ktype,topic,component,pages,source_text,scopes in K:
        kkey='KNOW-AKM6348-CH86-'+suffix;img=primary_asset(pages[0],assets)['path'];section='; '.join(f'86-{p}' for p in pages);vals=[kkey,domain,ktype,topic,component,SOURCE_KEY,DOCUMENT,section,'verifie_constructeur',None,source_text,img,'Structuré depuis les pages originales du scan AKM6348 français CH86; aucune valeur numérique non lisible ou non transcrite n’est inventée.'];lines.append("INSERT OR REPLACE INTO mems_knowledge_item(knowledge_key,domain,knowledge_type,topic,component_key,source_key,document,source_section,verification_level,legacy_rave_fact_key,source_text,image_ref,notes) VALUES("+','.join(sql_quote(v) for v in vals)+');')
        for scope in scopes:lines.append("INSERT OR IGNORE INTO mems_knowledge_scope(knowledge_key,scope_key,applicability) VALUES("+','.join(sql_quote(v) for v in [kkey,scope,'applies'])+');')
        for p in pages:
            for a in assets:
                if a['factory']==f'86-{p}':
                    ikey=f"RAVE:{DOCUMENT}:PDF:{a['pdf']}";lines.append("INSERT OR IGNORE INTO mems_rave_illustration_link(fact_key,illustration_key,link_role,notes) VALUES("+','.join(sql_quote(v) for v in [kkey,ikey,'source',f'Page source constructeur 86-{p} pour la connaissance structurée.'])+');');link_count+=1
    alias_count=0
    for component,aliases in ALIASES.items():
        for alias in aliases:
            lines.append("INSERT OR IGNORE INTO mems_term_alias(entity_type,entity_key,language,alias,normalized_alias) VALUES("+','.join(sql_quote(v) for v in ['component',component,'fr',alias,normalize_alias(alias)])+');');alias_count+=1
    lines+=['COMMIT;'];return ('\n'.join(lines)+'\n').encode('utf-8'),link_count,alias_count

def update_manifest(reference:Path,assets):
    p=reference/'manifest.json';data=json.loads(p.read_text(encoding='utf-8-sig'));assert data.get('database_revision')==20;batches=data.setdefault('research_enrichment_batches',[])
    if 'research_enrichment_1780.qz64' not in batches:batches.append('research_enrichment_1780.qz64')
    diagrams=data.setdefault('diagrams',{});visual=data.setdefault('visual_assets',{})
    for a in assets:
        seg=f' segment {a["seg"]}' if a['seg'] else '';diagrams[f'AKM6348 FR CH86 {a["factory"]}{seg}']=a['path'];visual[f'rave:{DOCUMENT}:PDF:{a["pdf"]}']={'path':a['path'],'asset_kind':a['kind'],'document':DOCUMENT,'source_page':a['factory'],'pdf_index':a['pdf'],'sha256':a['sha256']}
    p.write_text(json.dumps(data,indent=2,ensure_ascii=False)+'\n',encoding='utf-8',newline='\n')

def validate(db_path:Path,reference:Path,assets,link_count:int):
    db=sqlite3.connect(str(db_path))
    try:
        integrity=db.execute('PRAGMA integrity_check').fetchone()[0];uv=db.execute('PRAGMA user_version').fetchone()[0];rave=db.execute('SELECT COUNT(*) FROM mems_rave_fact').fetchone()[0];expert=db.execute('SELECT COUNT(*) FROM mems_expert_fact_external').fetchone()[0];reg=db.execute("SELECT COUNT(*) FROM mems_knowledge_item WHERE knowledge_key='KNOW-DOC-AKM6348-FR'").fetchone()[0];assets_db=db.execute("SELECT COUNT(*) FROM mems_rave_illustration WHERE document=?",(DOCUMENT,)).fetchone()[0];pages_db=db.execute("SELECT COUNT(DISTINCT source_page) FROM mems_rave_illustration WHERE document=?",(DOCUMENT,)).fetchone()[0];fold=db.execute("SELECT COUNT(*) FROM mems_rave_illustration WHERE document=? AND source_page='86-45'",(DOCUMENT,)).fetchone()[0];knowledge=db.execute("SELECT COUNT(*) FROM mems_knowledge_item WHERE document=?",(DOCUMENT,)).fetchone()[0];links=db.execute("SELECT COUNT(*) FROM mems_rave_illustration_link l JOIN mems_rave_illustration i ON i.illustration_key=l.illustration_key WHERE i.document=?",(DOCUMENT,)).fetchone()[0];scopes=db.execute("SELECT COUNT(*) FROM mems_applicability_scope WHERE scope_key LIKE 'SCOPE-AKM6348-%'").fetchone()[0]
        assert str(integrity).lower()=='ok';assert uv==20;assert rave==93;assert expert==105;assert reg==1;assert assets_db==EXPECTED_ASSETS;assert pages_db==EXPECTED_FACTORY_PAGES;assert fold==4;assert knowledge==len(K);assert links==link_count
        for a in assets:
            f=reference/a['path'];assert f.is_file() and sha256_file(f)==a['sha256']
        return dict(integrity=integrity,user_version=uv,rave=rave,expert=expert,assets=assets_db,pages=pages_db,foldout_segments=fold,knowledge=knowledge,links=links,scopes=scopes)
    finally:db.close()

def write_audit(reference:Path,qz:Path,sql_raw:bytes,assets,validation,alias_count):
    p=reference/'audits'/'akm6348_ch86_1780_audit.md';p.parent.mkdir(parents=True,exist_ok=True);mapping=[]
    for a in assets:
        seg=f' segment {a["seg"]}/4' if a['seg'] else '';mapping.append(f"- PDF p.{a['pdf']} -> constructeur `{a['factory']}`{seg}: `{a['filename']}` SHA-256 `{a['sha256']}`")
    text=f'''# AKM6348 français — CH86 — lot 1780\n\nDate: 2026-08-29\n\nProduction inchangée : `MEMSX64` BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`. Lot documentaire additif uniquement.\n\nSource vérifiée : `http://virgyl.f.free.fr/Download/Manuel/CH86.pdf`\n- SHA-256 source : `{EXPECTED_SOURCE_SHA}`\n- taille source vérifiée : 7 626 962 octets\n- pages PDF : 53\n- pages constructeur : 49 (`86-1` à `86-49`)\n- PDF p.49 : page blanche, exclue\n- page constructeur `86-45` : grand schéma replié réparti sur PDF p.45 à p.48, conservé en quatre segments sans créer de faux numéros constructeur\n\n## Résultat\n- captures conservées : {validation['assets']}\n- pages constructeur couvertes : {validation['pages']}\n- connaissances structurées : {validation['knowledge']}\n- liaisons connaissance -> capture : {validation['links']}\n- alias français ajoutés/tentés : {alias_count}\n- portées AKM6348 spécifiques présentes : {validation['scopes']}\n- `PRAGMA integrity_check` : `{validation['integrity']}`\n- `PRAGMA user_version` : {validation['user_version']}\n- faits RAVE historiques préservés : {validation['rave']}\n- faits experts historiques préservés : {validation['expert']}\n\nAucune valeur numérique difficilement lisible n'a été inventée. Les pages de caractéristiques restent disponibles comme captures constructeur et leurs connaissances indiquent leur présence sans transcrire de chiffres non vérifiés.\n\nLot `research_enrichment_1780.qz64` : {qz.stat().st_size} octets, SHA-256 `{sha256_file(qz)}`.\nSQL décompressé : {len(sql_raw)} octets, SHA-256 `{sha256_bytes(sql_raw)}`.\nManifest SHA-256 `{sha256_file(reference/'manifest.json')}`.\n\n## Correspondance exacte PDF -> page constructeur\n'''+ '\n'.join(mapping)+'\n';p.write_text(text,encoding='utf-8',newline='\n')

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--root',type=Path,default=Path('.'));ap.add_argument('--pdf',type=Path,required=True);ap.add_argument('--work',type=Path,default=Path('.tmp-akm6348-ch86-1780'));args=ap.parse_args();root=args.root.resolve();reference=root/'database'/'reference';work=(root/args.work).resolve();work.mkdir(parents=True,exist_ok=True);baseline=work/'baseline.sqlite';candidate=work/'candidate.sqlite';rebuild_reference_database(reference,baseline)
    db=sqlite3.connect(str(baseline))
    try:assert db.execute("SELECT COUNT(*) FROM mems_knowledge_item WHERE knowledge_key='KNOW-DOC-AKM6348-FR'").fetchone()[0]==1;assert db.execute("SELECT COUNT(*) FROM mems_applicability_scope WHERE scope_key=?",(GENERAL_SCOPE,)).fetchone()[0]==1
    finally:db.close()
    assets=render_assets(args.pdf.resolve(),reference);sql_raw,link_count,alias_count=build_sql(assets);qz=reference/'research_enrichment_1780.qz64';qz.write_bytes(encode_qz64(sql_raw));assert decode_qz64(qz)==sql_raw;shutil.copy2(baseline,candidate);db=sqlite3.connect(str(candidate))
    try:execute_project_sql(db,sql_raw);db.execute('PRAGMA user_version=20');db.commit()
    finally:db.close()
    update_manifest(reference,assets);validation=validate(candidate,reference,assets,link_count);write_audit(reference,qz,sql_raw,assets,validation,alias_count);manifest=json.loads((reference/'manifest.json').read_text(encoding='utf-8'));assert 'research_enrichment_1780.qz64' in manifest['research_enrichment_batches'];vals=[v for k,v in manifest.get('visual_assets',{}).items() if k.startswith(f'rave:{DOCUMENT}:PDF:')];assert len(vals)==EXPECTED_ASSETS
    print('AKM6348_CH86_1780_CANDIDATE_PASS');[print(f'{k}={v}') for k,v in validation.items()];print(f'knowledge_count={len(K)}');print(f'link_count={link_count}');print(f'alias_count={alias_count}');print(f'qz64_sha256={sha256_file(qz)}');print(f'sql_sha256={sha256_bytes(sql_raw)}');print(f'manifest_sha256={sha256_file(reference/"manifest.json")}');print(f'audit_sha256={sha256_file(reference/"audits"/"akm6348_ch86_1780_audit.md")}');return 0
if __name__=='__main__':raise SystemExit(main())
