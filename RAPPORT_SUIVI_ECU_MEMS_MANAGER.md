# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE POUR LES PROCHAINES DISCUSSIONS**
>
> Au début de chaque nouvelle discussion concernant ECU MEMS Manager, ce fichier doit être relu avant de reprendre le travail.
> L’assistant doit **mettre à jour ce rapport lui-même et le pousser régulièrement sur la branche `RAPPORT`**, au minimum après chaque étape technique importante, validation, découverte, changement de branche de travail ou décision d’architecture.
> La branche `RAPPORT` sert au suivi et à la transmission entre discussions. Elle ne doit pas servir à modifier le programme de production sauf demande explicite.

Dernière mise à jour : **24 août 2026 — NOUVELLE DÉCISION D’ARCHITECTURE : le 32 bits fonctionnel reste figé sur sa branche historique et sert de référence de secours ; il ne doit plus être modifié pour faire avancer le produit principal. BUILD #14 / v1.0.14 reste la référence matérielle prouvée. La voie officielle de développement redevient MEMS Manager x64 sur `MEMSX64`. Priorité absolue : connexion ECU réelle et protocole avant l’IA. Premier jalon : produire une version x64 complète du programme avec `mems_manager_x64.dll`, sans runtime/modèle IA local empaqueté pour isoler le cœur ECU, avec contrôles de compilation et d’architecture renforcés. Une fois le x64 validé sur vrai ECU (identification, 7D/80, Mode 4, retour au mode normal, déconnexion/reconnexion, acquisition), réintégrer immédiatement l’IA x64 déjà connue (`llama-server.exe` x64 + même Qwen GGUF) puis améliorer ses performances et sa pertinence diagnostique. La précédente idée de reconstruire #15 en x86 avant le x64 n’est plus la priorité active ; elle reste documentée comme historique.**

---

## 1. Règles de travail à conserver

- Dépôt principal : `mini56/ECU-Mems-Manager-Session`.
- Branche de développement x64 officielle : **`MEMSX64`**.
- Branche de transmission : **`RAPPORT`**.
- Branche historique 32 bits IA : **`lab-expert-engine`** ; elle doit rester intacte comme référence fonctionnelle.
- L’utilisateur compile et teste via **GitHub Actions sous Windows**, pas avec Qt Creator.
- **Ne plus modifier le 32 bits fonctionnel pour faire avancer la version principale.**
- Ne modifier que ce qui est demandé explicitement ou strictement nécessaire à la tâche en cours.
- Conserver l’interface sombre et responsive ; ne pas engager de refonte graphique non demandée.
- Les familles MEMS **1.2 / 1.3 / 1.6 / 1.9** doivent rester distinguées.
- Dans `memsinterface.h`, conserver impérativement : `void onProtocolCommandRequested(quint8 command);`.
- Ne jamais confondre polling normal `0x7D/0x80`, RAM/Mode 4, calibrations/cartes et données externes.
- **Priorité absolue de validation : connexion ECU et protocole.** Sans connexion fiable au calculateur, les autres fonctions ne rendent pas le programme utilisable.
- **Règle de communication build :** quand un artefact/version doit être testé par l’utilisateur, toujours donner **le numéro de build en premier**, sous une forme du type **`BUILD #14 — v1.0.14`**. Ne jamais demander à l’utilisateur d’identifier une version uniquement avec un hash Git. Le commit peut être indiqué ensuite comme référence technique secondaire.
- **Règle build ↔ version :** `GITHUB_RUN_NUMBER` produit le numéro de version affiché par le logiciel : build #14 → v1.0.14 ; #15 → v1.0.15 ; #100 → v1.1.0 ; #588 → v1.5.88 ; #662 → v1.6.62. Le calcul est `1.(build / 100).(build % 100)`.
- BUILD #14 / v1.0.14 reste la **référence de comparaison**, mais la voie de développement active est désormais x64.

---

## 2. Références historiques importantes

### 2.1 IA MEMS historique

Branche historique IA : `lab-expert-engine`.

Runtime/modèle déjà validés avant le chantier x64 :

