# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE POUR LES PROCHAINES DISCUSSIONS**
>
> Au début de chaque nouvelle discussion concernant ECU MEMS Manager, ce fichier doit être relu avant de reprendre le travail.
> L’assistant doit **mettre à jour ce rapport lui-même et le pousser régulièrement sur la branche `RAPPORT`**, au minimum après chaque étape technique importante, validation, découverte, changement de branche de travail ou décision d’architecture.
> La branche `RAPPORT` sert au suivi et à la transmission entre discussions. Elle ne doit pas servir à modifier le programme de production sauf demande explicite.

Dernière mise à jour : **24 août 2026 — le build historique #14 reste la référence réelle fonctionnelle : IA locale prête + base prête + ECU AANMP002/MNE101150. Le package MEMSX64 récent ferme à l’ouverture de IA MEMS. Le test A/B x64 sans `/utf-8` ferme exactement pareil : `/utf-8` est donc exclu comme cause. Plusieurs workflows de diagnostic suivants sont devenus rouges à cause de leurs propres erreurs de workflow, avant ou indépendamment du test du programme ; ces rouges ne constituent pas des régressions de MEMS Manager. Un diagnostic minimal “IA interface seule” a été préparé pour isoler `showEvent` de la base expert et de LocalAiClient. Règle utilisateur obligatoire : pour chaque version à tester, toujours annoncer d’abord le numéro de build/run lisible (`build #XX`) ; les hashes Git ne sont que des références techniques secondaires.**

---

## 1. Règles de travail à conserver

- Dépôt principal : `mini56/ECU-Mems-Manager-Session`.
- Branche de travail x64 : **`MEMSX64`**.
- Branche de transmission : **`RAPPORT`**.
- L’utilisateur compile et teste via **GitHub Actions sous Windows**, pas avec Qt Creator.
- Préserver la version 32 bits existante pendant le développement x64 parallèle.
- Ne modifier que ce qui est demandé explicitement ou strictement nécessaire à la tâche en cours.
- Conserver l’interface sombre et responsive ; ne pas engager de refonte graphique non demandée.
- Les familles MEMS **1.2 / 1.3 / 1.6 / 1.9** doivent rester distinguées.
- Dans `memsinterface.h`, conserver impérativement : `void onProtocolCommandRequested(quint8 command);`.
- Ne jamais confondre polling normal `0x7D/0x80`, RAM/Mode 4, calibrations/cartes et données externes.
- **Règle de communication build :** quand un artefact/version doit être testé par l’utilisateur, toujours donner **le numéro de build/run lisible en premier**, sous une forme du type **`build #14`** ou **`build #XX — run 327...`**. Ne jamais demander à l’utilisateur d’identifier une version uniquement avec un hash Git du type `890f68e...`. Le commit peut être indiqué ensuite, uniquement comme référence technique secondaire.

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

Correctifs historiques importants :

- `cbcb8a14189b1bc013cb3519b0ba33aa3f85c072` — `Fix IA MEMS history scroll and factual answers` ;
- `896a59f762e648ce50023121491693ee70ad162d` — `Build IA MEMS quality patch`.

Package historique IA validé :

- `IA_MEMS_FINAL_PACKAGE_OK.txt` ;
- source commit : `896a59f762e648ce50023121491693ee70ad162d` ;
- run : **32694753190** ;
- application historique : **32 bits** ;
- `llama-server.exe` : processus séparé **x64**.

### 2.2 Base Andrew Revill / MEMSTools

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

## 4. Nouvelle DLL protocole native x64 — VALIDÉE

Nom : **`mems_manager_x64.dll`**.

- sources : `librosco-x64/` ;
- DEF : `librosco-x64/mems_manager_x64.def` ;
- PE32+ AMD64 ;
- `frame80` : 28 octets ;
- `frame7d` : 32 octets ;
- `mems_data` : 60 octets ;
- les 22 exports historiques sont présents.

Sécurités conservées : refus d’une trame `0x7D` tronquée et retour d’état réel pour clear/reset.

---

