#!/usr/bin/env python3
from pathlib import Path
import json,re,xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parents[1]; TR=ROOT/'translations'

def text(el): return '' if el is None else ''.join(el.itertext())
def load_ts(lang):
    root=ET.parse(TR/f'ECUMemsManager_{lang}.ts').getroot(); out={}
    for ctx in root.findall('context'):
        for msg in ctx.findall('message'):
            src=text(msg.find('source')); val=text(msg.find('translation')) or src
            if src: out[src]=val
    return out

en_ts=load_ts('en'); fr_ts=load_ts('fr')
p=ROOT/'diagnosticpanel.cpp'; source=p.read_text(encoding='utf-8')
pat=re.compile(r'\b(?:tr|I18n::text)\(\s*"((?:\\.|[^"\\])*)"\s*\)')
matches=list(pat.finditer(source))
if not matches: raise SystemExit('No simple DiagnosticPanel strings found')
if len(matches)>190: raise SystemExit(f'Too many DiagnosticPanel strings: {len(matches)}')
out_en={}; out_fr={}; idx=[0]
def decode(raw):
    return raw.replace('\\n','\n').replace('\\"','"').replace('\\\\','\\')
def repl(m):
    idx[0]+=1; key=6799+idx[0]; src=decode(m.group(1))
    en=en_ts.get(src,src); fr=fr_ts.get(src,src)
    out_en[str(key)]=en; out_fr[str(key)]=fr
    safe=en.replace('*/','* /').replace('\n',' ')
    return f'I18n::text({key}) /* EN: {safe} */'
source=pat.sub(repl,source)
p.write_text(source,encoding='utf-8')
(TR/'en_diagnostic.json').write_text(json.dumps(out_en,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(TR/'fr_diagnostic.json').write_text(json.dumps(out_fr,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(f'DiagnosticPanel migrated: {idx[0]} strings, keys 6800-{6799+idx[0]}')
