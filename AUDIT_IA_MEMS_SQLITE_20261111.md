# Audit de la chaîne de réponses IA MEMS et des accès SQLite

Date : 11 novembre 2026.
Référentiel examiné : `mini56/ECU-Mems-Manager-Session`, branche
`cleanup-session-source-consolidation-20261109`, commit de base `2e242a14`.

## Constat prouvé par la source (pas de diagnostic supposé)

### Écran « Base de données »

- `database/MemsDatabaseExplorerV2.cpp` vers les lignes 216, 254–264 et 550–592 : l'écran interroge `search_documents` dans l'index de `MemsGlobalSearchIndex::indexPath()`. Il **n'interroge pas un fichier SQLite arbitraire**.
- `database/MemsGlobalSearchIndex.cpp` vers 568–668, 774–800 : l'index est régénéré depuis `MemsReferenceDatabase::databasePath()` et des XML; les deux fichiers sont mis en cache.
- `database/MemsReferenceDatabase.cpp` vers 449–528 : la base de référence est reconstruite à partir de `mems_reference_seed_*.qz64` et des enrichissements quand elle est absente ou que la révision change. Poser un nouveau fichier SQLite à la place ne garantirait donc pas son maintien.

### IA MEMS

- `iamemstab.cpp` vers 194, 523–617 : une couche documentaire capture les requêtes avant `IaMemsService::askWithLibrary()`.
- `expert/IaMemsDocumentaryPipeline.cpp` vers 167–295, 378–509 : lecture directe de `MEMSLibrary_Pack_001/knowledge.sqlite` et des tables `memslibrary_search`, `ravemems_operation`, `ravemems_step`, `ravemems_notice`. Sélection documentaire, menus contextuels et une réponse structurée dépendant de ce schéma.
- `expert/IaMemsLibraryIntegration.cpp` vers 594–762 : chemin de recherche structuré dans la base experte, puis bridge `MEMSLibrary`, enfin `groundingFor()` historique. Certains chemins passent par `[[MEMS_RESOLVED]]` / `[[MEMS_GROUNDED]]`, des propriétés Qt, plusieurs commandes et réponses de repli.
- `expert/IaMemsService.cpp` vers 428–457 : la base experte est ouverte depuis le chemin fixé `database/expert/ia_mems_reference_r20.sqlite`. La recherche `knowledgeAnswer()` (vers 830–1040) possède ses propres règles de pertinence, tri, filtres et réponses de secours.
- `expert/IaMemsLibraryBridge.cpp` vers 168–221 : la DLL et `MEMSLibrary_Pack_001` sont des dépendances, avec validation explicite de `pack_id=MEMSLibrary_Pack_001` et `document_count=47`. Ce code **n'ouvrira pas automatiquement notre nouveau document Rover de 482 pages**.
- `expert/LocalAiClient.cpp` vers 257–356 et 709–761 : `controlledTechnicalAnswer()` produit des réponses fixes sur IAC, MAP, SPI, injecteurs, bobine, etc. avant toute synthèse ou consultation documentaire dans ce composant. `controlledFollowUpAnswer()` court-circuite aussi le dialogue.
- `expert/LocalAiClient.cpp` vers 747–818 : pour une réponse documentaire non diagnostique, `grounding` est renvoyé **directement** sans génération. Le modèle peut aussi échouer et céder à `grounding`. Ne pas interpréter une sortie d'IA comme une preuve que le nouveau document a été consulté.

### Fichiers historiquement dénommés patch

L'inventaire des fichiers de la branche révèle **12 chemins contenant « patch »**, surtout `tools/patch_ia_*.py`, `patch_reference_*.py`, `temp_patch_memsx64_ai_package.py` et `IA_X64_PACKAGE_PATCH_STARTED.txt`.
Ces fichiers confirment l'existence de scripts de réécriture successifs dans le dépôt.
**La présence d'un script ne prouve pas qu'il s'exécute pendant la compilation** : les workflows consultés présentent plutôt des vérifications et des inclusions directes CMake. Les comportements problématiques décrits ci-dessus sont, eux, **actifs dans les fichiers C++ compilés**.

