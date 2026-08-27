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
- GitHub Actions qualité : **#88 — SUCCESS**, run **`33042796844`**, job `98419961592`.
- Artefact #88 : `ECU-MEMS-Manager-x64-BUILD-30-v1.0.30`, ID **`9634616803`**, taille **386 761 108** octets, SHA-256 **`bf646347db27b0406483196f3057c0828d5a1a7645d4c3eaeee06dcb854c825a`**.
- Aucun BUILD #31 sans demande explicite.
- 32 bits `lab-expert-engine` et rollback `MEMSX64-BUILD26-BASE` : **NE PAS TOUCHER**.

## TEST UTILISATEUR #86 — DÉFAUTS OBSERVÉS

#86 fonctionne réellement, IA/base prêtes, réponses plus rapides, mais les captures montrent des défauts de ciblage : `C4EST QUOI LA BOBINE?`, couleur de fil température SPi, `BROCHE ECU 1.3`, `BROCHE OBD 1.9`, `CADRAN TPM?`, liste des cadrans dans `L4ONGLET APERCU`, ainsi que d'anciennes fuites `<think>`/consignes internes.

## LOT QUALITÉ #88 — `5e707530...`

Diff validé avant push : exactement 3 fichiers IA, aucun protocole/UI/RAVE/packaging/32 bits/BUILD.

Corrections incluses :
- normalisation étroite `C4EST`→`C EST`, `L4ONGLET`→`L ONGLET`, sans conversion globale du chiffre 4 ;
- intentions `General/WireColor/Pinout` avant recherche experte ;
- une question de couleur de fil ne peut plus être satisfaite par une procédure de dépose ou un fait sans information couleur/fil ;
- si aucune couleur vérifiée n'existe, l'IA le dit sans en inventer ;
- une demande de brochage exige un fait câblage/connecteur/broche ;
- `ECU 1.3` / `OBD 1.9` peuvent fixer le contexte famille ;
- la définition générique MEMS 1.x ne détourne plus une demande de broche ;
- `TPM`→`RPM` uniquement en contexte cadran/régime ;
- `quel cadran dans l'onglet Aperçu` répond depuis le code réel : **11 cadrans** (RPM, température liquide, MAP, papillon, batterie, correction court terme, lambda, temps d'injection, température air, position IAC, avance) + indicateur état système ;
- self-test exact avec les formulations utilisateur.

### Validation #88 complète

- runtime ONNX officiel : SUCCESS ;
- protections protocole : SUCCESS ;
- compilation : SUCCESS ;
- tests déterministes : SUCCESS ;
- génération/validation base r20 avec RAVE 1680 : SUCCESS ;
- modèle Qwen + hashes : SUCCESS ;
- vrai `LocalAiClient` natif ONNX : SUCCESS en 7 s avec `C4EST QUOI LA BOBINE ?`, `BROCHE ECU 1.3`, `BROCHE OBD 1.9` + Qwen réel ;
- assemblage package : SUCCESS ;
- validation package : SUCCESS ;
- vrai `LocalAiClient` depuis package : SUCCESS en 8 s ;
- smoke launch ECU MEMS Manager : SUCCESS ;
- manifeste/hashes : SUCCESS ;
- upload artefact : SUCCESS.

## RAVE / BASE — LOT 1680 VALIDÉ

- 1660 : faits service RAVE ;
- 1670 : RCL0194 Mini MPi wiring ;
- 1680 : RCL0194ENG SPi Japon 97MY depuis VIN `SAXXNNAXKBD 134455`, pages 20.3/20.4/39.3 ;
- 1680 ajoute 14 faits constructeur + 14 miroirs experts ; total RAVE 60, expert 72 ; tous `verifie_constructeur` ;
- brochages vérifiés : O2, pompe, injecteur, purge, IAC, ECT, TPS, IAT, masse capteurs, prise diagnostic, etc. ;
- **aucune couleur de fil n'est inventée** : seules les relations de circuit non ambiguës ont été conservées.

## CONNAISSANCES / SÉCURITÉ À PRÉSERVER

Ralenti SPi 1993–96 850 ±25 ; SPi 1997+ et MPi 900 ±50 ; pression SPi ~1 bar, MPi 3,0 ±0,2 bar ; fan MPi97 105/98 °C ; SPi Japon 98/93 °C ; MEMS1.3 SPi / MEMS1.6 SPi / MEMS2J MPi distingués ; DTC supportés 1 ECT, 2 IAT, 10 pompe, 16 TPS, 20 chauffage lambda, 21 synchro, 22 fan1, 24 fan2 ; code23 preuve insuffisante.

Ne pas modifier protections protocole BUILD #30, MEMS1.9 F7/EF, 7D/80, W4, reconnexion1.9, RAM non validée, reset/clear/writes, `onProtocolCommandRequested(quint8)`, formule `raw-32768-correction`. UI dark/responsive inchangée. ONNX natif inchangé, aucun QProcess/llama/HTTP, base r20 read-only, aucune mutation ECU par LLM.

## PROCHAINE ACTION EXACTE

**Reprendre RAVE sur un lot séparé du code IA. Priorité documentaire utile après les captures : rechercher des données constructeur de couleurs de fils / connecteurs réellement vérifiables, sans inventer ni généraliser entre variantes. Ne modifier la base que si une source précise permet document + variante + section + preuve. Aucun BUILD #31.**