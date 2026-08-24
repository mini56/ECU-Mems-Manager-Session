# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE POUR LES PROCHAINES DISCUSSIONS**
>
> Au début de chaque nouvelle discussion concernant ECU MEMS Manager, ce fichier doit être relu avant de reprendre le travail.
> L’assistant doit **mettre à jour ce rapport lui-même et le pousser régulièrement sur la branche `RAPPORT`**, au minimum après chaque étape technique importante, validation, découverte, changement de branche de travail ou décision d’architecture.
> La branche `RAPPORT` sert au suivi et à la transmission entre discussions. Elle ne doit pas servir à modifier le programme de production sauf demande explicite.

Dernière mise à jour : **24 août 2026 — DÉCISION ACTIVE : le 32 bits fonctionnel reste figé sur sa branche historique `lab-expert-engine` et sert de référence de secours. BUILD #14 / v1.0.14 reste la référence matérielle prouvée. La voie officielle de développement est désormais MEMS Manager x64 sur `MEMSX64`. Priorité absolue : connexion ECU réelle et protocole. Premier jalon : version x64 complète du programme avec `mems_manager_x64.dll`, sans runtime/modèle IA local empaqueté, avec contrôles de compilation/architecture/ABI renforcés. Le menu/navigation doit être assaini dans cette nouvelle x64 : ordre unique défini par identité d’onglet, aucune dépendance à la position des clés de traduction, aucune reconnaissance par titre traduit, aucune réinsertion périodique par timers. L’ordre visuel des onglets fourni par l’utilisateur devient la référence de la future version. Une fois le cœur x64 validé sur vrai ECU, réintégrer immédiatement l’IA x64 connue (`llama-server.exe` x64 + même Qwen GGUF) puis améliorer ses performances et sa pertinence diagnostique.**

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
- **Priorité absolue de validation : connexion ECU et protocole.** Sans connexion fiable au calculateur, les autres fonctions ne rendent pas le programme réellement utilisable.
- **Règle de communication build :** toujours donner **le numéro de BUILD en premier**, sous une forme du type **`BUILD #14 — v1.0.14`**. Le hash Git n’est qu’une référence technique secondaire.
- **Règle build ↔ version :** `GITHUB_RUN_NUMBER` produit le numéro de version affiché par le logiciel : build #14 → v1.0.14 ; #15 → v1.0.15 ; #100 → v1.1.0 ; #588 → v1.5.88 ; #662 → v1.6.62. Calcul : `1.(build / 100).(build % 100)`.
- BUILD #14 / v1.0.14 reste la **référence de comparaison**, mais la voie de développement active est désormais x64.

---

## 2. Architecture historique 32 bits et IA — POINT À NE PLUS CONFONDRE

### 2.1 BUILD #14 / v1.0.14 — référence exacte

- workflow : `IA MEMS complete Windows package` ;
- run GitHub : **32690688435** ;
- `run_number` : **14** ;
- branche : `lab-expert-engine` ;
- source : `c2fdecac164a72ada572c13abc4f71f9e4f17273` ;
- conclusion GitHub : **success** ;
- **ECU MEMS Manager : x86 / 32 bits** ;
- compilation : Qt 5.15.2 `win32_mingw81` + MinGW 8.1 32 bits ;
- **DLL protocole : `prebuilt-librosco/librosco.dll` x86 / 32 bits** ;
- **`llama-server.exe` : x64 / 64 bits**, processus séparé ;
- modèle : **Qwen3-0.6B-Q8_0 GGUF** ;
- communication MEMS Manager ↔ IA : HTTP local `127.0.0.1:18089`.

Schéma :

`ECU MEMS Manager x86` → `librosco.dll x86` → ECU

et séparément :

`ECU MEMS Manager x86` → HTTP local → `llama-server.exe x64` → `Qwen3-0.6B-Q8_0.gguf`

### 2.2 BUILD #15 / v1.0.15 — historique utile

- run : **32692436132** ;
- source : `d1f600d3f2fe49872663b62b1f54f40d707834ad` ;
- titre : `Improve IA MEMS conversational relevance` ;
- même architecture générale que #14 : application/DLL protocole x86, llama-server x64, même Qwen.

Comparaison exacte #14 → #15 : **un seul fichier du programme change : `expert/LocalAiClient.cpp`**. Les autres différences sont des workflows/marqueurs GitHub.

