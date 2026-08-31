from pathlib import Path

p = Path('tools/rave_v2_new_extraction.py')
s = p.read_text(encoding='utf-8')

# Repair the one transfer corruption present in the pilot source.
s = s.replace("zone_rects=[fitz.Rect({['bbox']) for z in zones]", "zone_rects=[fitz.Rect(z['bbox']) for z in zones]")

# Preserve native PDF text colour in extracted spans and diagram zones.
s = s.replace(
    "parts.append(t); spans.append({'text':t,'bbox':[round(float(v),3) for v in s['bbox']],'font':s.get('font'),'size':float(s.get('size') or 8)})",
    "parts.append(t); spans.append({'text':t,'bbox':[round(float(v),3) for v in s['bbox']],'font':s.get('font'),'size':float(s.get('size') or 8),'color':int(s.get('color') or 0)})"
)
s = s.replace(
    "seen.add(key); z.append({'key':None,'source_text':t,'bbox':s['bbox'],'source_font_size':s['size'],'status':'translation_required'})",
    "seen.add(key); z.append({'key':None,'source_text':t,'bbox':s['bbox'],'source_font_size':s['size'],'source_color':int(s.get('color') or 0),'status':'translation_required'})"
)

start = s.index('def render(page,blks,zones,translations,block_translations,out,n):')
end = s.index('\ndef main():', start)

