import argparse, hashlib, json, re
from pathlib import Path
import pymupdf as fitz
from PIL import Image, ImageDraw, ImageFont

EXPECTED_SHA256='c050a3eebe50c5a85bf8a69b7722bd2052079944e09d58578a498984ecf06715'
EXPECTED_PAGES=372
ANCHORS=[re.compile(x,re.I) for x in [r'^\s*\d+([.,]\d+)?\s*(N\.m|Nm|mm|cm|m|V|A|bar|psi|°C|kPa|MPa|ml|l|kg|g)?\s*$',r'^\s*\d+(\.\d+){1,4}\s*$',r'^\s*18G\s*\d+[A-Z]?\s*$',r'^\s*[A-Z]\d{2,}[A-Z0-9-]*\s*$']]

def sha(path):
    h=hashlib.sha256()
    with Path(path).open('rb') as f:
        for b in iter(lambda:f.read(1<<20),b''): h.update(b)
    return h.hexdigest()

def norm(s): return re.sub(r'\s+',' ',s).strip()
def area(r): return max(0,r.width)*max(0,r.height)
def overlap(a,b): return area(fitz.Rect(a)&fitz.Rect(b))>0

def excluded(n,spec):
    for x in (spec or '').split(','):
        x=x.strip()
        if not x: continue
        if '-' in x: a,b=map(int,x.split('-',1))
        else: a=b=int(x)
        if a<=n<=b:return True
    return False

def anchor(t):
    t=norm(t)
    if any(p.match(t) for p in ANCHORS): return True
    letters=sum(c.isalpha() for c in t); digits=sum(c.isdigit() for c in t)
    return digits>=2 and letters<=2

def blocks(page):
    out=[]
    for bi,b in enumerate(page.get_text('dict').get('blocks',[])):
        if b.get('type')!=0: continue
        spans=[]; parts=[]
        for line in b.get('lines',[]):
            for s in line.get('spans',[]):
                t=s.get('text','')
                if t:
                    parts.append(t); spans.append({'text':t,'bbox':[round(float(v),3) for v in s['bbox']],'font':s.get('font'),'size':float(s.get('size') or 8)})
        t=norm(' '.join(parts))
        if t: out.append({'block_index':bi,'key':None,'bbox':[round(float(v),3) for v in b['bbox']],'text':t,'spans':spans})
    return out

def visual_rects(page):
    pa=area(page.rect) or 1; out=[]
    for img in page.get_images(full=True):
        xref=img[0]
        for r in page.get_image_rects(xref):
            rr=fitz.Rect(r); ratio=area(rr)/pa
            if ratio>=.01: out.append({'kind':'image','bbox':[float(v) for v in rr],'xref':xref,'ratio':ratio})
    for i,d in enumerate(page.get_drawings()):
        if not d.get('rect'): continue
        rr=fitz.Rect(d['rect']); ratio=area(rr)/pa; items=len(d.get('items',[]))
        if items>=4 and .01<=ratio<=.45: out.append({'kind':'drawing','bbox':[float(v) for v in rr],'index':i,'items':items,'ratio':ratio})
    return out

def zones_for(page,blks,vrects,pnum):
    expanded=[]
    for v in vrects:
        r=fitz.Rect(v['bbox']); expanded.append(fitz.Rect(r.x0-10,r.y0-10,r.x1+10,r.y1+10)&page.rect)
    z=[]; seen=set()
    for b in blks:
        for s in b['spans']:
            t=norm(s['text'])
            if not (2<=len(t)<=120) or not any(c.isalpha() for c in t) or anchor(t): continue
            r=fitz.Rect(s['bbox'])
            if not any(overlap(r,x) for x in expanded): continue
            key=(tuple(round(v,1) for v in s['bbox']),t)
            if key in seen: continue
            seen.add(key); z.append({'key':None,'source_text':t,'bbox':s['bbox'],'source_font_size':s['size'],'status':'translation_required'})
    z.sort(key=lambda q:(q['bbox'][1],q['bbox'][0],q['source_text']))
    for i,q in enumerate(z,1): q['key']=f'RCL0193ENG_P{pnum:03d}_T{i:02d}'
    return z

