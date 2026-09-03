## 2026-09-03 — RAVEMEMS V2 : lancement traitement de masse corpus RAVE

Demande utilisateur : lancer maintenant le traitement de masse du corpus RAVE sur GitHub.

Périmètre verrouillé avant pousse :
- moteur générique neutralisé exact : `745f41243adb202f638f4039016dedfff8f272d6` ;
- source documentaire figée : commit `643de091b474f4e27917a065bdf46d5a0c764276` ;
- entrée : ensemble complet des PDF sous `rave/` au commit source figé ;
- aucune adaptation, aucun profil et aucune correction spécifique PDF par PDF pendant ce run ;
- le workflow doit matérialiser le corpus, lancer le moteur générique, conserver un manifeste global et effectuer le replay visuel/audit sur chaque extraction réussie ;
- un PDF non compris doit être signalé comme échec réel, pas contourné ;
- `MEMSX64` reste protégé sur BUILD #103 SHA `1d6316bd1746d6f2b4cfb751cab88d18e27ef730` ; aucun BUILD #104.

Prochaine action exacte : pousser le workflow temporaire de traitement complet sur `tmp-ravemems-v2-foundation`, puis relever immédiatement le run GitHub Actions créé.
