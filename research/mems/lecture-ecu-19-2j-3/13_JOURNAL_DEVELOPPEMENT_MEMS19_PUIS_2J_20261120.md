# Journal de reprise — intégration MEMS 1.9 puis 2J

**Date de départ : 20 novembre 2026** — document de suivi actualisé pendant le chantier.
**Dépôt :** `mini56/ECU-Mems-Manager-Session` (privé).
**Branche unique conservée :** `fix-documentary-additive-session-161-20261118`.
**Règle :** pas de nouvelle branche, pas de patch provisoire, pas de verrou supplémentaire ; préserver ROSCO, les pages existantes, les traductions, l'Aide et la base documentaire.
**L'ordre décidé :** (1) finaliser MEMS 1.9, (2) construire le lecteur 2J, (3) vérifier ensemble et sur calculateurs réels.

## Références documentaires disponibles

- `rovermems-reference/ecu-19.go` — réveil ISO9141 MEMS 1.9 (source Go archivée à l'identique).
- `rovermems-reference/ecu-2j.go`, `ecu-2j-parse.go`, `ecu-2j-faults.go`, `ecu-auth.go`, `serialReadWrite.go` — sources de référence MEMS 2J.
- `10_AUDIT_2J_VARIANTES_MESURES_DEFAUTS_20261119.md` — requêtes, variantes, défauts, unités non prouvées.
- `11_TRAMES_CALCULS_ET_PREVALIDATION_19_2J_20261119.md` — checksums, trames synthétiques et contraintes.
- `12_BILAN_RECHERCHE_ET_DECODAGE_19_2J_AVANT_INTEGRATION_20261119.md` — état documentaire, y compris inconnues matérielles.
- Dépendance documentaire 1.9 : source `ecu-1x-shared.go` du même RoverMEMS, **consultée** ; le logiciel C++ utilise son propre librosco.

## Journal daté et vérifiable

| Date | Commit ou preuve | Travail effectivement réalisé | Validation |
| --- | --- | --- | --- |
| 20/11/2026, avant ce journal | [833ec22](https://github.com/mini56/ECU-Mems-Manager-Session/commit/833ec22220f9e8c5dff91956d5d54aec20dd2b02) | `connectToECU()` donne la priorité 1.9 au câble CH340 KKL identifié VID:PID 1A86:7523, et ROSCO au FT232 ; essais de repli conservés | Commit et diff vérifiés ; **compilation et ECU réel non testés** |
| 20/11/2026 | présent journal | Relecture `memsinterface_dispatch.cpp`, `memsinterface.cpp`, `memsinterface.h`, `serialadapterdetector.h`, `protocolcontext.h`, `mems19testtab.h`, `CMakeLists.txt` | Étude source faite, aucune modification du runtime à cette étape |

## Avancement effectif du 20/11/2026 — jalon logiciel MEMS 1.9

**Modifications sources réalisées sur la branche existante :**

| Commit GitHub | Fichier | Résultat |
| --- | --- | --- |
| [103145c](https://github.com/mini56/ECU-Mems-Manager-Session/commit/103145cca9a51be37de0a94037f018c2f7059df9) | `mems19protocol.h` (nouveau) | Fonctions sans I/O pour bits du réveil 0x16, lecture 55 + clés et confirmation E9. Testables sans ECU. |
| [6c2aa36](https://github.com/mini56/ECU-Mems-Manager-Session/commit/6c2aa3681f4ed5be41d438888bc2ac1f5cbfc2b8) | `serialadapterdetector.h` | Préférence CH340 1A86:7523 centralisée dans le détecteur déjà existant ; **ce n'est pas un verrou**. Les autres câbles gardent leur fonctionnement habituel. |
| [3ad0fd8](https://github.com/mini56/ECU-Mems-Manager-Session/commit/3ad0fd88e05ff5db82ad3edf35d860c92dc636f2) | `memsinterface_dispatch.cpp` | Le réveil à cinq bauds utilise un chrono commun afin de réduire la dérive cumulée des 10 bits ; parsing de la réponse par les fonctions isolées et priorité câble réutilisée. Aucun changement de décodeur ROSCO, de commande ECU ni de page UI. |
| [7433d66](https://github.com/mini56/ECU-Mems-Manager-Session/commit/7433d660d0065345a85bb2006d1dcef0fe4bf485) | `mems19protocol_test.cpp` (nouveau) | Auto-test : ordre des 10 bits de 0x16, complément 0x83→0x7C, fragments de réponse et E9, priorité CH340 vs FT232. |
| [c348087](https://github.com/mini56/ECU-Mems-Manager-Session/commit/c348087693760ab7b9a00d4b5b3d978fd5578ebb) | `CMakeLists.txt` | Ajout de la cible `mems19_protocol_selftest` compilée et exécutée au POST_BUILD lors du prochain Setup Windows. |

**Validation :** commits GitHub des modifications effectués. La suite de tests est **écrite, mais non encore exécutée par GitHub Actions** ; ne pas déclarer `PASS` avant un run réussi. Le signal BREAK, le réveil 1.9, les valeurs et les défauts sur calculateur réel restent non testés.

**Non-régression visée :** modules et appels ROSCO inchangés (`mems_connect`, `mems_init_link`, `mems_read`), mêmes pages, pas de nouvel écran, aucun changement de base de données ni de traduction. Le prochain build Windows doit tout de même confirmer la non-régression.

**Point technique à surveiller :** après la réponse E9, `performMems19Wakeup()` ferme le port et `tryRoscoConnect()` le rouvre. Cette reprise de session est héritée ; la bonne tenue du timing doit être mesurée sur un vrai MEMS 1.9. Il ne faut pas inventer un résultat.

**Après reprise :** exécuter un seul build/cohérent des sources actuelles (le workflow SESSION 175 ne se déclenche pas sur de simples changements de source), vérifier l'exécution de `mems19_protocol_selftest` dans les logs, tester identification D0, requêtes D1/D2/F0 et mesures 80/7D. Puis poursuivre le pilote 2J indépendant à partir des documents 10/11/12. Si le build est rouge, corriger les sources incriminées et documenter le vrai message d'erreur.

## Avancement additionnel — traçabilité des connexions 1.9

| Commit | Fichier | Action |
| --- | --- | --- |
| [973ccc3](https://github.com/mini56/ECU-Mems-Manager-Session/commit/973ccc34fc974555d71e6a99974916388f7a8f92) | `memsinterface.h` | Nouveau signal de diagnostic de réveil (données textuelles du port et du statut, aucun changement de condition de connexion). |
| [ee5a382](https://github.com/mini56/ECU-Mems-Manager-Session/commit/ee5a3823543716ea204771e33415cdfaae255790) | `memsinterface_dispatch.cpp` | Émission des détails déjà produits par `performMems19Wakeup()` et du résultat de reprise de session ROSCO. |
| [28ed48f](https://github.com/mini56/ECU-Mems-Manager-Session/commit/28ed48f78b792d9083d7e09e4bf0551e5fc25b36) | `mems19testtab.h` | Ajout d'une ligne `WAKEUP` dans le **journal déjà existant** de l'onglet Test ECU 1.9 ; aucune nouvelle page. |

**Statuts attendus dans les journaux** : `port-open-failed`, `break-control-unavailable`, `slow-init-break-control-failed`, `no-iso9141-key-bytes`, `key-ack-write-failed`, `address-confirmation-missing`, `iso9141-mems19-wakeup-ok`, `rosco-init-after-wakeup-failed`, `iso9141-and-rosco-link-ok`, et états d'annulation. Le message contient un **résultat d'essai**, pas une validation matérielle tant qu'il n'a pas été relevé lors d'un test réel.

**Contrôles restants :** compiler, exécuter l'autotest C++ et le smoke test installé ; la chaîne ROSCO historique et les pages principales n'ont pas été changées. La version SESSION 175 possède un workflow de compilation déclenché uniquement par une modification explicite de son fichier dédié ; cette tâche n'a pas encore déclenché ce workflow pour éviter un run/version 175 hors contexte.

## Points établis et limites

- Le code actuel effectue déjà le réveil 1.9 avec l'adresse 0x16 à cinq bauds, synchronisation 55 + deux octets clés, complément du second octet et confirmation E9.
- Après réveil, le programme ouvre une session ROSCO avec `mems_init_link()`, récupère l'ID D0 puis lit `mems_read()` ; le calculateur réel n'a pas encore confirmé la transition fermeture/réouverture du port.
- `mems19testtab.h` dispose déjà d'une capture d'évènements, mesures, D1/D2/F0, reconnexion et tests avancés. **Ne pas recréer d'écran.**
- Le lecteur 2J C++ n'est pas encore présent ; son transport est 10400 8N1 / Fast Init et n'utilise pas `mems_read()`.
- La capacité électrique réelle du CH340 KKL, les temporisations et les différences de firmware devront être confirmées ultérieurement sur calculateurs ; **cela ne bloque pas les tests logiciels sans ECU**.

## Checklist pour la personne qui reprend

- [x] Rassembler et enregistrer les sources de référence et l'audit documentaire.
- [x] Prioriser l'ordre de connexion selon le câble (commit 833ec22 ; non compilé).
- [x] Stabiliser dans le code les bits 5 bauds, le parsing 55/clé/E9 et la priorité de câble ; **autotests écrits, exécution Windows encore en attente**.
- [ ] Compiler et exécuter les autotests du code 1.9 dans le workflow Windows, en un run maîtrisé.
- [ ] Vérifier les sorties d'identification, de lecture et de défauts 1.9 sans modifier la sémantique historique ROSCO.
- [ ] Concevoir et porter le module 2J C++/Qt depuis la documentation, avec tests par trames, puis interface existante.
- [ ] Tests matériels des deux calculateurs et non-régression 1.2/1.3/1.6.

**Ne jamais marquer « validé sur ECU » uniquement parce que GitHub Actions est vert.**
