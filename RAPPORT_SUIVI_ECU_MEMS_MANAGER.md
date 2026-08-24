# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE POUR LES PROCHAINES DISCUSSIONS**
>
> Au début de chaque nouvelle discussion concernant ECU MEMS Manager, ce fichier doit être relu avant de reprendre le travail.
> L’assistant doit **mettre à jour ce rapport lui-même et le pousser régulièrement sur la branche `RAPPORT`**, au minimum après chaque étape technique importante, validation, découverte, changement de branche de travail ou décision d’architecture.
> Ce rapport est volontairement en **Markdown lisible directement sur GitHub**. Il ne doit pas être remplacé par un format encodé ou opaque.
> La branche `RAPPORT` sert au suivi et à la transmission entre discussions. Elle ne doit pas servir à modifier le programme de production sauf demande explicite.

Dernière mise à jour : **24 août 2026 — package portable ECU MEMS Manager x64 validé, prêt pour essai utilisateur Windows/ECU**

---

## 1. Règles de travail à conserver

- Dépôt principal : `mini56/ECU-Mems-Manager-Session`.
- Branche de travail x64 : **`MEMSX64`**.
- Branche de transmission : **`RAPPORT`**.
- L’utilisateur compile et teste via **GitHub Actions sous Windows**, pas avec Qt Creator.
- Ne modifier que ce qui est explicitement demandé et ne pas faire régresser les fonctions existantes.
- Conserver l’interface sombre et responsive.
- Les familles MEMS **1.2 / 1.3 / 1.6 / 1.9** doivent rester distinguées.
- Dans `memsinterface.h`, conserver impérativement `void onProtocolCommandRequested(quint8 command);`.
- Ne pas confondre polling `0x7D/0x80`, RAM/Mode 4, calibrations/cartes et données externes.

---

## 2. État IA MEMS validé avant le chantier x64

Branche historique IA : `lab-expert-engine`.

Derniers correctifs importants :

- `cbcb8a14189b1bc013cb3519b0ba33aa3f85c072` — `Fix IA MEMS history scroll and factual answers`
- `896a59f762e648ce50023121491693ee70ad162d` — `Build IA MEMS quality patch`

Package IA complet validé :

- modèle : `Qwen3-0.6B-Q8_0`
- runtime : `llama.cpp b10516 Windows x64 CPU`
- artefact : `ECU-MEMS-Manager-IA-MEMS-Windows-Lab`
- taille : environ 798 Mo
- SHA256 : `753550992b43fff6a569752f818f5b5d208de3210d734d6d38e381b830112ec3`

Ne pas modifier l’IA pendant le chantier x64 sauf demande explicite.

---

## 3. Base Andrew Revill / MEMSTools — lot 1600

Le lot complet est présent : `database/reference/research_enrichment_1600.qz64`, environ **33,3 Mo compressés**.

Couche de conservation complète :

- 478 730 propriétés DEF ;
- 45 934 propriétés DIM ;
- 3 524 519 cellules non vides de corrélations.

Le générateur utilise `csv.reader`, donc les virgules/champs CSV ne sont plus le blocage principal.

Le lot 1600 brut est livré et audité mais volontairement **non chargé dans le runtime SQLite 32 bits** pour éviter la saturation de l’espace d’adressage. Les éléments Andrew déjà compris sont intégrés sous forme sémantique dans les lots précédents, notamment 1500–1540.

Même en x64, ne pas décider automatiquement de charger les 3,5 millions de cellules brutes : il faudra mesurer mémoire, temps de démarrage et intérêt réel pour la recherche IA.

---

## 4. Pourquoi le chantier x64 a été lancé

La version actuelle 32 bits fonctionne sur Windows 11 64 bits grâce à WOW64. Le problème principal du 32 bits n’est donc pas Windows 11 mais la limitation d’espace mémoire, devenue gênante avec les grosses bases et les futurs usages IA.

Décision :

- conserver le MEMS Manager 32 bits actuel comme version compatible/historique ;
- développer en parallèle une vraie version **x64 native** ;
- ne pas doubler systématiquement les builds de production : la x64 reste un chantier/lab tant qu’elle n’est pas validée.

