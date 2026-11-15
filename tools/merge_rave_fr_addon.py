#!/usr/bin/env python3
"""Complete, non-destructive FR SESSION DB update from a verified additive RAVE bundle.

Input: currently installed French documentary SQLite and the 4-PDF RAVE
fragment ZIP. Output: ONE full native MEMSDOC1 encrypted language pack,
installable with 'Base de données -> Installer un pack sécurisé local'.

This tool NEVER changes the installed SQLite, ECU tables or original pictures.
It adds original source documents/pages, preserves all existing repair procedures,
tracks every addition and produces a source SHA-256 report. Only the native
SESSION application installs the resulting encrypted pack with its backup.
"""
from __future__ import annotations
from contextlib import closing
from datetime import datetime,timezone
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,sqlite3,struct,sys,tempfile,zipfile
from cryptography.hazmat.primitives.ciphers.aead import AESGCM

MAGIC=b'MEMSDOC1'
MAX_ARCHIVE=256*1024*1024
MAX_UNPACKED=700*1024*1024
MAX_FILES=2500
DB_NAME='mems_documentary_fr.sqlite'
CONTRACT=('documentary_database','documentary_documents','documentary_pages',
          'documentary_images','documentary_entries','documentary_fts',
          'documentary_operations','documentary_operation_steps',
          'documentary_operation_notices','documentary_import_history')
COPY_TABLES=('documentary_documents','documentary_pages','documentary_images','documentary_entries')
IMAGE_RX=re.compile(r'images/[A-Za-z0-9_.-]+\.(?:jpg|jpeg|png|webp)\Z',re.I)

def sha_file(p:Path)->str:
    with p.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()

def require(ok,err):
    if not ok:raise ValueError(err)

def count(db,table):
    return db.execute(f'SELECT COUNT(*) FROM "{table}"').fetchone()[0]

def digest_rows(db,table):
    digest=hashlib.sha256()
    for row in db.execute(f'SELECT * FROM "{table}" ORDER BY rowid'):
        digest.update(json.dumps(row,separators=(',',':'),ensure_ascii=False,default=str).encode('utf-8'))
        digest.update(b'\n')
    return digest.hexdigest()

def validate_db(db,language='fr'):
    row=db.execute('SELECT schema_version,language,publication_status FROM documentary_database WHERE singleton=1').fetchone()
    require(row is not None and tuple(row[:2])==(1,language),'DOCUMENTARY_DB_WRONG_LANGUAGE_OR_SCHEMA')
    require(row[2] in ('validated','source_inventory'),'DOCUMENTARY_PUBLISHED_STATUS_UNSUPPORTED')
    existing={r[0] for r in db.execute("SELECT name FROM sqlite_master WHERE type IN ('table','view')")}
    require(all(t in existing for t in CONTRACT),'DOCUMENTARY_REQUIRES_SESSION_144_COMMON_FR_SCHEMA')
    docs=db.execute('SELECT document_id,source_revision,source_pages FROM documentary_documents').fetchall()
    require(bool(docs),'DOCUMENTARY_DATABASE_EMPTY')
    require(count(db,'documentary_fts')==count(db,'documentary_entries'),'DOCUMENTARY_FTS_COUNT_MISMATCH')
    require(count(db,'documentary_import_history')==len(docs),'DOCUMENTARY_AUDIT_INCOMPLETE')
    require(count(db,'documentary_pages')==sum(x[2] for x in docs),'DOCUMENTARY_PAGE_COUNT_MISMATCH')
    for doc,revision,pages in docs:
        verify=db.execute('SELECT COUNT(*),MIN(physical_page),MAX(physical_page) FROM documentary_pages WHERE document_id=? AND source_revision=?',(doc,revision)).fetchone()
        require(verify==(pages,1,pages),'DOCUMENTARY_PAGE_GAP_'+doc)
    require(db.execute('PRAGMA integrity_check').fetchone()[0]=='ok','DOCUMENTARY_SQLITE_CORRUPT')
    require(not db.execute('PRAGMA foreign_key_check').fetchall(),'DOCUMENTARY_SOURCE_FOREIGN_KEY_ERROR')
    return {'documents':len(docs),'pages':sum(d[2] for d in docs),
            'entries':count(db,'documentary_entries'),'images':count(db,'documentary_images'),
            'procedures':count(db,'documentary_operations'),
            'steps':count(db,'documentary_operation_steps')}

