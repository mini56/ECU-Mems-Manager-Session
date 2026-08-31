from pathlib import Path

p = Path('RAPPORT_SUIVI_ECU_MEMS_MANAGER.md')
payload = b'''\n\n## RAVEMEMS TEST2 - ECHEC GARDE DE COMPLETUDE - NAMEERROR\n\n- Branche pilote : `tmp-rave-new-extraction-pilot`.\n- HEAD teste : `9936c111e18bc01ec966346cdb43fc69c8fef2b5`.\n- Run TEST2 reutilise : `33442157473`, job `99662651982`.\n- Checkout confirme dans les logs : `9936c111e18bc01ec966346cdb43fc69c8fef2b5`.\n- Resultat : **ECHEC** pendant `Run RAVEMEMS TEST2 on canonical raster-only page`.\n- Erreur exacte : `NameError: name 'ocr_words' is not defined` a la ligne appelant `localized_words = ocr_words(rendered)`.\n- Le rendu avait ete execute jusqu'au nouveau controle de completude ; l'artefact a quand meme ete charge, ID `9777755805`, SHA-256 ZIP `9a522088d00f90cee11af0d36ead9d5efbae4b72a68c77ccceebf7e34f9bdd82`.\n- Ce defaut appartient au garde de completude ajoute, pas a une preuve de regression du dessin.\n- Aucun verdict visuel nouveau n'est possible tant que ce garde ne s'execute pas.\n- `MEMSX64` reste totalement inchange.\n\n### PROCHAINE ACTION EXACTE\nIdentifier dans `tools/ravemems_test2_raster_text.py` le helper OCR existant et remplacer uniquement l'appel inexistant `ocr_words(rendered)` par l'appel correct, sans modifier le rendu ni les traductions. Relancer ensuite le meme TEST2 et inspecter manuellement l'artefact avant toute validation.\n'''
raw = p.read_bytes()
if b'## RAVEMEMS TEST2 - ECHEC GARDE DE COMPLETUDE - NAMEERROR' in raw:
    raise SystemExit('entry already present')
p.write_bytes(raw + payload)