- modèle : **Qwen3-0.6B-Q8_0** ;
- runtime : **llama.cpp b10516 Windows x64 CPU** ;
- port local : **127.0.0.1:18089** ;
- runtime ZIP : `llama-b10516-bin-win-cpu-x64.zip` ;
- SHA256 runtime : `fbbbc55e0eb2e1b07f9dcb9488616c98ed47d9003b90e15e7c8c7812c4307cd3` ;
- modèle final : `ai/models/ia-mems.gguf` ;
- SHA256 modèle : `9465e63a22add5354d9bb4b99e90117043c7124007664907259bd16d043bb031`.

**BUILD #14 / v1.0.14 — référence exacte :**

- workflow : `IA MEMS complete Windows package` ;
- run GitHub : **32690688435** ;
- `run_number` : **14** ;
- branche : `lab-expert-engine` ;
- source : **`c2fdecac164a72ada572c13abc4f71f9e4f17273`** ;
- titre du run : `Make VC++ runtime discovery version-independent` ;
- conclusion GitHub : **success** ;
- application historique : **32 bits** ;
- `llama-server.exe` séparé : **x64**.

**BUILD #15 / v1.0.15 — évolution historique suivante :**

- run GitHub : **32692436132** ;
- `run_number` : **15** ;
- source : **`d1f600d3f2fe49872663b62b1f54f40d707834ad`** ;
- titre : **`Improve IA MEMS conversational relevance`** ;
- conclusion GitHub : **success**.

L’objectif du #15 était d’améliorer la pertinence conversationnelle de l’IA : répondre d’abord à la question réellement posée, moins partir sur du contexte MEMS hors sujet, mieux tolérer les fautes, fournir des réponses courantes/factuelles plus naturelles, utiliser les données MEMS seulement lorsqu’elles sont pertinentes, et rendre la génération moins aléatoire.

Comparaison exacte #14 → #15 : **un seul fichier du programme change : `expert/LocalAiClient.cpp`**. Les autres différences sont des workflows/marqueurs GitHub.

Cette évolution reste utile pour la future IA x64, mais **elle n’impose plus de reconstruire un nouveau 32 bits avant d’avancer**.

Correctifs historiques ultérieurs à conserver comme références, sans les réintroduire en bloc :

- `cbcb8a14189b1bc013cb3519b0ba33aa3f85c072` — `Fix IA MEMS history scroll and factual answers` ;
- `896a59f762e648ce50023121491693ee70ad162d` — `Build IA MEMS quality patch` ;
- run final historique **32694753190** = run_number **16**, donc **ce n’est pas le build #14**.

### 2.2 Architecture exacte des BUILD #14 et #15 — POINT À NE PLUS CONFONDRE

Les BUILD #14 et #15 ont **la même architecture générale** :

- **ECU MEMS Manager : x86 / 32 bits** ;
- compilation application : Qt 5.15.2 `win32_mingw81` + MinGW 8.1 32 bits ;
- **DLL protocole chargée par MEMS Manager : `prebuilt-librosco/librosco.dll` x86 / 32 bits** ;
- **`llama-server.exe` : x64 / 64 bits**, processus séparé ;
- **modèle : Qwen3-0.6B-Q8_0 GGUF**, identique entre #14 et #15 ;
- communication MEMS Manager ↔ IA : **HTTP local sur `127.0.0.1:18089`**.

Schéma BUILD #14 / #15 :

`ECU MEMS Manager x86` → `librosco.dll x86` → ECU

et séparément :

`ECU MEMS Manager x86` → HTTP local → `llama-server.exe x64` → `Qwen3-0.6B-Q8_0.gguf`

Conséquences :

1. **L’IA n’était déjà pas 32 bits au BUILD #14.** Le serveur llama était x64.
2. Le modèle GGUF n’a pas à être « converti en 64 bits » : il reste le même fichier.
3. Une application x86 ne peut pas charger directement une DLL protocole x64 dans son processus ; c’est pourquoi BUILD #14/#15 utilisent encore `librosco.dll` x86.
4. La nouvelle **`mems_manager_x64.dll`** entre en jeu lorsque **ECU MEMS Manager lui-même est recompilé en x64**.
5. Architecture cible : `MEMS Manager x64 + mems_manager_x64.dll x64 + llama-server x64 + même Qwen GGUF`.
6. La communication HTTP avec llama-server découple l’architecture du serveur IA de celle de l’application.

