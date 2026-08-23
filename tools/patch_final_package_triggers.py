#!/usr/bin/env python3
from pathlib import Path

path = Path('.github/workflows/ia-mems-final-package.yml')
text = path.read_text(encoding='utf-8')

anchor = '      - "expert/**"\n      - "database/reference/manifest.json"\n'
replacement = ('      - "expert/**"\n'
               '      - "database/MemsReferenceDatabase.cpp"\n'
               '      - "database/reference/manifest.json"\n'
               '      - "database/reference/research_enrichment_1620.qz64"\n')

if '      - "database/MemsReferenceDatabase.cpp"\n' not in text:
    if anchor not in text:
        raise SystemExit('final package trigger anchor not found')
    text = text.replace(anchor, replacement, 1)

path.write_text(text, encoding='utf-8')
print('Final IA MEMS package now tracks main reference loader and Rover 1620')
