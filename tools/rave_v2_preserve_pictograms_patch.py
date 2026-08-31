from pathlib import Path

p=Path('tools/rave_v2_new_extraction.py')
s=p.read_text(encoding='utf-8')
needle="""            d.text((x0,yy),line,fill=fill,font=f)\n            yy+=lh\n\n    # On the pilot diagram, widen only the free label containers outwards."""
replacement="""            d.text((x0,yy),line,fill=fill,font=f)\n            yy+=lh\n\n    # Restore constructor pictograms that share a PDF text block with translated text.\n    # They are graphical source content, not language, so they remain pixel-identical.\n    if n==112:\n        ix0,iy0,ix1,iy1=[int(v*scale) for v in (318.0,337.0,349.0,374.0)]\n        icon=img.crop((ix0,iy0,ix1,iy1))\n        canvas.paste(icon,(ox+ix0,iy0))\n\n    # On the pilot diagram, widen only the free label containers outwards."""
if needle not in s:
    raise SystemExit('pictogram insertion point not found')
s=s.replace(needle,replacement,1)
p.write_text(s,encoding='utf-8')
