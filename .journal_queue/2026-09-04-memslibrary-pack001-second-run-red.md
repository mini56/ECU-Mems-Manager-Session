## 2026-09-04 — MEMSLibrary Pack 001 — second run Windows rouge sur verrou SHA inter-environnements

- Branche : `tmp-memslibrary-pack001`.
- Commit testé : `a90dbdb4c168307907fb808bb230406e20529f55`.
- Run : `33846879090`.
- Job : `100940620537`.
- Garde d'ascendance BUILD #103 : **PASS**.
- Téléchargement de l'artefact RAVEMEMS V2 47/47 : **PASS**, 47 bases SQLite trouvées.
- Construction Pack 001 A : 47 documents, 1 359 pages, 1 667 visuels, 5 453 entrées de recherche, SHA256 `827208b703324335828d72093994ec5f7bf9e0a6fd0cd3b0082d033bfa38628b`.
- Construction Pack 001 B : mêmes compteurs et même SHA256 `827208b703324335828d72093994ec5f7bf9e0a6fd0cd3b0082d033bfa38628b`.
- Reproductibilité dans le même environnement Windows : **PASS**.
- Échec uniquement parce que le workflow exigeait le SHA local Linux `581232a03db9f5aef7dc4b1b0a5c721048bbae595db992344ff605f3dd74db33`.
- Conclusion : pas de défaut de données ni de reproductibilité ; la représentation binaire SQLite diffère entre environnements SQLite/plateformes.
- Les étapes invariants, compilation ABI 2 et test dynamique DLL n'ont pas encore été exécutées sur ce run.
- `MEMSX64` reste BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`, aucun #104.

### Prochaine correction exacte avant nouvelle pousse

Modifier uniquement le workflow pour vérifier : SHA(A) = SHA(B), et vérifier que `manifest.json` annonce le SHA réel de `knowledge.sqlite`. Supprimer l'exigence erronée d'un SHA identique entre Linux et Windows. Aucun changement du constructeur, de la structure SQLite, de `MEMSLibrary.dll`, de l'ABI 2 ni de `MEMSX64`.