Le #15 améliorait la pertinence conversationnelle : répondre d’abord à la question réellement posée, ignorer le contexte MEMS hors sujet, mieux tolérer les fautes, utiliser les données techniques seulement quand elles sont pertinentes, diminuer l’aléatoire (`temperature` 0.7 → 0.25, `top_p` 0.8 → 0.9, `max_tokens` 520 → 460).

Cette évolution reste utile pour la future IA x64, mais **il n’est plus nécessaire de reconstruire un nouveau x86 avant d’avancer**.

### 2.3 Runtime/modèle IA connus et validés

- runtime : **llama.cpp b10516 Windows x64 CPU** ;
- runtime ZIP : `llama-b10516-bin-win-cpu-x64.zip` ;
- SHA256 runtime : `fbbbc55e0eb2e1b07f9dcb9488616c98ed47d9003b90e15e7c8c7812c4307cd3` ;
- modèle : **Qwen3-0.6B-Q8_0** ;
- fichier final : `ai/models/ia-mems.gguf` ;
- SHA256 modèle : `9465e63a22add5354d9bb4b99e90117043c7124007664907259bd16d043bb031` ;
- port : **127.0.0.1:18089**.

Le modèle GGUF n’a pas de version « 32 bits » ou « 64 bits ». Le serveur llama est déjà x64 et communique avec MEMS Manager par HTTP local.

---

## 3. BUILD #14 — VALIDATION MATÉRIELLE À CONSERVER

Le 24 août 2026, l’utilisateur a retesté BUILD #14 / v1.0.14 sur un vrai ECU.

### 3.1 Connexion réelle

- COM3 ;
- firmware : **AANMP002** ;
- ID : **AANMP002 — MNE101150** ;
- communication verte ;
- polling **7D/80** fonctionnel ;
- navigation stable ;
- IA stable connecté et déconnecté ;
- statut IA : `base prête • IA locale prête`.

### 3.2 Injection RAM Mode 4

- passage **7D/80 → Injection RAM Mode 4** fonctionnel ;
- retour au mode normal fonctionnel ;
- exemple : injection finale ≈ 2,47 ms ;
- `0x03C8 = 1233 ticks` ;
- `0x026E = 0` ;
- `0x0280 = 0`.

### 3.3 Trace matérielle de référence

Session `2026-08-24_18.14.txt` :

- 705 lignes ;
- environ 5 min 40 s ;
- moteur arrêté, lancement, démarrage puis ralenti ;
- RPM 0 → 1657 puis ~1180–1200 ;
- MAP ~100 → ~30–35 kPa ;
- batterie ~12,2 V → ~11,0 V au lancement puis ~13,8 V ;
- dwell ~6,274 ms arrêté, ~7,170 ms au lancement, puis ~3,2–3,4 ms moteur tournant ;
- `7D14-15` et erreur ralenti chaud évoluent réellement.

Cette trace est la référence matérielle à comparer aux futurs builds x64.

---

## 4. `librosco.dll` x86 — RÉFÉRENCE DE COMPATIBILITÉ

Fichier : `prebuilt-librosco/librosco.dll`.

- 49 672 octets ;
- SHA256 : `a5ad466f8f1a198a6f8259a5ea5ab59775815c8cbc9637aa584db9e9c811d38f` ;
- PE32 / x86 ;
- version interne : 0.1.12 ;
- `mems_data` : 60 octets ;
- **22 exports historiques**.

Commandes confirmées :

- init : `CA 75 F4 D0` ;
- polling : `0x80` puis `0x7D` ;
- trame 80 : 28 octets ;
- trame 7D : 32 octets ;
- IAC : `0xFB` ;
- clear faults : `0xCC` ;
- heartbeat : `0xF4` ;
- reset ECU : `0xFA` ;
- reset adjustments : `0x0F`.

Le 32 bits reste figé et sert de référence. Il ne doit pas être transformé pour poursuivre le produit principal.

---

## 5. Nouvelle DLL protocole native x64 — VALIDÉE TECHNIQUEMENT

Nom : **`mems_manager_x64.dll`**.

- sources : `librosco-x64/` ;
- DEF : `librosco-x64/mems_manager_x64.def` ;
- PE32+ AMD64 ;
- `frame80` : 28 octets ;
- `frame7d` : 32 octets ;
- `mems_data` : 60 octets ;
- **22 exports historiques** présents ;
- garde-fou sur trame 7D tronquée ;
- retours réels clear/reset.

Résultat historique de liaison x64 : workflow `build-ecu-mems-x64-link-smoke.yml`, run **32709187615** : EXE et DLL AMD64, import de `mems_manager_x64.dll`, aucune importation de l’ancienne `librosco.dll`.

