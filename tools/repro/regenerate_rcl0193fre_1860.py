#!/usr/bin/env python3
from pathlib import Path
import argparse, base64, hashlib, json, lzma, zlib
import fitz
from PIL import Image

SELECTED=[324,326,328,329,330,331,332,333,334,335,336,337,338,339,340,341,342,343,344,345,346,347,348,349,350,351,352,353,354,355,356,357,358,360,361,362,363,364,365,366,367,368,369,370,371]
SOURCE_SHA='0c7fef28d0d0f0673ba321d6625a019c005823103caa98afb3258114e1fec713'

def sha_bytes(b): return hashlib.sha256(b).hexdigest()
def sha_file(p): return sha_bytes(Path(p).read_bytes())

def make_qz(sql_bytes):
    raw=len(sql_bytes).to_bytes(4,'big') + zlib.compress(sql_bytes,9)
    return base64.encodebytes(raw)

def build(pdf, sql, out):
    pdf=Path(pdf); sql=Path(sql); out=Path(out); out.mkdir(parents=True,exist_ok=True)
    assert sha_file(pdf)==SOURCE_SHA,(sha_file(pdf),SOURCE_SHA)
    sql_bytes=sql.read_bytes(); qz=make_qz(sql_bytes)
    (out/'research_enrichment_1860.sql').write_bytes(sql_bytes)
    (out/'research_enrichment_1860.qz64').write_bytes(qz)
    doc=fitz.open(pdf); frames=[]; png_hashes={}
    for n in SELECTED:
        pix=doc[n-1].get_pixmap(dpi=150,colorspace=fitz.csGRAY,alpha=False)
        im=Image.frombytes('L',(pix.width,pix.height),pix.samples).convert('1')
        assert im.size==(1240,1755) and im.mode=='1'
        frames.append(im.copy())
        p=out/f'RCL0193FRE_PDF_{n:03d}.png'; im.save(p,'PNG',optimize=True); png_hashes[str(n)]=sha_file(p)
    tif=out/'rcl0193fre_1860_g4_150.tif'
    frames[0].save(tif,'TIFF',save_all=True,append_images=frames[1:],compression='group4',dpi=(150,150))
    xz=out/'rcl0193fre_1860_g4_150.tif.xz'
    xz.write_bytes(lzma.compress(tif.read_bytes(),format=lzma.FORMAT_XZ,preset=9|lzma.PRESET_EXTREME))
    manifest={
      'lot':1860,'document':'RCL0193FRE','source_pdf_sha256':SOURCE_SHA,'source_pdf_size':pdf.stat().st_size,
      'selected_pdf_pages':SELECTED,'excluded_between_324_371':[325,327,359],
      'render':{'PyMuPDF':getattr(fitz,'__version__','unknown'),'Pillow':Image.__version__ if hasattr(Image,'__version__') else 'unknown','dpi':150,'colorspace':'GRAY','convert_mode':'1-default-dither','tiff_compression':'group4','tiff_dpi':[150,150],'xz':'python-lzma preset=9|PRESET_EXTREME'},
      'files':{},'png_sha256':png_hashes,
      'expected_semantic_counts':{'knowledge':112,'scope':112,'specification':18,'specification_value':18,'procedure':100,'procedure_step':516,'procedure_requirement':29,'knowledge_relation':79,'term_alias_effective':64},
      'historical_unreproduced_candidate_hashes':{'sql':'1cf7cd58e71d436fc701401d841f0a4b521c9f386ff23a5720d8fb7a5b86fd54','qz64':'18e859c236a7e865b3416e0083f024e872bebda540c2dbf15ce233c0900c18be','tiff':'d25fd347ccf9fd795c635fffff5a0dcc2800bbe33e185416c9a188d034b8f2c4','xz':'fbd982995304a41f978fc61280b83fbead120591e898bb66716ad70685da9a08'},
      'classification':'audited deterministic regeneration from exact source; not claimed byte-identical to lost historical candidate'
    }
    for name in ['research_enrichment_1860.sql','research_enrichment_1860.qz64','rcl0193fre_1860_g4_150.tif','rcl0193fre_1860_g4_150.tif.xz']:
        p=out/name; manifest['files'][name]={'size':p.stat().st_size,'sha256':sha_file(p)}
    (out/'recovery_manifest_1860.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(manifest['files'],indent=2))

if __name__=='__main__':
    ap=argparse.ArgumentParser(); ap.add_argument('pdf'); ap.add_argument('sql'); ap.add_argument('out'); a=ap.parse_args(); build(a.pdf,a.sql,a.out)
