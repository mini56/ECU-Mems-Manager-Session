#!/usr/bin/env python3
from pathlib import Path
import json,re
ROOT=Path(__file__).resolve().parents[1]; TR=ROOT/'translations'
def read_json(p): return json.loads(p.read_text(encoding='utf-8')) if p.exists() else {}
p=ROOT/'analysistab.cpp'
source=p.read_text(encoding='utf-8')
# Migrate the 40 friendlyColumnName strings in source order only.
start=source.index('static QString friendlyColumnName')
end=source.index('//=============================================================================',start)
block=source[start:end]
pat=re.compile(r'I18n::text\("(?:\\.|[^"\\])*"\)')
matches=list(pat.finditer(block))
if len(matches)!=40: raise SystemExit(f'Analysis label count unexpected: {len(matches)}')
en=read_json(TR/'en_analysis.json'); idx=[0]
def repl(m):
 idx[0]+=1; key=6399+idx[0]; label=en.get(str(key),'')
 return f'I18n::text({key}) /* EN: {label} */'
block=pat.sub(repl,block)
source=source[:start]+block+source[end:]
p.write_text(source,encoding='utf-8')
print('Analysis labels migrated: 40')