Corrections MSVC minimales déjà connues :

- `or` → `||` ;
- `and` → `&&` ;
- autres opérateurs alternatifs corrigés dans les quelques sources concernées.

La DLL doit malgré tout être **recontrôlée automatiquement dans le nouveau build cible**, pour ne rien supposer.

---

## 6. Historique x64 utile, mais pas base active

Premier package portable x64 sans IA locale : run **32713710308**.

- 29 PE AMD64 ;
- 243 fichiers ;
- base 85 ECU / 140 affectations / 91 commandes ;
- smoke launch 5 s OK ;
- onglet IA stable en mode secours sans runtime local sur PC secondaire propre.

Ancien package x64 complet avec IA : run **32741977123** : build GitHub vert mais fermeture réelle 2–3 s après ouverture de IA MEMS.

Test A/B sans `/utf-8` : run **32753372131**, fermeture identique ; `/utf-8` exclu.

Autres diagnostics :

- 32717558263 : IA test PASS, rouge seulement sur persistance Git ;
- 32718093195 : sélection IA par sidebar, vert ;
- 32721284999 : construction/réutilisation cache expert x64, vert ;
- 32721873137 : rouge permissions workflow ;
- 32759202133 / build #6 diagnostic : zéro job, échec workflow avant compilation.

**Décision : ne plus réparer cet empilement de diagnostics comme voie principale. Construire un x64 propre centré ECU.**

---

## 7. NOUVELLE STRATÉGIE X64 OFFICIELLE — ECU D’ABORD

### 7.1 Premier jalon

Produire une **version x64 complète de MEMS Manager centrée sur le cœur ECU**, sans empaqueter pour l’instant le runtime/modèle IA local.

Ce n’est pas une décision de supprimer l’IA. C’est un jalon court destiné à répondre sans ambiguïté : **MEMS Manager x64 + nouvelle DLL x64 communiquent-ils correctement et durablement avec le vrai ECU ?**

Architecture :

- ECU MEMS Manager x64 ;
- Qt x64 ;
- `mems_manager_x64.dll` x64 ;
- aucune `librosco.dll` x86 dans le package ;
- base et fonctions existantes conservées ;
- runtime/modèle IA local non empaquetés au premier test ;
- aucune nouvelle fonction protocolaire simultanée.

### 7.2 Contrôles automatiques OBLIGATOIRES

Le workflow doit échouer immédiatement si un contrôle échoue :

1. compilation propre x64 de l’application ;
2. compilation propre de `mems_manager_x64.dll` ;
3. EXE PE32+ / AMD64 ;
4. DLL protocole PE32+ / AMD64 ;
5. DLL Qt/plugins x64 ;
6. `qsqlite.dll`, Qt SerialPort et dépendances essentielles x64 ;
7. EXE importe `mems_manager_x64.dll` ;
8. EXE n’importe pas `librosco.dll` ;
9. ancienne DLL x86 absente du package ;
10. contrôle des 22 exports ;
11. ABI/structures : `frame80=28`, `frame7d=32`, `mems_data=60` ;
12. tests parseurs/garde-fous : trames complètes/tronquées, clear/reset, commandes historiques ;
13. smoke launch de l’application packagée ;
14. ouverture SQLite et ressources indispensables ;
15. inventaire final de tous les PE pour empêcher tout mélange x86/x64.

Un workflow vert prouve la cohérence de construction x64, **pas encore le comportement sur vrai ECU**.

### 7.3 Validation matérielle prioritaire

Sur AANMP002/MNE101150 :

1. lancement et stabilité ;
2. port COM visible et sélectionnable ;
3. connexion ;
4. identification AANMP002 / MNE101150 ;
5. 7D/80 stable ;
6. comparaison des valeurs principales avec la trace BUILD #14 ;
7. acquisition/enregistrement ;
8. Injection RAM Mode 4 ;
9. lecture `0x03C8`, `0x026E`, `0x0280` ;
10. retour Mode 4 → 7D/80 ;
11. déconnexion ;
12. reconnexion ;
13. navigation sans perte de communication.

**Ce test réel est le critère qui transforme le jalon x64 en nouvelle base fonctionnelle.**

### 7.4 IA juste après validation ECU

Dès validation du cœur x64 :

