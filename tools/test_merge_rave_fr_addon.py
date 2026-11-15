#!/usr/bin/env python3
"""Native-schema regression for a cumulative encrypted four-PDF addition."""
from pathlib import Path
from tempfile import TemporaryDirectory
from contextlib import closing
import hashlib,importlib.util,os,sqlite3,struct,sys,zipfile,json
from cryptography.hazmat.primitives.ciphers.aead import AESGCM

# Synthetic fixture. This never uses or distributes original manufacturer pages.
from merge_rave_fr_addon import merge,validate_db,sha_file

SCHEMA='''
PRAGMA foreign_keys=ON;
CREATE TABLE documentary_database(singleton INTEGER PRIMARY KEY,schema_version INTEGER,language TEXT,collection_revision INTEGER,publication_status TEXT);
CREATE TABLE documentary_documents(document_id TEXT,source_revision TEXT,language TEXT,publisher TEXT,edition_code TEXT,source_filename TEXT,source_sha256 TEXT,source_pages INTEGER,extraction TEXT,PRIMARY KEY(document_id,source_revision));
CREATE TABLE documentary_pages(document_id TEXT,source_revision TEXT,physical_page INTEGER,structured_records INTEGER,indexed_state TEXT,PRIMARY KEY(document_id,source_revision,physical_page));
CREATE TABLE documentary_images(document_id TEXT,source_revision TEXT,physical_page INTEGER,relative_path TEXT,asset_sha256 TEXT,asset_bytes INTEGER,PRIMARY KEY(document_id,source_revision,physical_page));
CREATE TABLE documentary_entries(document_id TEXT,source_revision TEXT,entry_id TEXT,entry_type TEXT,source_page INTEGER,title TEXT,source_text TEXT,verification_status TEXT,source_table TEXT,raw_json TEXT,PRIMARY KEY(document_id,source_revision,entry_id));
CREATE TABLE documentary_import_history(import_sequence INTEGER PRIMARY KEY AUTOINCREMENT,document_id TEXT,source_revision TEXT,source_sha256 TEXT,source_pages INTEGER,import_kind TEXT,imported_utc TEXT,previous_snapshot_sha256 TEXT,entries_at_import INTEGER,procedures_at_import INTEGER,steps_at_import INTEGER,images_at_import INTEGER,UNIQUE(document_id,source_revision));
CREATE TRIGGER documentary_import_history_no_update BEFORE UPDATE ON documentary_import_history BEGIN SELECT RAISE(ABORT,'DOCUMENTARY_IMPORT_LOG_IMMUTABLE'); END;
CREATE TRIGGER documentary_import_history_no_delete BEFORE DELETE ON documentary_import_history BEGIN SELECT RAISE(ABORT,'DOCUMENTARY_IMPORT_LOG_IMMUTABLE'); END;
CREATE TABLE documentary_operations(document_id TEXT,source_revision TEXT,operation_id TEXT,source_entry_id TEXT,title TEXT,opening_page INTEGER,verification_status TEXT,PRIMARY KEY(document_id,source_revision,operation_id));
CREATE TABLE documentary_operation_steps(document_id TEXT,source_revision TEXT,operation_id TEXT,source_entry_id TEXT,step_number INTEGER,phase TEXT,source_page INTEGER,source_text TEXT,verification_status TEXT,PRIMARY KEY(document_id,source_revision,source_entry_id));
CREATE TABLE documentary_operation_notices(document_id TEXT,source_revision TEXT,operation_id TEXT,source_entry_id TEXT,source_page INTEGER,source_text TEXT,verification_status TEXT,PRIMARY KEY(document_id,source_revision,source_entry_id));
CREATE TABLE documentary_source_rows(table_name TEXT,source_index INTEGER,row_json TEXT,PRIMARY KEY(table_name,source_index));
CREATE INDEX idx_documentary_page ON documentary_entries(document_id,source_page);
CREATE INDEX idx_documentary_type ON documentary_entries(entry_type);
CREATE VIRTUAL TABLE documentary_fts USING fts5(title,source_text,content='documentary_entries',content_rowid='rowid',tokenize='unicode61 remove_diacritics 2');
CREATE TRIGGER documentary_fts_insert AFTER INSERT ON documentary_entries BEGIN INSERT INTO documentary_fts(rowid,title,source_text) VALUES(new.rowid,new.title,new.source_text); END;
'''

