# RAPPORT DE CONTINUITÉ — ECU MEMS MANAGER

> **RÈGLE OBLIGATOIRE** : relire ce fichier avant toute modification. Branche rapport : `RAPPORT`. Branche x64 active : `MEMSX64`.
>
> **RÈGLE QUALITÉ — AUCUNE RUSTINE** : rechercher la cause réelle et produire une solution propre, générale et maintenable. Ne pas supprimer une capacité juste pour faire passer un test.
>
> **SUIVI IMMÉDIAT** : avant chaque nouvelle étape, inscrire ici l’étape et son objectif ; après chaque résultat, l’inscrire avant la suite.
>
> **NOMMAGE UTILISATEUR** : `ECU MEMS Manager x64 #NN — Commit xxxxxxx`. `#NN` = GitHub Actions, pas le BUILD logiciel.

## ÉTAT COURANT — 27 AOÛT 2026

- Dépôt : `mini56/ECU-Mems-Manager-Session`.
- Branche x64 active : **`MEMSX64`**.
- HEAD x64 courant : **`d156469af53b4ba20e084439e2d62b691c9e199b`**.
- BUILD logiciel actif : **#30 / v1.0.30**.
- Dernier GitHub Actions validé : **#87 — SUCCESS**.
- Run #87 : `33041407944`, commit `d156469...`.
- Artefact #87 : `ECU-MEMS-Manager-x64-BUILD-30-v1.0.30`, ID `9634135544`, taille `386 753 794` octets, digest `sha256:ad01bbef0f82cdb59e8d2ed2cb99f606534f140a75db82a04f18aef38c0060a7`.
- Aucun BUILD #31 sans demande explicite.
- 32 bits `lab-expert-engine` : **NE PAS TOUCHER**.
- Rollback `MEMSX64-BUILD26-BASE` : **NE PAS TOUCHER**.

## TEST UTILISATEUR RÉEL #86 — 27 AOÛT 2026

L'utilisateur confirme : **« #86 fonctionne, les réponses arrivent un peu plus vite mais pas encore vraiment bien ciblées »** et fournit plusieurs captures réelles.

Constats confirmés :
- l'application et IA MEMS démarrent ; `base prête` et `IA locale prête` sont présents ;
- la latence est améliorée par rapport aux builds précédents ;
- certaines questions connues ne sont toujours pas routées vers une réponse déterministe lorsqu'elles contiennent une faute typique, par exemple **`C4EST QUOI LA BOBINE?`** ;
- lorsqu'une question tombe alors sur Qwen, une sortie peut encore commencer par **`<think>`** ;
- des morceaux de consignes internes / contexte système peuvent encore devenir visibles ;
- la réponse obtenue peut parler de son propre contexte au lieu de répondre directement.

Nouvelles captures utilisateur à traiter comme défauts réels :
1. **`COULEUR DE FIL DU CAPTEUR DE TEMPERATURE SPI ?`** → renvoie des faits de dépose ECT/IAT/MAP, sans aucune couleur de fil. Cause fonctionnelle probable : la recherche experte classe sur mots génériques (`température`, `capteur`) sans priorité forte pour l'intention `câblage/couleur`.
2. **`BROCHE ECU 1.3`** → renvoie une définition générique de MEMS 1.3 au lieu du brochage/connecteur.
3. **`BROCHE OBD 1.9 ?`** → renvoie une définition générique de MEMS 1.9 au lieu du brochage OBD/diagnostic.
4. **`CADRAN TPM?`** → aucune réponse utile ; la faute probable `TPM`→`RPM` n'est pas reconnue.
5. **`QUEL CADRAN DANS L4ONGLET APERCU?`** → répète la description générale de l'onglet Aperçu au lieu de lister les cadrans réellement présents.
6. Le message **« Moteur conversationnel local indisponible : le modèle local n'a pas produit de réponse exploitable dans la langue active »** apparaît encore sur certaines questions ; ces questions techniques structurées devraient autant que possible être résolues par routage/base avant d'appeler Qwen.

Conclusion : **#86 est fonctionnel et plus rapide, mais la qualité conversationnelle n'est pas validée.** Les défauts sont maintenant suffisamment précis pour corriger le routage par intention et la sélection de faits, sans changer ONNX ni l'UI.

## BUILD QUALITÉ #86 — À PRÉSERVER

### ECU MEMS Manager x64 #86 — Commit `50af0a0`

- Run `33004859269`, job `98296030990`, conclusion **SUCCESS**.
- Artifact `ECU-MEMS-Manager-x64-BUILD-30-v1.0.30`, ID `9620325660`.
- Taille `386 747 323` octets.
- Digest `sha256:1e398c4832ed2fc2b162d8eac59b5f2f43c368de2d7e041deae6606c2fd70962`.
- Compilation, protections protocole, tests déterministes, base r20, Qwen, vrai `LocalAiClient`, package, self-test depuis package, smoke launch, manifeste et upload : VERTS.

Lot qualité #81→#86 à préserver : prompt Qwen simplifié et anti-fuite ; nettoyage `<think>`/ChatML ; bobine/date fautive ; base r20 classée par pertinence/preuve ; distinction définition ≠ mesure live ; latence `/no_think`, 128 tokens normal / 192 diagnostic.

