# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE POUR LES PROCHAINES DISCUSSIONS**
>
> Au début de chaque nouvelle discussion concernant ECU MEMS Manager, ce fichier doit être relu avant de reprendre le travail.
> L’assistant doit **mettre à jour ce rapport lui-même et le pousser régulièrement sur la branche `RAPPORT`**, au minimum après chaque étape technique importante, validation, découverte, changement de branche de travail ou décision d’architecture.
> La branche `RAPPORT` sert au suivi et à la transmission entre discussions. Elle ne doit pas servir à modifier le programme de production sauf demande explicite.

Dernière mise à jour : **24 août 2026 — DÉCISION DE REPRISE : repartir du BUILD #14 / v1.0.14, run 32690688435, source `c2fdecac164a72ada572c13abc4f71f9e4f17273`, car c’est la dernière version réellement validée par l’utilisateur avec IA locale prête, base prête, ECU AANMP002/MNE101150 connecté, polling 7D/80, Injection RAM Mode 4 et navigation stable. Architecture clarifiée : BUILD #14 et BUILD #15 utilisent tous deux ECU MEMS Manager x86 + `prebuilt-librosco/librosco.dll` x86, tandis que `llama-server.exe` est déjà x64 et communique avec MEMS Manager par HTTP local. Le modèle Qwen GGUF est identique. Le passage x64 de MEMS Manager et le remplacement de `librosco.dll` x86 par `mems_manager_x64.dll` arrivent seulement après ces builds. La reprise doit donc valider #14 → #15 en 32 bits avant de refaire isolément le passage MEMS Manager x86 → x64.**

---

## 1. Règles de travail à conserver

- Dépôt principal : `mini56/ECU-Mems-Manager-Session`.
- Branche de travail x64 historique : **`MEMSX64`**.
- Branche de transmission : **`RAPPORT`**.
- L’utilisateur compile et teste via **GitHub Actions sous Windows**, pas avec Qt Creator.
- Préserver la version 32 bits existante pendant le développement x64 parallèle.
- Ne modifier que ce qui est demandé explicitement ou strictement nécessaire à la tâche en cours.
- Conserver l’interface sombre et responsive ; ne pas engager de refonte graphique non demandée.
- Les familles MEMS **1.2 / 1.3 / 1.6 / 1.9** doivent rester distinguées.
- Dans `memsinterface.h`, conserver impérativement : `void onProtocolCommandRequested(quint8 command);`.
- Ne jamais confondre polling normal `0x7D/0x80`, RAM/Mode 4, calibrations/cartes et données externes.
- **Règle de communication build :** quand un artefact/version doit être testé par l’utilisateur, toujours donner **le numéro de build en premier**, sous une forme du type **`BUILD #14 — v1.0.14`**. Ne jamais demander à l’utilisateur d’identifier une version uniquement avec un hash Git. Le commit peut être indiqué ensuite comme référence technique secondaire.
- **Règle build ↔ version :** `GITHUB_RUN_NUMBER` produit le numéro de version affiché par le logiciel : build #14 → v1.0.14 ; #15 → v1.0.15 ; #100 → v1.1.0 ; #588 → v1.5.88 ; #662 → v1.6.62. Le calcul est `1.(build / 100).(build % 100)`.
- **Règle de reprise actuelle :** ne pas prendre les derniers commits/runs MEMSX64 comme base fonctionnelle. La base fonctionnelle de reprise est **BUILD #14 / v1.0.14**.

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

**BUILD #14 / v1.0.14 — référence exacte de reprise :**

- workflow : `IA MEMS complete Windows package` ;
- run GitHub : **32690688435** ;
- `run_number` : **14** ;
- branche : `lab-expert-engine` ;
- source : **`c2fdecac164a72ada572c13abc4f71f9e4f17273`** ;
- titre du run : `Make VC++ runtime discovery version-independent` ;
- conclusion GitHub : **success** ;
- application historique : **32 bits** ;
- `llama-server.exe` séparé : **x64**.

**BUILD #15 / v1.0.15 — évolution prévue juste après :**

- run GitHub : **32692436132** ;
- `run_number` : **15** ;
- source : **`d1f600d3f2fe49872663b62b1f54f40d707834ad`** ;
- titre : **`Improve IA MEMS conversational relevance`** ;
- conclusion GitHub : **success**.

L’objectif du #15 était d’améliorer la pertinence conversationnelle de l’IA : répondre d’abord à la question réellement posée, moins partir sur du contexte MEMS hors sujet, mieux tolérer les fautes, fournir des réponses courantes/factuelles plus naturelles, utiliser les données MEMS seulement lorsqu’elles sont pertinentes, et rendre la génération moins aléatoire. **Ce changement doit être repris séparément depuis le #14 et testé avant d’aller plus loin.**

