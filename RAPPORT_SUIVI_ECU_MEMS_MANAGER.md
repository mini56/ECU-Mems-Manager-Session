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
- HEAD x64 courant après lot RAVE 1680 : **`d156469af53b4ba20e084439e2d62b691c9e199b`**.
- BUILD logiciel actif : **#30 / v1.0.30**.
- Dernier GitHub Actions validé avant 1680 : **#86 — SUCCESS**.
- GitHub Actions RAVE 1680 lancé : **#87**, run `33041407944`, commit `d156469...`.
- Aucun BUILD #31 sans demande explicite.
- 32 bits `lab-expert-engine` : **NE PAS TOUCHER**.
- Rollback `MEMSX64-BUILD26-BASE` : **NE PAS TOUCHER**.

## TEST UTILISATEUR RÉEL #86 — 27 AOÛT 2026

L'utilisateur confirme : **« #86 fonctionne, les réponses arrivent un peu plus vite mais pas encore vraiment bien ciblées »** et fournit des captures réelles.

Constats visibles à traiter comme défauts réels :
- l'application et IA MEMS démarrent ; `base prête` et `IA locale prête` sont présents ;
- la latence est améliorée par rapport aux builds précédents ;
- certaines questions connues ne sont toujours pas routées vers une réponse déterministe lorsqu'elles contiennent une faute typique, par exemple **`C4EST QUOI LA BOBINE?`** ;
- lorsqu'une question tombe alors sur Qwen, une sortie peut encore commencer par **`<think>`** ;
- des morceaux de consignes internes / contexte système peuvent encore devenir visibles, par exemple les directives de langue ou les consignes « ne pas inventer » ;
- la réponse obtenue peut donc parler de son propre contexte au lieu de répondre directement à la question utilisateur.

Conclusion : **#86 est fonctionnel et plus rapide, mais la qualité conversationnelle n'est pas encore validée.** Le self-test CI #86 n'a pas couvert suffisamment les variantes réelles/stochastiques montrées par ces captures.

### PROCHAINE CORRECTION QUALITÉ — OBJECTIF AVANT MODIFICATION

1. inspecter le normaliseur/routage des réponses contrôlées pour comprendre pourquoi `C4EST QUOI LA BOBINE?` échappe à la définition bobine ;
2. corriger la normalisation de fautes clavier fréquentes **sans transformer arbitrairement tous les chiffres du texte** ;
3. inspecter `cleanModelReply` et le filtre de fuite interne ;
4. faire en sorte qu'une réponse contenant `<think>`, ChatML ou une consigne interne soit **nettoyée/rejetée avant affichage**, y compris si la balise `<think>` n'est pas correctement fermée ;
5. ajouter des tests déterministes couvrant au minimum `C4EST QUOI LA BOBINE?` et des sorties synthétiques représentant les fuites réellement observées ;
6. ne pas masquer un défaut par une simple phrase de secours : corriger routage + sanitation à la source ;
7. conserver la latence #86, l'ONNX natif, l'UI, le protocole, le 32 bits et BUILD #30.

**Ne pousser cette correction qu'après avoir laissé #87 valider le lot RAVE 1680, afin de ne pas mélanger un éventuel défaut de base documentaire avec la correction IA.**

## BUILD QUALITÉ #86 — À PRÉSERVER

### ECU MEMS Manager x64 #86 — Commit `50af0a0`

- Run `33004859269`, job `98296030990`, conclusion **SUCCESS**.
- Artifact `ECU-MEMS-Manager-x64-BUILD-30-v1.0.30`, ID `9620325660`.
- Taille `386 747 323` octets.
- Digest `sha256:1e398c4832ed2fc2b162d8eac59b5f2f43c368de2d7e041deae6606c2fd70962`.
- Compilation, protections protocole, tests déterministes, base r20, Qwen, vrai `LocalAiClient`, package, self-test depuis package, smoke launch, manifeste et upload : VERTS.

