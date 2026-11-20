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

## Points établis et limites

- Le code actuel effectue déjà le réveil 1.9 avec l'adresse 0x16 à cinq bauds, synchronisation 55 + deux octets clés, complément du second octet et confirmation E9.
- Après réveil, le programme ouvre une session ROSCO avec `mems_init_link()`, récupère l'ID D0 puis lit `mems_read()` ; le calculateur réel n'a pas encore confirmé la transition fermeture/réouverture du port.
- `mems19testtab.h` dispose déjà d'une capture d'évènements, mesures, D1/D2/F0, reconnexion et tests avancés. **Ne pas recréer d'écran.**
- Le lecteur 2J C++ n'est pas encore présent ; son transport est 10400 8N1 / Fast Init et n'utilise pas `mems_read()`.
- La capacité électrique réelle du CH340 KKL, les temporisations et les différences de firmware devront être confirmées ultérieurement sur calculateurs ; **cela ne bloque pas les tests logiciels sans ECU**.

## Checklist pour la personne qui reprend

- [x] Rassembler et enregistrer les sources de référence et l'audit documentaire.
- [x] Prioriser l'ordre de connexion selon le câble (commit 833ec22 ; non compilé).
- [ ] Stabiliser et tester hors ECU la génération des bits 5 bauds, le parsing de la réponse 55/clé/E9 et le choix de l'adaptateur.
- [ ] Compiler et exécuter les autotests du code 1.9 dans le workflow Windows, en un run maîtrisé.
- [ ] Vérifier les sorties d'identification, de lecture et de défauts 1.9 sans modifier la sémantique historique ROSCO.
- [ ] Concevoir et porter le module 2J C++/Qt depuis la documentation, avec tests par trames, puis interface existante.
- [ ] Tests matériels des deux calculateurs et non-régression 1.2/1.3/1.6.

**Ne jamais marquer « validé sur ECU » uniquement parce que GitHub Actions est vert.**
