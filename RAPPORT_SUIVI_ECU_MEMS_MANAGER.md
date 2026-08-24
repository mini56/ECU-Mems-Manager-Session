# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE POUR LES PROCHAINES DISCUSSIONS**
>
> Au début de chaque nouvelle discussion concernant ECU MEMS Manager, ce fichier doit être relu avant de reprendre le travail.
> L’assistant doit **mettre à jour ce rapport lui-même et le pousser régulièrement sur la branche `RAPPORT`**, au minimum après chaque étape technique importante, validation, découverte, changement de branche de travail ou décision d’architecture.
> Ce rapport est volontairement en **Markdown lisible directement sur GitHub**. Il ne doit pas être remplacé par un format encodé ou opaque.
> La branche `RAPPORT` sert au suivi et à la transmission entre discussions. Elle ne doit pas servir à modifier le programme de production sauf demande explicite.

Dernière mise à jour : **24 août 2026 — `mems_manager_x64.dll` créée et validée AMD64 ; validation de l’application x64 en cours**

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

## 9. Validation de MEMS Manager complet en x64 — EN COURS

Un workflow séparé existe : `.github/workflows/build-ecu-mems-x64-link-smoke.yml`.

Il utilise :

- Windows x64 ;
- Qt 5.15.2 MSVC 2019 x64 ;
- `MEMS_USE_BUNDLED_LIBROSCO_X64=ON` ;
- la nouvelle `mems_manager_x64.dll`.

Le contrôle attendu est strict :

- `ecu_mems_manager.exe` doit être PE32+ AMD64 ;
- `mems_manager_x64.dll` doit être PE32+ AMD64 ;
- l’exécutable doit importer **`mems_manager_x64.dll`** ;
- il ne doit plus importer l’ancienne **`librosco.dll`** ;
- la DLL doit conserver les 22 exports de compatibilité.

Marqueur attendu : `ECU_MEMS_X64_LINK_VALIDATION.txt`.

**État au moment de cette mise à jour : le marqueur n’est pas encore présent. Ne pas déclarer l’application complète x64 validée tant que ce fichier n’a pas été généré par GitHub Actions.**

---

## 10. Prochaine action exacte

1. Attendre/contrôler le workflow `Build ECU MEMS Manager x64 link smoke`.
2. S’il échoue, lire le job/log exact et corriger uniquement le blocage x64.
3. Dès qu’il est vert, vérifier l’EXE et la DLL produite puis mettre à jour ce rapport.
4. Ensuite seulement préparer un package x64 autonome pour essai utilisateur, sans remplacer la version 32 bits actuelle.
5. Après validation réelle sur PC et ECU, commencer l’extension du moteur protocolaire par profils MEMS/modes, sans limiter la nouvelle DLL aux fonctions actuellement utilisées par l’interface.