Lot qualité #81→#86 à préserver : prompt Qwen simplifié et anti-fuite ; nettoyage `<think>`/ChatML ; bobine/date fautive ; base r20 classée par pertinence/preuve ; distinction définition ≠ mesure live ; latence `/no_think`, 128 tokens normal / 192 diagnostic.

## RAVE / ENRICHISSEMENT BASE — LOT 1680

Autorisation utilisateur : continuer RAVE et enrichir la base pendant les tests #86.

Règles : uniquement faits nouveaux vérifiables ; source/document/variante/section/preuve ; séparation stricte SPi/MPi/familles ; code23 reste `preuve_insuffisante` ; aucun brochage inventé ; aucun changement protocole/UI/32 bits/BUILD.

### AUDIT ET PUSH 1680

- 1660 : `2d614cf...`, faits service RAVE.
- 1670 : `2e63d70...`, RCL0194 Mini MPi wiring.
- Format moderne : SQL UTF-8 → `qCompress` compatible → Base64 texte ; ordre du manifeste.
- Base : `schema_version=1`, `database_revision=20`.
- Avant 1680 : 46 faits RAVE, 58 faits experts ; aucun `RAVE-WIR-SPIJ-*`.

Source 1680 : **RCL0194ENG, Mini Electrical Circuit Diagrams, 3rd Edition**, SPi Japon 97MY à partir du VIN `SAXXNNAXKBD 134455`, pages 20.3, 20.4, 39.3.

Lot 1680 : 14 faits constructeur miroirés dans `mems_rave_fact` + `mems_expert_fact_external` : périmètre/VIN, module relais, O2, pompe, injecteur, purge, IAC 4 phases, ECT, TPS, IAT, masse capteurs, diagnostic, chauffage collecteur, ventilateur SPi Japon par thermocontact/relais.

Validation locale sur copie exacte r20 #86 : 14+14 entrées, tous `verifie_constructeur`, total RAVE 60, total expert 72, SQL 17 194 octets, SHA-256 `dec464ad3d5446a78886cfc995a1dc18553aab4f86ac2676803a56ac62d916dd`, r20 inchangée.

Push atomique : **`d156469af53b4ba20e084439e2d62b691c9e199b`**, exactement 3 fichiers : lot 1680, manifeste, audit provenance. Aucun fichier IA/protocole/UI/32 bits modifié.

## CONNAISSANCES RAVE À PRÉSERVER

Ralenti SPi 1993–96 850 ±25 ; SPi 1997+ et MPi 900 ±50 ; pression SPi ~1 bar, MPi 3,0 ±0,2 bar ; fan MPi97 105/98 °C ; SPi Japon 98/93 °C ; pas de réglage ralenti par butée ; conflits résistance bobine conservés ; MEMS1.3 SPi / MEMS1.6 SPi / MEMS2J MPi distingués ; DTC fortement supportés 1 ECT, 2 IAT, 10 pompe, 16 TPS, 20 chauffage lambda, 21 synchro, 22 fan1, 24 fan2 ; code23 preuve insuffisante ; brochages MPi97 (1670) et SPi Japon 97MY (1680) strictement séparés.

## SÉCURITÉ / UI / IA — INCHANGÉS

Ne pas modifier les protections protocole BUILD #30, MEMS1.9 F7/EF, tailles 7D/80, W4, reconnexion1.9, failsafes, ports, RAM non validée, reset/clear/writes. Conserver `onProtocolCommandRequested(quint8)` et `raw-32768-correction`. UI dark/responsive inchangée. ONNX natif inchangé ; aucun QProcess/llama/HTTP ; base r20 read-only ; aucune mutation ECU accessible au LLM.

## PROCHAINE ACTION EXACTE

**Pendant #87, diagnostiquer sans pousser le défaut réel #86 : routage de `C4EST QUOI LA BOBINE?` et sanitation des sorties `<think>`/consignes internes. Dès que #87 est VERT, enregistrer son résultat RAVE puis pousser une correction qualité minimale et testée sur le HEAD incluant 1680. Aucun BUILD #31.**