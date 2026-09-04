## 2026-09-04 — MEMSLibrary Pack 001 — intention de correction du smoke test uniquement

Investigation du run `33847032178` terminée avant toute nouvelle pousse.

Cause exacte établie dans les données V2 validées :
- la clé interne du Workshop Manual est `DOC_RCL0193ENG`, alors que le smoke test comparait à `RCL0193ENG` ;
- la donnée attendue existe bien dans `ravemems_step`, page 53 : `Using feeler gauges, check primary gear end-float ... DATA: End-float = 0.089 to 0.165 mm.` ;
- une seconde occurrence existe page 187 ;
- la clé interne du document TestBook est de même forme : `DOC_RCL0221ENG`, pas `RCL0221ENG`.

La DLL a retourné `status=0` et 3 résultats : il ne s'agit donc pas d'un défaut d'ouverture SQLite ni du moteur de recherche. L'assertion du test utilisait des identifiants de publication au lieu des `document_key` réels du schéma V2.

### Prochaine pousse exacte

Modifier **uniquement** `memslibrary/tests/MEMSLibrarySmokeTest.cpp` pour attendre `DOC_RCL0193ENG` et `DOC_RCL0221ENG`. Ne modifier ni `MEMSLibrary.dll`, ni son ABI 2, ni le constructeur Pack 001, ni `knowledge.sqlite`, ni le workflow hors déclenchement normal, ni `MEMSX64`.

Objectif du nouveau run : aller jusqu'au test de recherche réel, au rejet du Pack 002 corrompu, à la revalidation du Pack 001, puis à l'artefact final si tout est vert.