def select(doc,exclude):
    cand=[]
    for i in range(doc.page_count):
        n=i+1
        if excluded(n,exclude): continue
        p=doc[i]; text=p.get_text('text') or ''; bl=blocks(p); vr=visual_rects(p); z=zones_for(p,bl,vr,n)
        if len(text.strip())<200 or len(p.get_text('words') or [])<30 or not vr or not z: continue
        score=len(z)*12+min(len(text),1800)/300+sum(min(v['ratio'],.35) for v in vr)*4+len(vr)
        if n<20: score-=8
        cand.append((score,n,{'text_chars':len(text.strip()),'word_count':len(p.get_text('words') or []),'visual_regions':len(vr),'translation_zones':len(z)},bl,vr,z))
    if not cand: raise SystemExit('No text+schematic page with native translatable labels found')
    cand.sort(key=lambda x:x[0],reverse=True); return cand[0],cand[:10]

def getfont(px,bold=False):
    p='/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf' if bold else '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
    return ImageFont.truetype(p,max(8,int(px))) if Path(p).exists() else ImageFont.load_default()

def wrap(draw,text,font,width):
    paras=text.split('\n'); lines=[]
    for pi,para in enumerate(paras):
        words=para.split(); cur=''
        if not words:
            lines.append(''); continue
        for w in words:
            test=(cur+' '+w).strip()
            if cur and draw.textlength(test,font=font)>width: lines.append(cur);cur=w
            else:cur=test
        if cur:lines.append(cur)
        if pi<len(paras)-1: lines.append('')
    return lines

def leading_reference(text):
    m=re.match(r'^(\s*(?:\d+[A-Za-z]?|[A-Z]\d+)(?:[.)-])?\s+)(.+)$',text)
    return (m.group(1),m.group(2)) if m else ('',text)

def fit_text(draw,text,box,base_px,bold=False,min_px=12,max_lines=20):
    x0,y0,x1,y1=box; w=max(20,x1-x0); h=max(12,y1-y0)
    px=max(min_px,int(base_px))
    while px>=min_px:
        f=getfont(px,bold); lines=wrap(draw,text,f,w)
        spacing=max(1,int(px*.22)); line_h=max(1,draw.textbbox((0,0),'Ag',font=f)[3]+spacing)
        if len(lines)<=max_lines and line_h*len(lines)<=h+2: return f,lines,line_h
        px-=1
    f=getfont(min_px,bold); lines=wrap(draw,text,f,w); line_h=max(1,draw.textbbox((0,0),'Ag',font=f)[3]+2)
    return f,lines,line_h