## 5. Liaison de MEMS Manager complet en x64 — VALIDÉE

Workflow : `.github/workflows/build-ecu-mems-x64-link-smoke.yml`.
Run : **32709187615**.

Corrections MSVC minimales :

- `5e53ef22c4b848397aa5abdd453378f76f1e3988` — `or` → `||` ;
- `722a8278c5974e4b016a59ed6b1de70baee01f40` — `and` → `&&` ;
- `09564db70f729e1a3d78f974918d0d24bafbc982` — autres `or` → `||`.

Résultat : EXE + `mems_manager_x64.dll` en AMD64, import de la nouvelle DLL oui, ancienne `librosco.dll` non.
Marqueur historique : `2e2165b8001482647d4a3606df4f4051ac63cf03`.

---

## 6. Premier package portable x64 sans IA — VALIDÉ

Workflow : `.github/workflows/build-ecu-mems-x64-package.yml`.
Run **32713710308**, marqueur `494c8993ffc218195639d974b7b2a7c81b3f8541`.

- 29 PE contrôlés AMD64 ;
- 243 fichiers ;
- base : 85 ECU / 140 affectations / 91 commandes ;
- smoke launch 5 s : OK ;
- ancienne `librosco.dll` absente.

Sur PC secondaire propre : navigation générale stable, base disponible, **onglet IA MEMS stable**, avec secours déterministe normal car runtime local absent. Défauts mojibake et responsive visibles mais non prioritaires.

Ce package est une référence A importante : **l’EXE x64 sait ouvrir IA MEMS sans fermeture dans ce package ancien.**

---

## 7. Persistance Windows à garder en tête

Chaque version est extraite dans son propre dossier, mais plusieurs états sont partagés hors dossier de l’EXE :

- QSettings utilisateur (`SerialDevice`, langue, thème, etc.) ;
- AppLocalData `/reference` ;
- AppLocalData `/ia-mems` ;
- variables `MEMS_AI_SERVER` / `MEMS_AI_MODEL` ;
- éventuel serveur déjà actif sur `127.0.0.1:18089`.

La base principale modifiable reste dossier-local : `<appdir>/database/ecu_mems_manager.sqlite`.

Le package x64 actuel est **portable** : il n’y a pas de désinstalleur à utiliser pour ce test.

---

## 8. Diagnostics GitHub IA x64 du 24 août 2026

- Run **32717558263** : test IA réel PASS, application vivante après 20 s ; rouge uniquement à cause d’un conflit Git lors de l’enregistrement du résultat.
- Run **32718093195** : sélection IA par barre latérale, VERT.
- Run **32721284999** : construction + réutilisation cache `ExpertRuntimeDatabase` x64, VERT.
- Run **32721873137** : rouge uniquement parce que le bot GitHub n’avait pas la permission `workflows` pour modifier un autre workflow.

Ces succès CI ne reproduisent pas la fermeture observée sur le package utilisateur récent.

Les tentatives de diagnostic “staged” suivantes ont ensuite produit plusieurs runs rouges **à cause des workflows eux-mêmes** : logique de persistance Git, déclenchement/écriture de marqueur et instrumentation trop complexe. Ces rouges ne doivent pas être interprétés comme des crashes supplémentaires de MEMS Manager. La stratégie est désormais de revenir à des workflows de diagnostic minimaux, une seule hypothèse à la fois.

---

## 9. Package MEMSX64 avec IA locale complète — BUILD VERT, TEST RÉEL EN ÉCHEC IA

Commit de packaging IA : **`58bd6a1f74da991ddf3b831fcfb55d2716755084`**.
Run : **32741977123**, totalement vert côté GitHub Actions.

Le workflow :

- télécharge/vérifie llama.cpp b10516 x64 ;
- télécharge/vérifie Qwen3-0.6B-Q8_0 ;
- ajoute VC++ x64 app-local ;
- teste réellement `/health` sur 18089 ;
- package runtime + modèle + licences.

