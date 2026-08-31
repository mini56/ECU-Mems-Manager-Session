import argparse, json, re
from pathlib import Path


def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--dir',required=True)
    ap.add_argument('--document',required=True)
    ap.add_argument('--page',type=int,required=True)
    a=ap.parse_args()
    root=Path(a.dir)
    blocks=json.loads((root/'text_blocks.json').read_text(encoding='utf-8'))
    out=[]
    for b in blocks:
        spans=b.get('spans',[])
        i=0
        while i < len(spans):
            s=spans[i]
            m=re.fullmatch(r'\s*(\d{1,3})\.\s*',s.get('text',''))
            if not m:
                i+=1; continue
            ref=m.group(1)
            j=i+1
            while j < len(spans) and not spans[j].get('text','').strip():
                j+=1
            if j>=len(spans):
                i+=1; continue
            text_span=spans[j]
            text=text_span.get('text','').strip()
            if not text or not any(c.isalpha() for c in text):
                i+=1; continue
            out.append({
                'key':f'{a.document}_P{a.page:03d}_CALL{int(ref):02d}',
                'constructor_reference':ref,
                'source_text':text,
                'reference_bbox':s.get('bbox'),
                'text_bbox':text_span.get('bbox'),
                'source_font':text_span.get('font'),
                'source_font_size':text_span.get('size'),
                'source_color':int(text_span.get('color') or 0),
                'source_block_index':b.get('block_index'),
                'reference_preserved':True
            })
            i=j+1
    out.sort(key=lambda x:int(x['constructor_reference']))
    (root/'callout_associations.json').write_text(json.dumps(out,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps({'page':a.page,'callout_associations':len(out),'references':[x['constructor_reference'] for x in out]},ensure_ascii=False))
    if not out:
        raise SystemExit('No constructor callout associations found')

if __name__=='__main__':
    main()