- réintégrer `llama-server.exe` x64 déjà connu ;
- même Qwen3-0.6B-Q8_0 au premier test pour garder une référence ;
- communication HTTP locale identique ;
- reprendre ensuite les améliorations de pertinence du BUILD #15 et les correctifs qualité utiles ;
- mesurer RAM, CPU, temps de démarrage, stabilité, temps de réponse et pertinence diagnostique ;
- ensuite seulement envisager un modèle/paramétrage plus performant si le gain est réel.

Objectif final : **la meilleure IA diagnostique possible, construite sur une connexion ECU x64 fiable.**

---

## 8. MENU / NAVIGATION — À RECONSTRUIRE PROPREMENT DANS LA X64

### 8.1 Ordre visuel de référence donné par l’utilisateur

L’ordre visible dans les vues/captures fournies par l’utilisateur devient **l’ordre officiel de la future version x64** :

1. **Aperçu**
2. **Injection**
3. **Réglages**
4. **Actionneurs**
5. **Erreurs**
6. **Diagnostic automatique**
7. **IA MEMS**
8. **Analyse**
9. **Toutes les mesures**
10. **ECU/ROSCO**
11. **Toutes les données**
12. **Base de données**
13. **Interactif**
14. **Test ECU 1.9**

Règle déjà confirmée séparément : **Injection doit rester entre Aperçu et Réglages. IA MEMS doit rester entre Diagnostic automatique et Analyse.**

Cet ordre doit être pris comme référence fonctionnelle/visuelle. Ne pas réordonner les onglets de sa propre initiative.

### 8.2 Cause identifiée du comportement actuel du menu

Le système de traduction `I18n` lui-même est **basé sur des clés** (`I18n::text(1001)`, etc.) et n’impose pas techniquement « 1er onglet = 1re clé ». Le problème vient du code d’interface qui a progressivement lié **position, titre traduit et reconstruction du menu**.

Constats sur `MEMSX64` :

- `navigationorderpatch.cpp` définit `kFinalTabCount = 13` ;
- sa table principale classe 13 onglets mais **n’intègre pas IA MEMS** ;
- elle reconnaît d’abord les onglets en comparant leur **titre traduit** (`I18n::text(...)`), puis utilise l’`objectName` seulement en secours ;
- elle reconstruit le menu plusieurs fois au démarrage par timers : **80, 450, 1550, 2550, 3750, 4700 ms** ;
- `taborderpatch.cpp` déplace/reconstruit encore Base de données et Interactif à **1400, 2400, 3600 ms** ;
- `iamemstab.cpp` constate que le menu final ne contient pas IA MEMS et l’insère manuellement à la ligne 6 ;
- IA MEMS est ensuite réinsérée/revérifiée par timers à **500, 1800, 3900, 5200, 6500 ms** ;
- le commentaire de `iamemstab.cpp` dit explicitement que Diagnostic automatique est rang 5 et Analyse rang 6, et que IA est insérée « entre les deux sans modifier la rank map » ;
- `uirebuild.cpp` contient encore `subtitleForIndex(index)` où index 0 → clé 7104, index 1 → 7105, etc. : déplacer un onglet peut donc associer un sous-titre à sa **position** plutôt qu’à son identité.

Conclusion : le menu actuel est devenu un empilement de réordonnancements et de rattrapages temporisés. **Les demandes successives de déplacement d’onglets après la mise en place de traductions/index ont effectivement pu contribuer au comportement incohérent.**

### 8.3 Architecture propre du futur menu x64

Le nouveau menu doit avoir **une source de vérité unique**.

Chaque onglet possède une identité stable indépendante de la langue et de sa position, par exemple :

- `overview_tab`
- `injection_tab`
- `settings_tab`
- `actuators_tab`
- `errors_tab`
- `diagnostic_tab`
- `ia_mems_tab`
- `analysis_tab`
- `summary_tab`
- `ecu_rosco_tab`
- `raw_tab`
- `database_tab`
- `interactive_tab`
- `mems19_test_tab`

Une table unique doit définir :

`identité stable → ordre → clé titre → clé sous-titre → icône`

Conséquences obligatoires :

- **ne plus reconnaître un onglet par son texte traduit** ;
- **ne plus utiliser l’index comme clé de traduction du contenu/sous-titre** ;
- **ne plus reconstruire/réinsérer les onglets par plusieurs séries de QTimer** ;
- construire la navigation une fois dans l’ordre officiel ;
- au `LanguageChange`, ne changer que les textes, jamais l’ordre ni l’identité ;
- si un onglet change un jour de position, modifier seulement son champ `ordre` ;
- ses clés FR/EN/ES/IT/PT/DE restent attachées à l’identité de la fonction ;
- IA MEMS doit faire partie de la table principale des 14 onglets, pas être ajoutée comme exception après coup.