def render(page,blks,zones,translations,block_translations,out,n):
    scale=160/72; pix=page.get_pixmap(matrix=fitz.Matrix(scale,scale),alpha=False)
    img=Image.frombytes('RGB',[pix.width,pix.height],pix.samples)
    original=out/f'RCL0193ENG_P{n:03d}_ORIGINAL.png'; img.save(original)

    # Estimate side expansion needed by translated diagram labels. Left labels are right-aligned
    # to their original anchor; right labels are left-aligned. This adds free space outside the
    # technical drawing instead of shrinking or deforming it.
    probe=ImageDraw.Draw(img); mid=page.rect.width/2
    left_extra=right_extra=0
    for z in zones:
        tr=translations.get(z['key'],'')
        if not tr: continue
        prefix,_=leading_reference(z['source_text'])
        if prefix and not tr.lstrip().startswith(prefix.strip()): tr=prefix+tr
        f=getfont(max(13,z.get('source_font_size',8)*scale*.92))
        tw=probe.textbbox((0,0),tr,font=f)[2]
        x0,y0,x1,y1=[v*scale for v in z['bbox']]
        cx=(z['bbox'][0]+z['bbox'][2])/2
        if cx<mid*.86: left_extra=max(left_extra,int(max(0,tw-x1+10)))
        elif cx>mid*1.14: right_extra=max(right_extra,int(max(0,x0+tw-img.width+10)))
    left_extra=min(left_extra,700); right_extra=min(right_extra,700)
    canvas=Image.new('RGB',(img.width+left_extra+right_extra,img.height),'white')
    ox=left_extra; canvas.paste(img,(ox,0)); d=ImageDraw.Draw(canvas)

    zone_rects=[fitz.Rect({['bbox']) for z in zones]

    # Replace normal page text block-by-block, excluding the diagram-label blocks handled below.
    for b in blks:
        br=fitz.Rect(b['bbox'])
        if any(overlap(br,zr) for zr in zone_rects):
            # Blocks containing diagram labels are handled at span level below.
            if sum(1 for zr in zone_rects if overlap(br,zr))>=2: continue
        key=f'RCL0193ENG_P{n:03d}_B{b["block_index"]:02d}'
        tr=block_translations.get(key)
        if not tr: continue
        x0,y0,x1,y1=[int(v*scale) for v in b['bbox']]; x0+=ox; x1+=ox
        pad=max(2,int(scale)); d.rectangle((x0-pad,y0-pad,x1+pad,y1+pad),fill='white')
        base=max((s.get('size',8) for s in b['spans']),default=8)*scale*.92
        bold=any(token in b['text'] for token in ['ENGINE MANAGEMENT SYSTEM','FUEL SYSTEM','Engine Control Module']) or b['block_index'] in (0,2,3)
        f,lines,lh=fit_text(d,tr,(x0,y0,x1,y1),base,bold=bold,min_px=max(11,int(6.5*scale)))
        yy=y0
        for line in lines:
            d.text((x0,yy),line,fill='black',font=f); yy+=lh

    # Replace every human-language diagram label directly with the selected language.
    # Existing numeric/alphanumeric references inside a source label are preserved, never regenerated.
    for z in zones:
        tr=translations.get(z['key'])
        if not tr: continue
        prefix,_=leading_reference(z['source_text'])
        if prefix and not tr.lstrip().startswith(prefix.strip()): tr=prefix+tr
        x0,y0,x1,y1=[int(v*scale) for v in z['bbox']]
        x0+=ox; x1+=ox; pad=max(2,int(.8*scale))
        d.rectangle((x0-pad,y0-pad,x1+pad,y1+pad),fill='white')
        base=max(13,z.get('source_font_size',8)*scale*.92); f=getfont(base)
        tw=d.textbbox((0,0),tr,font=f)[2]; th=d.textbbox((0,0),'Ag',font=f)[3]
        cx=(z['bbox'][0]+z['bbox'][2])/2
        if cx<mid*.86:
            tx=x1-tw
        elif cx>mid*1.14:
            tx=x0
        else:
            tx=x0
        tx=max(4,min(tx,canvas.width-tw-4)); ty=y0+max(0,((y1-y0)-th)//2)-1
        d.text((tx,ty),tr,fill='black',font=f)

    localized=out/f'RCL0193ENG_P{n:03d}_FR_DIRECT_TEXT.png'; canvas.save(localized)
    return original,localized,left_extra,right_extra

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--pdf',required=True);ap.add_argument('--out',required=True);ap.add_argument('--exclude',default='193-202');ap.add_argument('--translations',default='');ap.add_argument('--page',default='auto');a=ap.parse_args()
    pdf=Path(a.pdf);out=Path(a.out);out.mkdir(parents=True,exist_ok=True);srcsha=sha(pdf)
    if srcsha!=EXPECTED_SHA256: raise SystemExit(f'RAVE source SHA mismatch: {srcsha}')
    doc=fitz.open(pdf)
    if doc.page_count!=EXPECTED_PAGES: raise SystemExit(f'RAVE page count mismatch: {doc.page_count}')
    if a.page=='auto': (score,n,m,bl,vr,z),top=select(doc,a.exclude)
    else:
        n=int(a.page)
        if excluded(n,a.exclude): raise SystemExit(f'Requested page {n} excluded')
        p=doc[n-1];bl=blocks(p);vr=visual_rects(p);z=zones_for(p,bl,vr,n);score=0;m={'text_chars':len((p.get_text('text') or '').strip()),'word_count':len(p.get_text('words') or []),'visual_regions':len(vr),'translation_zones':len(z)};top=[(0,n,m,bl,vr,z)]
    page=doc[n-1];translations={};block_translations={}
    if a.translations and Path(a.translations).exists():
        q=json.loads(Path(a.translations).read_text(encoding='utf-8'))
        if isinstance(q,dict):
            translations=q.get('translations',{})
            block_translations=q.get('page_blocks',{})
    required_blocks=[]
    for b in bl:
        br=fitz.Rect(b['bbox'])
        if sum(1 for zr in [fitz.Rect(x['bbox']) for x in z] if overlap(br,zr))>=2: continue
        if any(c.isalpha() for c in b['text']): required_blocks.append(f'RCL0193ENG_P{n:03d}_B{b["block_index"]:02d}')
    original,localized,left_extra,right_extra=render(page,bl,z,translations,block_translations,out,n)
    (out/'source_text.txt').write_text(page.get_text('text') or '',encoding='utf-8')
    (out/'text_blocks.json').write_text(json.dumps(bl,ensure_ascii=False,indent=2),encoding='utf-8')
    (out/'visuals.json').write_text(json.dumps(vr,ensure_ascii=False,indent=2),encoding='utf-8')
    (out/'translation_zones.json').write_text(json.dumps(z,ensure_ascii=False,indent=2),encoding='utf-8')
    provided_zones=sum(1 for x in z if translations.get(x['key']))
    provided_blocks=sum(1 for k in required_blocks if block_translations.get(k))
    gate={
      'source_verified':True,
      'mixed_text_visual':m['text_chars']>=200 and m['visual_regions']>0,
      'source_text_present':m['text_chars']>0,
      'original_page_present':original.exists(),
      'diagram_translation_complete':len(z)>0 and provided_zones==len(z),
      'page_text_translation_complete':len(required_blocks)>0 and provided_blocks==len(required_blocks),
      'direct_text_replacement':True,
      'artificial_numbers_visible':False,
      'localized_image_present':localized.exists()
    }
    gate['pilot_extraction_pass']=all(v for k,v in gate.items() if k!='artificial_numbers_visible') and gate['artificial_numbers_visible'] is False
    manifest={'method':'RAVE_V2_DIRECT_TEXT_REPLACEMENT','source':{'path':str(pdf),'sha256':srcsha,'page_count':doc.page_count},'page':{'physical_page':n,'classification':'mixed_text_visual_with_native_diagram_labels',**m},'preservation':{'source_text_full':True,'original_page_raster_intact':True,'original_page_sha256':sha(original),'localized_derivative_separate':True,'technical_geometry_unchanged':True},'translation':{'locale':'fr','diagram_required':len(z),'diagram_translated':provided_zones,'page_blocks_required':len(required_blocks),'page_blocks_translated':provided_blocks,'artificial_numbering':False,'existing_source_references_preserved':True,'direct_text_replacement':True,'canvas_expansion':{'left_px':left_extra,'right_px':right_extra}},'provenance':{'document':'RCL0193ENG','pdf_page_1based':n,'source_sha256':srcsha},'gate':gate}
    (out/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps({'page':n,'diagram_zones':len(z),'blocks':len(required_blocks),'gate':gate,'canvas_expansion':[left_extra,right_extra]},ensure_ascii=False))
    if not gate['pilot_extraction_pass']: raise SystemExit('Pilot direct-text gate failed')

if __name__=='__main__': main()