Correctifs historiques ultérieurs à ne pas réintroduire en bloc :

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
4. La nouvelle **`mems_manager_x64.dll`** n’entre en jeu qu’au moment où **ECU MEMS Manager lui-même est recompilé en x64**.
5. Le passage x64 doit donc être traité comme une étape distincte :
   - avant : `MEMS Manager x86 + librosco.dll x86 + llama-server x64 + Qwen` ;
   - après : `MEMS Manager x64 + mems_manager_x64.dll x64 + le même llama-server x64 + le même Qwen`.
6. Le changement BUILD #14 → BUILD #15 **n’est pas un passage x64**. Il modifie uniquement la pertinence conversationnelle côté `expert/LocalAiClient.cpp`.

**Méthode de reconstruction obligatoire :** valider d’abord BUILD #14 → BUILD #15 en conservant l’application 32 bits et l’ancienne DLL x86. Ensuite seulement effectuer un build où le seul grand changement d’architecture est le passage de MEMS Manager en x64 + remplacement de la DLL protocole x86 par `mems_manager_x64.dll` x64, en conservant exactement le même runtime llama x64 et le même modèle Qwen.

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

---

## 4. Nouvelle DLL protocole native x64 — TRAVAIL HISTORIQUE VALIDÉ TECHNIQUEMENT, PAS BASE DE REPRISE

Nom : **`mems_manager_x64.dll`**.

- sources : `librosco-x64/` ;
- DEF : `librosco-x64/mems_manager_x64.def` ;
- PE32+ AMD64 ;
- `frame80` : 28 octets ;
- `frame7d` : 32 octets ;
- `mems_data` : 60 octets ;
- les 22 exports historiques sont présents.

Ce travail reste utile et documenté, mais **il ne doit pas être repris en bloc avant d’avoir reconstruit et validé les étapes à partir du BUILD #14**.

---

## 5. Liaison de MEMS Manager complet en x64 — HISTORIQUE TECHNIQUE

Workflow : `.github/workflows/build-ecu-mems-x64-link-smoke.yml`.
Run : **32709187615**.

Corrections MSVC minimales :

- `5e53ef22c4b848397aa5abdd453378f76f1e3988` — `or` → `||` ;
- `722a8278c5974e4b016a59ed6b1de70baee01f40` — `and` → `&&` ;
- `09564db70f729e1a3d78f974918d0d24bafbc982` — autres `or` → `||`.

Résultat technique : EXE + `mems_manager_x64.dll` en AMD64, import de la nouvelle DLL oui, ancienne `librosco.dll` non.

---

## 6. Premier package portable x64 sans IA — HISTORIQUE UTILE

Run **32713710308**.

- 29 PE contrôlés AMD64 ;
- 243 fichiers ;
- base : 85 ECU / 140 affectations / 91 commandes ;
- smoke launch 5 s : OK ;
- onglet IA stable en mode secours sans runtime local sur PC secondaire propre.

Ce package prouve certains éléments x64 mais **ne remplace pas le BUILD #14 comme base de reprise fonctionnelle réelle**.

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

## 8. Diagnostics x64 récents — À CONSERVER COMME HISTORIQUE, PAS COMME VOIE DE REPRISE

- Run **32717558263** : test IA réel PASS, application vivante après 20 s ; rouge uniquement à cause d’un conflit Git lors de l’enregistrement du résultat.
- Run **32718093195** : sélection IA par barre latérale, VERT.
- Run **32721284999** : construction + réutilisation cache `ExpertRuntimeDatabase` x64, VERT.
- Run **32721873137** : rouge lié aux permissions workflow.
- Package x64 complet IA run **32741977123** : build GitHub vert mais fermeture réelle de l’application à l’ouverture de IA MEMS.
- A/B sans `/utf-8` run **32753372131** : fermeture identique ; `/utf-8` exclu.
- Plusieurs workflows staged suivants ont été rouges à cause de leurs propres erreurs YAML/logique de workflow et ne constituent pas des résultats fonctionnels du programme.
- Le run **32759202133 / build #6 du workflow diagnostic** n’a créé aucun job : échec du workflow avant compilation, sans valeur diagnostique sur MEMS Manager.

**Décision : arrêter cette voie de diagnostic comme axe principal. Repartir de BUILD #14.**

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

Cette version démontre une régression dans le chemin de reconstruction x64, **mais la stratégie actuelle n’est plus de réparer directement ce sommet**. On remonte depuis la dernière base réellement validée : BUILD #14.

---

## 10. BUILD #14 / v1.0.14 — POINT DE REPRISE OFFICIEL

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
- **Qwen3-0.6B-Q8_0 : même modèle GGUF utilisé ensuite au #15** ;
- communication application ↔ IA par HTTP local sur `127.0.0.1:18089`.

### 10.2 IA locale

- onglet IA MEMS s’ouvre sans fermeture ;
- statut **`base prête • IA locale prête`** ;
- questions/réponses possibles ;
- IA stable connecté et déconnecté ;
- qualité des réponses encore perfectible, ce qui explique l’objectif du BUILD #15.

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

Cette trace est la référence matérielle pour comparer les futurs builds reconstruits.

### 10.6 Pourquoi repartir du BUILD #14