def fixture(root:Path,addon:bool):
    root.mkdir(parents=True,exist_ok=True)
    (root/'images').mkdir(exist_ok=True)
    dbpath=root/'mems_documentary_fr.sqlite'
    with closing(sqlite3.connect(dbpath)) as db:
        db.executescript(SCHEMA)
        db.execute('INSERT INTO documentary_database VALUES(1,1,?,1,?)',('fr','source_inventory'))
        if addon:
            documents=[('DOC_IR','ir07301f.pdf',24),('DOC_EL','elxn010f.pdf',210),('DOC_HR','hr01795f.pdf',98),('DOC_CD','cdxn010f.pdf',74)]
        else:
            documents=[('RCL0193FRE','RCL0193FRE.pdf',2),('AKM7169FRE','AKM7169FRE.pdf',1)]
        for doc,name,pages in documents:
            revision='source-a'
            digest=hashlib.sha256(doc.encode()).hexdigest()
            db.execute('INSERT INTO documentary_documents VALUES(?,?,?,?,?,?,?,?,?)',(doc,revision,'fr','Rover','test',name,digest,pages,'synthetic fixture'))
            for page in range(1,pages+1):
                raw=(f'FAKE_PAGE_{doc}_{page}'.encode())
                filename=f'images/{doc}_{page:04d}.jpg';(root/filename).write_bytes(raw)
                db.execute('INSERT INTO documentary_pages VALUES(?,?,?,?,?)',(doc,revision,page,0,'source_linked'))
                db.execute('INSERT INTO documentary_images VALUES(?,?,?,?,?,?)',(doc,revision,page,filename,hashlib.sha256(raw).hexdigest(),len(raw)))
                db.execute('INSERT INTO documentary_entries VALUES(?,?,?,?,?,?,?,?,?,?)',(doc,revision,f'page:{page:05d}','reference',page,f'{name} page {page}',f'Synthetic source text on page {page}','review_required','fake_test','{}'))
            extra=2 if not addon and doc=='RCL0193FRE' else 0
            if extra:
                db.execute('INSERT INTO documentary_entries VALUES(?,?,?,?,?,?,?,?,?,?)',(doc,revision,'procedure1','procedure',1,'Depose','Procedure test','source_checked','old','{}'))
                db.execute('INSERT INTO documentary_entries VALUES(?,?,?,?,?,?,?,?,?,?)',(doc,revision,'step1','step',1,'Etape','Étape 1','source_checked','old','{}'))
                db.execute('INSERT INTO documentary_operations VALUES(?,?,?,?,?,?,?)',(doc,revision,'op1','procedure1','Depose',1,'source_checked'))
                db.execute('INSERT INTO documentary_operation_steps VALUES(?,?,?,?,?,?,?,?,?)',(doc,revision,'op1','step1',1,'depose',1,'Étape 1','source_checked'))
            db.execute('INSERT INTO documentary_import_history(document_id,source_revision,source_sha256,source_pages,import_kind,imported_utc,previous_snapshot_sha256,entries_at_import,procedures_at_import,steps_at_import,images_at_import) VALUES(?,?,?,?,?,?,?,?,?,?,?)',(doc,revision,digest,pages,'source_build','2026-11-15T11:00:00+00:00',None,pages+extra,1 if extra else 0,1 if extra else 0,pages))
        db.commit()
        validate_db(db)
    return documents

def test():
  with TemporaryDirectory(prefix='rave-fr-smoke-') as tmp:
    tmp=Path(tmp)
    installed=tmp/'installed';donor=tmp/'donor'
    fixture(installed,False);fixture(donor,True)
    donor_zip=tmp/'addon.zip'
    with zipfile.ZipFile(donor_zip,'w',zipfile.ZIP_DEFLATED) as z:
      z.write(donor/'mems_documentary_fr.sqlite','mems_documentary_fr.sqlite')
      for f in sorted((donor/'images').glob('*')):z.write(f,'images/'+f.name)
    origin=installed/'mems_documentary_fr.sqlite'
    sha=sha_file(origin)
    output=tmp/'merged.encrypted'
    pwd='ONLY_TEST_PASSWORD_2026_SECURE'
    result=merge(origin,donor_zip,output,pwd)
    assert sha_file(origin)==sha
    assert output.is_file()
    assert result['previous']['documents']==2
    assert result['merged']['documents']==6
    assert result['merged']['pages']==409
    assert result['merged']['images']==409
    assert result['merged']['procedures']==1
    assert result['merged']['steps']==1
    assert result['merged']['entries']==411
    assert result['existing_installed_sqlite_unchanged']
    raw=output.read_bytes()
    key=hashlib.pbkdf2_hmac('sha256',pwd.encode(),raw[9:25],210000,32)
    recovered=AESGCM(key).decrypt(raw[25:37],raw[45:],raw[:45])
    assert len(recovered)==struct.unpack('<Q',raw[37:45])[0]
    replay=tmp/'recovered.zip';replay.write_bytes(recovered)
    with zipfile.ZipFile(replay) as a:
      assert len(a.namelist())==410  # SQLite + 409 immutable image files
      assert a.testzip() is None
      dst=tmp/'installed2';dst.mkdir();a.extractall(dst)
    try:
      merge(dst/'mems_documentary_fr.sqlite',donor_zip,tmp/'duplicate.encrypted',pwd)
    except ValueError as err:
      assert 'ALREADY_PRESENT' in str(err) or 'DUPLICAT' in str(err)
    else:raise AssertionError('duplicate source must be rejected')
    print('PASS_COMMON_SCHEMA_4_REAL_SOURCE_NAMES_CUMULATIVE_FR_ENCRYPTED_6_DOCUMENTS',json.dumps(result['merged'],sort_keys=True))
if __name__=='__main__':test()