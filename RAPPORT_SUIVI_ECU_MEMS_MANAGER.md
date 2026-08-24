# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE POUR LES PROCHAINES DISCUSSIONS**
>
> Au début de chaque nouvelle discussion concernant ECU MEMS Manager, ce fichier doit être relu avant de reprendre le travail.
> L’assistant doit **mettre à jour ce rapport lui-même et le pousser régulièrement sur la branche `RAPPORT`**, au minimum après chaque étape technique importante, validation, découverte, changement de branche de travail ou décision d’architecture.
> La branche `RAPPORT` sert au suivi et à la transmission entre discussions. Elle ne doit pas servir à modifier le programme de production sauf demande explicite.

Dernière mise à jour : **24 août 2026 — package portable MEMSX64 avec IA locale complète validé au vert par GitHub Actions, runtime llama.cpp + modèle Qwen inclus et testés avant packaging**

---

## 1. Règles de travail à conserver

- Dépôt principal : `mini56/ECU-Mems-Manager-Session`.
- Branche de travail x64 : **`MEMSX64`**.
- Branche de transmission : **`RAPPORT`**.
- L’utilisateur compile et teste via **GitHub Actions sous Windows**, pas avec Qt Creator.
- Préserver la version 32 bits existante pendant le développement x64 parallèle.
- Ne modifier que ce qui est demandé explicitement ou strictement nécessaire à la tâche en cours.
- Conserver l’interface sombre et responsive ; ne pas engager de refonte graphique non demandée.
- Ne pas modifier l’IA pendant un autre chantier sauf autorisation explicite.
- Les familles MEMS **1.2 / 1.3 / 1.6 / 1.9** doivent rester distinguées.
- Dans `memsinterface.h`, conserver impérativement : `void onProtocolCommandRequested(quint8 command);`.
- Ne jamais confondre polling normal `0x7D/0x80`, RAM/Mode 4, calibrations/cartes et données externes.

---

## 2. Références historiques importantes

### 2.1 IA MEMS avant x64

Branche historique IA : `lab-expert-engine`.

Package IA complet déjà validé avant le chantier x64 :

- modèle : **Qwen3-0.6B-Q8_0** ;
- runtime : **llama.cpp b10516 Windows x64 CPU** ;
- port local : **127.0.0.1:18089** ;
- runtime ZIP : `llama-b10516-bin-win-cpu-x64.zip` ;
- SHA256 runtime : `fbbbc55e0eb2e1b07f9dcb9488616c98ed47d9003b90e15e7c8c7812c4307cd3` ;
- modèle final : `ai/models/ia-mems.gguf` ;
- SHA256 modèle : `9465e63a22add5354d9bb4b99e90117043c7124007664907259bd16d043bb031`.

Derniers correctifs historiques importants :

- `cbcb8a14189b1bc013cb3519b0ba33aa3f85c072` — `Fix IA MEMS history scroll and factual answers` ;
- `896a59f762e648ce50023121491693ee70ad162d` — `Build IA MEMS quality patch`.

### 2.2 Base Andrew Revill / MEMSTools

Le lot brut complet 1600 reste conservé dans `database/reference/research_enrichment_1600.qz64`, environ 33,3 Mo compressés :

- 478 730 propriétés DEF ;
- 45 934 propriétés DIM ;
- 3 524 519 cellules non vides de corrélations.

Il ne faut pas décider automatiquement de charger les 3,5 millions de cellules brutes même en x64 sans mesurer mémoire, temps de démarrage et intérêt réel.

---

## 3. `librosco.dll` historique x86 — référence de compatibilité

Fichier : `prebuilt-librosco/librosco.dll`.

Identité vérifiée :

- taille : **49 672 octets** ;
- SHA256 : `a5ad466f8f1a198a6f8259a5ea5ab59775815c8cbc9637aa584db9e9c811d38f` ;
- architecture : PE32 / x86 ;
- version interne : **0.1.12** ;
- `mems_data` : **60 octets** ;
- **22 exports** nommés.

Les 22 exports historiques :