---

## 5. Audit complet de la `librosco.dll` historique — TERMINÉ

Fichier actuel : `prebuilt-librosco/librosco.dll`.

Identité vérifiée :

- taille : **49 672 octets** ;
- SHA-256 : `a5ad466f8f1a198a6f8259a5ea5ab59775815c8cbc9637aa584db9e9c811d38f` ;
- architecture : **IMAGE_FILE_MACHINE_I386 / PE32 / x86 32 bits** ;
- version interne retournée : **0.1.12** ;
- structure `mems_data` réellement utilisée : **60 octets** ;
- 22 exports nommés.

Rapports techniques disponibles sur `RAPPORT` :

- `RAPPORT_LIBROSCO_BINAIRE.md`
- `RAPPORT_LIBROSCO_DESASSEMBLAGE.md`
- `RAPPORT_LIBROSCO_LEOPOLD_COMPARAISON.md`

### 5.1 Les 22 exports du vrai binaire

`mems_cleanup`, `mems_clear_faults`, `mems_connect`, `mems_disconnect`, `mems_get_lib_version`, `mems_heartbeat`, `mems_init`, `mems_init_link`, `mems_is_connected`, `mems_lock`, `mems_move_iac`, `mems_openserial`, `mems_read`, `mems_read_iac_position`, `mems_read_raw`, `mems_read_serial`, `mems_reset_ECU`, `mems_reset_adjustments`, `mems_send_command`, `mems_test_actuator`, `mems_unlock`, `mems_write_serial`.

Le header Haro actuel déclare davantage de wrappers que la DLL n’en exporte. La primitive `mems_test_actuator()` permet néanmoins d’envoyer des commandes supplémentaires sans créer un wrapper pour chaque octet.

### 5.2 Commandes directement confirmées par désassemblage

- init : **`CA 75 F4 D0`** ;
- polling : `0x80` puis `0x7D` ;
- trame 80 : **28 octets** ;
- trame 7D : **32 octets** ;
- position IAC : `0xFB` ;
- effacement défauts : `0xCC` ;
- heartbeat : `0xF4` ;
- `mems_reset_ECU()` : **`0xFA`** ;
- `mems_reset_adjustments()` : **`0x0F`**.

Ces deux dernières valeurs sont la référence de compatibilité avec la DLL réellement utilisée, même lorsque certains forks externes attribuent d’autres significations à `FA` ou `0F`.

---

## 6. Sources ROSCO/librosco retrouvées

Sources principales à conserver dans la cartographie :

1. `colinbourassa/librosco` — source C historique ;
2. `haro78/MEMS-Scan` — header correspondant à celui utilisé par ECU MEMS Manager ;
3. `LeopoldG/Source-librosco` et dépôts associés — extensions nombreuses mais conflits sémantiques sur certaines commandes ;
4. `james-portman/rover-mems-agent` — autre implémentation utile pour comparer MEMS 1.x et MEMS 1.9.

La DLL publique Leopold examinée est une lignée plus étendue et ne doit pas être utilisée directement en remplacement de notre binaire historique sans contrôle commande par commande.

---

## 7. Cartographie protocolaire : règle d’architecture

Le fichier de travail `librosco-x64/ROSCO_COMMAND_CARTOGRAPHY.md` est maintenant présent sur `MEMSX64`.

Règle essentielle : **un octet de commande ne doit pas être interprété sans connaître la famille ECU et le mode diagnostic**.

Certaines valeurs peuvent avoir des significations différentes en mode diagnostic normal et en Mode 4. Exemples à traiter avec prudence : `F4`, `F7`, `F8`, `D1`, `D3`, `FA`, `0F`, `7C`.

Architecture cible :

- transport série générique ;
- moteur générique envoi/réception ;
- profils MEMS 1.2 / 1.3 / 1.6 / 1.9 ;
- contexte de mode explicite ;
- commandes classées `confirmé`, `source externe`, `expérimental`, `conflit` ;
- Mode 4/RAM/calibration/programming isolé du polling courant.

