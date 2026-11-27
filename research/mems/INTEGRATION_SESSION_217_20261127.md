# SESSION 217 — lot 1 / V1.30
Base 488dc8cd7dc6a8ec6e25bf8905597fb36e644006. Autorisation utilisateur 27/11/2026. Branche unique fix-documentary-additive-session-161-20261118.
| Action | Statut source | Validation |
|---|---|---|
| V130 correction injection µs / libellé | Intégrée sur profil NA reconnu ou KBHBG009/KBHBV001 avec trame valide | CI à exécuter, véhicule à tester |
| V130 cadran lambda % et bulle | Intégrée pour encodage standard reconnu ; autre profil historique | CI / véhicule |
| V129 régime 25 tr/min | Unité et aide corrigées ; conversion déjà présente préservée | CI / véhicule |
| V129 position chaude | Affichage absolu sur profil prouvé et trame valide ; aucune écriture CB | CI / véhicule |
| ACT-090 TPS | Comparaison tautologique supprimée, état non évalué sans défaut reconnu | CI / véhicule |
| ACT-004 lambda passive | Observation des transitions AANMP002 et KBHBG009, aucune commande | CI / véhicule |
| ACT-001 température moteur passive | Variation naturelle et défaut mémorisé ; substitution actuelle non démontrée | CI / véhicule |
| Reconnexion/journaux | Sources préexistantes conservées, test politique et journal dans CI | CI / véhicule |
| ACT-008/009/013 | Non intégrés (tests actifs lambda/jauge et CB) | banc et autorisation |
| V130 progression adaptation | Non intégrée, accès compteurs à confirmer | lecture seulement |
| ACT-106 et autres actions | À auditer / intégrer sans doublon | lots suivants |
Les seuils d'observation 300/600 mV et 3 pas température ne constituent pas des seuils constructeur ni un verdict de santé. Un résultat non concluant n'est pas une panne. Pas de changement de DLL, commande ni branche de recherche. CI et validation réelle restent distinctes.

## Suivi CI
- Commit initial 6203853bb7a30714ed883c9c0f7d738754540623, run 45978630149, tentatives 1 et 2 : échec du job tests sans aucune étape listée et journaux inaccessibles (BlobNotFound). Windows ignoré. Cause exacte non accessible. Aucun Setup validé.
- Revue source supplémentaire : export des diagnostics passifs MEMS 1.9, cadran sans valeur ni aiguille lorsque la correction n'est pas disponible, test synthétique MEMS 1.9 et égalité des clés des six catalogues. Nouvelle validation à exécuter. Aucune validation terrain.