### 2.3 Base Andrew Revill / MEMSTools

Le lot brut complet 1600 reste conservé dans `database/reference/research_enrichment_1600.qz64`, environ 33,3 Mo compressés :

- 478 730 propriétés DEF ;
- 45 934 propriétés DIM ;
- 3 524 519 cellules non vides de corrélations.

Ne pas charger automatiquement les 3,5 millions de cellules brutes sans mesurer mémoire, temps de démarrage et intérêt réel.

---

## 3. `librosco.dll` historique x86 — référence de compatibilité

Fichier : `prebuilt-librosco/librosco.dll`.

- taille : **49 672 octets** ;
- SHA256 : `a5ad466f8f1a198a6f8259a5ea5ab59775815c8cbc9637aa584db9e9c811d38f` ;
- PE32 / x86 ;
- version interne : **0.1.12** ;
- `mems_data` : **60 octets** ;
- **22 exports** historiques.

Commandes confirmées par désassemblage :

- init : `CA 75 F4 D0` ;
- polling : `0x80` puis `0x7D` ;
- trame 80 : 28 octets ;
- trame 7D : 32 octets ;
- IAC : `0xFB` ;
- clear faults : `0xCC` ;
- heartbeat : `0xF4` ;
- reset ECU : `0xFA` ;
- reset adjustments : `0x0F`.

Le 32 bits reste conservé tel quel comme référence ; il ne doit pas être transformé pour poursuivre la version principale.

---

## 4. Nouvelle DLL protocole native x64 — VALIDÉE TECHNIQUEMENT, À RECONTRÔLER DANS LE BUILD CIBLE

Nom : **`mems_manager_x64.dll`**.

- sources : `librosco-x64/` ;
- DEF : `librosco-x64/mems_manager_x64.def` ;
- PE32+ AMD64 ;
- `frame80` : 28 octets ;
- `frame7d` : 32 octets ;
- `mems_data` : 60 octets ;
- les **22 exports historiques** sont présents.

Sécurités déjà ajoutées : refus d’une trame `0x7D` tronquée et retour d’état réel pour clear/reset.

La DLL a déjà passé les contrôles techniques de compatibilité. Pour le nouveau build x64 propre, il faut néanmoins **revalider automatiquement** : architecture PE AMD64, liste des exports, tailles/packing des structures, liaison/import par l’EXE et absence totale de chargement de l’ancienne `librosco.dll` x86.

---

## 5. Liaison de MEMS Manager complet en x64 — HISTORIQUE TECHNIQUE UTILE

Workflow : `.github/workflows/build-ecu-mems-x64-link-smoke.yml`.
Run : **32709187615**.

Corrections MSVC minimales :

- `5e53ef22c4b848397aa5abdd453378f76f1e3988` — `or` → `||` ;
- `722a8278c5974e4b016a59ed6b1de70baee01f40` — `and` → `&&` ;
- `09564db70f729e1a3d78f974918d0d24bafbc982` — autres `or` → `||`.

Résultat technique déjà obtenu : EXE + `mems_manager_x64.dll` en AMD64, import de la nouvelle DLL oui, ancienne `librosco.dll` non.

Ce résultat montre que la conversion est faisable ; il faut maintenant la reconstruire proprement avec des contrôles plus stricts et valider le vrai ECU.

---

## 6. Premier package portable x64 sans IA — HISTORIQUE UTILE

Run **32713710308**.

- 29 PE contrôlés AMD64 ;
- 243 fichiers ;
- base : 85 ECU / 140 affectations / 91 commandes ;
- smoke launch 5 s : OK ;
- onglet IA stable en mode secours sans runtime local sur PC secondaire propre.

Ce package montre qu’une application x64 complète peut démarrer et naviguer sans runtime IA local. Il ne remplace pas la validation ECU réelle à effectuer sur le nouveau jalon x64.

---

## 7. Persistance Windows à garder en tête

Chaque version est extraite dans son propre dossier, mais plusieurs états sont partagés hors dossier de l’EXE :