Artefact : **`ECU-MEMS-Manager-x64-Windows-Test`**, id `9525708940`, environ 780 Mo, digest `sha256:d2127078f8a4db92103ecebd4fd9585f419b81edb6197ab7ce68265637c9d6c7`.

### 9.1 Test réel utilisateur du package récent

Sur le PC secondaire :

- application stable tant que IA MEMS n’est pas ouverte ;
- navigation dans les autres onglets possible ;
- dès ouverture de IA MEMS, affichage `base en chargement • IA locale en démarrage`, puis fermeture brutale environ 2–3 s après ;
- attendre plus de 2 minutes avant d’ouvrir IA ne change rien : l’ordre/reconstruction temporisée des onglets n’est donc pas la cause principale ;
- renommer `ai` en `ai_OFF` ne change rien ;
- après vérification dans le Gestionnaire des tâches, **aucun `llama-server.exe` n’est actif** ;
- avec `ai_OFF` + aucun serveur actif, l’ouverture IA ferme encore l’application.

Conclusion : **le crash récent n’est pas causé directement par le runtime llama.cpp ni par le modèle Qwen**. Il faut chercher une différence dans l’EXE x64 / environnement déclenché par l’ouverture de IA.

### 9.2 Comparaison des sources IA — aucune dérive du cœur IA

Comparaison directe `lab-expert-engine` ↔ `MEMSX64` :

- `iamemstab.cpp` : même blob Git **`8a054a67430c58cd0bcbfb36cf542be83bfdf1b4`** ;
- `expert/LocalAiClient.cpp` : même blob Git **`d438fa44cbf9436509ebc43367393b9f86140452`** ;
- `expert/ExpertRuntimeDatabase.cpp` : même blob Git **`85aacb8a9456c6e85c1b5923f195e09f05230af9`**.

Donc **le cœur de l’onglet IA, le client llama et la construction/ouverture de la base expert n’ont pas été modifiés entre ces branches**.

### 9.3 Piste `/utf-8` — ÉLIMINÉE PAR TEST RÉEL

Entre le premier package x64 stable et le package x64 récent, l’ajout de `/utf-8` à MSVC avait été identifié comme différence persistante de compilation :

- `7217b58b8236bceac83cf9a002da801459ab271c` — `Compile MEMSX64 application source strings as UTF-8` ;
- `5598ae99376912c4dfc2c902006a598290a2629d` — `Keep only MSVC UTF-8 source encoding fix`.

Un package A/B x64 sans `/utf-8` a donc été construit.

**Résultat utilisateur : le package sans `/utf-8` ferme exactement pareil à l’ouverture de IA MEMS.**

Conclusion : **`/utf-8` n’est pas la cause du crash IA et cette piste est fermée.** Ne pas y revenir sauf nouvel élément concret.

### 9.4 Package A/B x64 SANS `/utf-8` — TESTÉ, ÉCHEC IDENTIQUE

Workflow diagnostic : `.github/workflows/memsx64-ia-ab-no-utf8-package.yml`.

- run : **32753372131** ;
- résultat CI : **SUCCESS** ;
- artefact : **`ECU-MEMS-Manager-x64-IA-AB-NO-UTF8`** ;
- artifact id : **9530010669** ;
- taille : **91 027 767 octets** ;
- digest : **`sha256:deb57015a6fccb4d9e142603c1281be400fb161a16b8bf8ec1fdb6a977acef1a`**.

Test réel sur le PC utilisateur :

- lancement normal ;
- ouverture IA MEMS ;
- **fermeture identique**.

Ce résultat exclut `/utf-8` et confirme qu’il faut isoler les étapes réellement exécutées par l’ouverture de l’onglet IA.

### 9.5 Diagnostic minimal “IA interface seule” — PROCHAINE ISOLATION

Après plusieurs workflows staged rouges pour des raisons de workflow, un diagnostic beaucoup plus simple a été préparé sur `MEMSX64`.

Objectif : compiler un package où `IaMemsTab::showEvent()` appelle seulement `QWidget::showEvent(event)` puis retourne immédiatement :

