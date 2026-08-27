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
- GitHub Actions qualité courant : **#88**, run **`33042796844`**, commit `5e707530...`, état : **in_progress**.
- Dernier run entièrement validé : **#87 — SUCCESS**, commit `d156469...`.
- Aucun BUILD #31 sans demande explicite. 32 bits et rollback BUILD26 : NE PAS TOUCHER.

## TEST UTILISATEUR #86 — DÉFAUTS À CORRIGER

#86 fonctionne, IA/base prêtes et plus rapide, mais les captures montrent : `C4EST QUOI LA BOBINE?`, couleur de fil température SPi, `BROCHE ECU 1.3`, `BROCHE OBD 1.9`, `CADRAN TPM?`, liste des cadrans dans `L4ONGLET APERCU`, et anciennes fuites `<think>`/consignes internes.

## LOT QUALITÉ `5e707530...`

Diff validé avant push : exactement 3 fichiers IA, aucun protocole/UI/RAVE/packaging/32 bits/BUILD.

Corrections :
- normalisation étroite `C4EST`→`C EST`, `L4ONGLET`→`L ONGLET` ;
- intentions `General/WireColor/Pinout` ;
- couleur de fil exige une vraie information couleur/fil, sinon réponse d'absence sans invention ;
- brochage exige un fait câblage/connecteur/broche ;
- `ECU 1.3` / `OBD 1.9` fixent le contexte famille ;
- définition générique MEMS 1.x ne détourne plus les demandes de broche ;
- `TPM`→`RPM` seulement en contexte cadran/régime ;
- liste Aperçu depuis le code réel : 11 cadrans + état système ;
- self-test exact avec les formulations utilisateur.

## RUN #88 — RÉSULTATS INTERMÉDIAIRES VALIDÉS

Au dernier contrôle :
- runtime ONNX officiel : SUCCESS ;
- protections protocole : SUCCESS ;
- compilation complète : **SUCCESS** ;
- tests déterministes BUILD30 : **SUCCESS** ;
- génération/validation base experte r20 avec RAVE 1680 : **SUCCESS** ;
- modèle Qwen épinglé + hashes : **SUCCESS** ;
- vrai `LocalAiClient` natif ONNX : **SUCCESS** en 7 s, incluant `C4EST QUOI LA BOBINE ?`, `BROCHE ECU 1.3`, `BROCHE OBD 1.9` et Qwen réel ;
- assemblage package : **SUCCESS** ;
- validation package : **SUCCESS** ;
- vrai `LocalAiClient` depuis le package : **SUCCESS** en 8 s ;
- smoke launch application : en cours au dernier contrôle.

## RAVE / BASE — LOT 1680 VALIDÉ

1660 service RAVE ; 1670 RCL0194 MPi wiring ; 1680 RCL0194ENG SPi Japon 97MY depuis VIN `SAXXNNAXKBD 134455`. 1680 ajoute 14 faits constructeur + 14 miroirs experts ; total RAVE 60, expert 72 ; tous `verifie_constructeur`. Aucun code couleur de fil inventé.

## CONNAISSANCES / SÉCURITÉ À PRÉSERVER

Ralenti SPi 1993–96 850 ±25 ; SPi 1997+ et MPi 900 ±50 ; pression SPi ~1 bar, MPi 3,0 ±0,2 bar ; fan MPi97 105/98 °C ; SPi Japon 98/93 °C ; MEMS1.3 SPi / MEMS1.6 SPi / MEMS2J MPi distingués ; DTC supportés 1 ECT, 2 IAT, 10 pompe, 16 TPS, 20 chauffage lambda, 21 synchro, 22 fan1, 24 fan2 ; code23 preuve insuffisante.

Ne pas modifier protections protocole BUILD #30, MEMS1.9 F7/EF, 7D/80, W4, reconnexion1.9, RAM non validée, reset/clear/writes, `onProtocolCommandRequested(quint8)`, formule `raw-32768-correction`. UI dark/responsive inchangée. ONNX natif inchangé, aucun QProcess/llama/HTTP, base r20 read-only, aucune mutation ECU par LLM.

## PROCHAINE ACTION EXACTE

**Terminer le suivi #88 : smoke launch, manifeste, upload, récupérer métadonnées artefact et enregistrer le verdict complet. Ensuite reprendre RAVE sur un lot séparé. Aucun BUILD #31.**