## RAVE / ENRICHISSEMENT BASE — LOT 1680 VALIDÉ

Autorisation utilisateur : continuer RAVE et enrichir la base pendant les tests #86.

Règles : uniquement faits nouveaux vérifiables ; source/document/variante/section/preuve ; séparation stricte SPi/MPi/familles ; code23 reste `preuve_insuffisante` ; aucun brochage inventé ; aucun changement protocole/UI/32 bits/BUILD.

- 1660 : faits service RAVE.
- 1670 : RCL0194 Mini MPi wiring.
- 1680 : **RCL0194ENG, Mini Electrical Circuit Diagrams, 3rd Edition**, SPi Japon 97MY à partir du VIN `SAXXNNAXKBD 134455`, pages 20.3, 20.4, 39.3.
- Lot 1680 : 14 faits constructeur miroirés dans `mems_rave_fact` + `mems_expert_fact_external` : périmètre/VIN, module relais, O2, pompe, injecteur, purge, IAC 4 phases, ECT, TPS, IAT, masse capteurs, diagnostic, chauffage collecteur, ventilateur SPi Japon par thermocontact/relais.
- Validation locale avant push : 14+14 entrées, tous `verifie_constructeur`, total RAVE 60, total expert 72, SHA-256 SQL `dec464ad3d5446a78886cfc995a1dc18553aab4f86ac2676803a56ac62d916dd`.
- Push atomique : **`d156469af53b4ba20e084439e2d62b691c9e199b`**, exactement 3 fichiers documentaires/base.
- GitHub Actions **#87 SUCCESS** : compilation, génération r20, tests IA/ONNX, package final et artefact passés.

## CONNAISSANCES RAVE À PRÉSERVER

Ralenti SPi 1993–96 850 ±25 ; SPi 1997+ et MPi 900 ±50 ; pression SPi ~1 bar, MPi 3,0 ±0,2 bar ; fan MPi97 105/98 °C ; SPi Japon 98/93 °C ; pas de réglage ralenti par butée ; conflits résistance bobine conservés ; MEMS1.3 SPi / MEMS1.6 SPi / MEMS2J MPi distingués ; DTC fortement supportés 1 ECT, 2 IAT, 10 pompe, 16 TPS, 20 chauffage lambda, 21 synchro, 22 fan1, 24 fan2 ; code23 preuve insuffisante ; brochages MPi97 (1670) et SPi Japon 97MY (1680) strictement séparés.

## SÉCURITÉ / UI / IA — INCHANGÉS

Ne pas modifier les protections protocole BUILD #30, MEMS1.9 F7/EF, tailles 7D/80, W4, reconnexion1.9, failsafes, ports, RAM non validée, reset/clear/writes. Conserver `onProtocolCommandRequested(quint8)` et `raw-32768-correction`. UI dark/responsive inchangée. ONNX natif inchangé ; aucun QProcess/llama/HTTP ; base r20 read-only ; aucune mutation ECU accessible au LLM.

## PROCHAINE CORRECTION QUALITÉ — OBJECTIF AVANT MODIFICATION

Corriger sur le HEAD `d156469...` après #87, sans changer BUILD :
1. normaliser les fautes clavier fréquentes de manière contextuelle, notamment `C4EST`→`C EST` et `L4ONGLET`→`L ONGLET`, sans remplacer arbitrairement tous les chiffres ;
2. renforcer le filtre final : toute sortie contenant `<think>`, ChatML ou une consigne interne doit être nettoyée/rejetée avant affichage, même si la balise est incomplète ;
3. ajouter une **classification d'intention structurée** avant la recherche experte : `définition`, `mesure live`, `câblage/couleur`, `brochage/connecteur`, `interface/onglet`, `diagnostic` ;
4. pour `câblage/couleur` et `brochage/connecteur`, donner un poids dominant aux faits contenant explicitement `wire/câble/fil/couleur/broche/pin/connecteur/OBD/diagnostic` et empêcher une simple définition de famille de prendre la priorité ;
5. ne jamais inventer une couleur ou une broche : si aucun fait vérifié ne contient réellement l'information demandée, répondre clairement que la base ne possède pas encore cette donnée pour la variante identifiée ;
6. traiter `TPM` comme faute probable de `RPM` uniquement dans le contexte `cadran/mesure/régime`, pas globalement ;
7. pour `quel cadran dans l'onglet Aperçu`, répondre depuis les métadonnées/structure UI réelle, pas depuis Qwen ;
8. étendre les tests déterministes avec exactement les formulations utilisateur ci-dessus ;
9. préserver la latence #86, ONNX natif, RAVE 1680, protocole, UI, 32 bits et BUILD #30.

## PROCHAINE ACTION EXACTE

**Modifier uniquement la chaîne IA/routage sur le HEAD incluant RAVE 1680, ajouter les tests correspondant aux captures utilisateur, pousser un seul lot qualité, puis suivre le run complet jusqu'au package. Continuer ensuite RAVE sur un lot séparé. Aucun BUILD #31.**