`mems_cleanup`, `mems_clear_faults`, `mems_connect`, `mems_disconnect`, `mems_get_lib_version`, `mems_heartbeat`, `mems_init`, `mems_init_link`, `mems_is_connected`, `mems_lock`, `mems_move_iac`, `mems_openserial`, `mems_read`, `mems_read_iac_position`, `mems_read_raw`, `mems_read_serial`, `mems_reset_ECU`, `mems_reset_adjustments`, `mems_send_command`, `mems_test_actuator`, `mems_unlock`, `mems_write_serial`.

Commandes directement confirmées par désassemblage du vrai binaire :

- init : **`CA 75 F4 D0`** ;
- polling : `0x80` puis `0x7D` ;
- trame 80 : **28 octets** ;
- trame 7D : **32 octets** ;
- IAC : `0xFB` ;
- clear faults : `0xCC` ;
- heartbeat : `0xF4` ;
- reset ECU : **`0xFA`** ;
- reset adjustments : **`0x0F`**.

---

## 4. Nouvelle DLL protocole native x64 — VALIDÉE

Nom décidé : **`mems_manager_x64.dll`**.

- code source : `librosco-x64/` ;
- DEF : `librosco-x64/mems_manager_x64.def` ;
- target CMake interne conservé momentanément sous le nom `rosco` ;
- output réel : `mems_manager_x64.dll`.

Validation native obtenue :

- architecture **IMAGE_FILE_MACHINE_AMD64** ;
- format **PE32+** ;
- `frame80` : 28 octets ;
- `frame7d` : 32 octets ;
- `mems_data` : 60 octets ;
- les **22 exports historiques** sont présents.

Améliorations de sûreté conservées par rapport à l’ancien code :

- refus d’une trame `0x7D` tronquée ;
- clear/reset renvoient l’état réel de l’échange série au lieu de forcer un succès.

Règle de cartographie : un octet de commande ne doit pas être interprété sans connaître la famille ECU et le mode. Les conflits potentiels `F4`, `F7`, `F8`, `D1`, `D3`, `FA`, `0F`, `7C` doivent rester contextualisés.

---

## 5. Liaison de MEMS Manager complet en x64 — VALIDÉE

Workflow : `.github/workflows/build-ecu-mems-x64-link-smoke.yml`.

Run de validation : **32709187615**.

Corrections MSVC minimales appliquées dans `mainwindow.cpp`, sans changement de logique :

- `5e53ef22c4b848397aa5abdd453378f76f1e3988` — `or` → `||` ;
- `722a8278c5974e4b016a59ed6b1de70baee01f40` — `and` → `&&` ;
- `09564db70f729e1a3d78f974918d0d24bafbc982` — autres `or` → `||`.

Résultat final :

- `ecu_mems_manager.exe` : PE32+ AMD64 ;
- `mems_manager_x64.dll` : PE32+ AMD64 ;
- l’EXE importe `mems_manager_x64.dll` : oui ;
- l’EXE importe l’ancienne `librosco.dll` : non.

Marqueur final historique : commit `2e2165b8001482647d4a3606df4f4051ac63cf03`.

---

## 6. Premier package portable x64 sans IA — VALIDÉ puis testé sur PC secondaire

Workflow : `.github/workflows/build-ecu-mems-x64-package.yml`.

Premier package x64 validé sans le gros runtime IA :

- run **32713710308** ;
- commit marqueur : `494c8993ffc218195639d974b7b2a7c81b3f8541` ;
- 29 PE contrôlés, tous AMD64 ;
- 243 fichiers ;
- base : **85 ECU / 140 affectations / 91 commandes** ;
- smoke launch 5 secondes : OK ;
- aucune ancienne `librosco.dll` x86.

Essai réel utilisateur sur un PC secondaire qui n’avait jamais eu ECU MEMS Manager :

