## 2026-09-04 — MEMSLibrary Pack 001 — troisième run Windows rouge sur assertion de recherche

- Branche : `tmp-memslibrary-pack001`.
- Commit testé : `cc1212e11ca413c6b86ae7cf1a6857684a718d01`.
- Run : `33847032178`.
- Job : `100941090343`.
- Garde d'ascendance BUILD #103 : **PASS**.
- Téléchargement artefact RAVEMEMS V2 47/47 : **PASS**, 47 SQLite.
- Reproductibilité Pack 001 Windows : **PASS**, deux constructions identiques SHA256 `827208b703324335828d72093994ec5f7bf9e0a6fd0cd3b0082d033bfa38628b`, manifestes cohérents.
- Invariants Pack 001 : **PASS** — 47 documents, 1 359 pages, 5 453 entrées de recherche, 0 review ouverte, `integrity_check=ok`, 0 FK cassée.
- Configuration CMake x64 : **PASS**.
- Compilation `MEMSLibrary.dll` ABI 2 avec WinSQLite : **PASS**.
- Fixture Pack 002 corrompue : créée.
- Échec au smoke test de recherche : appel `MEMSLibrary_SearchPack("primary gear end float")` retourne `status=0` et `count=3`, mais l'assertion attendue `RCL0193ENG` page 53 contenant `0.089` ou `0.165` n'est pas satisfaite.
- Le test n'a donc pas encore atteint la validation d'isolation du Pack 002 corrompu, l'assemblage final ni l'upload de l'artefact.
- Aucun verdict VERT Pack 001 n'est revendiqué.
- `MEMSX64` reste strictement BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`; aucun #104.

### Investigation avant toute nouvelle pousse

Inspecter les trois entrées réelles retournées par `memslibrary_search` et les enregistrements/provenances sous-jacents pour déterminer si l'assertion de test est incorrecte ou si l'indexation/recherche générique doit être corrigée. Ne pousser aucune correction avant d'avoir établi la cause exacte et journalisé l'intention correspondante.