- **pas de `startKnowledgeLoad()`** ;
- **pas de `ExpertRuntimeDatabase` lancé par l’onglet** ;
- **pas de `LocalAiClient::initialize()`** ;
- **pas de runtime llama/modèle dans le package**.

Nom d’artefact prévu : **`ECU-MEMS-Manager-x64-IA-UI-ONLY-DIAG`**.

Référence technique du workflow : commit `890f68e1eeff270acabda0ba3b2169a7e2765946`.

**Important : ne pas présenter ce hash comme le nom du build à l’utilisateur. Attendre que GitHub Actions attribue le numéro de build/run, puis annoncer d’abord `build #XX` au moment du test.**

Interprétation attendue :

- si IA interface seule reste ouverte : le crash est dans la base expert ou LocalAiClient, pas dans le simple affichage de l’onglet ;
- si IA interface seule ferme encore : chercher dans le cycle de vie de l’onglet/navigation/widget lui-même.

---

## 10. BUILD #14 — RÉFÉRENCE HISTORIQUE RÉELLE VALIDÉE SUR ECU

Le 24 août 2026, l’utilisateur a retesté **ECU MEMS Manager v1.0.14 / build #14** sur le même PC de test.

**Précision d’architecture importante :** ce build historique de l’application est **32 bits**. Son `llama-server.exe` est un processus séparé **x64**. Ne pas présenter le build #14 comme une application x64.

### 10.1 IA locale

Build #14 :

- onglet IA MEMS s’ouvre sans fermeture ;
- statut **`base prête • IA locale prête`** ;
- questions/réponses possibles ;
- réponses parfois hors sujet ou insuffisantes, qualité à améliorer ultérieurement, mais moteur IA réellement fonctionnel ;
- l’IA reste stable également avec ECU connecté.

Le build #14 prouve donc que **l’architecture IA historique (application 32 bits + base expert + serveur llama.cpp x64 séparé + modèle Qwen) fonctionne réellement sur ce PC**.

### 10.2 Connexion ECU réelle

Essai effectué contact mis, moteur initialement arrêté, sans actionneur/reset/effacement de défaut.

Captures utilisateur :

- port : **COM3** ;
- firmware : **AANMP002** ;
- ID affiché : **AANMP002 — MNE101150** ;
- communication verte ;
- mode normal : **Diagnostic ECU (7D/80)** ;
- IA MEMS pendant connexion : **`ECU connecté • base prête • IA locale prête`**.

### 10.3 Navigation et lectures observées

Le build #14 reste ouvert et navigable sur :

- Aperçu ;
- Réglages ;
- Actionneurs ;
- Erreurs ;
- Diagnostic automatique ;
- IA MEMS ;
- Analyse ;
- Toutes les mesures ;
- ECU/ROSCO ;
- Toutes les données.

L’utilisateur a finalement aussi ouvert Injection pendant la série de captures :

- passage **Diagnostic ECU (7D/80) → Injection RAM (Mode 4)** fonctionnel ;
- puis retour au mode diagnostic normal fonctionnel ;
- exemple capture Injection : injection finale ≈ **2,47 ms**, base RAM `0x03C8 = 1233 ticks`, transitoire `0x026E = 0 ticks`, état `0x0280 = 0` ;
- ne pas généraliser ces valeurs à d’autres conditions moteur sans analyse.

Autres valeurs de captures à contact/moteur selon instant : MAP ≈ 100 kPa moteur arrêté, batterie ≈ 12,2 V, liquide ≈ 25 °C, air ≈ 29 °C, lambda ≈ 435 mV. Une autre capture montre moteur autour de 1195 tr/min et batterie 13,5 V, preuve que la session a aussi observé le moteur en fonctionnement à un moment du test.

### 10.4 Enregistrement réel de la session

L’utilisateur a enregistré la session `2026-08-24_18.14.txt` pendant ce même essai.

Points de référence observés :

