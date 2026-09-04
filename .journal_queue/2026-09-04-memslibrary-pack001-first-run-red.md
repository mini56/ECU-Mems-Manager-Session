## 2026-09-04 — MEMSLibrary Pack 001 — premier run Windows rouge avant tests techniques

- Branche active : `tmp-memslibrary-pack001`.
- Commit technique testé : `34f47190b125a0e675441dfa6dcbab87ea92e212` (`Add canonical MEMSLibrary Pack001 builder and ABI2 search`).
- Workflow : `TMP MEMSLibrary Pack001`.
- Run : `33846715376`.
- Job : `100940117106`.
- Conclusion : **FAILURE**, uniquement sur le garde `Verify protected production base ancestry`.
- Cause exacte : `actions/checkout@v4` a utilisé `fetch-depth: 1`; le commit protégé `1d6316bd1746d6f2b4cfb751cab88d18e27ef730` n'était donc pas présent dans le clone local et `git merge-base --is-ancestor` a répondu `fatal: Not a valid commit name`.
- Aucun téléchargement de l'artefact 47/47, aucune construction Pack 001, aucune compilation ABI 2 et aucun test SQLite/DLL n'ont été exécutés sur ce run.
- `MEMSX64` reste strictement inchangé sur BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`; aucun #104.

### Prochaine correction exacte avant nouvelle pousse

Modifier uniquement le checkout du workflow temporaire pour utiliser `fetch-depth: 0`, afin que le garde d'ascendance puisse réellement vérifier la présence de BUILD #103 dans l'historique. Aucun changement du constructeur Pack 001, de `MEMSLibrary.dll`, de l'ABI 2, des données RAVEMEMS V2 ni de `MEMSX64`.
