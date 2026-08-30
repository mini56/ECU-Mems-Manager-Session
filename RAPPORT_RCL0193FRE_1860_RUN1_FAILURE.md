# RCL0193FRE — lot 1860 — échec run 1

Date : 2026-08-30.

## Run
- GitHub Actions : `33302467286`.
- Commit temporaire : `908da678c67dcc6066a8991b69b0feb6e7923cdc`.
- `MEMSX64` vérifié intact sur BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`.

## Résultat
- Checkout : ✅
- Vérification production #101 : ✅
- Pillow : ✅
- Installation/validation 1860 : ❌ avant toute modification finale.
- Commit final 1860 : non exécuté.
- Vérification distante : non exécutée.

## Cause exacte
Le garde SHA du transport TIFF a correctement bloqué le lot :
- SHA xz attendu : `fbd982995304a41f978fc61280b83fbead120591e898bb66716ad70685da9a08`.
- SHA xz obtenu après décodage du blob transport : `6a8f5c154a8b6c995d4c3f169d6dbb0e7581eff99a33df3a4a07d301e78c6eb6`.

Conclusion : le fichier Base64 rattaché au commit temporaire n'est pas le transport TIFF complet validé localement (fragment/incomplétude de transport). Les données 1860/QZ64 ne sont pas remises en cause et aucun fichier final n'a été committé.

## Prochaine action
Retrouver le transport TIFF G4 complet déjà généré localement, vérifier son SHA xz `fbd982...` et TIFF `d25fd3...`, remplacer uniquement le transport temporaire, journaliser avant nouvelle pousse, puis relancer le même validateur sans modifier le contenu technique 1860.
