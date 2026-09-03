#!/usr/bin/env python3
from __future__ import annotations
import argparse,json,subprocess,sys
from pathlib import Path
from typing import Any
from source_input import discover_pdf_sources,probe_pdf,source_kind

def _safe_output_name(identity:str,index:int)->str:
    cleaned=''.join(ch for ch in identity.upper() if ch.isalnum() or ch in {'-','_'})
    return (cleaned or f'DOCUMENT_{index:04d}')[:120]

def main()->int:
    p=argparse.ArgumentParser(description='Extract one or many PDFs from a PDF, directory or archive source'); p.add_argument('--source',type=Path,required=True); p.add_argument('--schema',type=Path,required=True); p.add_argument('--out',type=Path,required=True); a=p.parse_args()
    out=a.out.expanduser().resolve(); out.mkdir(parents=True,exist_ok=True); pdfs=discover_pdf_sources(a.source,out/'_resolved_source'); single=len(pdfs)==1; extractor=Path(__file__).resolve().with_name('ravemems_generic_precise_extract.py'); results=[]; failures=0
    for index,pdf in enumerate(pdfs,1):
        probe=probe_pdf(pdf); document_out=out if single else out/_safe_output_name(str(probe['identity']),index); document_out.mkdir(parents=True,exist_ok=True)
        completed=subprocess.run([sys.executable,str(extractor),'--source',str(pdf),'--schema',str(a.schema.expanduser().resolve()),'--out',str(document_out)],text=True)
        record:dict[str,Any]={'input_pdf':str(pdf),'document_identity':probe['identity'],'publication_code':probe['publication_code'],'language':probe['language'],'page_count':probe['page_count'],'output_directory':str(document_out),'return_code':completed.returncode}; manifest=document_out/'manifest.json'
        if manifest.exists(): record['manifest']=json.loads(manifest.read_text(encoding='utf-8'))
        if completed.returncode!=0: failures+=1
        results.append(record)
    source_manifest={'source':str(a.source.expanduser().resolve()),'source_kind':source_kind(a.source),'pdf_count':len(pdfs),'failure_count':failures,'results':results}; (out/'source_manifest.json').write_text(json.dumps(source_manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8'); print('SOURCE_KIND',source_manifest['source_kind']); print('SOURCE_PDF_COUNT',len(pdfs)); print('SOURCE_FAILURE_COUNT',failures); return 1 if failures else 0

if __name__=='__main__': raise SystemExit(main())
