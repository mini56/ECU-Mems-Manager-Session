## 2026-10-20 — SUCCESS : ECU MEMS MANAGER LIGHT BUILD #122 — INSTALLATEUR RÉEL
- Run GitHub Actions `39461449398`: **completed / SUCCESS** sur branche unique de test `tmp-ia-clarification-test-1b106ee`; branche `MEMSX64` et `main` non modifiées.
- **Windows Setup publié et disponible** `ECU-MEMS-Manager-Setup-v1.0.122-LIGHT.exe` ; taille 446638123 octets ; SHA256 `9eb6b17a9eb9a2aacb818570dd08fd882a35feef305d4cb7b1e3a9c85f957f71`.
- Release privée : `https://github.com/mini56/ECU-Mems-Manager-Session/releases/tag/mems-build122-candidate-39461449398`
- EXE direct : `https://github.com/mini56/ECU-Mems-Manager-Session/releases/download/mems-build122-candidate-39461449398/ECU-MEMS-Manager-Setup-v1.0.122-LIGHT.exe`
- Nom Setup et version affichée dans Inno : `ECU MEMS Manager LIGHT`, dossier `C:\Program Files\ECU MEMS Manager LIGHT`, propre raccourci `C:\Users\Public\Desktop\ECU MEMS Manager LIGHT.lnk`. **Installation silencieuse réelle Windows et résolution COM du raccourci validées**, cible `C:\Program Files\ECU MEMS Manager LIGHT\ecu_mems_manager.exe`.
- VRAI menu Windows testé après construction asynchrone : `LIGHT_REAL_WINDOWS_GUI_11_MENU_ITEMS_PASS`, `LIGHT_QT_WIDGETS_11_VISIBLE_3_HIDDEN_14_PAGES_PASS` : 11 liens visibles et exactement 3 liens masqués (Test ECU 1.9, IA MEMS, ECU / ROSCO), les 14 pages et le code conservés. Le mode LIGHT ne supprime pas les pages.
- Tests `WINDOWS_SQLITE_VIRGIN_INSTALL_PASS`, `WINDOWS_SQLITE_EXISTING_INSTALL_PRESERVED_PASS` passés. Modèle Qwen ONNX et bibliothèques RAVE protégés.
- Build121 rouge uniquement à cause du garde PowerShell de vérification du raccourci : accès `$shortcuts[0]` sur chaîne scalaire = premier caractère ; BUILD122 corrige et vérifie le fichier `.lnk` complet.
- Le setup est prêt pour **essais de l'utilisateur sur son PC réel** ; ne pas prétendre avoir testé le PC personnel ou ECU physique.
