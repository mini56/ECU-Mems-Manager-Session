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
- HEAD x64 courant : **`5e707530352f5e4da3a04e832f62acfa7054ef2f`**.
- BUILD logiciel actif : **#30 / v1.0.30**.
- GitHub Actions qualité courant : **#88**, run **`33042796844`**, commit `5e707530...`, état au dernier contrôle : **in_progress**.
- Dernier run entièrement validé : **#87 — SUCCESS**, run `33041407944`, commit `d156469...`.
- Artefact #87 : ID `9634135544`, taille `386 753 794`, SHA-256 `ad01bbef0f82cdb59e8d2ed2cb99f606534f140a75db82a04f18aef38c0060a7`.
- Aucun BUILD #31 sans demande explicite.
- 32 bits `lab-expert-engine` et rollback `MEMSX64-BUILD26-BASE` : **NE PAS TOUCHER**.

## TEST UTILISATEUR RÉEL #86

#86 fonctionne réellement, IA/base prêtes, réponses plus rapides, mais captures mal ciblées : `C4EST QUOI LA BOBINE?`, couleur de fil température SPi, `BROCHE ECU 1.3`, `BROCHE OBD 1.9`, `CADRAN TPM?`, liste des cadrans dans `L4ONGLET APERCU`. Des sorties `<think>` / consignes internes avaient aussi été observées.

## LOT QUALITÉ POUSSÉ SUR MEMSX64

Le lot a été préparé sur `tmp-ia-targeting-86`, comparé au HEAD #87, puis `MEMSX64` a été avancée une seule fois en fast-forward sur `5e707530...`.

Diff : exactement 3 fichiers IA, aucun protocole/UI/RAVE/packaging/32 bits/BUILD :
- `expert/IaMemsService.cpp` : +153/-12 ;
- `expert/LocalAiClient.cpp` : +41/-9 ;
- `expert/LocalAiOnnxSelfTest.cpp` : +28/-4.

Corrections :
- `C4EST`→`C EST`, `L4ONGLET`→`L ONGLET` de façon étroite ;
- intentions `General/WireColor/Pinout` avant recherche experte ;
- couleur de fil : seuls des faits contenant réellement une information de fil/couleur peuvent répondre ; sinon absence vérifiée annoncée sans invention ;
- brochage : seuls faits câblage/connecteur/broche retenus ;
- `ECU 1.3` / `OBD 1.9` fixent le contexte famille ;
- définition générique MEMS 1.x ne détourne plus une demande de broche ;
- `TPM`→`RPM` uniquement en contexte cadran/régime ;
- `quel cadran dans l'onglet Aperçu` répond depuis le code réel : 11 cadrans + indicateur état système ;
- self-test exact : `C4EST QUOI LA BOBINE ?`, date fautive, `BROCHE ECU 1.3`, `BROCHE OBD 1.9`, Qwen réel, anti-fuite.

## RAVE / BASE — LOT 1680 VALIDÉ

- 1660 : faits service RAVE ; 1670 : RCL0194 Mini MPi wiring ; 1680 : RCL0194ENG SPi Japon 97MY depuis VIN `SAXXNNAXKBD 134455`, pages 20.3/20.4/39.3.
- 1680 : 14 faits constructeur + 14 miroirs experts ; total RAVE 60, expert 72 ; tous `verifie_constructeur`.
- Brochages vérifiés : O2, pompe, injecteur, purge, IAC, ECT, TPS, IAT, masse capteurs, prise diagnostic, etc.
- Aucune couleur de fil inventée.

## CONNAISSANCES / SÉCURITÉ À PRÉSERVER

Ralenti SPi 1993–96 850 ±25 ; SPi 1997+ et MPi 900 ±50 ; pression SPi ~1 bar, MPi 3,0 ±0,2 bar ; fan MPi97 105/98 °C ; SPi Japon 98/93 °C ; MEMS1.3 SPi / MEMS1.6 SPi / MEMS2J MPi distingués ; DTC fortement supportés 1 ECT, 2 IAT, 10 pompe, 16 TPS, 20 chauffage lambda, 21 synchro, 22 fan1, 24 fan2 ; code23 preuve insuffisante.

Ne pas modifier protections protocole BUILD #30, MEMS1.9 F7/EF, 7D/80, W4, reconnexion1.9, RAM non validée, reset/clear/writes, `onProtocolCommandRequested(quint8)`, formule `raw-32768-correction`. UI dark/responsive inchangée. ONNX natif inchangé, aucun QProcess/llama/HTTP, base r20 read-only, aucune mutation ECU par LLM.

## PROCHAINE ACTION EXACTE

**Suivre le run #88 (`33042796844`) jusqu'au verdict complet : compilation, tests exacts, base r20, Qwen, package, lancement, artefact. Enregistrer chaque résultat significatif. Puis reprendre RAVE sur un lot séparé. Aucun BUILD #31.**