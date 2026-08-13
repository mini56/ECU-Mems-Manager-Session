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

def read_json(path): return json.loads(path.read_text(encoding='utf-8')) if path.exists() else {}
def decode_literal(raw): return raw.replace('\\n','\n').replace('\\"','"').replace('\\\\','\\')

en_ts=load_ts('en'); fr_ts=load_ts('fr')
pat=re.compile(r'\btr\s*\(\s*((?:"(?:\\.|[^"\\])*"\s*)+)\)',re.S)
litpat=re.compile(r'"((?:\\.|[^"\\])*)"')
modules=[
 ('summarytab.cpp',6687,'summary'),
 ('diagnosticpanel.cpp',6897,'diagnostic'),
 ('mainwindow.cpp',7079,'mainwindow'),
 ('optionsdialog.cpp',6108,'options')
]
for filename,start,name in modules:
    p=ROOT/filename; source=p.read_text(encoding='utf-8')
    matches=list(pat.finditer(source))
    if not matches:
        print(f'{filename}: 0 multiline tr strings'); continue
    en_path=TR/f'en_{name}.json'; fr_path=TR/f'fr_{name}.json'
    out_en=read_json(en_path); out_fr=read_json(fr_path); idx=[0]
    def repl(m):
        idx[0]+=1; key=start+idx[0]-1
        parts=litpat.findall(m.group(1)); src=''.join(decode_literal(x) for x in parts)
        en=en_ts.get(src,src); fr=fr_ts.get(src,src)
        out_en[str(key)]=en; out_fr[str(key)]=fr
        safe=en.replace('*/','* /').replace('\n',' ')
        return f'I18n::text({key}) /* EN: {safe} */'
    source=pat.sub(repl,source)
    p.write_text(source,encoding='utf-8')
    en_path.write_text(json.dumps(out_en,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    fr_path.write_text(json.dumps(out_fr,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(f'{filename}: migrated {idx[0]} multiline strings, keys {start}-{start+idx[0]-1}')