def verify_addon(archive:zipfile.ZipFile,root:Path):
    require(len(archive.infolist())<MAX_FILES,'ADDON_TOO_MANY_ASSETS')
    entries=[x for x in archive.infolist() if not x.is_dir()]
    names=[x.filename for x in entries]
    require(len(names)==len(set(names)),'ADDON_DUPLICATE_PATHS')
    require(DB_NAME in names,'ADDON_LANGUAGE_SQLITE_MISSING')
    require(all(n==DB_NAME or IMAGE_RX.fullmatch(n) for n in names),'ADDON_UNEXPECTED_ASSET')
    require(sum(e.file_size for e in entries)<MAX_UNPACKED,'ADDON_UNPACKED_TOO_LARGE')
    require(not any((e.external_attr>>16)&0o170000==0o120000 for e in entries),'ADDON_SYMLINK_REJECTED')
    require(archive.testzip() is None,'ADDON_ZIP_CORRUPT')
    source=root/DB_NAME
    source.write_bytes(archive.read(DB_NAME))
    with closing(sqlite3.connect(f'file:{source}?mode=ro',uri=True)) as db:
        db.execute('PRAGMA foreign_keys=ON')
        info=validate_db(db)
        require(info['documents']==4 and info['pages']==406 and info['images']==406,'ADDON_FOUR_SOURCE_MANUALS_NOT_COMPLETE')
        require(info['procedures']==0 and info['steps']==0,'ADDON_UNREVIEWED_PROCEDURES_PRESENT')
        details=db.execute('SELECT document_id,source_revision,source_filename,source_sha256,source_pages FROM documentary_documents ORDER BY source_filename').fetchall()
        require({x[2].lower() for x in details}=={'ir07301f.pdf','elxn010f.pdf','hr01795f.pdf','cdxn010f.pdf'},'ADDON_ORIGINAL_RAVE_NAMES_MISMATCH')
        all_images=db.execute('SELECT relative_path,asset_sha256,asset_bytes FROM documentary_images').fetchall()
        require({x[0] for x in all_images}==set(names)-{DB_NAME},'ADDON_UNREFERENCED_OR_MISSING_IMAGES')
        for name,expected,size in all_images:
            require(0<size<=15*1024*1024 and archive.getinfo(name).file_size==size,'ADDON_IMAGE_SIZE_INVALID')
            require(hashlib.sha256(archive.read(name)).hexdigest()==expected,'ADDON_IMAGE_SHA_MISMATCH')
    return info,details

def find_installed_db():
    local=os.environ.get('LOCALAPPDATA')
    if not local:return None
    root=Path(local)
    matches=[]
    for path in root.rglob(DB_NAME):
        if tuple(part.lower() for part in path.parts[-4:-1])==('documentary','bases','fr'):
            matches.append(path)
            if len(matches)>5:break
    return matches[0] if len(matches)==1 else None

