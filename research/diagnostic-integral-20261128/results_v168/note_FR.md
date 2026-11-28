# V1.68 — Qualification des champs de trame 68K

{
  "firmwares": 449,
  "68k_firmwares": 104,
  "entries": 30839,
  "reclassified": 2572,
  "new_chunk_sites": 2,
  "firmwares_with_chunks": 1,
  "statuses": {
    "STATIC_LINKED": 118,
    "SELECTOR_CANDIDATE": 295,
    "STATIC_TABLE_CONTEXT_LINKED": 19239,
    "STATIC_TABLE_LINKED": 5075,
    "INTERNAL_STATE_TABLE_LINKED": 3540,
    "FRAME_HEADER_COMPARISON": 104,
    "OTHER_FRAME_FIELD_LINKED": 104,
    "PRIMARY_SERVICE_FIELD_LINKED": 1511,
    "SUBFUNCTION_FIELD_LINKED": 853
  },
  "physical": "NOT_TESTED",
  "complete": false
}

Blocs annexes inclus ; indices prouvés par propagation de constantes et branches, pas une validation des fonctions ni de la trame série complète. Les 295 sélecteurs 80196 restent à traiter. Aucun diesel, aucune modification Manager/DLL.