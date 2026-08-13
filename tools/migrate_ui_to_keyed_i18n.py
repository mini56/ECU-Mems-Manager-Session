#!/usr/bin/env python3
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]
active={'main.cpp','mainwindow.cpp','optionsdialog.cpp','summarytab.cpp','diagnosticpanel.cpp','analysistab.cpp','captureviewer.cpp','helpviewer.cpp','aboutbox.cpp'}
patterns=[('tr-call',re.compile(r'\btr\s*\(')),('text-literal',re.compile(r'I18n::text\s*\(\s*"'))]
count=0
for name in sorted(active):
    p=ROOT/name
    if not p.exists(): continue
    for no,line in enumerate(p.read_text(encoding='utf-8',errors='replace').splitlines(),1):
        for kind,pat in patterns:
            if pat.search(line):
                count+=1
                print(f'{kind}: {name}:{no}: {line.strip()[:180]}')
print(f'ACTIVE_REMAINING_CALL_LINES={count}')
