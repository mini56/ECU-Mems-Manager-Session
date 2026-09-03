## 2026-09-03 — TEST ALEATOIRE DU PIPELINE ZERO-DEFAUT SUR UN AUTRE PDF ANGLAIS

Demande utilisateur : lancer le test tel qu'il est sur un autre PDF RAVE choisi au hasard.

Tirage : `rave/Mini Tech Bulletins/R8411BU.PDF`.

Source figée :
- commit RAVE : `643de091b474f4e27917a065bdf46d5a0c764276`
- blob PDF : `4948dca6152b13a0e19f8acc25362b33547276a3`
- taille : 18 097 octets

Règle du test : ne modifier aucun algorithme validé. Réutiliser le même extracteur PASS2, le même ordre de lecture, le même rendu visuel exact, le même audit SQLite et le même replay de validation. Les compteurs attendus (pages, opérations, étapes, visuels, liens) sont dynamiques car le document n'est pas RCL0193ENG.

Production protégée : `MEMSX64` doit rester exactement sur BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`.