- l’application reste ouverte ;
- navigation étendue dans pratiquement tous les onglets sans plantage ;
- la base MEMS est disponible ;
- l’onglet IA passe proprement en secours avec : `Moteur llama.cpp local absent du dossier IA.` ;
- ceci était normal car ce premier ZIP x64 ne contenait ni dossier `ai/`, ni `llama-server.exe`, ni `.gguf` ;
- défauts mojibake/UTF-8 visibles ;
- défauts responsive/rognage visibles en 1920×1080 sur certaines pages ;
- ces défauts ne sont pas la priorité actuelle et ne doivent pas déclencher une refonte sans demande explicite.

Conclusion obtenue : le noyau x64 et la navigation générale sont stables sur un PC propre lorsque l’IA locale n’est pas présente.

---

## 7. Persistance Windows et différence PC principal / PC secondaire

L’utilisateur a précisé que chaque version est téléchargée, extraite et exécutée depuis **son propre dossier séparé**. Une contamination directe entre dossiers de versions est donc peu probable.

En revanche, MEMS Manager utilise plusieurs états persistants hors dossier de l’EXE.

### 7.1 QSettings partagé

`main.cpp` configure :

- application : `ECU Mems Manager` ;
- organisation : `ECU Mems Manager` ;
- `QSettings(QSettings::IniFormat, QSettings::UserScope, PROJECTNAME)`.

Réglages persistants notamment :

- `SerialDevice` ;
- `TemperatureUnits` ;
- `Theme` ;
- `Language` ;
- `LanguageConfigured` ;
- `DesktopShortcut`.

Des versions situées dans des dossiers différents peuvent donc partager ces réglages utilisateur.

### 7.2 AppLocalData partagé

Base/cache référence :

- `QStandardPaths::AppLocalDataLocation/reference` ;
- index global : `reference/mems_global_search_r1.sqlite`.

Cache IA :

- `QStandardPaths::AppLocalDataLocation/ia-mems` ;
- DB : `ia_mems_reference_r<manifestRevision>.sqlite`.

Point important : si la DB IA exacte existe déjà, `ExpertRuntimeDatabase::buildOrOpen()` peut la réutiliser directement. C’est une différence plausible entre PC principal et PC propre même si SQLite lui-même n’est pas lié à x86/x64.

La base principale modifiable reste en revanche dossier-local : `<appdir>/database/ecu_mems_manager.sqlite`.

### 7.3 Variables d’environnement et serveur externe

`LocalAiClient::discoverAssets()` cherche d’abord :

- `MEMS_AI_SERVER` ;
- `MEMS_AI_MODEL` ;

puis dans le dossier courant de l’application.

Un ancien chemin stocké dans une variable d’environnement peut donc faire utiliser à une nouvelle version un runtime/modèle situé ailleurs. MEMS Manager ne scanne toutefois pas arbitrairement le disque à la recherche d’anciens dossiers.

Un serveur déjà actif sur **127.0.0.1:18089** peut également influer sur le comportement.

### 7.4 Désinstalleur

Le désinstalleur actuel coche par défaut **conserver le profil utilisateur**.

- en gardant le profil : QSettings/AppLocalData ne sont pas supprimés ;
- si la suppression du profil est explicitement activée : le QSettings utilisateur et l’AppLocalData peuvent être supprimés ;
- les variables d’environnement `MEMS_AI_SERVER` / `MEMS_AI_MODEL` ne sont pas effacées par le désinstalleur ;
- un runtime externe arbitraire n’est pas supprimé ;
- un `llama-server.exe` externe déjà lancé n’est pas automatiquement neutralisé.

Comme plusieurs versions partagent la même identité Qt, supprimer le profil depuis une version installée peut toucher des données partagées par d’autres versions. Préférer une isolation/rénommage réversible plutôt qu’une suppression brutale pour le diagnostic.

---

## 8. Diagnostics GitHub x64 IA du 24 août 2026

Plusieurs workflows de diagnostic ont été ajoutés sur `MEMSX64` pour reproduire le comportement sans modifier la logique normale du programme.

### 8.1 Run 32717558263 — affiché rouge mais test IA réellement PASS

Workflow IA x64 smoke :

- compilation x64 : OK ;
- lancement IA forcé pour le test : OK ;
- résultat réel : **`IA_X64_SMOKE=PASS`** ;
- application encore vivante après **20 secondes**.

