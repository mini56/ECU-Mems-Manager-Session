## 2026-09-05 — RÉSULTAT TEST — BRIDGE IA MEMSLibrary FILTRÉ : VERT

Autorisation de reprise : utilisateur `OK ON CONTINUE`, puis confirmation `OK TU AVANCE`.

Branche temporaire : `tmp-ai-memslibrary-filtered-bridge`
Base stricte : BUILD #104 `a55427affeec84643f916621df6247adf29e80fb`
HEAD testé : `c62386a540a7bd89007fb8f5b0835603ca634591`
Workflow temporaire : `TMP IA MEMSLibrary Filtered Bridge`
Run : **33955239592**
Job : **101277276899**
Conclusion : **SUCCESS**

Périmètre vérifié par le workflow : 6 fichiers seulement par rapport à #104 :
- `.github/workflows/tmp-ai-memslibrary-filtered-bridge.yml`
- `expert/IaMemsLibraryBridge.cpp`
- `expert/IaMemsLibraryBridge.h`
- `expert/IaMemsLibraryBridgeSelfTest.cpp`
- `expert/bridge-selftest/CMakeLists.txt`
- `memslibrary/include/MEMSLibrary.h`

Fichiers de production explicitement prouvés inchangés :
- `CMakeLists.txt`
- `iamemstab.cpp`
- `expert/IaMemsDiagramCatalog.cpp`
- `expert/IaMemsDiagramCatalog.h`
- `expert/IaMemsLibraryIntegration.cpp`

Entrées runtime épinglées et vérifiées :
- DLL corrigée artifact `9965560001`, digest `sha256:f6987c821d312a505019e02cfdd3b254bc04e1a3b86657fc1f3f87dd3e99c491`
- DLL SHA256 `77d73dd7c64fa2271fa26bb5d724b1c3b0c343c0669af3af5647236546437b7a`
- Pack001 SQLite SHA256 `827208b703324335828d72093994ec5f7bf9e0a6fd0cd3b0082d033bfa38628b`
- catalogue visuel V2 propre artifact `9954393075`, digest `sha256:b056ada5b925648824274f6b04cfa95b81fb6ba8bd07c101f3210757f5ef4df8`

Cause racine reproduite et corrigée côté bridge :
- l'ancien moteur SQL fait des `LIKE` sous-chaîne ; le terme français `axial` pouvait donc correspondre à `coaxial` dans les opérations antenne/coax p342-p343 ;
- #104 arrêtait ensuite la collecte dès qu'il avait assez de premiers résultats, ce qui pouvait injecter cette mauvaise preuve dans Qwen et dans le routage visuel ;
- le bridge temporaire utilise maintenant l'export additif `MEMSLibrary_SearchPackFiltered(...)` quand disponible, conserve la provenance structurée, vérifie les termes par mots complets avant acceptation et classe les candidats au lieu d'accepter le premier faux positif ;
- l'ancien export ABI2 reste un fallback compatible si le nouvel export n'est pas disponible.

Question réelle testée :
`Quel est le jeu axial du pignon primaire et comment le contrôler ?`

Sortie binaire exacte :
`IA_MEMSLIBRARY_BRIDGE_FILTERED_PASS primary=DOC_RCL0193ENG:p53 revision=REV_RCL0193ENG_SOURCE language=en visual=VIS_P0053_001 battery=DOC_RCL0221ENG:p20 throttle=DOC_RCL0195ENG:p35 axial_coaxial_rejected=1 p342=0`

Preuves obligatoires validées :
- document : `DOC_RCL0193ENG`
- page : `53`
- révision : `REV_RCL0193ENG_SOURCE`
- langue source : `en`
- opération / contenu : `12.21.28`, `ENGINE CRANKSHAFT PRIMARY GEAR END-FLOAT`
- contrôle : `Using feeler gauges...`
- valeur : `0.089 to 0.165 mm`
- p342/p343 antenne/coax : **absentes**
- garde sous-chaîne `axial -> coaxial` : **rejetée**

Visuel :
- le catalogue V2 propre contient bien `VIS_P0053_001` et `VIS_P0053_002` pour `RCL0193ENG` p53 ;
- le routage visuel existant, sans modification, a suivi la preuve corrigée et a sélectionné `VIS_P0053_001` ;
- aucune modification de la logique visuelle/UI n'a été nécessaire.

Régressions documentaires distinctes validées :
- batterie : `DOC_RCL0221ENG:p20`
- throttle potentiometer : `DOC_RCL0195ENG:p35`

Statut : **bridge + DLL + Pack001 + rattachement visuel de la preuve = VERT en test isolé réel Windows x64**.

Important :
- `MEMSX64` n'a pas été modifiée ;
- BUILD #104 reste la production ;
- aucun BUILD #105 n'a été lancé ;
- ce run n'est pas encore une validation du package applicatif complet.

PROCHAINE ACTION EXACTE : faire un préflight applicatif temporaire complet depuis cette branche, en compilant le vrai exécutable et en stageant la DLL corrigée + Pack001 + référence V2 propre, puis exécuter les self-tests existants et le nouveau test bridge dans le même runtime. Ne pas pousser sur `MEMSX64` et ne pas lancer BUILD #105 avant que ce préflight applicatif complet soit VERT et journalisé.