- QSettings utilisateur (`SerialDevice`, langue, thème, etc.) ;
- AppLocalData `/reference` ;
- AppLocalData `/ia-mems` ;
- variables `MEMS_AI_SERVER` / `MEMS_AI_MODEL` ;
- éventuel serveur déjà actif sur `127.0.0.1:18089`.

La base principale modifiable reste dossier-local : `<appdir>/database/ecu_mems_manager.sqlite`.

---

## 8. Diagnostics x64 récents — À CONSERVER COMME HISTORIQUE, PAS COMME VOIE ACTIVE

- Run **32717558263** : test IA réel PASS, application vivante après 20 s ; rouge uniquement à cause d’un conflit Git lors de l’enregistrement du résultat.
- Run **32718093195** : sélection IA par barre latérale, VERT.
- Run **32721284999** : construction + réutilisation cache `ExpertRuntimeDatabase` x64, VERT.
- Run **32721873137** : rouge lié aux permissions workflow.
- Package x64 complet IA run **32741977123** : build GitHub vert mais fermeture réelle de l’application à l’ouverture de IA MEMS.
- A/B sans `/utf-8` run **32753372131** : fermeture identique ; `/utf-8` exclu.
- Plusieurs workflows staged suivants ont été rouges à cause de leurs propres erreurs YAML/logique de workflow et ne constituent pas des résultats fonctionnels du programme.
- Le run **32759202133 / build #6 du workflow diagnostic** n’a créé aucun job : échec du workflow avant compilation, sans valeur diagnostique sur MEMS Manager.

**Décision : ne plus réparer cet empilement de diagnostics comme axe principal. Construire un jalon x64 propre centré ECU.**

---

## 9. Package MEMSX64 avec IA locale complète — ÉCHEC RÉEL À NE PAS PRENDRE COMME BASE

Run : **32741977123**.

Sur le PC utilisateur :

- application stable tant que IA MEMS n’est pas ouverte ;
- ouverture IA MEMS → fermeture brutale 2–3 s après ;
- attendre >2 min ne change rien ;
- renommer `ai` en `ai_OFF` ne change rien ;
- aucun `llama-server.exe` actif ;
- sans `/utf-8`, fermeture identique.

Cette version démontre une régression dans l’ancien chemin x64, mais elle ne doit plus être patchée indéfiniment. Les connaissances acquises restent utiles pour les contrôles du nouveau jalon.

---

## 10. BUILD #14 / v1.0.14 — RÉFÉRENCE MATÉRIELLE FIGÉE

Le 24 août 2026, l’utilisateur a retesté **ECU MEMS Manager v1.0.14 / BUILD #14**.

### 10.1 Identification exacte

- workflow : `IA MEMS complete Windows package` ;
- run : **32690688435** ;
- `run_number=14` ;
- source : **`c2fdecac164a72ada572c13abc4f71f9e4f17273`** ;
- branche historique : `lab-expert-engine` ;
- **ECU MEMS Manager : x86 / 32 bits** ;
- **DLL protocole : `prebuilt-librosco/librosco.dll` x86 / 32 bits** ;
- **serveur llama.cpp : x64 / 64 bits, processus séparé** ;
- **Qwen3-0.6B-Q8_0 : modèle GGUF** ;
- communication application ↔ IA par HTTP local sur `127.0.0.1:18089`.

### 10.2 IA locale

- onglet IA MEMS s’ouvre sans fermeture ;
- statut **`base prête • IA locale prête`** ;
- questions/réponses possibles ;
- IA stable connecté et déconnecté ;
- qualité des réponses encore perfectible.

### 10.3 Connexion ECU réelle

- COM3 ;
- firmware **AANMP002** ;
- ID **AANMP002 — MNE101150** ;
- communication verte ;
- polling normal **7D/80** fonctionnel ;
- IA reste stable avec ECU connecté.

### 10.4 Injection RAM Mode 4

- passage **7D/80 → Injection RAM Mode 4** fonctionnel ;
- retour au mode normal fonctionnel ;
- exemple : injection finale ≈ 2,47 ms ; `0x03C8=1233 ticks` ; `0x026E=0` ; `0x0280=0`.