Le run a été marqué rouge uniquement parce que l’étape finale d’enregistrement Git a échoué : le workflow avait des modifications locales de test non commitées, puis un autre commit avait avancé la branche. Le `git pull --rebase` a refusé de travailler avec les changements locaux et le push est ensuite devenu non-fast-forward.

Donc ce rouge **n’indique pas un crash IA**.

### 8.2 Run 32718093195 — diagnostic sélection IA par barre latérale : VERT

Le workflow a compilé l’application et sélectionné l’IA via le chemin de navigation latérale. Toutes les étapes sont passées au vert.

### 8.3 Run 32721284999 — cache `ExpertRuntimeDatabase` x64 : VERT

Le self-test a construit le cache IA puis rouvert/réutilisé le même cache en x64 : **succès**.

Cette validation réduit la probabilité qu’une simple réutilisation d’un SQLite IA existant soit intrinsèquement incompatible avec le binaire x64.

### 8.4 Run 32721873137 — rouge de permissions, pas erreur de package

Le workflow temporaire a correctement généré localement la modification du workflow de package, mais le push a été refusé par GitHub :

`refusing to allow a GitHub App to create or update workflow ... without workflows permission`

Cause exacte : le token `GITHUB_TOKEN` du bot du workflow ne peut pas modifier un autre fichier `.github/workflows/...` de cette manière.

La correction a ensuite été appliquée directement par l’assistant via GitHub, sans passer par ce bot auto-modificateur.

---

## 9. Package MEMSX64 avec IA locale complète — VALIDÉ AU VERT

### 9.1 Modification du workflow

Commit direct sur `MEMSX64` :

**`58bd6a1f74da991ddf3b831fcfb55d2716755084` — `Bundle validated local IA runtime in x64 package`**.

Le workflow `.github/workflows/build-ecu-mems-x64-package.yml` télécharge désormais pendant le build :

- le runtime historique validé **llama.cpp b10516 Windows x64 CPU** ;
- le modèle historique validé **Qwen3-0.6B-Q8_0** ;
- vérifie les SHA256 attendus ;
- copie les DLL VC++ x64 app-local nécessaires ;
- ajoute les licences llama.cpp et Qwen ;
- lance réellement `llama-server.exe` avec `ai-mems.gguf` ;
- attend `/health` sur `127.0.0.1:18089` ;
- n’assemble le package que si le moteur IA a réellement chargé le modèle ;
- vérifie ensuite la présence du runtime, du modèle, des DLL VC++ et des licences dans le package final.

### 9.2 Run 32741977123 — VERT COMPLET

Résultat GitHub Actions : **SUCCESS**.

Étapes explicitement validées :

- Qt x64 : OK ;
- configuration x64 : OK ;
- compilation application : OK ;
- téléchargement + SHA256 runtime IA : OK ;
- téléchargement + SHA256 modèle IA : OK ;
- **llama-server x64 lancé et modèle chargé : OK** ;
- assemblage portable : OK ;
- architecture/contenu : OK ;
- base MEMS hors ligne : OK ;
- lancement fumée de l’application : OK ;
- upload artefact : OK ;
- marqueur final : OK.

Marqueur final `ECU_MEMS_X64_PACKAGE_VALIDATION.txt` :

- `workflow_run_id=32741977123` ;
- application = **PE32+ AMD64** ;
- DLL protocole = **PE32+ AMD64** ;
- import `mems_manager_x64.dll` = oui ;
- import historique `librosco.dll` = non ;
- package contient historique `librosco.dll` = non ;
- PE contrôlés : **90** ;
- nombre de fichiers : **309** ;
- taille totale avant rapport : **779 909 977 octets** ;
- SHA256 EXE : `6884c89fa25c061173de5094c0322a71f9cee51da90efb824cf2d9208cf6279a` ;
- SHA256 DLL protocole : `6c3369525398f548d745c7d2769c75c59d2bf7834d660fb108ea3878baa23482`.

Commit automatique du marqueur final :

