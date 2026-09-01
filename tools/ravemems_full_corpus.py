#!/usr/bin/env python3
from __future__ import annotations
import argparse,hashlib,json,re,sqlite3,statistics
from collections import defaultdict
from pathlib import Path
import fitz,pytesseract
from PIL import Image
from pytesseract import Output
NONEN=re.compile(r'(^|[/_.-])(fre|fra|fr|deu|ger|de|ita|it|spa|es|por|pt)([/_.-]|$)',re.I)
def shab(b):return hashlib.sha256(b).hexdigest()
def shaf(p):
 h=hashlib.sha256();f=p.open('rb')
 for c in iter(lambda:f.read(1048576),b''):h.update(c)
 f.close();return h.hexdigest()
def jwrite(p,x):p.write_text(json.dumps(x,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def key(p):return re.sub(r'[^A-Za-z0-9]+','_',p.as_posix()).strip('_').upper() or 'DOC'
def box(v):return [round(float(x),3) for x in v]
def tree(p):
 d={}
 for s in p.read_text(errors='replace').splitlines():
  m=re.match(r'^\d+\s+\w+\s+([0-9a-f]{40})\s+(-|\d+)\t(.+)$',s)
  if m:d[m.group(3)]={'blob_sha':m.group(1),'size':None if m.group(2)=='-' else int(m.group(2))}
 return d
def evidence(t):
 s=' '+re.sub(r'\s+',' ',t.lower())+' '
 en=sum(s.count(w) for w in [' the ',' and ',' warning',' caution',' removal',' refit',' connector',' engine',' vehicle',' circuit',' diagnosis',' testbook'])
 ne=sum(s.count(w) for w in [' le ',' la ',' les ',' et ',' der ',' die ',' und ',' il ',' lo ',' el ',' los '])
 return {'english_score':en,'non_english_score':ne,'looks_english':en>=ne}
def ocr(img):
 d=pytesseract.image_to_data(img,lang='eng',config='--psm 3',output_type=Output.DICT);g=defaultdict(list)
 for i,t in enumerate(d['text']):
  t=(t or '').strip()
  if not t:continue
  try:c=float(d['conf'][i])
  except:c=-1
  if c<20:continue
  x,y,w,h=(int(d[k][i]) for k in ('left','top','width','height'))
  g[(int(d['block_num'][i]),int(d['par_num'][i]))].append((t,c,[x,y,x+w,y+h]))
 out=[]
 for ws in g.values():
  b=[w[2] for w in ws];cs=[w[1] for w in ws if w[1]>=0]
  out.append({'text':' '.join(w[0] for w in ws),'bbox':[min(x[0] for x in b),min(x[1] for x in b),max(x[2] for x in b),max(x[3] for x in b)],'mean_confidence':round(statistics.mean(cs),2) if cs else None,'word_count':len(ws)})
 return sorted(out,key=lambda x:(x['bbox'][1],x['bbox'][0]))
def initdb(p):
 if p.exists():p.unlink()
 d=sqlite3.connect(p);d.execute('PRAGMA foreign_keys=ON');d.executescript('''
 CREATE TABLE document(document_key TEXT PRIMARY KEY,relative_path TEXT UNIQUE,source_blob_sha TEXT,source_sha256 TEXT,source_size INT,page_count INT,source_language TEXT,selection_reason TEXT,english_evidence_json TEXT);
 CREATE TABLE page(page_key TEXT PRIMARY KEY,document_key TEXT REFERENCES document,physical_page INT,page_class TEXT,native_text INT,ocr_used INT,source_text_exact TEXT,source_text_sha256 TEXT,image_occurrence_count INT,vector_drawing_count INT,needs_review INT,review_reason TEXT,UNIQUE(document_key,physical_page));
 CREATE TABLE line(line_key TEXT PRIMARY KEY,page_key TEXT REFERENCES page,native_order INT,block_index INT,line_in_block INT,source_text TEXT,bbox_json TEXT,spans_json TEXT);
 CREATE TABLE content(item_key TEXT PRIMARY KEY,page_key TEXT REFERENCES page,order_index INT,item_type TEXT,source_text TEXT,source_line_ids_json TEXT,bbox_json TEXT);
 CREATE TABLE visual_asset(visual_key TEXT PRIMARY KEY,document_key TEXT REFERENCES document,xref INT,ext TEXT,relative_path TEXT,sha256 TEXT,width INT,height INT,UNIQUE(document_key,xref));
 CREATE TABLE visual_occurrence(occurrence_key TEXT PRIMARY KEY,page_key TEXT REFERENCES page,visual_key TEXT REFERENCES visual_asset,bbox_json TEXT);
 CREATE TABLE ocr_region(region_key TEXT PRIMARY KEY,page_key TEXT REFERENCES page,order_index INT,source_text TEXT,bbox_json TEXT,mean_confidence REAL,word_count INT);
 CREATE INDEX ixp ON page(document_key,physical_page);CREATE INDEX ixc ON content(page_key,order_index);CREATE INDEX ixo ON ocr_region(page_key,order_index);CREATE INDEX ixv ON visual_occurrence(page_key);''');return d
def native(page):
 z=page.get_text('dict',sort=False);ls=[];bs=[]
 for bi,b in enumerate(z.get('blocks',[])):
  if b.get('type')!=0:continue
  ids=[]
  for li,l in enumerate(b.get('lines',[])):
   sp=[{'text':s.get('text',''),'font':s.get('font'),'size':round(float(s.get('size',0)),3),'flags':int(s.get('flags',0)),'color':int(s.get('color',0)),'bbox':box(s.get('bbox',(0,0,0,0)))} for s in l.get('spans',[])]
   ls.append({'block':bi,'line':li,'text':''.join(x['text'] for x in sp),'bbox':box(l.get('bbox',(0,0,0,0))),'spans':sp});ids.append(len(ls)-1)
  if ids:bs.append({'block':bi,'bbox':box(b.get('bbox',(0,0,0,0))),'ids':ids})
 return ls,bs
def main():
 a=argparse.ArgumentParser();a.add_argument('--source-root',type=Path,required=True);a.add_argument('--git-tree',type=Path,required=True);a.add_argument('--source-commit',required=True);a.add_argument('--out',type=Path,required=True);a=a.parse_args()
 root=a.source_root.resolve();out=a.out.resolve();out.mkdir(parents=True,exist_ok=True);(out/'assets').mkdir(exist_ok=True);gt=tree(a.git_tree)
 allpdf=sorted(p for p in root.rglob('*') if p.is_file() and p.suffix.lower()=='.pdf');sel=[];skip=[]
 for p in allpdf:
  r=p.relative_to(root);(skip if NONEN.search(r.as_posix()) else sel).append((p,r))
 db=initdb(out/'ravemems_full_corpus.sqlite');audit=[];review=[];err=[];cnt=defaultdict(int)
 for di,(pdf,rel) in enumerate(sel,1):
  repo='rave/'+rel.as_posix();meta=gt.get(repo,{});dk=f'DOC_{di:04d}_{key(rel)}'
  try:doc=fitz.open(pdf)
  except Exception as e:err.append({'document':repo,'error':f'open_failed: {e}'});continue
  try:
   idx=sorted(set(list(range(min(8,doc.page_count)))+list(range(max(8,doc.page_count-2),doc.page_count))))
   ev=evidence('\n'.join(doc[i].get_text('text',sort=False) for i in idx));db.execute('INSERT INTO document VALUES(?,?,?,?,?,?,?,?,?)',(dk,repo,meta.get('blob_sha'),shaf(pdf),meta.get('size'),doc.page_count,'en','canonical_rave_not_explicitly_non_english',json.dumps(ev)))
   ad=out/'assets'/dk;ad.mkdir(parents=True,exist_ok=True);assets={}
   for pi in range(doc.page_count):
    p=doc[pi];pn=pi+1;pk=f'{dk}_P{pn:04d}';txt=p.get_text('text',sort=False);ls,bs=native(p) if txt.strip() else ([],[]);occ=0;occrows=[];ocrrows=[]
    try:vectors=len(p.get_drawings())
    except:vectors=0
    for im in p.get_images(full=True):
     x=int(im[0]);
     if x<=0:continue
     if x not in assets:
      try:
       z=doc.extract_image(x);blob=z['image'];ext=(z.get('ext') or 'bin').lower();vk=f'{dk}_X{x}';rp=f'assets/{dk}/{vk}.{ext}';(out/rp).write_bytes(blob);assets[x]=vk;db.execute('INSERT OR IGNORE INTO visual_asset VALUES(?,?,?,?,?,?,?,?)',(vk,dk,x,ext,rp,shab(blob),int(z.get('width') or 0),int(z.get('height') or 0)))
      except Exception as e:review.append({'page_key':pk,'document':repo,'physical_page':pn,'reason':f'image_extract_failed_xref_{x}: {e}'});continue
     try:rects=p.get_image_rects(x)
     except:rects=[]
     for r in rects or [fitz.Rect(0,0,0,0)]:occ+=1;occrows.append((f'{pk}_VIS{occ:03d}',pk,assets[x],json.dumps(box(r))))
    oc=0;rv=0;why=None;cl='blank';src=txt or ''
    if txt.strip():cl='mixed_native' if occ or vectors else 'text_native'
    elif occ or vectors:
     cl='visual_no_native_text';oc=1;rv=1;why='ocr_fallback_no_native_pdf_text'
     try:
      pix=p.get_pixmap(matrix=fitz.Matrix(1.5,1.5),alpha=False);rp=f'assets/{dk}/{pk}_NO_NATIVE_RENDER.png';pix.save(str(out/rp));regs=ocr(Image.open(out/rp).convert('RGB'))
      if regs:
       cl='visual_ocr';src='\n'.join(x['text'] for x in regs)
       for ri,x in enumerate(regs):ocrrows.append((f'{pk}_OCR{ri+1:03d}',pk,ri,x['text'],json.dumps(x['bbox']),x['mean_confidence'],x['word_count']))
      else:review.append({'page_key':pk,'document':repo,'physical_page':pn,'reason':'ocr_returned_no_regions'})
     except Exception as e:why=f'ocr_failed: {e}';review.append({'page_key':pk,'document':repo,'physical_page':pn,'reason':why})
    db.execute('INSERT INTO page VALUES(?,?,?,?,?,?,?,?,?,?,?,?)',(pk,dk,pn,cl,1 if txt.strip() else 0,oc,src,shab(src.encode()) if src else None,occ,vectors,rv,why));db.executemany('INSERT INTO visual_occurrence VALUES(?,?,?,?)',occrows);db.executemany('INSERT INTO ocr_region VALUES(?,?,?,?,?,?,?)',ocrrows)
    if txt.strip():
     lids=[]
     for li,x in enumerate(ls):
      lk=f'{pk}_L{li:04d}';lids.append(lk);db.execute('INSERT INTO line VALUES(?,?,?,?,?,?,?,?)',(lk,pk,li,x['block'],x['line'],x['text'],json.dumps(x['bbox']),json.dumps(x['spans'],ensure_ascii=False)))
     for ii,b in enumerate(bs):
      ids=[lids[j] for j in b['ids']];s=' '.join(ls[j]['text'].strip() for j in b['ids'] if ls[j]['text'].strip());db.execute('INSERT INTO content VALUES(?,?,?,?,?,?,?)',(f'{pk}_I{ii:03d}',pk,ii,'native_text_block',s,json.dumps(ids),json.dumps(b['bbox'])))
    if rv and not any(x.get('page_key')==pk for x in review):review.append({'page_key':pk,'document':repo,'physical_page':pn,'reason':why})
    cnt['pages_accounted']+=1;cnt['native_text_pages']+=bool(txt.strip());cnt['ocr_pages']+=oc;cnt['ocr_regions']+=len(ocrrows);cnt['blank_pages']+=cl=='blank';cnt['visual_occurrences']+=occ;cnt['vector_drawing_objects']+=vectors;cnt['native_lines']+=len(ls);cnt['content_items']+=len(bs)
   cnt['documents_processed']+=1;cnt['pages_expected_all_opened_documents']+=doc.page_count;audit.append({'document_key':dk,'relative_path':repo,'source_blob_sha':meta.get('blob_sha'),'source_size':meta.get('size'),'source_sha256':shaf(pdf),'page_count':doc.page_count,'english_evidence':ev,'processed':True})
  except Exception as e:err.append({'document':repo,'error':f'processing_failed: {e}'})
  finally:doc.close();db.commit()
 for p,r in skip:audit.append({'relative_path':'rave/'+r.as_posix(),'processed':False,'skip_reason':'explicit_non_english_filename_or_path'})
 fatal_review=[x for x in review if (x.get('reason') or '').startswith(('ocr_failed:','image_extract_failed'))]
 cnt['documents_found_pdf']=len(allpdf);cnt['documents_selected_english']=len(sel);cnt['documents_skipped_explicit_non_english']=len(skip);cnt['needs_review_pages']=len({x.get('page_key') for x in review if x.get('page_key')});cnt['execution_failure_reviews']=len(fatal_review)
 integ=db.execute('PRAGMA integrity_check').fetchone()[0];fk=db.execute('PRAGMA foreign_key_check').fetchall();dbc={t:db.execute(f'SELECT COUNT(*) FROM {t}').fetchone()[0] for t in ['document','page','line','content','visual_asset','visual_occurrence','ocr_region']};db.close();pm=cnt['pages_accounted']==cnt['pages_expected_all_opened_documents'];clean=not err and not fatal_review;ok=clean and cnt['documents_processed']==cnt['documents_selected_english'] and pm and integ.lower()=='ok' and not fk
 jwrite(out/'document_audit.json',audit);jwrite(out/'needs_review.json',review);m={'method':'RAVEMEMS_FULL_CORPUS','source_commit':a.source_commit,'source_root':'main/rave','source_language_policy':'English source only; explicit non-English filename/path variants skipped','capture_scope':'ALL_RAVE_CONTENT_NO_ECU_FILTER','translation_performed_by_github':False,'localization_layer':'MEMS Manager','ocr_policy':'native PDF text first; OCR only when native text is absent; raster and vector-only pages rendered for fallback','uncertainty_policy':'preserve and mark needs_review; never discard page data','counts':dict(cnt),'database_counts':dbc,'checks':{'all_selected_documents_opened':cnt['documents_processed']==cnt['documents_selected_english'],'all_pages_accounted':pm,'sqlite_integrity_ok':integ.lower()=='ok','sqlite_foreign_keys_ok':not fk,'no_processing_errors':clean,'no_execution_failures_in_needs_review':not fatal_review},'errors':err,'execution_failures_in_needs_review':fatal_review,'pass':ok};jwrite(out/'manifest.json',m);print(json.dumps(m,indent=2));
 if not ok:raise SystemExit('RAVEMEMS full corpus completeness gate failed')
if __name__=='__main__':main()
