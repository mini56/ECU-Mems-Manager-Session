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
        if '-' in x:
            a,b=map(int,x.split('-',1))
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
                    parts.append(t); spans.append({'text':t,'bbox':[round(float(v),3) for v in s['bbox']],'font':s.get('font'),'size':s.get('size')})
        t=norm(' '.join(parts))
        if t: out.append({'block_index':bi,'bbox':[round(float(v),3) for v in b['bbox']],'text':t,'spans':spans})
    return out

def visual_rects(page):
    pa=area(page.rect) or 1
    out=[]
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
            if not (2<=len(t)<=90) or not any(c.isalpha() for c in t) or anchor(t): continue
            r=fitz.Rect(s['bbox'])
            if not any(overlap(r,x) for x in expanded): continue
            key=(tuple(round(v,1) for v in s['bbox']),t)
            if key in seen: continue
            seen.add(key); z.append({'key':None,'source_text':t,'bbox':s['bbox'],'status':'translation_required'})
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
    cand.sort(key=lambda x:x[0],reverse=True)
    return cand[0],cand[:10]

def getfont(size,bold=False):
    p='/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf' if bold else '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
    return ImageFont.truetype(p,size) if Path(p).exists() else ImageFont.load_default()

def wrap(draw,text,f,width):
    lines=[]; cur=''
    for w in text.split():
        t=(cur+' '+w).strip()
        if cur and draw.textlength(t,font=f)>width: lines.append(cur);cur=w
        else:cur=t
    if cur:lines.append(cur)
    return lines

def render(page,zones,translations,out,n):
    scale=160/72; pix=page.get_pixmap(matrix=fitz.Matrix(scale,scale),alpha=False)
    img=Image.frombytes('RGB',[pix.width,pix.height],pix.samples)
    original=out/f'RCL0193ENG_P{n:03d}_ORIGINAL.png'; img.save(original)
    lw=max(560,int(img.width*.42)); canvas=Image.new('RGB',(img.width+lw,img.height),'white');canvas.paste(img,(0,0));d=ImageDraw.Draw(canvas)
    nf=getfont(max(16,int(14*scale)),True);lf=getfont(max(16,int(12*scale)));hf=getfont(max(20,int(16*scale)),True)
    for i,z in enumerate(zones,1):
        x0,y0,x1,y1=[int(v*scale) for v in z['bbox']]; pad=max(2,int(1.5*scale));d.rectangle((x0-pad,y0-pad,x1+pad,y1+pad),fill='white')
        cx,cy=x0+max(10,int(7*scale)),y0+max(10,int(7*scale));rad=max(10,int(8*scale));d.ellipse((cx-rad,cy-rad,cx+rad,cy+rad),outline='black',width=max(1,int(scale)))
        lab=str(i);bb=d.textbbox((0,0),lab,font=nf);d.text((cx-(bb[2]-bb[0])/2,cy-(bb[3]-bb[1])/2-1),lab,fill='black',font=nf)
    x=img.width+24;y=24;d.text((x,y),'LÉGENDE TRADUITE — FR',fill='black',font=hf);y+=int(34*scale)
    for i,z in enumerate(zones,1):
        t=translations.get(z['key'],z['source_text'])
        for line in wrap(d,f'{i}. {t}',lf,lw-48): d.text((x,y),line,fill='black',font=lf);y+=int(17*scale)
        y+=int(4*scale)
        if y>img.height-int(55*scale): d.text((x,img.height-int(38*scale)),'[needs_review: légende trop longue]',fill='black',font=lf);break
    loc=out/f'RCL0193ENG_P{n:03d}_FR_NUMBERED_LEGEND.png';canvas.save(loc)
    return original,loc

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--pdf',required=True);ap.add_argument('--out',required=True);ap.add_argument('--exclude',default='193-202');ap.add_argument('--translations',default='');ap.add_argument('--page',default='auto');a=ap.parse_args()
    pdf=Path(a.pdf);out=Path(a.out);out.mkdir(parents=True,exist_ok=True);srcsha=sha(pdf)
    if srcsha!=EXPECTED_SHA256: raise SystemExit(f'RAVE source SHA mismatch: {srcsha}')
    doc=fitz.open(pdf)
    if doc.page_count!=EXPECTED_PAGES: raise SystemExit(f'RAVE page count mismatch: {doc.page_count}')
    if a.page=='auto':
        (score,n,m,bl,vr,z),top=select(doc,a.exclude)
    else:
        n=int(a.page)
        if excluded(n,a.exclude): raise SystemExit(f'Requested page {n} excluded')
        p=doc[n-1];bl=blocks(p);vr=visual_rects(p);z=zones_for(p,bl,vr,n);score=0;m={'text_chars':len((p.get_text('text') or '').strip()),'word_count':len(p.get_text('words') or []),'visual_regions':len(vr),'translation_zones':len(z)};top=[(0,n,m,bl,vr,z)]
    page=doc[n-1];translations={}
    if a.translations and Path(a.translations).exists():
        q=json.loads(Path(a.translations).read_text(encoding='utf-8'));translations=q.get('translations',q) if isinstance(q,dict) else {}
    original,localized=render(page,z,translations,out,n)
    (out/'source_text.txt').write_text(page.get_text('text') or '',encoding='utf-8')
    (out/'text_blocks.json').write_text(json.dumps(bl,ensure_ascii=False,indent=2),encoding='utf-8')
    (out/'visuals.json').write_text(json.dumps(vr,ensure_ascii=False,indent=2),encoding='utf-8')
    (out/'translation_zones.json').write_text(json.dumps(z,ensure_ascii=False,indent=2),encoding='utf-8')
    template={'locale':'fr','page':n,'translations':{x['key']:x['source_text'] for x in z}};(out/'translation_template_fr.json').write_text(json.dumps(template,ensure_ascii=False,indent=2),encoding='utf-8')
    translated=sum(1 for x in z if translations.get(x['key']))
    gate={'source_verified':True,'mixed_text_visual':m['text_chars']>=200 and m['visual_regions']>0,'source_text_present':m['text_chars']>0,'original_page_present':original.exists(),'translation_zones_present':len(z)>0}
    gate['pilot_extraction_pass']=all(gate.values())
    manifest={'method':'RAVE_V2_NEW_EXTRACTION','source':{'path':str(pdf),'sha256':srcsha,'page_count':doc.page_count},'page':{'physical_page':n,'classification':'mixed_text_visual_with_native_diagram_labels','selection_score':round(score,4),**m},'preservation':{'source_text_full':True,'original_page_raster_intact':True,'original_page_sha256':sha(original),'localized_derivative_separate':True},'translation':{'locale':'fr','required_count':len(z),'provided_translation_count':translated,'legend_inside_same_displayed_image':True,'status':'translated' if z and translated==len(z) else 'needs_translation'},'provenance':{'document':'RCL0193ENG','pdf_page_1based':n,'source_sha256':srcsha},'gate':gate,'candidate_ranking':[{'page':x[1],'score':round(x[0],4),**x[2]} for x in top]}
    (out/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps({'page':n,'zones':len(z),'translation_status':manifest['translation']['status'],'gate':gate},ensure_ascii=False))
    if not gate['pilot_extraction_pass']: raise SystemExit('Pilot extraction gate failed')

if __name__=='__main__':main()