new_render = r'''def pdf_color_rgb(value):
    value=int(value or 0)
    return ((value >> 16) & 255, (value >> 8) & 255, value & 255)

def span_union(block):
    boxes=[x.get('bbox') for x in block.get('spans',[]) if x.get('bbox')]
    if not boxes:
        return block['bbox']
    return [min(b[0] for b in boxes),min(b[1] for b in boxes),max(b[2] for b in boxes),max(b[3] for b in boxes)]

def render(page,blks,zones,translations,block_translations,out,n):
    scale=160/72
    pix=page.get_pixmap(matrix=fitz.Matrix(scale,scale),alpha=False)
    img=Image.frombytes('RGB',[pix.width,pix.height],pix.samples)
    original=out/f'RCL0193ENG_P{n:03d}_ORIGINAL.png'
    img.save(original)

    # The translated language may need more room. Do not shrink the technical drawing:
    # extend free canvas around it. Page 112 is the pilot proving this behaviour.
    left_extra=int(160*scale) if n==112 else 0
    right_extra=int(130*scale) if n==112 else 0
    canvas=Image.new('RGB',(img.width+left_extra+right_extra,img.height),'white')
    ox=left_extra
    canvas.paste(img,(ox,0))
    d=ImageDraw.Draw(canvas)
    mid=page.rect.width/2
    zone_rects=[fitz.Rect(z['bbox']) for z in zones]

    # Translate ordinary page text while preserving source style colour. Erase only the
    # native text span union, not the complete PDF block, so nearby pictograms remain intact.
    for b in blks:
        br=fitz.Rect(b['bbox'])
        if sum(1 for zr in zone_rects if overlap(br,zr))>=2:
            continue
        key=f'RCL0193ENG_P{n:03d}_B{b["block_index"]:02d}'
        tr=block_translations.get(key)
        if not tr:
            continue
        ub=span_union(b)
        x0,y0,x1,y1=[int(v*scale) for v in ub]
        x0+=ox; x1+=ox
        pad=max(2,int(.8*scale))
        d.rectangle((x0-pad,y0-pad,x1+pad,y1+pad),fill='white')
        base=max((sp.get('size',8) for sp in b['spans']),default=8)*scale*.92
        bold=any(token in b['text'] for token in ['ENGINE MANAGEMENT SYSTEM','FUEL SYSTEM','Engine Control Module','NOTE:']) or b['block_index'] in (0,2,3)
        fill=pdf_color_rgb(next((sp.get('color',0) for sp in b['spans'] if sp.get('text','').strip()),0))
        f,lines,lh=fit_text(d,tr,(x0,y0,x1,y1),base,bold=bold,min_px=max(11,int(6.5*scale)))
        yy=y0
        for line in lines:
            d.text((x0,yy),line,fill=fill,font=f)
            yy+=lh

    # On the pilot diagram, widen only the free label containers outwards. The ECM box,
    # connection lines and technical core stay at their original coordinates.
    if n==112:
        ytop=int(469.5*scale); ybot=int(646.0*scale)
        old_left=int(57.0*scale)+ox; left_inner=int(256.0*scale)+ox
        right_inner=int(342.0*scale)+ox; old_right=int(535.0*scale)+ox
        new_left=18; new_right=canvas.width-18
        # remove only old outer vertical borders
        d.rectangle((old_left-2,ytop,old_left+2,ybot),fill='white')
        d.rectangle((old_right-2,ytop,old_right+2,ybot),fill='white')
        # extend the original label boxes outwards
        d.line((new_left,ytop,left_inner,ytop),fill='black',width=2)
        d.line((new_left,ybot,left_inner,ybot),fill='black',width=2)
        d.line((new_left,ytop,new_left,ybot),fill='black',width=2)
        d.line((right_inner,ytop,new_right,ytop),fill='black',width=2)
        d.line((right_inner,ybot,new_right,ybot),fill='black',width=2)
        d.line((new_right,ytop,new_right,ybot),fill='black',width=2)
    else:
        new_left=None; new_right=None; left_inner=None; right_inner=None

    # Replace human-language labels directly. Existing constructor references remain intact;
    # no artificial visible numbering is introduced. Preserve the source text colour.
    for z in zones:
        tr=translations.get(z['key'])
        if not tr:
            continue
        prefix,_=leading_reference(z['source_text'])
        if prefix and not tr.lstrip().startswith(prefix.strip()):
            tr=prefix+tr
        x0,y0,x1,y1=[int(v*scale) for v in z['bbox']]
        x0+=ox; x1+=ox
        pad=max(2,int(.8*scale))
        d.rectangle((x0-pad,y0-pad,x1+pad,y1+pad),fill='white')
        base=max(12,min(17,z.get('source_font_size',8)*scale*.92))
        f=getfont(base)
        fill=pdf_color_rgb(z.get('source_color',0))
        th=d.textbbox((0,0),'Ag',font=f)[3]
        cx=(z['bbox'][0]+z['bbox'][2])/2

        if n==112 and z['key']=='RCL0193ENG_P112_T01':
            avail_left=new_left+8; avail_right=left_inner-8
            tw=d.textbbox((0,0),tr,font=f)[2]
            tx=avail_left+max(0,(avail_right-avail_left-tw)//2)
        elif n==112 and z['key']=='RCL0193ENG_P112_T02':
            avail_left=right_inner+8; avail_right=new_right-8
            tw=d.textbbox((0,0),tr,font=f)[2]
            tx=avail_left+max(0,(avail_right-avail_left-tw)//2)
        elif n==112 and cx<mid*.86:
            avail_left=new_left+12; avail_right=left_inner-10
            f,lines,lh=fit_text(d,tr,(avail_left,y0,avail_right,y1),base,min_px=11,max_lines=2)
            yy=y0+max(0,((y1-y0)-lh*len(lines))//2)
            for line in lines:
                d.text((avail_left,yy),line,fill=fill,font=f); yy+=lh
            continue
        elif n==112 and cx>mid*1.14:
            avail_left=right_inner+12; avail_right=new_right-12
            f,lines,lh=fit_text(d,tr,(avail_left,y0,avail_right,y1),base,min_px=11,max_lines=2)
            yy=y0+max(0,((y1-y0)-lh*len(lines))//2)
            for line in lines:
                d.text((avail_left,yy),line,fill=fill,font=f); yy+=lh
            continue
        else:
            tw=d.textbbox((0,0),tr,font=f)[2]
            tx=x0
        tw=d.textbbox((0,0),tr,font=f)[2]
        tx=max(4,min(tx,canvas.width-tw-4))
        ty=y0+max(0,((y1-y0)-th)//2)-1
        d.text((tx,ty),tr,fill=fill,font=f)

    localized=out/f'RCL0193ENG_P{n:03d}_FR_DIRECT_TEXT.png'
    canvas.save(localized)
    return original,localized,left_extra,right_extra
'''

s = s[:start] + new_render + s[end:]
p.write_text(s, encoding='utf-8')
