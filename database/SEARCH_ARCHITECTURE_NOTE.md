# MEMS search architecture

The primary search index is built synchronously by `MemsGlobalSearchIndex`.

- SQLite technical/reference rows are indexed once by the primary builder.
- XML technical sheets are indexed as complete documents and line-by-line records by the same builder.
- Normalized individual terms are stored in `search_terms`.
- `MemsXmlRowIndexer.cpp` is UI scrolling only and must not write search data.
- `MemsSearchCompletenessPatch.cpp` is XML rendering only and must not write search data.
- Runtime/timer-based secondary writes to the search database are forbidden.
