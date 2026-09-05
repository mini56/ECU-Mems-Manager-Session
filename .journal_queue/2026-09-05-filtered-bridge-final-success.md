## 2026-09-05 — BRIDGE IA / MEMSLibrary FILTRÉ — VALIDATION FINALE VERTE

Branche temporaire : `tmp-ai-memslibrary-filtered-bridge`
Base exacte protégée : BUILD #104 `a55427affeec84643f916621df6247adf29e80fb`
HEAD final testé : `981b110f5506c9938ffcb7f7720920b898123c16`

Run GitHub Actions : **`33955854456`**
Job : **`101278959443`**
Conclusion : **SUCCESS**.

Preuve terminale réelle du self-test :
`IA_MEMSLIBRARY_BRIDGE_FILTERED_PASS primary=DOC_RCL0193ENG:p53 revision=REV_RCL0193ENG_SOURCE language=en primary_sources=6 page_pure=1 visual=VIS_P0053_001 battery=DOC_RCL0221ENG:p20 throttle=DOC_RCL0195ENG:p35 axial_coaxial_rejected=1 p342=0`

Autres preuves du run :
- `BRIDGE_SCOPE_PASS changed=6` ;
- `PINNED_RUNTIME_INPUTS_PASS` ;
- DLL corrigée SHA256 `77d73dd7c64fa2271fa26bb5d724b1c3b0c343c0669af3af5647236546437b7a` ;
- Pack001 SHA256 inchangé `827208b703324335828d72093994ec5f7bf9e0a6fd0cd3b0082d033bfa38628b` ;
- `P53_VISUAL_PROVENANCE_PASS assets=VIS_P0053_001,VIS_P0053_002` ;
- `PROTECTED_PRODUCTION_FILES_UNCHANGED_PASS`.

Question réelle validée :
`Quel est le jeu axial du pignon primaire et comment le contrôler ?`

Résultat validé :
- document `DOC_RCL0193ENG` ;
- révision `REV_RCL0193ENG_SOURCE` ;
- langue source `en` ;
- page physique **53** ;
- 6 extraits et **tous strictement p53** (`page_pure=1`) ;
- preuve contenant opération `12.21.28`, valeur `0.089–0.165 mm` et contrôle aux `feeler gauges` ;
- visuel réellement suivi depuis la preuve : `VIS_P0053_001` ;
- faux positif `axial` -> `coaxial` rejeté ;
- page 342 exclue (`p342=0`).

Régressions générales incluses :
- batterie : `DOC_RCL0221ENG:p20` ;
- throttle potentiometer : `DOC_RCL0195ENG:p35`.

La correction générale comprend :
- résolution dynamique additive de `MEMSLibrary_SearchPackFiltered(...)` ;
- conservation de l'ancien export ABI2 en fallback ;
- validation en tokens exacts afin de rejeter les faux positifs de sous-chaîne ;
- ranking où la précision du meilleur résultat prime sur le volume de résultats génériques ;
- regroupement et revalidation par document + révision + langue + page ;
- contexte final envoyé à Qwen limité strictement à la page sélectionnée ;
- aucune valeur RCL0193/p53 hardcodée dans le bridge.

Compare BUILD #104 -> HEAD final :
- statut `ahead` ;
- `ahead_by=10` ;
- `behind_by=0` ;
- merge-base = BUILD #104 exact ;
- exactement 6 fichiers autorisés modifiés/ajoutés :
  1. `.github/workflows/tmp-ai-memslibrary-filtered-bridge.yml`
  2. `expert/IaMemsLibraryBridge.cpp`
  3. `expert/IaMemsLibraryBridge.h`
  4. `expert/IaMemsLibraryBridgeSelfTest.cpp`
  5. `expert/bridge-selftest/CMakeLists.txt`
  6. `memslibrary/include/MEMSLibrary.h`

Fichiers production protégés explicitement inchangés :
- `CMakeLists.txt`
- `iamemstab.cpp`
- `expert/IaMemsDiagramCatalog.cpp`
- `expert/IaMemsDiagramCatalog.h`
- `expert/IaMemsLibraryIntegration.cpp`

### ÉTAT À CE CHECKPOINT
La correction bridge + DLL + preuve documentaire + suivi visuel est **VERTE SUR BRANCHE TEMPORAIRE**.
La production BUILD #104 reste **inchangée** et ne bénéficie pas encore de cette correction.
**Aucun BUILD #105 n'a été lancé.**

### PROCHAINE ACTION EXACTE
STOP à ce checkpoint. Attendre l'autorisation explicite de l'utilisateur avant toute promotion/intégration de la correction validée vers `MEMSX64` et avant tout lancement d'un BUILD #105. Lors de l'autorisation : journaliser d'abord le plan de promotion dans ce rapport n°2, puis intégrer uniquement les fichiers applicatifs nécessaires et la nouvelle DLL validée, lancer le préflight/build prévu, tester, puis journaliser immédiatement le résultat.