---

## 8. Nouvelle DLL x64 — NOM DÉCIDÉ ET VALIDÉ

Décision utilisateur : la nouvelle DLL ne doit pas s’appeler `librosco64.dll`.

Nom retenu : **`mems_manager_x64.dll`**.

Signification :

- `librosco.dll` = ancienne DLL historique x86 32 bits ;
- `mems_manager_x64.dll` = nouvelle DLL native x64 d’ECU MEMS Manager.

Le code source se trouve actuellement dans le sous-dossier historique `librosco-x64/` pour éviter une migration de chemins inutile pendant les premiers tests. Le target CMake interne reste momentanément `rosco` pour préserver la compatibilité source, mais son `OUTPUT_NAME` est **`mems_manager_x64`**.

Fichier d’exports : `librosco-x64/mems_manager_x64.def`.

### 8.1 Validation GitHub Actions obtenue

Marqueur : `MEMS_MANAGER_X64_DLL_VALIDATION.txt`.

Résultat : **VERT**.

- filename : `mems_manager_x64.dll`
- architecture : **IMAGE_FILE_MACHINE_AMD64**
- format : **PE32+**
- `frame80` : 28 octets
- `frame7d` : 32 octets
- `mems_data` : 60 octets
- exports : **22 noms historiques de compatibilité exactement présents**
- taille de la DLL validée : **16 384 octets**
- SHA256 : `eb524976bbd25ae7cebf01520c209e92b3ee35a40f42c8551d49e3ea222f532f`

Commit automatique de validation : **`667bb8c9190d326de93b28314b2e2703fc1d9063`** — `Validate mems_manager_x64 native DLL [skip ci]`.

### 8.2 Améliorations de sûreté conservées

Deux comportements manifestement erronés de l’ancien code ne sont pas reproduits :

- une trame `0x7D` tronquée est refusée ;
- `clear/reset` renvoient l’état réel de l’échange série au lieu de forcer un succès après acquisition du mutex.

Ces différences concernent les chemins d’erreur, pas la signification des commandes ECU.

---

## 9. Validation de MEMS Manager complet en x64 — VALIDÉE

Workflow : `.github/workflows/build-ecu-mems-x64-link-smoke.yml`.

Configuration utilisée :

- Windows x64 ;
- Qt 5.15.2 MSVC 2019 x64 ;
- `MEMS_USE_BUNDLED_LIBROSCO_X64=ON` ;
- nouvelle `mems_manager_x64.dll`.

Le contrôle final demandé était :

- `ecu_mems_manager.exe` PE32+ AMD64 ;
- `mems_manager_x64.dll` PE32+ AMD64 ;
- import de **`mems_manager_x64.dll`** ;
- absence d’import de l’ancienne **`librosco.dll`** ;
- conservation des 22 exports de compatibilité.

Marqueur : `ECU_MEMS_X64_LINK_VALIDATION.txt`.

### 9.1 Run n° 32709187615 — historique des blocages MSVC

Le premier passage du run **`32709187615`** a confirmé :

- installation de Qt 5.15.2 MSVC x64 : **OK** ;
- configuration CMake x64 : **OK** ;
- compilation de `mems_manager_x64.dll` dans le build de l’application : **OK** ;
- compilation du self-test de recherche : **OK** ;
- compilation de nombreux fichiers C++ de MEMS Manager : **OK**.

Le premier blocage était en `mainwindow.cpp` ligne ~902 : utilisation de l’opérateur alternatif `or` avec MSVC. Les nombreuses erreurs suivantes étaient des cascades syntaxiques.

La reprise du 24 août 2026 a ensuite suivi strictement la règle : **une incompatibilité observée, une correction minimale, un nouveau build**.

Corrections appliquées dans `mainwindow.cpp`, sans changement de logique :