### 10.5 Enregistrement matériel de référence

Session `2026-08-24_18.14.txt` :

- **705 lignes de mesures** ;
- environ **5 min 40 s** ;
- capture moteur arrêté, lancement, démarrage et ralenti stabilisé ;
- RPM 0 → 1657 puis ~1180–1200 ;
- MAP ~100 → ~30–35 kPa ;
- batterie ~12,2 → ~11,0 au lancement puis ~13,8 V ;
- dwell ~6,274 ms arrêté, ~7,170 ms lancement, puis ~3,2–3,4 ms moteur tournant ;
- `7D14-15` et erreur ralenti chaud évoluent réellement.

Cette trace est la référence matérielle pour comparer les futurs builds x64.

### 10.6 Pourquoi garder BUILD #14

Parce qu’il est **le dernier état pour lequel nous avons une validation réelle et complète**, pas seulement un build GitHub vert : programme stable, IA locale prête, base expert prête, vrai ECU connecté, identification correcte, polling 7D/80, Mode 4, retour normal, navigation et acquisition réelle.

**Il reste donc figé comme référence et solution de secours ; il n’est plus la branche où développer l’avenir du programme.**

### 10.7 BUILD #15 — historique utile, pas prérequis actif

BUILD #15 / v1.0.15 correspondait à `Improve IA MEMS conversational relevance` et modifiait seulement `expert/LocalAiClient.cpp` côté programme.

Les améliorations pourront être reprises dans l’IA x64 après validation du cœur ECU. Il n’est plus nécessaire de reconstruire/tester un nouveau x86 #15 avant de poursuivre le x64, sauf demande explicite.

---

## 11. Détection automatique câble / port COM — exigence future

La détection automatique doit être générale à MEMS Manager, pas réservée au MEMS 1.9 :

- détecter automatiquement les interfaces ;
- afficher par exemple `COM5 — FTDI FT232 — câble détecté` ;
- plusieurs interfaces : COM + type ;
- à Connexion, vérifier interface/protocole ;
- messages explicites : `câble incompatible`, `aucun ECU détecté`, `mauvais type d’interface`.

Cette évolution vient **après validation de la connexion x64 de base**, afin de ne pas mélanger architecture et nouvelles fonctionnalités.

---

## 12. Injection / RAM — règles à ne pas perdre

- Ne jamais calculer le temps d’injection à partir du polling normal `0x7D/0x80`.
- Adresses RAM importantes : **`0x03C8`, `0x026E`, `0x0280`**.
- Onglet Injection entre Aperçu et Réglage.
- Dwell/temps bobine dans cet onglet.
- Éviter de mélanger inutilement plusieurs modes de lecture dans une même page.
- Le dwell arrêté/lancement ne doit pas être comparé directement à une plage moteur tournant.

---

## 13. NOUVELLE STRATÉGIE X64 OFFICIELLE — ECU D’ABORD

### 13.1 Objectif du premier jalon

Produire une **version x64 complète de MEMS Manager centrée sur le cœur ECU**, sans empaqueter pour l’instant le runtime/modèle IA local.

Ce jalon n’est pas une version finale sans IA. C’est une étape de validation d’architecture destinée à répondre sans ambiguïté à la question : **MEMS Manager x64 + nouvelle DLL x64 communiquent-ils correctement et durablement avec le vrai ECU ?**

### 13.2 Architecture cible du jalon

- ECU MEMS Manager : **x64** ;
- Qt : **x64** ;
- `mems_manager_x64.dll` : **x64** ;
- ancienne `prebuilt-librosco/librosco.dll` x86 : **absente du package et non importée** ;
- base de données et fonctions existantes conservées ;
- runtime/model IA local non empaquetés pour cette première validation ;
- aucune refonte UI ni nouvelle fonctionnalité simultanée.

### 13.3 Contrôles de compilation et de package OBLIGATOIRES

Le workflow x64 doit échouer immédiatement si un de ces contrôles échoue :

