#!/usr/bin/env python3
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]
patterns=[
    ('tr-call',re.compile(r'\btr\s*\(')),
    ('text-literal',re.compile(r'I18n::text\s*\(\s*"')),
]
count=0
for p in sorted(list(ROOT.glob('*.cpp'))+list(ROOT.glob('*.h'))):
    if p.name in {'i18n.cpp','i18n.h'}: continue
    lines=p.read_text(encoding='utf-8',errors='replace').splitlines()
    for no,line in enumerate(lines,1):
        for name,pat in patterns:
            if pat.search(line):
                count+=1
                print(f'{name}: {p.name}:{no}: {line.strip()[:180]}')
print(f'REMAINING_CALL_LINES={count}')