## Risque concret pour notre nouvelle SQLite Rover de 482 pages

Une base fraîche, même `PRAGMA integrity_check=ok`, peut ne rien changer à l'écran ou aux réponses IA, parce que les deux chaînes lisent **des fichiers et des structures différents**. Une question technique peut recevoir une réponse codée en dur, une ancienne fiche RAVE, une réponse de secours ou une synthèse du modèle, sans accéder à la nouvelle SQLite.

Aucune de ces observations ne prouve à elle seule qu'une réponse effectivement affichée chez l'utilisateur a été fausse : cela doit être démontré par des tests installés.

## Correction d'architecture à effectuer **dans les sources, sans patch**

1. Définir **un unique référentiel documentaire/versionné** pour le corpus Rover de 482 pages, avec tables de pages, procédures, phases, étapes, notices, mesures, figures, rattachements, origines, langues et niveaux de validation.
2. Créer **un unique lecteur SQLite en lecture seule** utilisable par les deux consommateurs : écran de la base et IA documentaire. Le lecteur retourne des résultats typés, pas seulement du texte de synthèse.
3. Faire utiliser à l'écran de base de données **ce référentiel** pour les requêtes, détails et liens PDF/schéma. Les fonctions ECU, mesures directes et réglages restent isolés.
4. Faire utiliser à l'IA **la même recherche**. Tout résultat doit comporter identifiant de source, pages physiques, référence constructeur, texte original, contexte moteur/ECU, état de validation et éventuels schémas.
5. Unifier le routage : question documentaire → recherche de faits; question diagnostique → mesures et moteur expert, avec faits documentaires seulement quand leur contexte est compatible; question sur l'interface → aide logicielle. **Aucune réponse technique figée ne doit gagner sur une preuve vérifiée.**
6. Refuser le mécanisme « pas de réponse à la question = recherche sur une autre rubrique approximative » sans signaler l'incertitude. Différencier **fait validé**, **texte source OCR non vérifié** et **absence de donnée**.
7. Conserver le modèle local comme outil de formulation contrôlée, **jamais comme source de vérité**. Les avertissements et unités doivent être cités avec leur provenance et ne doivent pas être perdus par reformulation.
8. Installer une nouvelle base **en un seul bloc** après contrôles de schéma, de 482 pages, d'intégrité SQLite, de source PDF, de relations multipages et d'application Win10/Win11. Sauvegarder l'ancienne et prévoir un rollback atomique.

## Batterie d'essais d'acceptation avant Setup

- Demander dans l'écran BD `12.65.12`, `Timing Chain and Gears`, `88 Nm`, `22 Nm`, et retrouver les pages 92 et 93 et les bonnes étapes.
- Poser les mêmes questions à l'IA MEMS. Vérifier **l'égalité de la référence source** avec l'écran BD, et non seulement la similarité des phrases.
- Prouver que l'IA reconnaît un schéma correspondant à une procédure et le retrouve sur disque, mais **n'invente aucune liaison** pour une figure indéterminée.
- Demander une procédure absente ou incompatible avec la version ECU : aucun fait de remplacement inventé; réponse motivée.
- Débrancher volontairement la nouvelle SQLite pendant un test : erreur explicite, **aucune réponse documentaire présentée comme sourcée** depuis un ancien chemin silencieux.
- Contrôler qu'une page OCR non vérifiée est bien consultable en documentation, mais jamais présentée comme couple de serrage constructeur définitivement validé.
- Vérifier la conservation des fonctions diagnostics/actionneurs/réglages du programme avant un Setup.

## État de ce document

**Audit de lecture du code uniquement.** Aucun comportement C++ corrigé, aucune compilation et aucun Setup déclenchés par cet audit. Ce rapport sert de base à une refonte propre des *sources existantes*, sans empilement de nouveaux correctifs.
