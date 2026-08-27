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

Défauts confirmés par captures :
1. `C4EST QUOI LA BOBINE?` peut échapper à la définition contrôlée ;
2. une sortie Qwen peut encore montrer `<think>` / consignes internes dans certains cas observés ;
3. `COULEUR DE FIL DU CAPTEUR DE TEMPERATURE SPI ?` renvoie des procédures ECT/IAT/MAP sans couleur ;
4. `BROCHE ECU 1.3` et `BROCHE OBD 1.9 ?` renvoient la définition générique de la famille au lieu du brochage ;
5. `CADRAN TPM?` n'est pas compris comme RPM ;
6. `QUEL CADRAN DANS L4ONGLET APERCU?` renvoie seulement la description générale de l'onglet ;
7. le fallback « modèle local n'a pas produit de réponse exploitable » apparaît encore lorsque des questions structurées tombent inutilement sur Qwen.

Conclusion : **#86 fonctionne et est plus rapide, mais le ciblage des réponses doit être corrigé.**

## CORRECTION QUALITÉ EN PRÉPARATION — `tmp-ia-targeting-86`

Branche temporaire créée depuis `d156469...` afin de préparer le lot complet sans déclencher plusieurs builds `MEMSX64`.

### Production préparée

- `LocalAiClient.cpp` — commit **`a221b3696ea2f7bc0331fecb0cca16773ffd0a33`** :
  - normalisation étroite `C4EST`→`C EST`, `L4ONGLET`→`L ONGLET`, sans conversion globale du chiffre 4 ;
  - détection explicite câblage/broche/OBD/ROSCO ;
  - les réponses contrôlées MAP/IAC/SPI/ECU/bobine ne court-circuitent plus une demande de câblage ;
  - la définition générique MEMS 1.x n'est déclenchée que par une demande de définition/version.
- `IaMemsService.cpp` — commit **`ea6bbc2f3d60ea0a7715fd1f906e5eab6cbccb46`** :
  - classification `General / WireColor / Pinout` avant recherche experte ;
  - une demande de couleur exige un fait contenant réellement une information de fil/couleur ;
  - une demande de brochage exige un fait de câblage/connecteur/broche ;
  - aucune couleur/broche inventée : absence de donnée vérifiée signalée explicitement ;
  - `ECU 1.3` / `OBD 1.9` peuvent fixer le contexte famille ;
  - `TPM`→`RPM` seulement dans un contexte cadran/régime ;
  - `L4ONGLET` normalisé ;
  - réponse cadrans Aperçu tirée du code réel : **11 cadrans** (RPM, liquide, MAP, papillon, batterie, correction court terme, lambda, temps injection, température air, position IAC, avance) + indicateur état système.

### Self-test exact ajouté

- `LocalAiOnnxSelfTest.cpp` — commit temporaire **`5e707530352f5e4da3a04e832f62acfa7054ef2f`**.
- Le vrai `LocalAiClient` doit maintenant réussir successivement :
  1. `C4EST QUOI LA BOBINE ?` → définition bobine déterministe exploitable ;
  2. question de date fautive ;
  3. `BROCHE ECU 1.3` + grounding brochage → le grounding gagne, aucune définition MEMS générique ;
  4. `BROCHE OBD 1.9 ?` + grounding OBD → idem ;
  5. génération Qwen réelle `OK` ;
  6. aucune fuite `<think>`/ChatML/directive interne.

### PROCHAINE ÉTAPE AVANT PUSH MEMSX64

**Comparer `d156469...` à `tmp-ia-targeting-86` et vérifier que seuls les 3 fichiers IA prévus ont changé. Si le diff est propre, déplacer `MEMSX64` une seule fois sur `5e707530...`, puis suivre le run complet.**

## BUILD QUALITÉ #86 — À PRÉSERVER

- Commit `50af0a0`, run `33004859269`, SUCCESS.
- Artefact ID `9620325660`, taille `386 747 323`, SHA-256 `1e398c4832ed2fc2b162d8eac59b5f2f43c368de2d7e041deae6606c2fd70962`.
- ONNX natif, `/no_think`, 128 tokens normal / 192 diagnostic, base r20 read-only, package complet et smoke launch validés.

## RAVE / BASE — LOT 1680 VALIDÉ

- 1660 : faits service RAVE.
- 1670 : RCL0194 Mini MPi wiring.
- 1680 : RCL0194ENG, SPi Japon 97MY depuis VIN `SAXXNNAXKBD 134455`, pages 20.3/20.4/39.3.
- 14 faits constructeur dans `mems_rave_fact` + 14 miroirs experts ; total RAVE 60, expert 72 ; tous `verifie_constructeur`.
- Brochages vérifiés 1680 : O2, pompe, injecteur, purge, IAC, ECT, TPS, IAT, masse capteurs, prise diagnostic, etc.
- **Aucune couleur de fil ne doit être inventée** : 1680 n'a conservé que les relations de circuit visuellement non ambiguës.
- Commit `d156469...`; GitHub Actions **#87 SUCCESS** et artefact publié.

## CONNAISSANCES RAVE À PRÉSERVER

Ralenti SPi 1993–96 850 ±25 ; SPi 1997+ et MPi 900 ±50 ; pression SPi ~1 bar, MPi 3,0 ±0,2 bar ; fan MPi97 105/98 °C ; SPi Japon 98/93 °C ; pas de réglage ralenti par butée ; conflits résistance bobine conservés ; MEMS1.3 SPi / MEMS1.6 SPi / MEMS2J MPi distingués ; DTC fortement supportés 1 ECT, 2 IAT, 10 pompe, 16 TPS, 20 chauffage lambda, 21 synchro, 22 fan1, 24 fan2 ; code23 preuve insuffisante ; brochages MPi97 (1670) et SPi Japon 97MY (1680) séparés.

## SÉCURITÉ / UI / IA — INCHANGÉS

Ne pas modifier les protections protocole BUILD #30, MEMS1.9 F7/EF, tailles 7D/80, W4, reconnexion1.9, failsafes, ports, RAM non validée, reset/clear/writes. Conserver `onProtocolCommandRequested(quint8)` et `raw-32768-correction`. UI dark/responsive inchangée. ONNX natif inchangé ; aucun QProcess/llama/HTTP ; base r20 read-only ; aucune mutation ECU accessible au LLM. Aucun BUILD #31 sans demande explicite.

## PROCHAINE ACTION EXACTE

**Comparer le lot temporaire aux 3 fichiers attendus. Si propre, avancer `MEMSX64` une seule fois sur `5e707530...`, suivre le prochain GitHub Actions jusqu'au package complet, enregistrer le résultat, puis reprendre RAVE sur un lot séparé.**