def merge(current:Path,addon:Path,output:Path,password:str,progress=None)->dict:
    """Prepare encrypted full replacement; NEVER write to the installed database."""
    notify=progress or (lambda msg:None)
    current=current.resolve();addon=addon.resolve();output=output.resolve()
    require(current.is_file() and current.name==DB_NAME,'INSTALLED_FRENCH_SQLITE_NOT_SELECTED')
    require(addon.is_file() and addon.suffix.lower()=='.zip' and addon.stat().st_size<MAX_ARCHIVE,'ADDON_ZIP_NOT_FOUND_OR_TOO_LARGE')
    require(len(password)>=12,'PACK_PASSWORD_TOO_SHORT')
    require(output!=addon and output!=current,'OUTPUT_MUST_BE_DIFFERENT')
    require(not output.exists(),'OUTPUT_EXISTS_CHOOSE_NEW_FILENAME')
    original_sha=sha_file(current)
    original_directory=current.parent
    output.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='mems-rave-merge-') as work:
        root=Path(work);merged=root/'full';merged.mkdir()
        stagingdb=merged/DB_NAME
        shutil.copy2(current,stagingdb)
        with zipfile.ZipFile(addon) as zip:
            notify('Vérification des 406 pages et de leurs images')
            add_info,add_docs=verify_addon(zip,root)
            with closing(sqlite3.connect(stagingdb)) as db:
                db.execute('PRAGMA foreign_keys=ON')
                old=validate_db(db)
                require(old['procedures']>0 and old['steps']>0,'ORIGINAL_FRENCH_STRUCTURED_PROCEDURES_MISSING')
                before={t:digest_rows(db,t) for t in ('documentary_operations','documentary_operation_steps','documentary_operation_notices')}
                existing_files={name.lower() for (name,) in db.execute('SELECT source_filename FROM documentary_documents')}
                require(not existing_files.intersection({row[2].lower() for row in add_docs}),'RAVE_SOURCE_ALREADY_PRESENT_PREVENT_DUPLICATION')
                existing_images={p[0] for p in db.execute('SELECT relative_path FROM documentary_images')}
                addon_images={p[0] for p in sqlite3.connect(root/DB_NAME).execute('SELECT relative_path FROM documentary_images')}
                require(not existing_images.intersection(addon_images),'RAVE_IMAGE_FILENAME_COLLISION')
                db.execute('ATTACH DATABASE ? AS donor',(str(root/DB_NAME),))
                donor_identities=db.execute('SELECT document_id,source_revision FROM donor.documentary_documents').fetchall()
                require(len(donor_identities)==4,'DONOR_SOURCE_COUNT_CHANGED')
                db.execute('BEGIN IMMEDIATE')
                notify('Ajout des quatre documents dans une seule base française')
                for table in COPY_TABLES:
                    cols=[x[1] for x in db.execute(f'PRAGMA table_info({table})')]
                    donor_cols=[x[1] for x in db.execute(f'PRAGMA donor.table_info({table})')]
                    require(cols==donor_cols,'INCOMPATIBLE_COMMON_DOCUMENTARY_SCHEMA_'+table)
                    quoted=','.join('"'+c+'"' for c in cols)
                    db.execute(f'INSERT INTO "{table}" ({quoted}) SELECT {quoted} FROM donor."{table}"')
                imported_at=datetime.now(timezone.utc).isoformat()
                for doc,revision,name,digest,pages in add_docs:
                    db.execute('INSERT INTO documentary_import_history(document_id,source_revision,source_sha256,source_pages,import_kind,imported_utc,previous_snapshot_sha256,entries_at_import,procedures_at_import,steps_at_import,images_at_import) VALUES(?,?,?,?,?,?,?,?,?,?,?)',(doc,revision,digest,pages,'added_pdf',imported_at,original_sha,pages,0,0,pages))
                db.execute('UPDATE documentary_database SET collection_revision=collection_revision+1,publication_status=\'source_inventory\' WHERE singleton=1')
                after=validate_db(db)
                require(after['documents']==old['documents']+4 and after['pages']==old['pages']+406,'MERGE_LOST_OR_DUPLICATED_SOURCE_PAGES')
                require(after['images']==old['images']+406 and after['entries']==old['entries']+406,'MERGE_LOST_OR_DUPLICATED_SOURCE_ENTRIES')
                require(after['procedures']==old['procedures'] and after['steps']==old['steps'],'ORIGINAL_WORKSHOP_OPERATIONS_CHANGED')
                require(all(digest_rows(db,t)==before[t] for t in before),'ORIGINAL_WORKSHOP_SOURCE_REWRITTEN')
                db.commit()
                db.execute('DETACH DATABASE donor')
            # Put BOTH existing and four new source-image sets into the same full ZIP.
            images=root/'full'/'images';images.mkdir()
            with closing(sqlite3.connect(stagingdb)) as db:
                records=db.execute('SELECT relative_path,asset_sha256,asset_bytes FROM documentary_images ORDER BY relative_path').fetchall()
            old_count=0
            for name,digest,size in records:
                require(IMAGE_RX.fullmatch(name),'UNSAFE_IMAGE_FILENAME')
                target=merged/name
                if name in existing_images:
                    source=original_directory/name
                    require(source.is_file() and source.stat().st_size==size and sha_file(source)==digest,'INSTALLED_OLD_SOURCE_FACSIMILE_MISSING_OR_CHANGED')
                    shutil.copy2(source,target)
                    old_count+=1
                else:
                    image=zip.read(name)
                    require(len(image)==size and hashlib.sha256(image).hexdigest()==digest,'ADDED_SOURCE_IMAGE_CORRUPT')
                    target.write_bytes(image)
            require(old_count==old['images'],'ORIGINAL_IMAGES_NOT_ALL_PRESERVED')
            notify('Contrôle de l’index et du contenu d’origine avant chiffrement')
            with closing(sqlite3.connect(f'file:{stagingdb}?mode=ro',uri=True)) as db:
                finish=validate_db(db)
                db.execute('PRAGMA foreign_keys=ON')
                require(not db.execute('PRAGMA foreign_key_check').fetchall(),'FINAL_FOREIGN_KEY_ERROR')
                for doc,revision,name,hashtext,pages in add_docs:
                    require(db.execute('SELECT COUNT(*) FROM documentary_entries WHERE document_id=? AND source_revision=? AND verification_status=\'review_required\'',(doc,revision)).fetchone()[0]==pages,'NEW_OCR_STATUS_NOT_REVIEW_REQUIRED')
            sourcezip=root/'entire_fr.zip'
            with zipfile.ZipFile(sourcezip,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
                z.write(stagingdb,DB_NAME)
                for name,_,_ in records:z.write(merged/name,name)
            require(sourcezip.stat().st_size<MAX_ARCHIVE,'FR_RESULT_TOO_LARGE_FOR_SESSION_INSTALLER')
            require(sum((merged/name).stat().st_size for name,_,_ in records)+stagingdb.stat().st_size<MAX_UNPACKED,'FR_RESULT_UNPACKED_TOO_LARGE')
            with zipfile.ZipFile(sourcezip) as z:require(z.testzip() is None,'FINAL_FR_ZIP_CRC_FAILED')
            notify('Chiffrement AES-256 de la base française complète')
            plain=sourcezip.read_bytes()
            salt,nonce=os.urandom(16),os.urandom(12)
            header=MAGIC+bytes([1])+salt+nonce+struct.pack('<Q',len(plain))
            key=hashlib.pbkdf2_hmac('sha256',password.encode('utf-8'),salt,210000,32)
            encrypted=header+AESGCM(key).encrypt(nonce,plain,header)
            partial=output.with_name(output.name+'.part')
            try:
                partial.write_bytes(encrypted)
                require(partial.stat().st_size==sourcezip.stat().st_size+61,'ENCRYPTED_LENGTH_MISMATCH')
                # Readback authentication catches corrupted final output.
                data=partial.read_bytes()
                require(AESGCM(key).decrypt(nonce,data[45:],data[:45])[:2]==b'PK','AES_GCM_READBACK_FAILED')
                partial.replace(output)
            finally:
                partial.unlink(missing_ok=True)
            report={'result':'COMPLETE_FR_ENCRYPTED_PACK_READY','existing_installed_sqlite_unchanged':sha_file(current)==original_sha,
              'language':'fr','existing_database_sha256':original_sha,'new_encrypted_file':output.name,
              'new_encrypted_sha256':sha_file(output),'new_encrypted_size_bytes':output.stat().st_size,
              'previous':old,'merged':finish,
              'added_documents':[{'id':doc,'source':name,'original_export_sha256':hashtext,'pages':pages} for doc,revision,name,hashtext,pages in add_docs],
              'source_documents_preserved':old['documents'],'source_pages_preserved':old['pages'],
              'workshop_procedures_preserved':old['procedures'],'workshop_steps_preserved':old['steps'],
              'first_installation_pending':True,'encrypted':True,
              'installed_by':'SESSION -> Base de données -> Mise à jour -> Installer un pack sécurisé local'}
            report_path=output.with_suffix(output.suffix+'.audit.json')
            report_path.write_text(json.dumps(report,ensure_ascii=False,sort_keys=True,indent=2)+'\n',encoding='utf-8')
            return report

def gui():
    import tkinter as tk
    from tkinter import filedialog,messagebox,simpledialog,ttk
    root=tk.Tk();root.withdraw()
    messagebox.showinfo('RAVE MINI — Ajout FR','Ce programme conserve la base française installée et les anciennes procédures. Il crée un nouveau pack sécurisé, à installer ensuite dans SESSION.')
    installed=find_installed_db()
    if not installed:
        name=filedialog.askopenfilename(title='Choisir la base documentaire française installée',filetypes=[('Base SQLite française',DB_NAME),('SQLite','*.sqlite')]);
        if not name:return
        installed=Path(name)
    addon=filedialog.askopenfilename(title='Choisir MEMS-RAVE-FR-4-DOCUMENTS-AJOUT.zip',filetypes=[('Pack complémentaire RAVE','*.zip')])
    if not addon:return
    out=filedialog.asksaveasfilename(title='Enregistrer la base FR complète chiffrée',defaultextension='.encrypted',initialfile='MEMS-DOC-FR-RAVE-6-DOCUMENTS-SESSION.encrypted',filetypes=[('Pack documentaire sécurisé','*.encrypted')])
    if not out:return
    password=simpledialog.askstring('Mot de passe administrateur','Indiquer le mot de passe de mise à jour des bases :',show='*',parent=root)
    if not password:return
    confirm=simpledialog.askstring('Confirmer','Confirmer le même mot de passe :',show='*',parent=root)
    if confirm!=password:
        messagebox.showerror('Mot de passe','Les mots de passe ne correspondent pas.');return
    try:
        report=merge(installed,Path(addon),Path(out),password)
    except Exception as ex:
        messagebox.showerror('Ajout interrompu',str(ex)+'\n\nLa base française installée n’a pas été modifiée.');return
    messagebox.showinfo('Base française prête',f"{report['merged']['documents']} documents, {report['merged']['pages']} pages, {report['merged']['procedures']} procédures conservées.\n\nPack chiffré : {out}\n\nDans SESSION : Base de données > Mise à jour > Installer un pack sécurisé local.")

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--installed',type=Path)
    p.add_argument('--addon',type=Path)
    p.add_argument('--output',type=Path)
    p.add_argument('--password-env',default='MEMS_DOCUMENTARY_PUBLISH_PASSWORD')
    a=p.parse_args()
    if a.installed or a.addon or a.output:
        if not (a.installed and a.addon and a.output):p.error('installed, addon and output all required')
        password=os.environ.get(a.password_env,'')
        result=merge(a.installed,a.addon,a.output,password,progress=print)
        print(json.dumps(result,ensure_ascii=False,indent=2))
    else:gui()
if __name__=='__main__':main()