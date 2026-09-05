## 2026-09-05 — BRIDGE IA / MEMSLibrary FILTRÉ — RUN 1 VERT MAIS ISOLATION DE PAGE À DURCIR

Branche temporaire : `tmp-ai-memslibrary-filtered-bridge`
Base protégée : BUILD #104 `a55427affeec84643f916621df6247adf29e80fb`
HEAD testé : `c62386a540a7bd89007fb8f5b0835603ca634591`

Run : `33955239592`
Job : `101277276899`
Conclusion GitHub Actions : **SUCCESS**.

Preuves réelles du run :
- garde d'ascendance BUILD #104 et périmètre : PASS ;
- DLL corrigée épinglée SHA256 `77d73dd7c64fa2271fa26bb5d724b1c3b0c343c0669af3af5647236546437b7a` : PASS ;
- Pack001 byte-identique SHA256 `827208b703324335828d72093994ec5f7bf9e0a6fd0cd3b0082d033bfa38628b` : PASS ;
- visuels p53 `VIS_P0053_001` et `VIS_P0053_002` présents et vérifiés : PASS ;
- question réelle `Quel est le jeu axial du pignon primaire et comment le contrôler ?` : `DOC_RCL0193ENG:p53`, révision `REV_RCL0193ENG_SOURCE`, langue `en`, p342=0 ;
- visuel réellement sélectionné : `VIS_P0053_001` ;
- batterie : `DOC_RCL0221ENG:p20` ;
- throttle potentiometer : `DOC_RCL0195ENG:p35` ;
- faux positif `axial` -> `coaxial` rejeté ;
- fichiers production protégés (`CMakeLists.txt`, `iamemstab.cpp`, catalogue visuel, intégration service) inchangés.

Constat post-run avant intégration production : le bridge sélectionne correctement p53, mais la construction finale des extraits conserve encore des candidats de la même provenance document/révision/langue provenant d'autres pages. Sur le cas primaire, le classement peut donc ajouter après les extraits p53 une ligne d'une autre page (ex. p92). Le self-test interdit déjà p342/coaxial mais ne verrouille pas encore l'absence de toute page différente de la page sélectionnée.

Ce point est traité comme défaut résiduel de sélection de preuve : **ne pas déclarer la chaîne finale verte et ne pas lancer BUILD #105**.

### CORRECTION SUIVANTE AUTORISÉE DANS LE MÊME PÉRIMÈTRE

Modifier uniquement `expert/IaMemsLibraryBridge.cpp` et le self-test associé afin que, lorsqu'une page physique valide a été sélectionnée, les extraits finaux soient strictement limités à `document + revision + source_language + selectedPage`. Aucune modification de l'UI, du catalogue visuel, du service d'intégration, du protocole ECU ou de MEMSX64.

Relancer le workflow temporaire et exiger :
- p53 uniquement pour la question primaire ;
- aucune p342/p343/coaxial/aerial ;
- aucune autre page (p92/p187/p196/etc.) dans le contexte primaire ;
- `VIS_P0053_001` ou `VIS_P0053_002` ;
- régressions batterie/throttle toujours vertes.

PROCHAINE ACTION EXACTE : durcir l'isolation à la page sélectionnée sur la branche temporaire, relancer le test, journaliser le résultat. Aucun BUILD #105.