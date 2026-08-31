from pathlib import Path

p = Path('RAPPORT_SUIVI_ECU_MEMS_MANAGER.md')
payload = b'''\n\n## RAVEMEMS TEST2 - DIAGNOSTIC DU HELPER OCR APRES NAMEERROR\n\n- Inspection de `tools/ravemems_test2_raster_text.py` au HEAD pilote `9936c111e18bc01ec966346cdb43fc69c8fef2b5`.\n- Resultat : **aucun helper OCR reutilisable n'existe actuellement**.\n- L'OCR source est code directement dans `main()` avec `pytesseract.image_to_data(image, lang="eng", config="--psm 3", output_type=Output.DICT)`, filtre de confiance `>= 25` et construction de la liste `words`.\n- Le nouvel appel `ocr_words(rendered)` est donc un appel vers une fonction inexistante.\n- Correction propre retenue : factoriser exactement ce bloc OCR existant dans une fonction `ocr_words(image)` sans changer langue, PSM, seuil, champs ou ordre ; utiliser ensuite cette meme fonction pour l'image source et pour le garde de completude sur l'image localisee.\n- Aucun changement de traduction, bbox, geometrie, reference technique ou logique de rendu n'est autorise dans cette correction.\n- `MEMSX64` reste totalement inchange.\n\n### PROCHAINE ACTION EXACTE\nPousser uniquement la factorisation OCR identique ci-dessus sur `tmp-rave-new-extraction-pilot`, compiler, relancer le meme TEST2 sur la page canonique, puis inspecter manuellement le PNG avant toute validation.\n'''
raw = p.read_bytes()
if b'## RAVEMEMS TEST2 - DIAGNOSTIC DU HELPER OCR APRES NAMEERROR' in raw:
    raise SystemExit('entry already present')
p.write_bytes(raw + payload)
