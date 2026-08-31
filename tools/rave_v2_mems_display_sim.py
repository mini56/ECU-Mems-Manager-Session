import argparse, hashlib, json
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

SCALE=160/72

def font(px,bold=False):
    p='/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf' if bold else '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
    return ImageFont.truetype(p,max(8,int(px))) if Path(p).exists() else ImageFont.load_default()

def rgb(v):
    v=int(v or 0)
    return ((v>>16)&255,(v>>8)&255,v&255)

def crop_sha(img,box):
    h=hashlib.sha256(); h.update(img.crop(box).tobytes()); return h.hexdigest()

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--dir',required=True)
    ap.add_argument('--translations',required=True)
    ap.add_argument('--out',required=True)
    a=ap.parse_args()
    root=Path(a.dir)
    tr=json.loads(Path(a.translations).read_text(encoding='utf-8'))
    callouts=json.loads((root/'callout_associations.json').read_text(encoding='utf-8'))
    blocks=json.loads((root/'text_blocks.json').read_text(encoding='utf-8'))
    man=json.loads((root/'manifest.json').read_text(encoding='utf-8'))
    page=int(man['page']['physical_page']); doc=man['provenance']['document']
    original=Image.open(root/f'{doc}_P{page:03d}_ORIGINAL.png').convert('RGB')

    required={c['key'] for c in callouts}; supplied=set(tr.get('callouts',{})); missing=sorted(required-supplied)
    if missing: raise SystemExit(f'Missing callout translations: {missing}')

    probe=ImageDraw.Draw(original); f_call=font(10*SCALE)
    left=[c for c in callouts if int(c['constructor_reference'])<=7]
    right=[c for c in callouts if int(c['constructor_reference'])>=8]
    left_need=max((probe.textlength(tr['callouts'][c['key']],font=f_call) for c in left),default=0)
    right_need=max((probe.textlength(tr['callouts'][c['key']],font=f_call) for c in right),default=0)
    left_available=(315-65.5)*SCALE
    right_available=(575-344.5)*SCALE
    middle_extra=max(0,int(left_need-left_available+30))
    right_extra=max(0,int(right_need-right_available+30))
    middle_extra=min(middle_extra,700); right_extra=min(right_extra,900)
    total_extra=middle_extra+right_extra

    canvas=Image.new('RGB',(original.width+total_extra,original.height),'white')
    canvas.paste(original,(0,0)); d=ImageDraw.Draw(canvas)

    # Move the constructor header pictogram into added free space when the translated title is wider.
    # The pictogram itself is copied pixel-identically; only its surrounding free-space position changes.
    icon_box=(int(452*SCALE),int(17*SCALE),int(553*SCALE),int(59*SCALE))
    icon=original.crop(icon_box)
    if total_extra:
        d.rectangle(icon_box,fill='white')
        canvas.paste(icon,(icon_box[0]+total_extra,icon_box[1]))
        # extend the horizontal header rule through the newly created free area
        y=int(58*SCALE)
        d.line((int(41*SCALE),y,icon_box[0]+total_extra-15,y),fill='black',width=2)

    block_map={b['block_index']:b for b in blocks}
    overflow=[]
    for bi in (1,2,3):
        key=f'{doc}_P{page:03d}_B{bi:02d}'; text=tr.get('page_blocks',{}).get(key)
        if not text or bi not in block_map: continue
        b=block_map[bi]; x0,y0,x1,y1=[int(v*SCALE) for v in b['bbox']]
        d.rectangle((x0-3,y0-3,x1+3,y1+3),fill='white')
        size=max((s.get('size',10) for s in b.get('spans',[])),default=10)*SCALE
        f=font(size,True); fill=rgb(next((s.get('color',0) for s in b.get('spans',[]) if s.get('text','').strip()),0))
        if bi==1:
            max_w=(icon_box[0]+total_extra-18)-x0
        else:
            max_w=canvas.width-x0-30
        while d.textlength(text,font=f)>max_w and size>9*SCALE:
            size-=1; f=font(size,True)
        if d.textlength(text,font=f)>max_w:
            overflow.append(key)
        d.text((x0,y0),text,fill=fill,font=f)

    list_blocks=[b for b in blocks if b['block_index'] in (5,6)]
    if not list_blocks: raise SystemExit('Expected numbered callout list blocks not found')
    ly0=min(int(b['bbox'][1]*SCALE) for b in list_blocks)-4
    ly1=max(int(b['bbox'][3]*SCALE) for b in list_blocks)+6
    d.rectangle((int(40*SCALE),ly0,canvas.width-int(20*SCALE),ly1),fill='white')

    left_ref_x=int(51.2*SCALE); left_text_x=int(65.5*SCALE)
    right_ref_x=int(324.65*SCALE+middle_extra); right_text_x=int(344.5*SCALE+middle_extra)
    f_ref=font(10*SCALE,True); f_txt=font(10*SCALE,False)
    row_h=int(12*SCALE); start_y=int(498.25*SCALE)
    left_limit=right_ref_x-int(15*SCALE)
    right_limit=canvas.width-int(20*SCALE)

    for c in left:
        idx=int(c['constructor_reference'])-1; y=start_y+idx*row_h
        txt=tr['callouts'][c['key']]
        if left_text_x+d.textlength(txt,font=f_txt)>left_limit:
            overflow.append(c['key'])
        d.text((left_ref_x,y),c['constructor_reference']+'.',fill=rgb(c.get('source_color',0)),font=f_ref)
        d.text((left_text_x,y),txt,fill=rgb(c.get('source_color',0)),font=f_txt)
    for c in right:
        idx=int(c['constructor_reference'])-8; y=start_y+idx*row_h
        txt=tr['callouts'][c['key']]
        if right_text_x+d.textlength(txt,font=f_txt)>right_limit:
            overflow.append(c['key'])
        d.text((right_ref_x,y),c['constructor_reference']+'.',fill=rgb(c.get('source_color',0)),font=f_ref)
        d.text((right_text_x,y),txt,fill=rgb(c.get('source_color',0)),font=f_txt)

    tech_box=(int(75*SCALE),int(120*SCALE),int(555*SCALE),int(490*SCALE))
    before=crop_sha(original,tech_box); after=crop_sha(canvas,tech_box); technical_identical=(before==after)

    out=Path(a.out); canvas.save(out)
    sim={
      'method':'MEMS_MANAGER_DISPLAY_SIMULATION','source_page':page,'locale':tr.get('locale'),
      'github_source_translation':False,'translation_layer':'external display layer supplied to simulator',
      'constructor_callout_count':len(callouts),'constructor_references_preserved':[c['constructor_reference'] for c in callouts],
      'technical_illustration_identical':technical_identical,'technical_crop_sha256_before':before,'technical_crop_sha256_after':after,
      'canvas_extra_middle_px':middle_extra,'canvas_extra_right_px':right_extra,'canvas_total_extra_px':total_extra,
      'missing_translations':missing,'overflow_items':sorted(set(overflow)),'text_truncation_allowed':False,
      'pass':technical_identical and not missing and not overflow and len(callouts)==14
    }
    (root/'mems_display_sim_manifest.json').write_text(json.dumps(sim,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps(sim,ensure_ascii=False))
    if not sim['pass']: raise SystemExit('MEMS display simulation gate failed')

if __name__=='__main__': main()
