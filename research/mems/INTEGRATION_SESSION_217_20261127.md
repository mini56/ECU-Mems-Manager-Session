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

- Commit logiciel final 04303fe1acd6af4c40e0a40bbc3d9bb31828b003, run 45978846416 : échec du job tests sans aucune étape listée, Windows ignoré. CI bloquée avant validation, cause exacte non accessible. Aucun Setup disponible ni validation sur véhicule. Ne pas passer au lot suivant avant recette et reprise du run. Les fonctions intégrées ne sont pas marquées testées ou validées.

- Run 45978846416 tentative 2 : tests Linux (dont conversions, observation passive, reconnexion et six catalogues) réussis ; tests Qt diagnostic/IA et relecture MEMS 1.6 NA réussis. Échec compilation Windows du test ECUCommunicationLogSelfTest.cpp, ligne 63 : Qt 5.15.2 qlist.h:915, erreurs MSVC C2653/C3861 (stdext::make_checked_array_iterator absent). Correction source : comparaison exacte des listes de noms par taille puis par élément, sans supprimer ni affaiblir le contrôle. Le workflow SESSION 217 se déclenche également sur cette source de test. Aucun changement fonctionnel Manager/DLL, aucune commande ECU. Nouvelle CI et Setup à vérifier.

## Validation SESSION 217 — 27/11/2026
- Run 45981906933, tentative 1, commit logiciel a1d5fd9e77cf9a07e88f773d43b102177da40933 : succès complet Linux et Windows. Tests diagnostic Qt/IA, relecture MEMS 1.6 NA, journalisation sans écrasement, compilation, installation et deux démarrages Windows, 16 pages et mise à jour validés par CI.
- Setup privé brouillon : ECU-Mems-Manager-Session-217-Setup.exe, 602121300 octets ; SHA256 0972e5edf082632aa6c90eb081ff444c3cd35a4ce36815c8fdc0429f34f8fdb2. Empreinte identique dans le journal de compilation, le contrôle de publication et les métadonnées de l'asset GitHub. Release : https://github.com/mini56/ECU-Mems-Manager-Session/releases/tag/untagged-7c7279dd678841a7824c. Aucune publication publique.
- Lot 217 intégré et validé en CI ; essais sur véhicule et validation réelle des comportements ECU restant à faire. Les tests actifs lambda, jauge et CB ne sont pas validés par ce run.
## Décisions utilisateur pour le lot suivant — à intégrer
- Actionneurs, colonne droite : moteur pas-à-pas, test de jauge de température, voyant État système réduit en hauteur. Partie gauche inchangée, deux thèmes, pas de défilement vertical des pages principales.
- Jauge : boutons Minimum, Maximum et arrêt selon procédure validée ; commentaire visible expliquant l'indisponibilité (connexion, profil inconnu, sortie absente, moteur tournant, régime inconnu). Minimum/Maximum désactivés moteur tournant ; uniquement moteur arrêté, contact mis, régime frais connu et compatibilité établie. Annulation sûre si démarrage, selon procédure ECU validée. Aucun affichage simulant une mesure réelle de position de l'aiguille.
- Diagnostic automatique : tests capteurs, notamment température moteur et lambda ; préparer jauge et sonde température avant les prochains essais véhicule. Aucun envoi de commande active non validée. Ces décisions sont des exigences de réalisation, pas une intégration effective.