Parce qu’il est **le dernier état pour lequel nous avons une validation réelle et complète**, et pas seulement un build GitHub vert :

- programme stable ;
- IA locale réellement prête ;
- base expert prête ;
- vrai ECU connecté ;
- identification ECU correcte ;
- polling 7D/80 ;
- Injection RAM Mode 4 ;
- retour au polling normal ;
- navigation multi-onglets ;
- acquisition/enregistrement réel moteur arrêté + démarrage + moteur tournant.

**Il devient donc la base de reconstruction. Les changements après #14 doivent être réintroduits un par un et validés, pas fusionnés en bloc.**

### 10.7 Première étape après reprise : reconstruire le BUILD #15

Le BUILD #15 / v1.0.15 correspondait à : **`Improve IA MEMS conversational relevance`**.

Le premier travail à refaire depuis #14 est donc **uniquement l’amélioration de la qualité des réponses IA**, sans passage x64 simultané, sans autre changement d’interface/protocole.

Comparaison exacte #14 → #15 : **un seul fichier du programme change : `expert/LocalAiClient.cpp`**. Les autres différences sont des workflows/marqueurs GitHub.

Validation demandée après cette étape :

1. le programme reste ouvert ;
2. IA MEMS affiche `base prête • IA locale prête` ;
3. les questions connues produisent des réponses plus pertinentes ;
4. connexion ECU toujours fonctionnelle ;
5. 7D/80 toujours fonctionnel ;
6. Mode 4 Injection toujours fonctionnel ;
7. retour Mode 4 → 7D/80 toujours fonctionnel.

Seulement après validation, reprendre l’étape suivante historiquement prévue.

### 10.8 Étape d’architecture ultérieure : passage x64 isolé

Une fois le BUILD #15 validé en x86, le passage x64 doit être refait comme **une étape isolée** :

- recompiler ECU MEMS Manager en x64 ;
- remplacer `librosco.dll` x86 par `mems_manager_x64.dll` x64 ;
- conserver **le même `llama-server.exe` x64** déjà validé au BUILD #14 ;
- conserver **le même Qwen3-0.6B-Q8_0.gguf** ;
- ne pas modifier simultanément le prompt IA, la base expert, l’interface ou le protocole au-delà de ce qui est strictement nécessaire au passage x64.

Validation immédiate du premier build x64 reconstruit : ouverture IA, `base prête`, `IA locale prête`, questions simples, stabilité 20 s+, puis connexion ECU, 7D/80, Mode 4, retour Mode 4 → 7D/80 et navigation.

---

## 11. Détection automatique câble / port COM — exigence future

La détection automatique doit être générale à MEMS Manager, pas réservée au MEMS 1.9 :

- détecter automatiquement les interfaces ;
- afficher par exemple `COM5 — FTDI FT232 — câble détecté` ;
- plusieurs interfaces : COM + type ;
- à Connexion, vérifier interface/protocole ;
- messages explicites : `câble incompatible`, `aucun ECU détecté`, `mauvais type d’interface`.

Ne pas introduire ce chantier pendant la reconstruction #14 → #15 sauf demande explicite.

---

## 12. Injection / RAM — règles à ne pas perdre

- Ne jamais calculer le temps d’injection à partir du polling normal `0x7D/0x80`.
- Adresses RAM importantes : **`0x03C8`, `0x026E`, `0x0280`**.
- Onglet Injection entre Aperçu et Réglage.
- Dwell/temps bobine dans cet onglet.
- Éviter de mélanger inutilement plusieurs modes de lecture dans une même page.
- Le dwell arrêté/lancement ne doit pas être comparé directement à une plage moteur tournant.

---

## 13. PROCHAINE ACTION EXACTE — NOUVELLE STRATÉGIE

**Abandonner comme priorité la réparation directe du package x64 récent.**

Reprise :

1. **BUILD #14 / v1.0.14** = base source `c2fdecac164a72ada572c13abc4f71f9e4f17273` ;
2. reconstruire seulement l’évolution qui devait donner **BUILD #15 / v1.0.15** : amélioration de la pertinence conversationnelle IA dans `expert/LocalAiClient.cpp` ;
3. conserver à cette étape **MEMS Manager x86 + `librosco.dll` x86 + llama-server x64 + Qwen identique** ;
4. produire un nouveau build clairement numéroté ;
5. tester IA locale + base ;
6. tester ensuite le vrai ECU avec la checklist validée du #14 ;
7. seulement après succès, refaire **le passage x64 isolé** : MEMS Manager x64 + `mems_manager_x64.dll` x64, sans changer le serveur llama x64 ni le modèle Qwen ;
8. valider immédiatement IA + ECU sur ce premier x64 avant toute évolution supplémentaire ;
9. poursuivre ensuite changement par changement avec validation entre chaque étape.

**Objectif : identifier exactement la première étape qui casse quelque chose, au lieu d’essayer de diagnostiquer un état final ayant accumulé plusieurs changements.**
