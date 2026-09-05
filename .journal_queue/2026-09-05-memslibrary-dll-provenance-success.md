## 2026-09-05 — MEMSLibrary.dll — CORRECTION FILTRAGE / PROVENANCE VALIDEE

### BRANCHE / BASE / RUN
- branche temporaire : `tmp-memslibrary-dll-provenance-fix`
- base exacte : `9cf3d00840cec1749ef9358f708d3e68a2c8bc4d`
- HEAD valide : `a768d0784da451367e9ee08efa11f4621e13656b`
- workflow : `TMP MEMSLibrary DLL Provenance Fix`
- run : `33953367833`
- job : `101272147404`
- verdict : **SUCCESS**

### CORRECTION REALISEE
L ABI2 historique reste intacte et `MEMSLibrary_GetAbiVersion()` reste a `2`.
Un nouvel export additif et retrocompatible a ete ajoute :
`MEMSLibrary_SearchPackFiltered(...)`.

Il permet des filtres exacts optionnels sur les dimensions deja presentes dans Pack001 :
- `document_key` ;
- `revision_key` ;
- `source_language` ;
- `entity_kind`.

Le resultat etendu restitue egalement :
- `document_key` ;
- `revision_key` ;
- `source_language` ;
- page, type/cle d entite, titre et corps.

Les filtres sont appliques avec parametres SQLite lies et egalite stricte (`COLLATE BINARY`). L ancien `MEMSLibrary_SearchPack(...)` n a pas ete supprime ni remplace.

### PREUVE DE PERIMETRE
Diff entre la base Pack001 validee et le HEAD : exactement 4 fichiers :
- `.github/workflows/tmp-memslibrary-dll-provenance-fix.yml`
- `memslibrary/include/MEMSLibrary.h`
- `memslibrary/src/MEMSLibrary.cpp`
- `memslibrary/tests/MEMSLibrarySmokeTest.cpp`

`tools/build_memslibrary_pack.py` est reste inchange. Aucun fichier de l application, de `MEMSX64`, de RAVE, de RAVEMEMS V2 ou du contenu Pack001 n a ete modifie.

### PACK001 RECONSTRUIT ET INTACT
Le workflow a recharge les 47 bases RAVEMEMS V2 validees puis reconstruit Pack001 deux fois :
- documents : 47
- pages : 1359
- visuels : 1667
- entrees recherche : 5453
- reviews ouvertes : 0
- `PRAGMA integrity_check=ok`
- `foreign_key_check=[]`
- SHA256 Pack001 : `827208b703324335828d72093994ec5f7bf9e0a6fd0cd3b0082d033bfa38628b`

Ce SHA est identique a la reference Pack001 historique. Apres tous les tests DLL, le Pack est encore byte-identique avec le meme SHA.

### REGRESSION REELLE PIGNON PRIMAIRE
Sortie exacte du smoke test :
`MEMSLIBRARY_PROVENANCE_PASS abi=2 pack=MEMSLibrary_Pack_001 documents=47 historical_primary=DOC_RCL0193ENG:p53 historical_battery=DOC_RCL0221ENG:p20 filtered_primary=DOC_RCL0193ENG:p53 language=en revision=REV_RCL0193ENG_SOURCE wrong_document=0 wrong_language=0 wrong_revision=0 p342=0 corrupt_pack_isolated=1`

Donc la DLL valide maintenant explicitement :
- resultat correct : `DOC_RCL0193ENG` page 53 ;
- langue retournee : `en` ;
- revision retournee : `REV_RCL0193ENG_SOURCE` ;
- mauvais document : 0 resultat ;
- mauvaise langue : 0 resultat ;
- mauvaise revision : 0 resultat ;
- page 342 antenne/coax : 0 resultat dans la recherche contrainte ;
- Pack002 corrompu toujours rejete sans casser Pack001 ;
- anciens smoke tests ABI2 toujours verts.

### DLL / ARTEFACT
- SHA256 nouvelle `MEMSLibrary.dll` : `77d73dd7c64fa2271fa26bb5d724b1c3b0c343c0669af3af5647236546437b7a`
- artefact : `MEMSLibrary-DLL-Provenance-33953367833`
- Artifact ID : `9965560001`
- taille ZIP : `1042528` octets
- SHA256 ZIP : `f6987c821d312a505019e02cfdd3b254bc04e1a3b86657fc1f3f87dd3e99c491`

### ETAT IMPORTANT
**La DLL est corrigee et validee de maniere isolee.**
Cela ne rend pas encore le BUILD #104 ni la chaine IA complete verts : le bridge de production du BUILD #104 appelle toujours l ancien export `MEMSLibrary_SearchPack(...)` et n utilise donc pas encore les filtres/provenance nouvellement disponibles.

Aucune integration dans `IaMemsLibraryBridge.cpp`, aucun nouveau build `MEMSX64`, aucune modification production n ont ete faits dans cette etape.

### PROCHAINE ACTION EXACTE
**STOP apres correction DLL.** Attendre une autorisation explicite de l utilisateur avant toute integration du nouvel export `MEMSLibrary_SearchPackFiltered(...)` dans le bridge MEMS Manager et avant tout nouveau build/test applicatif. La chaine globale ne pourra etre declaree verte qu apres cette integration future et un test reel dans la langue utilisateur montrant la bonne procedure et la bonne preuve visuelle.
