## 2026-10-20 — BUILD #118 : MASQUER TROIS ENTREES DU MENU — SUCCESS

Demande utilisateur explicite : retirer UNIQUEMENT les entrées `TEST ECU 1.9`, `IA MEMS`, `ECU ROSCO` du menu; préserver les pages, leurs fonctions et l'ensemble des autres entrées.

- Branche temporaire existante : `tmp-ia-clarification-test-1b106ee`.
- Commit correction code `4d50dcdf3953dd4d8bb54ac376075304153e13c4`, fichier `navigationorderpatch.cpp`. 14 pages QTabWidget et leurs indices inchangés, seule visibilité QListWidgetItem masquée sur rangs 6/9/13. 11 entrées visibles; mise en page sidebar ajustée. Page/IA/protocoles/DB non modifiés.
- Workflow #118 temporaire créé par commit `0e49e7488acb2eb281a6bec7967d8f664332c367`, compilation source directe, aucun patch injecté.
- **Run GitHub Actions 39412658072 SUCCESS** ; Windows x64 compile et tests. Marqueurs passés `SIDEBAR_HIDES_ONLY_IA_MEMS_ECU_ROSCO_TEST_ECU_19_PASS`, `DIRECT_SOURCE_NO_PATCH_PASS`, `WINDOWS_SQLITE_VIRGIN_INSTALL_PASS`, `WINDOWS_SQLITE_EXISTING_INSTALL_PRESERVED_PASS`, `CLEAN_BUILD_PACKAGE_PASS visuals=1667 legacy=0 docs=47 pages=1359 search=5453 corrected_dll=1`. Test de démarrage GUI réussi, installateur Inno Setup généré/publié.
- Installer **`ECU-MEMS-Manager-Setup-v1.0.118-TEST.exe`** 446776615 octets. SHA256 `e27cc3353bab4aac10cf748faa8ed093a6ea8095d93ebb66000c5ef86db9a401`. Disponible à `https://github.com/mini56/ECU-Mems-Manager-Session/releases/download/mems-build118-candidate-39412658072/ECU-MEMS-Manager-Setup-v1.0.118-TEST.exe`.
- Release privée `https://github.com/mini56/ECU-Mems-Manager-Session/releases/tag/mems-build118-candidate-39412658072` ; archive portable aussi disponible. Runtime Qwen ONNX / corpus RAVEMEMS / MEMSLibrary et correction SQLite #117 préservés.
- Branches `MEMSX64` et `main` intactes. Installeur distinct du #117; version prête aux tests utilisateur (aucune affirmation de validation utilisateur réelle).
