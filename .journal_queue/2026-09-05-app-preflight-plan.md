## 2026-09-05 — PLAN AVANT POUSSE — PRÉFLIGHT APPLICATIF COMPLET DU BRIDGE FILTRÉ

Point de départ validé :
- bridge temporaire `tmp-ai-memslibrary-filtered-bridge`
- HEAD vert `c62386a540a7bd89007fb8f5b0835603ca634591`
- run bridge `33955239592` SUCCESS
- DLL corrigée SHA256 `77d73dd7c64fa2271fa26bb5d724b1c3b0c343c0669af3af5647236546437b7a`
- Pack001 SQLite SHA256 `827208b703324335828d72093994ec5f7bf9e0a6fd0cd3b0082d033bfa38628b`
- preuve réelle p53 + visuel `VIS_P0053_001` validés.

Prochaine étape autorisée par la reprise utilisateur : **préflight du vrai exécutable**, toujours hors production.

Méthode :
1. créer `tmp-ai-memslibrary-app-preflight` depuis le HEAD vert `c62386a...` ;
2. ajouter uniquement un workflow temporaire de préflight complet ;
3. compiler le vrai `ecu_mems_manager.exe` en Windows x64 avec la même configuration Qt/ONNX/librosco que la voie MEMSX64 ;
4. télécharger et épingler la base runtime IA déjà validée, la référence V2 propre, la DLL MEMSLibrary corrigée et le Pack001 inchangé ;
5. stager dans le runtime de test la DLL corrigée + Pack001 + référence V2 propre ;
6. exécuter les self-tests existants (`ia_response_selftest`, `ia_mems_diagram_selftest`, `rosco_abi_selftest`) + le nouveau `ia_memslibrary_bridge_selftest` ;
7. assembler un **artefact temporaire de préflight**, sans nom BUILD production, puis vérifier intégrité SQLite, architecture x64 et effectuer un smoke launch de l'exécutable ;
8. journaliser immédiatement le résultat.

Contraintes :
- `MEMSX64` reste sur BUILD #104 `a55427affeec84643f916621df6247adf29e80fb` ;
- aucun BUILD #105 ;
- aucun changement UI, scroll, catalogue visuel, protocole ECU, acquisition, RAM, écriture/reset ;
- aucun changement RAVE/RAVEMEMS V2/Pack001 ;
- aucun changement de `memsx64.yml` ; le préflight utilise son propre workflow temporaire ;
- pas de promotion en production tant que ce préflight applicatif complet n'est pas VERT et journalisé.

PROCHAINE ACTION EXACTE : créer la branche temporaire de préflight, ajouter le workflow dédié, lancer le vrai build applicatif x64 et vérifier tous les tests + smoke launch avec la DLL corrigée.
