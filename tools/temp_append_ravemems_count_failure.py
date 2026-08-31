from pathlib import Path

p = Path('RAPPORT_SUIVI_ECU_MEMS_MANAGER.md')
payload = b'''\n\n## RAVEMEMS TEST2 - ECHEC DU CORRECTIF DE COMPLETUDE - RUN 33444635412\n\n- Branche pilote : `tmp-rave-new-extraction-pilot`.\n- HEAD avant tentative : `b4d59b6c7d80d8e9aee0514d31daf3f9d779f8f9`.\n- Objectif : corriger uniquement les quatre defauts visuels releves apres le run TEST2 vert mais refuse manuellement, puis ajouter un garde de completude linguistique.\n- Resultat reel : **ECHEC avant toute modification finale du pilote**.\n- Etape en echec : `Fix TEST2 translation fixture`.\n- Erreur exacte : `AssertionError: 34` sur `assert len(check['operations']) == 35`.\n- Cause : le garde de comptage du script temporaire etait faux. Le calcul exact est `33 operations initiales - 1 doublon R042 + 1 operation R026 + 1 remplacement EARTH = 34 operations`.\n- Aucun commit final de correction n'a ete cree par ce run.\n- Aucun nouveau TEST2 n'a ete execute apres cet echec.\n- `MEMSX64` reste totalement inchange.\n\n### PROCHAINE ACTION EXACTE\nCorriger uniquement le garde du script temporaire de `35` vers `34`, relancer le meme correctif de completude, verifier le commit final obtenu, puis relancer TEST2 sur la page raster canonique et inspecter manuellement le PNG avant toute validation.\n'''
raw = p.read_bytes()
if b'## RAVEMEMS TEST2 - ECHEC DU CORRECTIF DE COMPLETUDE - RUN 33444635412' in raw:
    raise SystemExit('entry already present')
p.write_bytes(raw + payload)
