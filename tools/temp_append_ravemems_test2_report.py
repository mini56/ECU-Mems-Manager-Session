from pathlib import Path

p = Path('RAPPORT_SUIVI_ECU_MEMS_MANAGER.md')
s = p.read_text(encoding='utf-8', errors='strict')
marker = '## 2026-08-31 - RAVEMEMS TEST2 - PREMIER RUN GITHUB EN ECHEC AVANT RENDU'
if marker in s:
    raise SystemExit('TEST2 failure already recorded; refusing duplicate append')

appendix = r'''
## 2026-08-31 - RAVEMEMS TEST2 - PREMIER RUN GITHUB EN ECHEC AVANT RENDU

TEST2 a été lancé sur la branche `tmp-rave-new-extraction-pilot`, commit `a19b1aa233bc8870e15a1e9dd5b49a5cfbfce7ab` (`Add RAVEMEMS TEST2 raster embedded-text pilot`). Source canonique testée : `rave/xn/cdxn990e.pdf`, page physique 7, cas raster seul avec texte humain intégré dans les pixels et absence de couche texte PDF native.

GitHub Actions : workflow `RAVEMEMS TEST2 raster embedded text`, run `33442157473`, run_number 1. Conclusion : **FAILURE avant génération du rendu localisé**. L'artefact de diagnostic a néanmoins été conservé : ID `9776650909`, nom `ravemems-test2-cdxn-page7-raster-text`, digest ZIP SHA-256 `1c80b9bccb187f6f23947d69486043c9663dd3ab6c65c16302974e7590c533d4`.

Cause exacte : Tesseract a bien extrait le raster et du texte, mais sur le runner GitHub le titre `HOW TO USE THE CIRCUIT DIAGRAMS` n'a pas été regroupé en un unique paragraphe OCR. Le garde actuel `find_paragraph()` exigeait un bloc/paragraphe unique contenant le titre complet et a donc arrêté le test avec `expected 1 match, got 0`. Le test s'est arrêté avant tout masquage/rendu ; aucune page localisée n'est donc validée par ce run.

Ce résultat ne remet pas en cause la règle `ravemems` ni le recours OCR de dernier ressort pour ce type de page : il montre que les zones raster ne doivent pas dépendre du découpage arbitraire `block_num/par_num` produit par Tesseract. La correction doit rendre l'association robuste au morcellement OCR en recherchant une séquence de mots/une zone géométrique, sans relâcher les gardes de conservation des repères techniques et sans changer la source ou la méthode de rendu.

### Incident de journalisation immédiatement traité

La première tentative de consignation automatique du présent échec a créé le commit temporaire `e280ab6f1353f4e41dfc40e2f5473dbde2269f44`, mais le workflow de journalisation run `33442743367` a été rejeté avant création de job. Le rapport n'avait donc pas été modifié par cette tentative. Conformément à la règle fondamentale du rapport, toute correction de TEST2 est restée suspendue jusqu'au rétablissement du canal de journalisation. Le canal a été simplifié en workflow minimal + script Python UTF-8 séparé avant de poursuivre.

### PROCHAINE ACTION EXACTE

Corriger uniquement la résolution des zones OCR du TEST2 pour accepter un titre/texte réparti sur plusieurs blocs Tesseract, puis relancer le même TEST2 sur `cdxn990e.pdf` page 7. Conserver les gardes : source canonique, zéro texte PDF natif, repères techniques immuables pixel-identiques, aucune numérotation artificielle visible, aucun texte tronqué et aucun changement hors zones de traduction. `MEMSX64` reste inchangé BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`.
'''

new = s.rstrip('\n') + '\n\n' + appendix.strip() + '\n'
p.write_text(new, encoding='utf-8', errors='strict', newline='\n')
check = p.read_text(encoding='utf-8', errors='strict')
for token in (marker, '33442157473', '33442743367', 'a19b1aa233bc8870e15a1e9dd5b49a5cfbfce7ab'):
    assert token in check, token
print('REPORT_TEST2_FAILURE_APPEND_OK')
