## Lot opérations auto-session — 147 exports sources, CI à confirmer

[Journal des opérations et du traçage](JOURNAL_DEVELOPPEMENT_DLL_AUTO_IO_SESSION205_20261124.md). Aucun changement de Manager 205. Le tracé legacy et délais configurables restent à faire.

## Lot auto-connexion moteur — 138 exports sources, validation à venir

[Journal auto-connexion](JOURNAL_DEVELOPPEMENT_DLL_AUTO_CONNECT_SESSION205_20261124.md). T04–T08, aucun changement Manager. Le run des adaptateurs est distinct.

## Nouveau lot détection native des adaptateurs — 133 exports prévus

[Journal adaptateurs](JOURNAL_DEVELOPPEMENT_DLL_ADAPTERS_SESSION205_20261124.md). T01–T03, lecture seule ; auto-connexion à suivre. Manager 205 inchangé.

## Dernière validation : RC5 — 130 exports

[Run vert 45370170574](https://github.com/mini56/ECU-Mems-Manager-Session/actions/runs/45370170574) : 11/11 CTest, 130 exports, 15 imports, 16 pages, Setup indépendant installé et désinstallé. Aucun ECU physique.

## Historique RC5 — 130 exports validés

[Journal RC5](JOURNAL_DEVELOPPEMENT_DLL_RC5_SESSION205_20261124.md). Aucun changement du Manager 205.

## Dernière validation logicielle : MEMS3 — 123 exports

[Run Windows vert 45367580697](https://github.com/mini56/ECU-Mems-Manager-Session/actions/runs/45367580697) : 10/10 CTest, 123 exports, 15 imports, 16 pages et Setup indépendant installé puis désinstallé. Aucun ECU physique.

## Historique du lot MEMS 3 — 123 exports validés

[Journal du lot MEMS 3](JOURNAL_DEVELOPPEMENT_DLL_MEMS3_SESSION205_20261124.md). Aucun changement du Manager 205. Dernier lot validé : 114 exports MEMS 2J.

# `mems_manager_x64.dll` — moteur protocolaire x64 d’ECU MEMS Manager

Cette arborescence construit la **nouvelle DLL Windows x64 native d’ECU MEMS Manager**.

Nom du binaire : **`mems_manager_x64.dll`**.

L’ancienne bibliothèque historique reste distincte :

- `librosco.dll` = DLL historique x86 / 32 bits (celle de LeopoldG est la référence fonctionnelle avec **45 exports**) ;
- `mems_manager_x64.dll` = nouvelle DLL native x64 développée pour ECU MEMS Manager.

Le sous-dossier conserve pour l’instant le nom historique `librosco-x64` et le header public `rosco.h` afin de limiter les régressions pendant la migration. Cela ne change pas le nom du nouveau binaire.

## Référence ACTIVE : SESSION 205 — 114 exports DLL VALIDÉS, MEMS 2J ajouté

**DLL native x64 compilée : 114 exports réels.** Les **105 premiers noms/signatures/ordinaux** sont conservés sans changement. Le lot suivant ajoute **9 fonctions de session MEMS 2J (ordinals 106 à 114)**, toujours **sans modification de Manager 205**.

- [Journal d'intégration MEMS 2J, commandes, transactions, traçage TX/RX et limites](JOURNAL_DEVELOPPEMENT_DLL_MEMS2J_SESSION205_20261123.md).
- [**Run Windows VERT n° 45251694520, copie de la vraie Session 205**](https://github.com/mini56/ECU-Mems-Manager-Session/actions/runs/45251694520) : **9/9 tests CTest réussis**, **114/114 exports et ordinaux PE x64 corrects**, **15 imports réels Manager résolus** ; vérification des 16 pages de l'EXE 205 ; véritable Setup officiel 205 vérifié SHA256, puis Setup de test avec AppId séparé **créé/installé/testé/désinstallé**. Aucune Release et aucune MAJ.
- Cette validation prouve la **compatibilité logicielle et le périmètre automatisé couvert**. Les ECU MEMS 2J et MEMS 1.9 physiques n'ont pas été branchés pendant ces essais : **aucun résultat matériel n'est revendiqué**. La DLL reste à compléter (MEMS 3, RC5, 5AS, auto-détection et autres fonctions inventoriées).

**Référence officielle de l'EXE : Session 205, commit `b5cd9b61bbaed6251f17559885481b1bd03f4380`, et Setup publié. Le programme lui-même n'a pas été modifié.**

---

### Historique : lot MEMS 1.9 — 105 exports

## Référence précédente : SESSION 205 — 105 exports DLL validés, MEMS 1.9 ajouté

**Nouvelle DLL native x64 : 105 fonctions exportées.** Les **91 exports historiques** et les **9 nouveaux exports Lucas 5AS** conservent strictement leurs numéros et signatures. Le lot MEMS 1.9 a ajouté **5 exports (101 à 105)** sans modifier Manager.

- [Journal technique du réveil ISO 9141 à 5 bauds et de la connexion MEMS 1.9](JOURNAL_DEVELOPPEMENT_DLL_MEMS19_SESSION205_20261123.md).
- [**Run VERT n° 45246024037 avec la vraie Session 205**](https://github.com/mini56/ECU-Mems-Manager-Session/actions/runs/45246024037) : **8/8 tests réussis, 105 exports exacts, 15 imports Manager résolus**, démarrage du vrai programme et validation de ses 16 pages, **vrai Setup 205 vérifié**, Setup d'essai indépendant créé/installé/testé/désinstallé avec notre nouvelle DLL. Aucun Setup ni changement du programme publié.
- **Limite :** aucune validation du nouveau réveil MEMS 1.9 sur un véritable ECU ; la compatibilité prouvée concerne les vérifications automatisées et les simulations de transport série. **La DLL complète reste en développement.**

Les tests sur la version 205 utilisent son commit de référence `b5cd9b61bbaed6251f17559885481b1bd03f4380`, sans modifier ses sources ni sa Release.

---

### Historique du lot Lucas 5AS — 100 exports

## Référence antérieure : SESSION 205 — ajout Lucas 5AS dans la DLL

La cible de compatibilité est **le véritable exécutable et le Setup publiés de SESSION 205**, commit `b5cd9b61bbaed6251f17559885481b1bd03f4380`, **pas** SESSION 200. Son en-tête ROSCO est identique à celui de SESSION 200 mais ses binaires et son Setup doivent être testés séparément.

**Lot Lucas 5AS validé précédemment : 100 exports déclarés (91 historiques inchangés + 9 Lucas 5AS en lecture seule).** Le code et les tests sont conservés uniquement sur `dll-manager`. Le programme Manager n'est **jamais modifié** : le build de vérification le compile dans un répertoire Windows temporaire et extrait un Setup de test indépendant.

- [Journal précis des neuf fonctions créées, des limites et des runs](JOURNAL_DEVELOPPEMENT_DLL_SESSION205_20261123.md)
- [**Run VERT de validation complète SESSION 205 n° 45240083756**](https://github.com/mini56/ECU-Mems-Manager-Session/actions/runs/45240083756) : **6/6 CTests**, **100 exports exacts**, **15 imports réels du Manager résolus**, **test des 16 pages du véritable EXE 205**, **Setup 205 officiel vérifié**, création/installation/test/désinstallation d'un **Setup d'essai indépendant** avec notre nouvelle DLL. **Aucune publication de Release/MAJ, aucune modification de Session 205.**
- Les essais sur ECU physique, la programmation et les fonctions non encore transférées ne sont **pas** couverts par la compatibilité binaire.

**Les résultats de SESSION 200 conservés plus bas sont uniquement l'historique.**

---

## Inventaire des fonctions ECU et préparation du moteur DLL — 23/11/2026

- **[Rapport d'inventaire ECU complet du corpus disponible (SESSION 200 / DLL)](INVENTAIRE_EXHAUSTIF_ECU_DLL_SESSION200_20261123.md)** : toutes les fonctions ECU identifiées dans les sources actuelles, les archives et les recherches examinées, avec statuts `DLL / Manager / recherche / non confirmé`, écarts de couverture, conflits protocolaires et feuille de route.
- **[Matrice de contrôle de 91 exports + déclaration non implémentée](MATRICE_91_EXPORTS_ET_CONTRAT_SESSION200_20261123.csv)** : suivi vérifiable des noms, ordinaux, signatures déclarées et preuves CI.
- **[Chiffrage nominatif de 84 nouvelles fonctions ECU proposées](CHIFFRAGE_84_FONCTIONS_DLL_SESSION200_20261123.md)** : répartition par famille, 84 noms distincts, statuts Manager/recherche ; total prévisionnel de 175 exports avec les 91 existants, sous réserve de conception détaillée.

**Ce lot est exclusivement documentaire :** aucune modification du programme SESSION 200, de ses workflows de publication ni du code de la DLL ; aucun nouveau Setup, aucune Release ni aucun run déclenché.

---

## Référence des tests active — SESSION 200 (22 novembre 2026)

**Référence unique actuelle : version 200**, tag publié `mems-session-build-200`, commit `3737dffa3e30e0b9167bda34fa661958b757cd28`, run officiel n° 45061468323 : succès. **Les tests de la DLL ne doivent plus prendre Session 197 comme cible.**

L'environnement de validation crée une copie temporaire et isolée de **Session 200**, sans changer Manager, ses workflows, son Setup installé ni le site MAJ.

- `reference-session200/rosco.h` : en-tête exact de Session 200, Git blob `dd8d88048ce4e879ef728b6bcd72837730bec654`, identique à l'en-tête de 197.
- `manager_dll_contract_selftest.cpp`, `validate_protocol_guards.py` et `validate_session200_binaries.py` : contrat ABI, signatures, 91 exports et imports réels de Session 200.
- `verify_session200_windows.ps1` : compilation DLL x64, tests CTest, compilation de la vraie Session 200 avec notre DLL, inspection des PE et exécution du test de démarrage des 16 pages existant dans Session 200, sans ECU et sans installation.

**Résultat effectif du 22/11/2026 : RUN VERT #45072564428**, [journal de la validation DLL / Session 200](https://github.com/mini56/ECU-Mems-Manager-Session/actions/runs/45072564428), commit testé `91974c9b243bfdf519ce251c32176a8aa2b4d7aa`.

- **DLL Windows x64 compilée** ; **4/4 tests CTest réussis**.
- **91 exports physiques** conformes (noms et ordinaux) ; **15 imports Manager** correctement résolus, sans `librosco.dll` x86.
- **Véritable Session 200 compilée et liée** avec notre nouvelle DLL.
- **Session 200 réellement démarrée en mode test Qt hors-écran**, vérification automatique des **16 pages**, navigation, thèmes, langues et fonctions d'interface : `SESSION200_UI_SMOKE_WITH_NEW_DLL_PASS`, `Checks PASS`, fermeture code `0`.
- La validation a utilisé exclusivement un **worktree Git éphémère**. Aucune source du programme Session 200, aucun workflow du programme, aucune Release, aucun Setup installé et aucun calculateur physique n'ont été modifiés.

**Validation du véritable Setup avec notre DLL : RUN VERT #45134425955, 23/11/2026** — [journal Windows / installation et désinstallation](https://github.com/mini56/ECU-Mems-Manager-Session/actions/runs/45134425955). Workflow de validation dédié, commit `12fb64ff74fc9861dd56e5154955cf490f8e3f41`.

Cette étape a utilisé le **Setup officiel de Session 200 effectivement publié** (`ECU-Mems-Manager-Session-200-Setup.exe`), dont le SHA256 a été vérifié avec le fichier publié `SESSION-200-Setup.sha256`. L'installateur officiel a été extrait sur un **runner GitHub Windows éphémère**. Le fichier `ecu_mems_manager.exe` original et toutes les dépendances distribuées ont été conservés ; **seule `mems_manager_x64.dll` a été remplacée dans la copie temporaire**. Un **nouveau Setup Inno Setup de test** a été créé avec **AppId différent**, installation par utilisateur, aucun raccourci, aucune publication et aucune interaction avec le PC utilisateur.

Contrôles réels du Setup isolé :

- **4/4 CTest DLL réussis**, 91 exports (noms/ordinals) conformes, 15 imports de l'exécutable Session 200 résolus.
- `SESSION200_OFFICIAL_RELEASE_SETUP_SHA256_PASS` : téléchargement vérifié du **vrai Setup 200 publié**.
- `SESSION200_ORIGINAL_RELEASE_EXE_NEW_DLL_ABI_PASS` : le binaire **officiel publié** est compatible au niveau des imports avec la **nouvelle DLL**, sans recompilation du programme.
- `SESSION200_INDEPENDENT_NEW_DLL_SETUP_BUILD_PASS` : un **vrai Setup de test** a été compilé. Son SHA256 de journal est `9744E43FEF97E6DA7EE0BADA36796AD06A57943A6DAC43D5ACCF6B6D3382737C` (pas de publication ni d'artefact utilisateur).
- `SESSION200_INSTALLED_EXE_AND_NEW_DLL_SHA256_ABI_PASS` : après **installation effective**, l'exécutable installé est **identique octet pour octet à l'exécutable officiel 200**, et la DLL installée est identique à notre DLL recompilée.
- `SESSION200_ISOLATED_SETUP_INSTALLED_16PAGE_UI_PASS` : démarrage et **test intégré des 16 pages**, navigation, contrôles UI, thèmes, langues, analyse, `Checks PASS` et sortie normale.
- `SESSION200_ISOLATED_SETUP_UNINSTALL_PASS` : désinstallation de la copie test, présence des fichiers exe/DLL de test contrôlée comme absente après suppression.
- `SESSION200_NEW_DLL_COMPLETE_SETUP_TEST_PASS` : validation globale verte, **sans toucher aux sources, workflows, Release ou installateur officiel Session 200**.

**Verdict et limites :** **compatibilité du Setup de test installé avec notre nouvelle DLL validée sur Windows hébergé**. Aucun ECU physique n'était connecté : les connexions réelles, les lectures, les effets des commandes sur calculateurs et les configurations matérielles non couvertes restent à vérifier **avant toute substitution de DLL sur un poste utilisateur**. Les résultats historiques sur Session 197 restent archivés ci-dessous ; la référence active est **Session 200**.

---

## Compatibilité réelle avec MEMS Manager Session 197 — en cours de validation

**Référence unique de l'application :** commit `9105d9838e3fd5f81feeb99c0481c454d5d6a5e4` (Session 197 verte), branche `fix-documentary-additive-session-161-20261118`.

Attention : la racine de la branche `dll-manager` contient une version **antérieure** de certains fichiers Manager (`rosco.h`, `memsinterface_dispatch.cpp`, etc.). **Ne pas compiler la racine de `dll-manager` et appeler cela « compatible Session 197 ».** Les fichiers de Manager 197 ne doivent pas être remplacés dans le dépôt.

Contenu du dossier DLL pour cette validation :

- `reference-session197/rosco.h` : **copie binaire exacte** de l'en-tête public de Session 197 (Git blob `dd8d88048ce4e879ef728b6bcd72837730bec654`). Son ABI est contrôlé dans `manager_dll_contract_selftest.cpp` ; ce test n'utilise plus l'en-tête antérieur de `dll-manager`.
- `validate_protocol_guards.py` : contrôle statique l'API et les commandes **Session 197** et signale explicitement si la racine de compilation n'est pas Session 197.
- `verify_session197_windows.ps1` : récupère le **commit exact Session 197** dans un **worktree Git temporaire**, copie notre seul dossier `librosco-x64` dans cette zone jetable, exécute l'audit source, compile la DLL x64, exécute les tests CTest, compile l'exécutable Session 197 avec `MEMS_USE_BUNDLED_LIBROSCO_X64=ON`, puis vérifie l'édition des liens et les imports PE.
- `validate_session197_binaries.py` : inspecte **les véritables binaires** Windows AMD64, 91 noms/ordinals d'exports et les imports du Manager 197 pour vérifier l'absence d'appel vers `librosco.dll` x86.

Sur un poste Windows équipé de Visual Studio 2022 C++, de Git, de CMake, de Python et de Qt 5.15.2 MSVC2019 x64, depuis le dépôt `dll-manager` :

```powershell
pwsh -NoProfile -File .\librosco-x64\verify_session197_windows.ps1 -QtPrefix 'C:/Qt/5.15.2/msvc2019_64'
```

Pour contrôler **uniquement** la DLL, sans compiler le programme (résultat qui n'établit pas sa compatibilité réelle avec Manager) : ajouter `-DllOnly`.

**Validation Windows x64 effectuée le 22/11/2026 : RUN VERT #45047497347** — [journal GitHub Actions](https://github.com/mini56/ECU-Mems-Manager-Session/actions/runs/45047497347), commit source validé `f1bec15c6fa21836eb2b1555366eb03d93b6ab2d`.

- `mems_manager_x64.dll` compilée avec **Visual Studio 2022**, architecture AMD64.
- **4/4 tests CTest validés**, dont l'ABI x64, le contrat d'en-tête Session 197, les trames série simulées et le catalogue protocolaire.
- **91 exports physiques** avec noms et ordinaux attendus.
- **Vraie copie de Session 197 compilée avec la nouvelle DLL**, éditions de liens réussies ; **15 imports** résolus et aucune importation de la DLL historique x86.
- **Test réel du programme en mode interface isolée, validé le 22/11/2026 : RUN VERT #45071552369** — [journal GitHub Actions](https://github.com/mini56/ECU-Mems-Manager-Session/actions/runs/45071552369), commit `34e86cf604db98ee787e8d0cf41f9888b11b730f`.
  - L'exécutable **Session 197 reconstruit avec notre DLL** a été lancé via son option existante `--session-ui-smoke-test`, en rendu Qt sans écran et dans un profil de test.
  - `SESSION197_UI_SMOKE_WITH_NEW_DLL_PASS` et trace `Checks PASS` / `Qt event loop returned: 0` dans les logs du runner.
  - Le test de l'application parcourt les **16 pages**, la navigation, les langues, les thèmes et plusieurs fonctionnalités d'interface / d'analyse hors ECU, y compris les tailles d'écran et la disposition « Autres calculateurs ».
  - Ce run confirme également **4/4 CTest**, **91 exports**, **15 imports résolus** et liaison avec Session 197.
- Les tests hors ligne ne prouvent **ni la lecture, la connexion et les effets réels des commandes sur ECU physique**, ni le fonctionnement d'un Setup **installé** avec cette DLL, ni chaque interaction non couverte par le scénario automatique. Aucune DLL en production ni programme installé n'a été remplacé.

**Verdict :** compatibilité **de compilation, liaison, ABI et démarrage/test automatisé des 16 pages** validée avec Session 197 dans un environnement isolé. Le test sur ECU réel et celui du programme installé restent à effectuer avant une substitution de la DLL actuelle.

**Règle de protection :** aucun fichier de Session 197, aucun Setup ou dépôt MAJ n'a été modifié. La compilation de test travaille uniquement sur une copie temporaire, sans toucher à la version installée. L'export historique `mems_interactive_mode` est déclaré dans l'en-tête 197 mais reste **non implémenté/non exporté** : il n'est pas appelé par le binaire 197 actuellement ; il ne doit pas être masqué par une fonction factice.

## Correspondance fonctionnelle arrêtée pour Session — 22 novembre 2026

**Décision définitive pour notre DLL**, confirmée par l'utilisateur et cohérente avec MemsFCR/Andrew Jackson :

| Action demandée | Octet de commande | Export conservé par MEMS Manager Session |
|---|---|---|
| Réinitialisation des **réglages ajustables** (Reset Adjustments) | **`0x0F`** | `mems_reset_adjustments` |
| Effacement des **paramètres calculés et appris** (Reset ECU dans l'interface) | **`0xFA`** | `mems_reset_ECU` |

Les noms hérités d'autres DLL ne modifient **jamais** cette correspondance. L'export séparé `mems_reset_ECU_leopold` utilise toujours `0xAF` mais **n'est pas substitué** à la commande Manager `0xFA`.

Cette décision fixe le comportement attendu de la DLL compatible Session. Aucun fichier de Manager, aucun ordinal, aucune trame envoyée par les deux fonctions existantes n'est changé. Les références historiques contradictoires demeurent consignées uniquement pour traçabilité.

## Reconstruction complète LeopoldG (branche isolée, 21/11/2026)

**Référence prioritaire :** `LeopoldG/sources-mems-rosco/librosco.dll`, binaire PE32 de 63 161 octets, **45 fonctions exportées**, comparées directement à sa table d'exports et à ses commandes machine. Cette version est distincte de l'ancienne DLL de référence Manager à 22 exports.

- **45/45 noms historiques recréés**, ordinals 1–45 conservés. La commande derrière `mems_reset_ECU` garde volontairement la valeur historique de Manager (FA), non celle de LeopoldG (AF). Les 20 actions précédemment regroupées utilisent une fonction individuelle qui appelle la même primitive de dialogue/écho/réponse.
- **Conflit binaire vérifié :** Manager-22 utilise `mems_reset_ECU=FA` et `mems_reset_adjustments=0F`. LeopoldG-45 utilise `mems_reset_ADJ=FA`, `mems_reset_ECU=AF`, `mems_reset_EMI=0F` et `mems_Save=81`. La nouvelle DLL conserve la signification de Manager pour les fonctions déjà utilisées, et propose `mems_reset_ECU_leopold=AF` comme fonction supplémentaire explicitement nommée. Le bouton de reset existant pour les familles ROSCO doit respecter ces valeurs ; les chemins MEMS 1.9 K-Line conservent leurs propres commandes historiques `0F/FA` dans Manager, sans substitution.
- **8 exports modernes préservés ou ajoutés :** `mems_reset_adjustments` (alias vers la commande 0x0F des réglages ajustables, via `mems_reset_EMI`), `mems_save_adjustments` (alias vers Save original), 4 transports MEMS 1.9, et 2 fonctions de correction secondaire de richesse `7B/7C`. **État de la branche `dll-manager` : 91 exports (53 historiques/étendus, 1 reset LeopoldG distinct, 11 fonctions par famille, 6 fonctions d'analyse AANMP002, 20 fonctions de trames/mesures/défauts MEMS 2J, MEMS 3 et RC5).** La fonction de réinitialisation ECU LeopoldG originale est désormais `mems_reset_ECU_leopold`; cela empêche sa confusion avec la fonction `mems_reset_ECU` déjà utilisée par Manager.
- **Correction de carburant secondaire :** `mems_fuel_trim_2_plus` / `mems_fuel_trim_2_minus`. Cette adaptation n'est PAS une commande d'activation des injecteurs ; une valeur distincte n'est pas garantie sur toutes les variantes. Les commandes ne sont pas raccordées à une action automatique de l'interface. Sur MEMS 1.9, `7C` est également utilisé à l'initialisation : ne jamais utiliser la correction secondaire sur une famille/mode non confirmés.
- **Conversion température °F** de la DLL d'origine récupérée à partir du code x87 : décalage non signé sur 8 bits par 55, multiplication 1,8, ajout 32 et troncature modulo 256.
- **Aucun échange ECU** pendant les tests automatisés. Le workflow `.github/workflows/rosco-45-exports-x64-audit.yml` compile exclusivement la DLL Windows x64, vérifie la présence physique des 53 noms avec `GetProcAddress`, le code AMD64, les constantes et l'ABI, puis conserve le binaire en artefact **non publié dans Manager**.

**Attention :** compilation et tests sur PC sans matériel ne constituent pas une preuve de compatibilité physique avec tous les calculateurs. L'interface SESSION et ses Setup publics ne sont pas modifiés par ce workflow. Pour des fonctions permanentes ou de réinitialisation, confirmation utilisateur préalable et essai sur ECU réel requis.

## Référence binaire contrôlée

Ancienne DLL : `prebuilt-librosco/librosco.dll`

- taille : 49 672 octets ;
- SHA-256 : `a5ad466f8f1a198a6f8259a5ea5ab59775815c8cbc9637aa584db9e9c811d38f` ;
- architecture : PE32 / x86 ;
- version retournée : 0.1.12 ;
- structure `mems_data` : 60 octets ;
- 22 exports nommés.

Le header utilisé par ECU MEMS Manager est le header Haro/MEMS-Scan étendu, pas le header Colin 0.1.12 d’origine.

## Compatibilité conservée

`mems_manager_x64.dll` conserve les 22 noms exportés historiques nécessaires à la compatibilité, la structure Haro de 60 octets et les commandes directement confirmées par désassemblage :

- initialisation `CA 75 F4 D0` ;
- polling `80` (28 octets) puis `7D` (32 octets) ;
- position IAC `FB` ;
- effacement défauts `CC` ;
- heartbeat `F4` ;
- reset ECU `FA` ;
- reset adjustments `0F` ;
- primitive générique `mems_test_actuator()` ;
- primitives basses lecture/écriture/envoi de commande.

## Différences volontaires de sûreté

Le chemin de succès normal reproduit le comportement utile du binaire historique. Deux comportements erronés de l’ancien code ne sont pas reproduits :

- une trame `0x7D` tronquée est refusée au lieu d’être acceptée à cause de l’ancien test de longueur mal parenthésé ;
- `clear/reset` renvoient réellement l’état de l’échange série au lieu de forcer un succès après acquisition du mutex.

Ces différences concernent uniquement les chemins d’erreur et ne doivent pas être confondues avec une différence de protocole.

## Architecture de migration

Le target CMake interne reste momentanément nommé `rosco` pour que le code existant puisse être lié sans réécriture massive. La propriété `OUTPUT_NAME` impose toutefois le nom physique **`mems_manager_x64.dll`** et le fichier d’exports est `mems_manager_x64.def`.

La future bibliothèque doit rester extensible et distinguer explicitement :

- MEMS 1.2 / 1.3 / 1.6 / 1.9 ;
- mode diagnostic courant ;
- Mode 4 / RAM / calibration / programmation ;
- commandes validées, expérimentales et ambiguës ;
- transport série et logique protocolaire.

La cartographie détaillée est conservée dans `ROSCO_COMMAND_CARTOGRAPHY.md`.

## Validation automatique

Le workflow dédié `rosco-45-exports-x64-audit.yml` vérifie :

- production exacte de `mems_manager_x64.dll` ;
- PE32+ / AMD64 ;
- tailles ABI `frame80=28`, `frame7d=32`, `mems_data=60` ;
- les 45 exports originaux de LeopoldG avec leurs ordinals + les 8 exports additifs ;
- test ABI natif ;
- publication du marqueur `MEMS_MANAGER_X64_DLL_VALIDATION.txt`.

Le workflow d'intégration de Manager devra ultérieurement vérifier que l’exécutable x64 d’ECU MEMS Manager importe **`mems_manager_x64.dll`** et n’importe plus l’ancienne `librosco.dll` 32 bits.


## Correction isolée de compatibilité — projet DLL Manager (22/11/2026)

- Branche unique de DLL : `dll-manager` ; seules les sources, les tests et la documentation dans `librosco-x64/` sont modifiées.
- `mems_reset_ECU` (ordinal historique 39) transmet à nouveau `FA`, comme la DLL à 22 exports de Manager, tandis que `mems_reset_adjustments` (ordinal 46) transmet `0F`.
- `mems_reset_ADJ=FA`, `mems_reset_EMI=0F`, `mems_Save=81` et toutes les autres commandes existantes restent présentes.
- `mems_reset_ECU_leopold=AF` est ajouté en ordinal 54, sans changer les 53 noms et ordinals antérieurs.
- Le test série simulé vérifie le byte transmis et les erreurs de réponse de chaque réinitialisation; le test d'ABI vérifie les exports réels après compilation Windows.
- Cette étape corrige les sources de la DLL candidate **sans remplacer la DLL de SESSION 196**, sans modification du programme ou de la détection de câbles.
- **Pas de confirmation matérielle** tant qu'un essai sur calculateur réel n'a pas été réalisé. Les anciens comptes rendus de 53 exports et workflows GitHub visent les versions d'audit antérieures.

## Extension MEMS 1.2 / 1.3 / 1.6 / 1.9 et injection transitoire

La DLL inclut maintenant les nouvelles unités suivantes (sans modification du programme) :

- `family.c` / `family.h` : API pour les quatre familles indiquées, avec identification, lecture des mesures, position/déplacement IAC, défauts, adaptations, commandes et sauvegarde. Les trois familles ROSCO réutilisent les primitives historiques. MEMS 1.9 réutilise le protocole K-Line déjà présent dans la DLL après le réveil ISO 9141 effectué par le programme. Les commandes restent déclenchées explicitement ; aucune réinitialisation n'est effectuée pendant la connexion.
- `transient_injection.c` / `transient_injection.h` : six fonctions de décodage **en lecture seule** fondées sur le test AANMP002 existant dans Manager : contrôle du mode `F0`, identification `D1`, trame `80`, réponse exacte Mode 4 `0x40` (et autres mots 0x37/0x64), lecture de la paire `0x0280/0x0281`, et comparaison des mots `0x026E/0x03C8` en ticks/temps.
- **Précision** : le calcul `03C8+S(026E)` est explicitement un **candidat de comparaison** et ne devient pas une correction d'injection validée par le seul calcul. Le traitement existant de la capture (entrée/sortie Mode 4, cadence rapide, confirmation de l'ECU, restauration de session) reste intact dans Manager. La DLL n'ouvre ni ne modifie une session Mode 4 pour ces six fonctions pures.
- `serial_protocol_selftest.c` inclut les tests simulés de toutes les familles et les décodeurs d'injection. `abi_selftest.c` vérifie 71 noms d'exports physiques quand le test Windows est exécuté. `validate_protocol_guards.py` valide l'ordre des 71 exports et leurs déclarations/sources.

**Statut de validation** : code ajouté et contrôles statiques à effectuer ; les tests compilés de la DLL Windows et les échanges réels sur ECU n'ont pas été effectués par cette modification. Les fonctions primitives d'origine déjà vérifiées par l'utilisateur ne sont pas réécrites. Le Manager installé, la détection des câbles, son interface et son Setup ne sont pas modifiés.

## Extension protocoles publiés — MEMS 2J / MEMS 3 / RC5 (22/11/2026)

La présence de fonctionnalités prouvées par les outils publics RoverMEMS et MEMS Mapper (Andrew Revill) motive la reprise de protocoles diagnostiques réels, plutôt qu'une liste d'exports sans traitement.

Les nouveaux fichiers `protocol_catalogue.c/.h` ajoutent **20 véritables fonctions techniques** :

- MEMS 2J : encoder/décoder les trames avec longueur et checksum additif ; construire requêtes de mesures `21 XX`, effacement `31 CB` (préparation de la trame uniquement), clé `27 02` ; calculer la clé de défi ; identifier la nature des réponses (positive, négative, en attente) ; décoder les mesures 2J ayant une échelle documentée et consulter les bits bruts de défauts.
- MEMS 3 : trames `B8 13 F7 [LEN] [PAYLOAD] [XOR]` séparées du 2J ; requêtes `21 XX` ; réponses ; lecture des températures, pression, papillon, régime, lambda, bobines et états de défaut.
- RC5 (module airbag/SRS, pas ECU moteur) : construction et contrôle de trames, extraction des codes DTC et statut historique.
- Les formats et équations sont repris de la **documentation de protocoles et des travaux déjà consignés dans le projet**. Les sources Go de RoverMEMS sont sous licence GPLv3. Les obligations de licence doivent être étudiées avant diffusion de composants dérivés.
- Les fonctions d'encodage et d'analyse ne remplacent pas automatiquement le transport série QSerialPort actuellement utilisé par Manager : **aucune intégration ni modification de Manager**.
- `protocol_catalogue_selftest.c` teste les séquences de référence synthétiques : checksum 2J, XOR 3, messages négatifs, clé, trame RC5 et lectures mesurées. Les vecteurs de test ne sont pas présentés comme des captures de calculateurs réels.
- Le projet `CMakeLists.txt` contient la compilation du module et son exécutable de test. Les 91 noms sont spécifiés dans `mems_manager_x64.def`, tout en conservant les ordinals 1 à 71.
- **À confirmer par compilation Windows** : création effective de la DLL et exécution des tests. Aucun test matériel n'a été lancé dans ce chantier. Les fonctions de programmation/antidémarrage d'Andrew Revill ne sont pas assimilées à des lectures sans conséquence.

Références : https://andrewrevill.co.uk/MEMS3FamiliesSupported.htm et https://github.com/james-portman/rover-mems-agent