### 8.4 Règle traduction à retenir

L’ancienne prérogative « 1er onglet = 1re clé, 2e onglet = 2e clé… » ne doit **plus être utilisée comme dépendance technique** dans la nouvelle x64.

Les numéros de clés peuvent rester tels quels pour préserver les traductions existantes, mais chaque clé doit être rattachée à une **fonction/identité**, pas à une position visuelle.

Exemple conceptuel :

`injection_tab → ordre 2 → clé titre Injection → clé sous-titre Injection → icône injecteur`

Si Injection était un jour déplacé, ses clés ne bougeraient pas.

---

## 9. Détection automatique câble / port COM — exigence future

Après validation de la connexion x64 de base :

- détection automatique des interfaces ;
- affichage type `COM5 — FTDI FT232 — câble détecté` ;
- plusieurs interfaces : COM + type ;
- validation du câble/protocole au clic Connexion ;
- messages clairs : `câble incompatible`, `aucun ECU détecté`, `mauvais type d’interface` ;
- cette détection est générale, pas réservée au MEMS 1.9.

Ne pas introduire cette nouvelle fonction avant la première validation matérielle x64, pour ne pas mélanger migration d’architecture et nouvelle logique de connexion.

---

## 10. Injection / RAM — règles à ne pas perdre

- Ne jamais calculer le temps d’injection depuis le polling normal `0x7D/0x80`.
- Adresses RAM importantes : **`0x03C8`, `0x026E`, `0x0280`**.
- Injection entre Aperçu et Réglages.
- Dwell/temps bobine dans cet onglet.
- Éviter de mélanger inutilement plusieurs modes de lecture dans une même page.
- Le dwell arrêté/lancement ne doit pas être comparé directement à une plage moteur tournant.

---

## 11. Base Andrew Revill / MEMSTools

Lot brut complet conservé dans `database/reference/research_enrichment_1600.qz64`, environ 33,3 Mo compressés :

- 478 730 propriétés DEF ;
- 45 934 propriétés DIM ;
- 3 524 519 cellules non vides de corrélations.

Ne pas charger automatiquement les 3,5 millions de cellules sans mesurer RAM, démarrage et utilité diagnostique.

---

## 12. Persistance Windows à garder en tête

Même avec des dossiers portables distincts, certains états sont partagés :

- QSettings (`SerialDevice`, langue, thème, etc.) ;
- AppLocalData `/reference` ;
- AppLocalData `/ia-mems` ;
- variables `MEMS_AI_SERVER` / `MEMS_AI_MODEL` ;
- éventuel serveur sur `127.0.0.1:18089`.

Base principale modifiable : `<appdir>/database/ecu_mems_manager.sqlite`.

---

## 13. PROCHAINE ACTION EXACTE DANS LA PROCHAINE DISCUSSION

1. **Lire ce rapport avant toute modification.**
2. Ne pas toucher à `lab-expert-engine` / BUILD #14 32 bits.
3. Travailler sur `MEMSX64`.
4. Construire un nouveau jalon x64 propre : MEMS Manager x64 + `mems_manager_x64.dll` x64, sans runtime/modèle IA local empaqueté au premier test.
5. En même temps, remplacer le système de menu à patches/timers par une navigation déterministe à **14 onglets**, dans l’ordre officiel de la section 8.1, avec identité → ordre → clés de traduction → icône.
6. Ne toucher à aucun comportement protocolaire pour corriger le menu.
7. Ajouter les 15 contrôles de compilation/architecture/ABI/package de la section 7.2.
8. Ne demander un test utilisateur que lorsque le build est vert et l’artefact disponible.
9. Annoncer **BUILD #XX — v1.x.xx** en premier.
10. Test utilisateur prioritaire : connexion AANMP002/MNE101150, 7D/80, acquisition, Mode 4, retour normal, déconnexion/reconnexion et stabilité/navigation.
11. Si validé, ce BUILD devient la nouvelle base x64.
12. Réintégrer immédiatement ensuite l’IA x64 connue, puis travailler sur la qualité et les performances diagnostiques.

**Principe directeur : 32 bits figé comme référence ; avenir en x64 ; ECU d’abord ; menu propre et déterministe ; contrôles automatiques stricts ; IA réintégrée dès validation du cœur. Pas d’empilement de patches ni de réordonnancements temporisés.**
