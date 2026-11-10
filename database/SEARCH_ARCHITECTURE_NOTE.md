# MEMS search architecture

The primary search index is built synchronously by `MemsGlobalSearchIndex`.

- SQLite technical/reference rows are indexed once by the primary builder.
- XML technical sheets are indexed as complete documents and line-by-line records by the same builder.
- Normalized individual terms are stored in `search_terms`.
- `MemsXmlRowIndexer.cpp` is UI scrolling only and must not write search data.
- `MemsSearchCompletenessPatch.cpp` is XML rendering only and must not write search data.
- Runtime/timer-based secondary writes to the search database are forbidden.

## Références visuelles hors ligne

Les références d'image provenant des résultats indexés sont des chemins relatifs
au dossier installé `database/reference`, jamais des images intégrées à SQLite.
Le rendu `MemsDatabaseContent` transforme une référence valide, par exemple
`images/rave/RCL0193FRE_PDF_346.png`, en lien vers le fichier installé.
Un lien n'est créé que si le fichier existe réellement dans ce dossier ; sinon
la référence reste visible avec un avertissement explicite. Le navigateur natif
`MemsDatabaseExplorerV2` ouvre les images en fenêtre avec zoom et n'autorise
pas l'ouverture de chemins sortant de `database/reference`.
Le test `mems_database_content_selftest` vérifie la présence de cette image
RAVE, son lien et le rejet des chemins invalides. Le workflow BUILD 136 vérifie
également sa présence dans la distribution et après installation Windows.
