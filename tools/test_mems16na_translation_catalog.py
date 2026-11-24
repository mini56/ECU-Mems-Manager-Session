from pathlib import Path
import json
keys={str(n) for n in range(29000,29049)}
for lang in ('fr','en','de','es','it','pt'):
    root=Path('translations',lang)
    target=root/f'{lang}_mems16na.json'
    d=json.loads(target.read_text(encoding='utf-8'))
    assert set(d)==keys and all(isinstance(v,str) and v.strip() for v in d.values()),lang
    for p in list(root.glob('*.json'))+list(Path('translations').glob(lang+'*.json')):
        if p!=target:
            assert not(keys & set(json.loads(p.read_text(encoding='utf-8-sig')))),p
    for k in ('29032','29034','29040'): assert d[k].count('%1')==1,(lang,k)
print('MEMS16NA_TRANSLATIONS_OK: 49 keys, six languages, no ID collisions')
