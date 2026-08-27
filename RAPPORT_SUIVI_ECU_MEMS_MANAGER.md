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
- HEAD x64 courant avant push qualité : **`d156469af53b4ba20e084439e2d62b691c9e199b`**.
- BUILD logiciel actif : **#30 / v1.0.30**.
- Dernier GitHub Actions validé : **#87 — SUCCESS**, run `33041407944`.
- Artefact #87 : ID `9634135544`, taille `386 753 794`, SHA-256 `ad01bbef0f82cdb59e8d2ed2cb99f606534f140a75db82a04f18aef38c0060a7`.
- Aucun BUILD #31 sans demande explicite.
- 32 bits `lab-expert-engine` et rollback `MEMSX64-BUILD26-BASE` : **NE PAS TOUCHER**.

## TEST UTILISATEUR RÉEL #86

#86 fonctionne réellement, IA/base prêtes, et répond plus vite, mais les captures montrent des réponses mal ciblées : `C4EST QUOI LA BOBINE?`, couleur de fil température SPi, `BROCHE ECU 1.3`, `BROCHE OBD 1.9`, `CADRAN TPM?`, et liste des cadrans dans `L4ONGLET APERCU`. Des sorties `<think>` / consignes internes avaient aussi été observées. Ces défauts sont à corriger sans changer ONNX, l'UI ou le protocole.

## LOT QUALITÉ PRÉPARÉ — `tmp-ia-targeting-86`

Branche temporaire créée depuis `d156469...` pour éviter plusieurs builds intermédiaires.

- `LocalAiClient.cpp` — commit `a221b3696ea2f7bc0331fecb0cca16773ffd0a33` : correction étroite `C4EST`/`L4ONGLET`, détection câblage/broche/OBD/ROSCO, définitions contrôlées interdites de court-circuiter une intention câblage, définition générique MEMS 1.x restreinte aux vraies demandes de définition/version.
- `IaMemsService.cpp` — commit `ea6bbc2f3d60ea0a7715fd1f906e5eab6cbccb46` : classification `General/WireColor/Pinout`, filtrage strict des faits par intention, aucune couleur/broche inventée, contexte `ECU 1.3` / `OBD 1.9`, correction contextuelle `TPM`→`RPM`, `L4ONGLET`, réponse exacte depuis le code réel pour les **11 cadrans Aperçu** + état système.
- `LocalAiOnnxSelfTest.cpp` — commit `5e707530352f5e4da3a04e832f62acfa7054ef2f` : couvre `C4EST QUOI LA BOBINE ?`, date fautive, `BROCHE ECU 1.3`, `BROCHE OBD 1.9`, Qwen réel et absence de fuite interne.

### DIFF VALIDÉ AVANT PUSH

Comparaison `d156469...` → `5e707530...` : **ahead 3, behind 0, exactement 3 fichiers modifiés** :
- `expert/IaMemsService.cpp` : +153/-12 ;
- `expert/LocalAiClient.cpp` : +41/-9 ;
- `expert/LocalAiOnnxSelfTest.cpp` : +28/-4.

**Aucun fichier protocole, UI, RAVE, packaging, 32 bits ou BUILD n'est modifié.** Le lot est propre pour un unique fast-forward de `MEMSX64`.

## RAVE / BASE — LOT 1680 VALIDÉ

- 1660 : faits service RAVE ; 1670 : RCL0194 Mini MPi wiring ; 1680 : RCL0194ENG SPi Japon 97MY depuis VIN `SAXXNNAXKBD 134455`, pages 20.3/20.4/39.3.
- 1680 ajoute 14 faits constructeur + 14 miroirs experts ; total RAVE 60, expert 72 ; tous `verifie_constructeur`.
- Brochages vérifiés : O2, pompe, injecteur, purge, IAC, ECT, TPS, IAT, masse capteurs, prise diagnostic, etc.
- **Aucune couleur de fil n'est inventée** : 1680 ne conserve que les relations de circuit non ambiguës.
- Commit `d156469...`, GitHub Actions #87 SUCCESS.

## CONNAISSANCES / SÉCURITÉ À PRÉSERVER

Ralenti SPi 1993–96 850 ±25 ; SPi 1997+ et MPi 900 ±50 ; pression SPi ~1 bar, MPi 3,0 ±0,2 bar ; fan MPi97 105/98 °C ; SPi Japon 98/93 °C ; MEMS1.3 SPi / MEMS1.6 SPi / MEMS2J MPi distingués ; DTC fortement supportés 1 ECT, 2 IAT, 10 pompe, 16 TPS, 20 chauffage lambda, 21 synchro, 22 fan1, 24 fan2 ; code23 preuve insuffisante.

Ne pas modifier protections protocole BUILD #30, MEMS1.9 F7/EF, 7D/80, W4, reconnexion1.9, RAM non validée, reset/clear/writes, `onProtocolCommandRequested(quint8)`, formule `raw-32768-correction`. UI dark/responsive inchangée. ONNX natif inchangé, aucun QProcess/llama/HTTP, base r20 read-only, aucune mutation ECU par LLM.

## PROCHAINE ACTION EXACTE

**Faire un unique fast-forward de `MEMSX64` sur `5e707530352f5e4da3a04e832f62acfa7054ef2f`, suivre le GitHub Actions complet jusqu'au package, enregistrer le résultat, puis reprendre RAVE sur un lot séparé. Aucun BUILD #31.**