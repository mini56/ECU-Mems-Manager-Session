# ECU MEMS Manager SESSION — audit de consolidation du code source

**Mise a jour : 10 novembre 2026.** Branche de travail : `cleanup-session-source-consolidation-20261109`.

## Source de reference protegee

- Version installable de reference : **BUILD 134**, commit `e59247313a18cdc171e6fe0fe332218422ac6940`, branche `dev-complete-build117-14-pages-20261109`.
- Toute correction dans cette branche est une correction des **sources definitives**, pas un nouveau module surcharge ni un patch d'affichage. Aucun Setup, Release ou fusion de nettoyage n'est autorise sans validation distincte.
- Maintenir **14 pages officielles**, leur ordre, les mises en page validees, le stockage SQLite, l'IA ONNX, MEMSLibrary/RAVE, les protocoles et protections ECU/ROSCO. L'interface porte le nom **ECU MEMS Manager SESSION** ; les identifiants internes du stockage restent compatibles.

## Modules consolidés etormais possedes par le code source

| Responsabilite | Source(s) active(s) | Modules de surcharge supprimes |
|---|---|---|
| Navigation officielle 14 pages, menu, entete, style | `uishell.cpp`, `darkstyle.cpp`, `mainwindow.cpp` | `uirebuild.cpp`, `navigationorderpatch.cpp/.h`, `visualcompletion.cpp`, `strictvisualrebuild.cpp`, `darkfixes.cpp`, `sidebarbubblecleaner.cpp`, `readabilityfix.cpp` |
| Page Reglages et cadrans | `settingspage.cpp` | `settingstabdark.cpp`, `settingsvisualpatch.cpp` |
| Apercu, etats ECU, signaux injection | `overviewpage.cpp`, `overviewramlive.cpp`, `mainwindow.cpp` | `overviewrebuild.cpp`, `systemstateconnectionfix.cpp` |
| Erreurs / Actionneurs | `ecupages.cpp` | `majorpagesrebuild.cpp`, anciens correctifs d'Erreurs |
| Donnees brutes et adaptation du contenu | `rawpage.cpp` | `dedicatedpagesrebuild.cpp` |
| Fenetre, dialogue de fermeture, unicite des instances | `mainwindow.cpp`, `singleinstance.cpp`, `nativewindowstyle.cpp` | `mainwindowtitlebarfix.cpp`, `singleinstanceforce.cpp` |
| Recherche MEMS et consultation des fiches | `database/MemsDatabaseBrowser.cpp`, `MemsDatabaseExplorerV2.cpp`, `MemsDatabaseUniversalAccess.cpp`, `MemsReferenceSheetRenderer.h` | `MemsDatabaseDiagramSearchPatch.cpp`, `MemsSearchCompletenessPatch.cpp`, `MemsDatabaseGlobalUi.cpp`, `MemsDatabaseUsability.cpp`, `MemsXmlRowIndexer.cpp` |
| IA documentaire | `iamemstab.cpp`, `expert/IaMemsDocumentaryPipeline.cpp` | `database/IaDocumentaryLockPatch.cpp` |
| Connexion et securite protocolaire | `memsinterface.cpp`, `memsinterface_dispatch.cpp`, `protocolcontext.h` | Macros de renommage et `#include "memsinterface.cpp"` supprimes ; appels aux gardes de securite explicites dans la source |
| Test RAM AANMP002 et journal | `injectionramtest.cpp`, `injectionramtestlogger.cpp` | Filtres d'application et surveillance globale periodique elimines |
| Demarrage, splash, ressources Qt | `main.cpp`, `resources.qrc` | `splashtextfix.cpp`, `main_keyed.cpp`, `qrc_resources.cpp` genere et non utilise |

**Important :** `memsinterface_dispatch.cpp` n'est pas un patch graphique. C'est le module actif des protections, de la detection et de la connexion ECU. Ses fonctions doivent rester preservees.

## Dernieres operations source (10/11/2026)

- `fc2eeba6` : sources ECU compilees separement et gardes de securite appeles explicitement.
- `3becc966` : explorateur MEMS initialise par la page qui le possede, sans filtre global au demarrage.
- `ecc443af` : synchronisation des fiches traduites lors de la preparation initiale, avant l'index, au lieu d'une verification chaque seconde.
- `a50e53ff` / `6a87bcc7` : une seule chaine de presentation des resultats de la base, sans second controleur toutes les 280 ms.
- `83f862ce` : donnees directes de l'Apercu, plage temps de bobine/injection 0–5 ms.
- `363bc136` : bouton du test RAM 2 phases directement dans la page ROSCO.
- `c0e64645` : correction du test de journal Windows pour les fins de ligne CRLF.
- `0650c5b3` : branchement unique des signaux de suivi des modes ECU au constructeur de `MEMSInterface`.
- `ea636c68` : note du cadran RPM integree directement a sa disposition Qt, sans deplacement differe ni coordonnees absolues.
- `d80627d1` : retrait de `qrc_resources.cpp`, genere et non compile ; garde CI contre le retour de ce fichier et du repositionnement differe de la note.

## Validations constatees sur GitHub Actions

Workflow : `.github/workflows/session-cleanup-source-validation.yml`.

- **42787477090 : SUCCESS** — compilation Windows, IA ONNX, ROSCO et tests IA multilingue, diagrammes MEMS, fiches de base, journal RAM.
- **42791666397 : SUCCESS** — consolidation avant le suivi des modes ECU.
- **42792329174 : SUCCESS** — commit `0650c5b3`, y compris les controles de source et tests.
- **42792714170 : SUCCESS** — commit `ea636c68`, y compris compilation et tests.
- **42793149697 : en cours au dernier controle** — commit `d80627d1`. Ne pas le presenter comme vert avant lecture de sa conclusion.

Le CI refuse les modules obsolete references, les `Q_COREAPP_STARTUP_FUNCTION` dans les sources natives compilees et les inclusions textuelles de fichiers `.cpp`. Il verifie egalement plusieurs contrats de proprietaire unique.

## Limites de validation / prochaines etapes

1. Lire et journaliser la conclusion de la derniere compilation. Corriger toute erreur dans sa **source d'origine**, pas au moyen d'une surcharge.
2. Continuer l'audit des ajustements et abonnements internes qui pourraient rester redondants, sans supprimer les traitements normaux de Qt (timers de mesure, debounce, redimensionnement).
3. Faire des tests **reels sous Windows** : ouverture, fermeture, traduction, navigation sur les **14 pages**, tailles sur plusieurs resolutions, pages Reglages/Erreurs/Apercu/ROSCO, consultation documentaire et journal RAM.
4. Tester sur vrai calculateur ou simulateur les cas de connexion, deconnexion, Mode 3/Mode 4, identification MEMS 1.9 et les refus de commandes dangereuses. Une compilation verte **ne prouve pas** ces tests materiels.
5. Tester l'installation propre, la mise a jour et la preservation SQLite **avant** toute fusion ou nouvel installateur.

**Regle immuable :** le nettoyage doit produire un seul code de reference qui conserve toutes les fonctions valides. Ne jamais supprimer une fonction utile uniquement parce qu'elle provient historiquement d'un correctif.