1. compilation propre en x64 de l’application ;
2. compilation propre de `mems_manager_x64.dll` ;
3. vérification PE32+ / AMD64 de `ecu_mems_manager.exe` ;
4. vérification PE32+ / AMD64 de `mems_manager_x64.dll` ;
5. contrôle des DLL Qt et plugins chargés : architecture x64 ;
6. contrôle de `qsqlite.dll`, Qt SerialPort et dépendances essentielles : x64 ;
7. contrôle que l’EXE importe **`mems_manager_x64.dll`** ;
8. contrôle que l’EXE **n’importe pas `librosco.dll`** ;
9. contrôle que l’ancienne DLL x86 n’est pas présente dans le package ;
10. contrôle des **22 exports historiques** de la nouvelle DLL ;
11. contrôle des structures/ABI : `frame80 = 28`, `frame7d = 32`, `mems_data = 60`, avec `static_assert`/self-test lorsque possible ;
12. tests des parseurs et garde-fous connus : trames complètes/tronquées, codes de retour clear/reset, commandes historiques ;
13. smoke launch de l’application packagée pendant une durée suffisante pour détecter une fermeture immédiate ;
14. test d’ouverture de la base SQLite et des ressources indispensables ;
15. inventaire final du package pour empêcher tout mélange x86/x64 involontaire.

Un workflow vert après ces contrôles signifie que **la construction et le package sont cohérents x64**. Cela ne remplace toutefois pas le test matériel ECU.

### 13.4 Validation matérielle PRIORITAIRE

Sur le vrai ECU AANMP002/MNE101150 déjà utilisé comme référence :

1. lancement et stabilité générale ;
2. port COM visible et sélectionnable ;
3. connexion ECU ;
4. identification AANMP002 / MNE101150 ;
5. communication `7D/80` stable ;
6. comparaison des valeurs principales avec la trace de référence BUILD #14 ;
7. acquisition/enregistrement ;
8. passage en Injection RAM Mode 4 ;
9. lecture `0x03C8`, `0x026E`, `0x0280` ;
10. retour Mode 4 → 7D/80 ;
11. déconnexion ;
12. reconnexion ;
13. navigation dans les onglets principaux sans perte de communication.

**Ce test réel est le critère qui transforme le jalon x64 en nouvelle base fonctionnelle.**

### 13.5 IA juste après validation ECU

Dès que le cœur x64 est validé, réintégrer l’IA sans changer simultanément le protocole ECU :

- `llama-server.exe` **x64 déjà validé** ;
- même `Qwen3-0.6B-Q8_0.gguf` au premier test pour garder un point de comparaison ;
- communication HTTP locale identique ;
- réintégrer ensuite les améliorations de pertinence du BUILD #15 et les correctifs qualité utiles ;
- mesurer démarrage, RAM, CPU, stabilité, temps de réponse et pertinence diagnostique ;
- seulement après stabilité, envisager un modèle ou des paramètres plus performants si cela apporte réellement un meilleur diagnostic.

L’objectif final reste **la meilleure IA diagnostique possible**, mais elle doit être construite sur une connexion ECU x64 fiable.

---

## 14. PROCHAINE ACTION EXACTE

1. **Ne pas toucher à la branche 32 bits fonctionnelle.**
2. Utiliser `MEMSX64` comme branche de développement officielle.
3. Reprendre les éléments x64 déjà techniquement validés, mais reconstruire un **workflow/package propre** plutôt que patcher les workflows de diagnostic récents.
4. Produire le premier jalon **MEMS Manager x64 + `mems_manager_x64.dll` x64, sans runtime/modèle IA local empaqueté**.
5. Ajouter tous les contrôles de compilation/PE/imports/exports/ABI/package listés en 13.3.
6. Ne demander un test utilisateur que lorsque ce build est vert et que l’artefact est disponible.
7. Annoncer alors **BUILD #XX — v1.x.xx** en premier.
8. Tester en priorité la connexion au vrai ECU et la checklist 13.4.
9. Si la connexion est validée, ce build devient la nouvelle base x64.
10. Réintégrer immédiatement l’IA x64 à partir du runtime/modèle connus, puis travailler sur ses performances et sa pertinence diagnostique.

**Principe directeur : ECU d’abord, architecture x64 propre, contrôles automatiques stricts, puis IA. Pas de demi-mesures ni d’empilement de patches sans validation intermédiaire.**