1. ligne ~902 : `or` → `||` dans la condition `idle_switch / uk3` ; commit **`5e53ef22c4b848397aa5abdd453378f76f1e3988`** — `Fix MSVC logical operator in mainwindow` ;
2. ligne ~967 : les trois `and` → `&&` dans la condition IAC ; commit **`722a8278c5974e4b016a59ed6b1de70baee01f40`** — `Fix next MSVC logical and operators in mainwindow` ;
3. ligne ~971 : les deux `or` → `||` dans la condition DTC ; commit **`09564db70f729e1a3d78f974918d0d24bafbc982`** — `Fix next MSVC logical or operators in mainwindow`.

### 9.2 Validation complète obtenue — VERT

Après la troisième correction, le workflow **`Build ECU MEMS Manager x64 link smoke`**, run **`32709187615`**, est passé entièrement au vert :

- installation Qt 5.15.2 MSVC 2019 x64 : **OK** ;
- configuration CMake x64 : **OK** ;
- compilation de `mems_manager_x64.dll` : **OK** ;
- compilation et édition de liens de `ecu_mems_manager.exe` : **OK** ;
- validation PE/imports : **OK** ;
- upload de l’artefact `ecu-mems-manager-x64-link-smoke` : **OK** ;
- écriture du marqueur final `ECU_MEMS_X64_LINK_VALIDATION.txt` : **OK**.

Validation finale enregistrée :

- `ecu_mems_manager.exe` : **PE32+ AMD64** ;
- taille EXE : **1 823 744 octets** ;
- SHA256 EXE : `3abcb0b6c3cb2982d642829cbd398dd1069da8a3d7bb1458ae8813dfa541a8cc` ;
- DLL protocole : **`mems_manager_x64.dll` PE32+ AMD64** ;
- taille DLL : **16 384 octets** ;
- SHA256 DLL : `720f8c8448fea6a8da94ef3398a14a84abb515e7f2fbc33b6e8bb880cb8f0999` ;
- l’EXE importe `mems_manager_x64.dll` : **oui** ;
- l’EXE importe l’ancienne `librosco.dll` : **non** ;
- exports de compatibilité de la DLL : **22**.

Commit automatique du marqueur vert sur `MEMSX64` : **`2e2165b8001482647d4a3606df4f4051ac63cf03`** — `Validate ECU MEMS Manager native x64 link [skip ci]`.

Artefact GitHub Actions :

- nom : `ecu-mems-manager-x64-link-smoke` ;
- artifact id : **`9514435735`** ;
- taille ZIP côté GitHub : **689 020 octets** ;
- digest : `sha256:ccc2e246de54cada01a3aef8c74b5d903ba8f62313560bfef98ae4b6404b873b`.

**État : la chaîne de compilation et de liaison native x64 est validée.**

---

## 10. Package portable x64 autonome — VALIDÉ

Workflow : `.github/workflows/build-ecu-mems-x64-package.yml`.

Objectif : produire un dossier Windows x64 de test réellement autonome à partir de la branche `MEMSX64`, sans remplacer la version 32 bits et sans embarquer l’ancienne `librosco.dll` x86.

### 10.1 Corrections du workflow de package

Le premier essai a montré deux problèmes de validation du packaging, pas du programme :

1. sous Visual Studio/MSVC, les ressources générées/copées par CMake sont placées sous `build-package-x64/Release/`. Les chemins du workflow vers `translations` et `database` ont donc été corrigés vers `Release/translations` et `Release/database` ; commit **`fc13dd54ef4f148645b6c8258f8fdc288b322a83`** — `Fix x64 package Release resource paths` ;
2. `Qt5Charts.dll` était exigée arbitrairement par le validateur alors que `windeployqt` et la table d’imports de l’EXE montrent qu’elle n’est pas une dépendance runtime de ce binaire. Cette fausse exigence a été retirée ; commit **`c6e2de29e746b074756be6bfa929d4824af80021`** — `Stop requiring unused Qt5Charts runtime DLL`.

Le workflow a aussi été rendu traçable avec un marqueur de démarrage et un marqueur d’échec ; commit **`9ea322a9faefbe11c1af5485afe4694dc9dbf4fa`** — `Add x64 package workflow run markers`.

