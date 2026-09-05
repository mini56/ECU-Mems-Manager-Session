## 2026-09-05 — AUDIT DE RECUL — BASELINE APPLICATIF RETENU : BUILD #101

Suite au changement de stratégie demandé par l'utilisateur, comparaison factuelle des builds de production récents afin de déterminer jusqu'où revenir avant de reconstruire la fondation RAVE/RAVEMEMS/MEMSLibrary.

### BUILD #105 — REJETÉ COMME BASELINE
- commit : `1b106eed05e1fd665b857f73a719f02ee6b6b2ac`
- run : `33956856201`
- SUCCESS
- diff #104 -> #105 : 1 commit, 6 fichiers ; modification majeure de `expert/IaMemsLibraryBridge.cpp`, ajout du self-test bridge, extension du header MEMSLibrary et adaptation du workflow.
- raison du rejet : ce build contient précisément le raccordement provenance/MEMSLibrary que la nouvelle stratégie demande de reconstruire après une nouvelle extraction.

### BUILD #104 — REJETÉ COMME BASELINE
- commit : `a55427affeec84643f916621df6247adf29e80fb`
- run : `33921162926`
- SUCCESS
- diff #103 -> #104 : 4 commits ; introduction de `IaMemsLibraryBridge.*`, `IaMemsLibraryIntegration.cpp`, header `memslibrary/include/MEMSLibrary.h`, modification importante du catalogue visuel et de `iamemstab.cpp`, nouveau packaging V2.
- raison du rejet : c'est le point où l'application reçoit directement la chaîne V2/MEMSLibrary et où la logique visuelle documentaire problématique entre dans le produit.

### BUILD #103 — REJETÉ COMME BASELINE
- commit : `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`
- run : `33334306835`
- SUCCESS
- diff #102 -> #103 : données documentaires uniquement, avec nouvel audit AKM7169, lot `research_enrichment_1870.qz64`, modification du manifest et nombreuses images RAVE AKM7169.
- raison du rejet : encore postérieur à l'intégration documentaire massive et ajoute de nouvelles données/images issues de l'ancienne chaîne.

### BUILD #102 — REJETÉ COMME BASELINE
- commit : `06eca1a478db3d32e9ae88d040e1a34e2cc98650`
- run : `33326675806`
- SUCCESS
- message : `Build #102: integrate validated reference database through batch 1860`.
- diff #101 -> #102 : un commit massif ajoutant audits, manifest, qz64 et un très grand nombre d'images/captures RAVE, notamment RCL0193ENG/FRE et AKM6348.
- raison du rejet : c'est le point d'intégration de la fondation documentaire historique que l'utilisateur demande maintenant de refaire depuis les PDF sources.

### BUILD #101 — BASELINE APPLICATIF RETENU
- commit : `22dbe75ed14e0a61e694159d505ef72245116b48`
- run : `33178021830`
- conclusion : SUCCESS
- message : `Build #101: fix #100 real-test IA regressions`.
- parent #100 : `cd6e52c714ee35a3fffb405797f14a58c2da4fa5`.
- diff #100 -> #101 : seulement 4 fichiers applicatifs (`expert/IaMemsConversationRouting.h`, `expert/IaMemsService.cpp`, `expert/IaResponseLogicTest.cpp`, `iamemstab.cpp`), 37 additions / 7 suppressions ; aucune intégration massive de données RAVE.

### DÉCISION
Le **baseline applicatif de reprise est BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`**.

Important : cela ne signifie pas que les données documentaires déjà présentes dans #101 sont validées pour la nouvelle fondation. La nouvelle reconstruction RAVE ne doit réutiliser comme vérité que les **47 PDF RAVE sources canoniques** et les outils RAVEMEMS V2 corrigés. Les données dérivées historiques seront considérées comme comparaison/audit, pas comme source de reconstruction.

#105 reste figé comme dernier build installé/testé ; il n'est pas supprimé ni réécrit. Aucun #106 ne doit être lancé avant reconstruction et validation de la nouvelle fondation.

### PROCHAINE ACTION EXACTE
Auditer puis corriger **RAVEMEMS V2 avant toute nouvelle extraction** :
1. moteur d'extraction texte/page/opération ;
2. extraction et classification des visuels ;
3. exclusion structurelle des icônes, logos, pictogrammes et décorations de page ;
4. relations exactes visuel -> page -> section/opération ;
5. assemblage SQLite sans mélange de document/révision/langue ;
6. seulement après correction du moteur, relancer une extraction depuis les 47 PDF RAVE sources et auditer résultats/images avant de reconstruire SQLite puis MEMSLibrary.dll.