**`ef2bbcafac348abcb9ac4c8edbb53b5772718849` — `Validate ECU MEMS Manager portable x64 package [skip ci]`**.

### 9.3 Artefact prêt pour essai utilisateur

- nom : **`ECU-MEMS-Manager-x64-Windows-Test`** ;
- artifact id : **`9525708940`** ;
- taille GitHub : **779 959 273 octets** ;
- digest ZIP GitHub : **`sha256:d2127078f8a4db92103ecebd4fd9585f419b81edb6197ab7ce68265637c9d6c7`** ;
- run : **32741977123** ;
- expiration GitHub annoncée : **22 novembre 2026**.

Le connecteur utilisé par l’assistant ne peut pas rapatrier localement cet artefact car il dépasse sa limite de téléchargement de 512 Mio. Cela ne remet pas en cause l’artefact GitHub : l’upload du workflow est vert et l’artefact est bien présent dans le run.

---

## 10. Ce qui reste à valider sur une vraie machine

Le package x64 complet avec IA est maintenant validé côté build et packaging, mais il reste les essais réels :

1. télécharger l’artefact du run **32741977123** ;
2. l’extraire dans son propre dossier ;
3. lancer `ecu_mems_manager.exe` ;
4. vérifier que l’application reste ouverte ;
5. ouvrir **IA MEMS** et vérifier que le statut passe bien à IA locale prête et non plus `Moteur llama.cpp local absent...` ;
6. poser une ou deux questions simples à l’IA et vérifier la réponse réelle ;
7. ensuite seulement tester câble/COM + ECU réel.

Si le PC principal ferme encore alors que le PC secondaire reste stable avec exactement le même package, isoler sans destruction :

- variables `MEMS_AI_SERVER` / `MEMS_AI_MODEL` ;
- éventuel `llama-server.exe` déjà actif / port 18089 ;
- QSettings partagé ;
- AppLocalData référence/search ;
- AppLocalData `ia-mems` ;
- puis seulement les différences machine (VC runtime, antivirus/EDR, pilote/graphique, etc.).

Ne supprimer aucun profil/cache avant d’avoir essayé un renommage/isolation réversible.

---

## 11. Détection automatique câble / port COM — exigence future à conserver

La détection automatique doit être générale à MEMS Manager, pas réservée au MEMS 1.9.

Comportement cible :

- détecter automatiquement les interfaces branchées ;
- afficher par exemple `COM5 — FTDI FT232 — câble détecté` ;
- si plusieurs interfaces existent, afficher COM + type d’interface ;
- lors de **Connecter**, vérifier que l’interface répond au protocole attendu ;
- messages explicites selon le cas : `câble incompatible`, `aucun ECU détecté`, `mauvais type d’interface`.

La validation câble/ECU de la x64 reste en attente d’un test physique utilisateur.

---

## 12. Injection / RAM — règles à ne pas perdre

- Ne jamais calculer le temps d’injection à partir du polling normal `0x7D/0x80`.
- Les trames normales ne contiennent pas les adresses RAM étudiées pour l’injection.
- Adresses RAM importantes déjà identifiées dans les recherches : **`0x03C8`, `0x026E`, `0x0280`**.
- L’onglet Injection est placé entre **Aperçu** et **Réglage**.
- Le dwell / temps bobine appartient à cet onglet.
- Éviter de mélanger des modes de lecture différents dans la même page si cela augmente inutilement le trafic et ralentit le polling.
- Plage dwell de référence affichée : environ **1,9 à 3,1 ms vers 14 V**.

---

## 13. Prochaine action exacte

**Priorité immédiate : essai utilisateur du nouvel artefact x64 complet avec IA du run 32741977123.**

Ne pas reprendre pour l’instant l’extension protocolaire ni la refonte responsive tant que ce package n’a pas été essayé sur le PC utilisateur.

Après validation application + IA sur PC réel :

1. test câble et détection COM ;
2. test connexion ECU ;
3. contrôle des mesures normales déjà connues ;
4. ensuite seulement reprendre l’extension du moteur protocolaire par familles/modes ou la tâche explicitement demandée par l’utilisateur.