- avant démarrage : 0 tr/min, MAP ≈ 100 kPa, batterie ≈ 12,2 V, dwell ≈ 6,274 ms ;
- lancement autour de 18:25:13 : régime ≈ 167 tr/min et chute batterie ≈ 11,3 V ;
- démarrage : environ 1320 tr/min puis pointe ≈ 1657 tr/min ;
- stabilisation ensuite autour de 1180–1200 tr/min ;
- MAP moteur tournant ≈ 30–35 kPa ;
- batterie remonte vers ≈ 13,8 V ;
- dwell moteur tournant descend autour de ≈ 3,2–3,4 ms ;
- `7D14-15` brut évolue réellement, donc l’erreur ralenti chaud n’est pas une donnée figée.

Cette trace doit être conservée comme référence matérielle pour les comparaisons de futurs builds.

### 10.5 Points visibles mais secondaires

- L’IA répond encore à côté sur plusieurs questions générales (`quel jour`, nombre de données, câble, etc.).
- Certaines interprétations/valeurs doivent être revues séparément ; ne pas les corriger pendant l’enquête de crash.
- Le dwell affiché à contact/moteur arrêté n’est pas à comparer directement à la plage de contrôle ~1,9–3,1 ms vers 14 V moteur en fonctionnement.
- Le désordre visuel de réinsertion des onglets au démarrage existe dans le code, mais le test d’attente >2 min montre qu’il n’est pas la cause principale de la fermeture IA du package récent.

### 10.6 Conclusion de référence

**Build #14 = dernière référence historique réelle actuellement prouvée stable pour : IA locale prête + base prête + connexion ECU AANMP002/MNE101150 + COM3 + navigation + 7D/80 + Mode 4 Injection.**

Il ne valide pas à lui seul l’EXE x64, car l’application du build #14 est 32 bits.

---

## 11. Détection automatique câble / port COM — exigence future

La détection automatique doit être générale à MEMS Manager, pas réservée au MEMS 1.9 :

- détecter automatiquement les interfaces ;
- afficher par exemple `COM5 — FTDI FT232 — câble détecté` ;
- plusieurs interfaces : COM + type ;
- à Connexion, vérifier interface/protocole ;
- messages explicites : `câble incompatible`, `aucun ECU détecté`, `mauvais type d’interface`.

Le build #14 confirme qu’une connexion réelle fonctionne sur COM3, mais **la détection automatique générale reste à développer/tester**.

---

## 12. Injection / RAM — règles à ne pas perdre

- Ne jamais calculer le temps d’injection à partir du polling normal `0x7D/0x80`.
- Adresses RAM importantes : **`0x03C8`, `0x026E`, `0x0280`**.
- Onglet Injection entre Aperçu et Réglage.
- Dwell/temps bobine dans cet onglet.
- Éviter de mélanger inutilement plusieurs modes de lecture dans une même page.
- Plage dwell de référence : environ **1,9 à 3,1 ms vers 14 V** dans les conditions pertinentes.

---

## 13. Prochaine action exacte

**Priorité immédiate : obtenir un build GitHub Actions vert du diagnostic minimal `IA interface seule`.**

Ne pas demander de nouveau test utilisateur tant que ce diagnostic n’est pas vert et que son artefact n’est pas disponible.

Une fois l’artefact produit :

1. annoncer à l’utilisateur **le numéro de build/run en premier : `build #XX`** ;
2. donner éventuellement le run GitHub complet ensuite ;
3. le hash Git n’est qu’une référence secondaire ;
4. extraire le package dans un nouveau dossier ;
5. lancer `ecu_mems_manager.exe` ;
6. ouvrir IA MEMS et attendre 15–20 s ;
7. constater uniquement : reste ouvert ou ferme encore.

Pas besoin d’ECU pour ce diagnostic.

Si IA interface seule reste ouverte, isoler ensuite séparément :

- **base expert seule** ;
- puis **LocalAiClient seul**.

Si IA interface seule ferme encore, concentrer l’enquête sur le cycle de vie de l’onglet/navigation/widget, sans toucher au protocole ECU.

Ne pas reprendre pour l’instant la refonte responsive ou l’extension protocolaire tant que cette régression IA n’est pas localisée et corrigée.