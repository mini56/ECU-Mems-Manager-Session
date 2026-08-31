from pathlib import Path

p = Path('RAPPORT_SUIVI_ECU_MEMS_MANAGER.md')
s = p.read_text(encoding='utf-8', errors='strict')
marker = '## 2026-08-31 - RAVEMEMS TEST2 - DIAGNOSTIC OCR AFFINE AVANT CORRECTION'
if marker in s:
    raise SystemExit('diagnostic already recorded')
appendix = r'''
## 2026-08-31 - RAVEMEMS TEST2 - DIAGNOSTIC OCR AFFINE AVANT CORRECTION

Inspection de l'artefact du run `33442157473` (ID `9776650909`) : la région OCR `CDXN_P007_PIX_R001`, bbox `[1216, 100, 2150, 143]`, contient tous les mots du titre mais le texte enregistré est `TO USE HOW THE CIRCUIT DIAGRAMS`. Le problème n'est donc pas, pour ce titre précis, un réel découpage en plusieurs paragraphes Tesseract comme supposé dans le diagnostic initial.

Cause affinée : `paragraph_entries()` reprend les mots Tesseract puis les retrie avec `sorted(words, key=lambda x: (x["top"], x["left"]))`. Les quelques pixels de variation verticale des grandes lettres font passer `TO USE` avant `HOW`, alors que l'ordre natif fourni par Tesseract est bien `HOW TO USE THE CIRCUIT DIAGRAMS`. Le même tri peut également désordonner des phrases multilignes ailleurs sur cette page.

### CORRECTION AUTORISEE AVANT LA PROCHAINE POUSSE

Modifier uniquement l'association OCR dans `tools/ravemems_test2_raster_text.py` afin de conserver l'ordre natif des mots fourni par Tesseract à l'intérieur de chaque groupe bloc/paragraphe, au lieu de les retrier par coordonnées. Ne modifier ni les traductions, ni les masques, ni les règles de conservation pixel, ni la source `cdxn990e.pdf` page 7, ni `MEMSX64`. Relancer ensuite exactement le même TEST2 et inspecter le rendu réel avant toute validation.
'''
p.write_text(s.rstrip('\n') + '\n\n' + appendix.strip() + '\n', encoding='utf-8', errors='strict', newline='\n')
check = p.read_text(encoding='utf-8', errors='strict')
assert marker in check
assert 'TO USE HOW THE CIRCUIT DIAGRAMS' in check
assert 'HOW TO USE THE CIRCUIT DIAGRAMS' in check
print('REPORT_OCR_DIAGNOSTIC_APPEND_OK')
