## 2026-09-04 — MEMSLibrary Pack 001 — validation finale Windows x64 VERTE

- Branche : `tmp-memslibrary-pack001`.
- Commit final testé : `9cf3d00840cec1749ef9358f708d3e68a2c8bc4d` (`Fix Pack001 smoke test document keys`).
- Workflow : `TMP MEMSLibrary Pack001`.
- Run : `33847448261`.
- Job : `100942383982`.
- Conclusion : **SUCCESS / VERT**.

### Pack 001 validé

- Source : artefact RAVEMEMS V2 strict 47/47 du run `33810202288`.
- 47 bases source retrouvées et intégrées.
- 47 documents.
- 1 359 pages.
- 1 667 visuels.
- 5 453 entrées de recherche.
- 0 review ouverte.
- `integrity_check=ok`.
- 0 FK cassée.
- Construction répétée deux fois sous Windows : identique SHA256 `827208b703324335828d72093994ec5f7bf9e0a6fd0cd3b0082d033bfa38628b`.
- SHA annoncé par `manifest.json` = SHA réel du `knowledge.sqlite`.

### MEMSLibrary.dll ABI 2 validée

- Configuration CMake x64 : PASS.
- Compilation MSVC Windows x64 : PASS.
- `MEMSLibrary.dll` SHA256 : `0be5967e4210dceb23a81e02936f84d58988a34aefac21a488ebb3ef2d1fd63f`.
- Chargement dynamique et exports ABI 2 : PASS.
- Validation du Pack 001 en lecture seule : PASS.
- Recherche réelle `primary gear end float` : PASS, `DOC_RCL0193ENG` page 53 avec la donnée 0.089 à 0.165 mm.
- Recherche réelle `battery restoration procedure` : PASS, `DOC_RCL0221ENG` page 20.
- Pack 002 volontairement corrompu : rejeté.
- Pack 001 revalidé après le test du Pack 002 corrompu : PASS ; isolation démontrée.
- Marqueur : `MEMSLIBRARY_PACK001_PASS abi=2 pack=MEMSLibrary_Pack_001 documents=47 primary=DOC_RCL0193ENG:p53 battery=DOC_RCL0221ENG:p20 corrupt_pack_isolated=1`.
- Marqueur final workflow : `MEMSLIBRARY_PACK001_WINDOWS_X64_PASS`.

### Artefact final

- Nom : `MEMSLibrary-Pack001-33847448261`.
- Artifact ID : `9927059742`.
- Taille ZIP : 1 041 083 octets.
- SHA256 ZIP : `ffa9c42783fe443ffc3949cf2166083291adb08505b3fca228b03003a904698a`.
- Contenu : `MEMSLibrary.dll` + `MEMSLibrary_Pack_001/knowledge.sqlite` + `MEMSLibrary_Pack_001/manifest.json`.

### Production protégée

`MEMSX64` vérifié après le run : toujours BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`. Aucun BUILD #104 et aucune modification production.