`windeployqt` identifie comme dépendances directes de l’EXE : `Qt5Core`, `Qt5Gui`, `Qt5Network`, `Qt5SerialPort`, `Qt5Sql`, `Qt5Widgets`, et ajoute `Qt5Svg` pour les plugins SVG. Il déploie aussi les plugins Windows nécessaires, notamment `platforms/qwindows.dll` et `sqldrivers/qsqlite.dll`.

### 10.2 Run n° 32713710308 — VERT

Le run **`32713710308`** est entièrement passé au vert :

- compilation native x64 : **OK** ;
- assemblage du dossier portable : **OK** ;
- validation architecture/contenu : **OK** ;
- validation de la base MEMS hors ligne : **OK** ;
- lancement fumée de l’application pendant 5 secondes : **OK** ;
- upload de l’artefact : **OK** ;
- écriture du marqueur `ECU_MEMS_X64_PACKAGE_VALIDATION.txt` : **OK**.

Valeurs du marqueur final :

- application : **PE32+ AMD64** ;
- DLL protocole : **`mems_manager_x64.dll` PE32+ AMD64** ;
- l’EXE importe `mems_manager_x64.dll` : **oui** ;
- l’EXE importe `librosco.dll` : **non** ;
- le package contient `librosco.dll` : **non** ;
- binaires PE contrôlés : **29**, tous AMD64 ;
- nombre total de fichiers : **243** ;
- taille totale avant ajout du rapport de validation : **90 988 029 octets** ;
- SHA256 EXE : `2ee1635c62f3d51bbf9057dc368a9110353fa6631bd23c8e61998fabdac5197a` ;
- SHA256 DLL : `0543a7355aa02d0c5c73b645b425c85cde211991d575ac12a9c53a79745907d2`.

Contrôle de la base reconstruite depuis les quatre seeds `.qz64` :

- **85 ECU** ;
- **140 fitments / affectations** ;
- **91 commandes protocole**.

Le smoke-test a lancé `ecu_mems_manager.exe` depuis le dossier portable et vérifié qu’il restait actif pendant 5 secondes : **OK**.

Commit automatique du marqueur vert sur `MEMSX64` : **`494c8993ffc218195639d974b7b2a7c81b3f8541`** — `Validate ECU MEMS Manager portable x64 package [skip ci]`.

Artefact GitHub Actions prêt pour essai utilisateur :

- nom : **`ECU-MEMS-Manager-x64-Windows-Test`** ;
- artifact id : **`9515199032`** ;
- taille ZIP : **46 258 185 octets** ;
- digest ZIP : `sha256:89e27ba769279d9568a913211dc62d5bdb6b8026a500ea98c3b9fa78acba0fde` ;
- expiration GitHub annoncée : **22 novembre 2026**.

Le package x64 de test n’intègre pas volontairement le gros package IA local ; le chantier x64 actuel n’a pas modifié l’IA.

**État actuel : compilation x64, liaison x64 et package portable autonome sont validés par GitHub Actions. Il reste la validation fonctionnelle réelle sur le PC de l’utilisateur et sur un ECU.**

---

## 11. Prochaine action exacte

1. Faire télécharger et extraire l’artefact **`ECU-MEMS-Manager-x64-Windows-Test`** du run **`32713710308`**.
2. Sur le PC utilisateur, lancer `ecu_mems_manager.exe` directement depuis le dossier extrait, sans toucher à l’installation 32 bits actuelle.
3. Vérifier d’abord : démarrage, interface sombre/responsive, traductions, onglets, base MEMS, absence d’erreur DLL.
4. Vérifier ensuite la détection du câble/port COM et la connexion à un ECU réel, puis les lectures normales déjà connues.
5. Ne considérer la x64 comme fonctionnellement validée qu’après ce test PC + ECU.
6. Consigner immédiatement le résultat de ce test dans ce rapport.
7. Ensuite seulement reprendre l’extension du moteur protocolaire par profils MEMS/modes, sans limiter `mems_manager_x64.dll` aux seules fonctions actuellement utilisées par l’interface.
