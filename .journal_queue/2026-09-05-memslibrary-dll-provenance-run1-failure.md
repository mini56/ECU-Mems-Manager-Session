## 2026-09-05 — MEMSLibrary.dll provenance — RUN 1 ROUGE AVANT COMPILATION

### RUN
- branche : `tmp-memslibrary-dll-provenance-fix`
- HEAD : `c8819442226ede3ab5610ffa32df39b071537356`
- workflow : `TMP MEMSLibrary DLL Provenance Fix`
- run : `33953308189`
- job : `101271976565`
- verdict : **FAILURE**

### POINT EXACT DE L ECHEC
Le workflow s est arrete a l etape `Verify exact Pack001 ancestry and change scope`, avant telechargement du corpus, avant reconstruction du Pack001 et avant compilation/test de `MEMSLibrary.dll`. Aucun verdict technique ne peut donc encore etre donne sur la correction DLL.

### CAUSE RACINE
Le garde PowerShell utilisait directement l expression Git `$baseline..HEAD` dans les commandes `git diff`. PowerShell n a pas transmis la plage Git `9cf3d...HEAD` comme un argument valide : Git a affiche son aide `usage: git diff ...`, puis la liste `$changed` n a pas contenu les trois fichiers DLL attendus. Le workflow a ensuite leve :
`Expected DLL correction file missing from diff: memslibrary/include/MEMSLibrary.h`.

Ce defaut concerne uniquement la syntaxe du workflow temporaire de controle de perimetre ; il ne prouve ni un echec de compilation ni un echec de la logique DLL.

### CORRECTION AUTORISEE SUIVANTE
Corriger uniquement ce garde temporaire en construisant explicitement la plage Git, par exemple `$range = "$($baseline)..HEAD"`, puis utiliser `$range` dans les deux `git diff`. Ne modifier ni le header, ni `MEMSLibrary.cpp`, ni les tests pendant cette correction de workflow. Relancer ensuite le run complet.

### PROCHAINE ACTION EXACTE
Apres preuve que cette entree est ajoutee au rapport maitre, corriger uniquement `.github/workflows/tmp-memslibrary-dll-provenance-fix.yml` sur la branche temporaire, puis observer le nouveau run. Aucun changement `MEMSX64` / BUILD #104.
