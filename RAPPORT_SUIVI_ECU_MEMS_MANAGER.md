# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE POUR LES PROCHAINES DISCUSSIONS**
>
> Au début de chaque nouvelle discussion concernant ECU MEMS Manager, ce fichier doit être relu avant de reprendre le travail.
> L’assistant doit **mettre à jour ce rapport lui-même et le pousser régulièrement sur la branche `RAPPORT`**, au minimum après chaque étape technique importante, validation, découverte, changement de branche de travail ou décision d’architecture.
> La branche `RAPPORT` sert au suivi et à la transmission entre discussions. Elle ne doit pas servir à modifier le programme de production sauf demande explicite.

Dernière mise à jour : **24 août 2026 — le build #14 est validé sur PC réel avec IA locale prête + base prête + ECU AANMP002/MNE101150 connecté sur COM3 ; le package x64 complet plus récent ferme toujours à l’ouverture de IA MEMS, même avec dossier `ai` neutralisé et aucun `llama-server.exe` actif. La priorité est maintenant d’identifier la régression apparue après #14.**

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

Sur PC secondaire propre : navigation générale stable, base disponible, IA en secours normal car runtime absent. Défauts mojibake et responsive visibles mais non prioritaires.

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

Conclusion : **le crash récent n’est pas causé directement par le runtime llama.cpp ni par le modèle Qwen**. La régression doit être recherchée après le build #14 dans l’état du programme/package autour de l’onglet IA, de son chargement expert, de la navigation ou d’un autre changement postérieur.

---

## 10. BUILD #14 — NOUVELLE RÉFÉRENCE RÉELLE VALIDÉE SUR ECU

Le 24 août 2026, l’utilisateur a retesté **ECU MEMS Manager v1.0.14 / build #14** sur le même environnement réel.

### 10.1 IA locale

Build #14 :

- onglet IA MEMS s’ouvre sans fermeture ;
- statut **`base prête • IA locale prête`** ;
- questions/réponses possibles ;
- réponses parfois hors sujet ou insuffisantes, qualité à améliorer ultérieurement, mais moteur IA réellement fonctionnel ;
- l’IA reste stable également avec ECU connecté.

Le build #14 prouve donc que **Qt x64 + onglet IA + base expert + llama.cpp + modèle Qwen peuvent fonctionner ensemble sur ce PC**.

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

### 10.4 Points visibles mais secondaires

- L’IA répond encore à côté sur plusieurs questions générales (`quel jour`, nombre de données, câble, etc.).
- Certaines interprétations/valeurs doivent être revues séparément ; ne pas les corriger pendant l’enquête de crash.
- Le dwell affiché à contact/moteur arrêté n’est pas à comparer directement à la plage de contrôle ~1,9–3,1 ms vers 14 V moteur en fonctionnement.
- Le désordre visuel de réinsertion des onglets au démarrage existe dans le code, mais le test d’attente >2 min montre qu’il n’est pas la cause principale de la fermeture IA du package récent.

### 10.5 Conclusion de référence

**Build #14 = dernière référence réelle actuellement prouvée stable pour : x64 + IA locale prête + base prête + connexion ECU AANMP002/MNE101150 + COM3 + navigation + 7D/80 + Mode 4 Injection.**

La recherche doit maintenant identifier le **premier changement après #14** qui introduit la fermeture de l’onglet IA dans le package récent.

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

**Priorité immédiate : identifier la régression apparue après le build #14.**

Méthode :

1. rattacher précisément le build #14 fonctionnel à son commit/workflow/artifact ;
2. comparer les changements entre #14 et le package actuel ;
3. isoler le premier changement qui fait fermer IA MEMS ;
4. corriger uniquement cette régression, sans refonte graphique ni changement protocolaire parasite ;
5. reconstruire un package x64 de test ;
6. revalider sur le même PC : IA locale prête d’abord, puis connexion ECU réelle.

Ne pas reprendre pour l’instant la refonte responsive ou l’extension protocolaire tant que cette régression IA n’est pas localisée et corrigée.