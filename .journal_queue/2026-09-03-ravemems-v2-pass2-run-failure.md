## 2026-09-03 — RAVEMEMS V2 RCL0193ENG — RESULTAT RUN PASS 2 : ECHEC MECANIQUE AVANT EXTRACTION

Branche technique : `tmp-ravemems-v2-foundation` uniquement. Production `MEMSX64` toujours strictement protegee sur BUILD #103, commit `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`. Aucun BUILD #104. Source unique : `RCL0193ENG` / `rave/xn/wmxn990e.pdf`.

### Run exact
- workflow run : `33791176267` ;
- job : `100767867354` ;
- commit workflow : `bfd8d72a62c7ac6813e6fa4891e988835c6fde9f` ;
- commit wrapper teste : `5e988996c6288f7694e650193d557a0d1a65a707`.

### Etapes validees avant echec
- checkout du SHA exact : PASS ;
- garde production BUILD #103 : PASS ;
- validation Python/foundation : PASS ;
- schema self-tests : PASS ;
- source RCL0193ENG exacte, commit `643de091b474f4e27917a065bdf46d5a0c764276`, blob `82263eb36bb194dfa969d0471d10ef11078ce521` : PASS.

### Echec exact
L'extraction n'a pas demarre. Erreur :
`TypeError: Pass2SemanticParser.parse_page() takes 5 positional arguments but 6 were given`

Cause localisee : le parseur courant appelle `parse_page(physical_page, page_key, lines, page_width, page_height)` alors que le wrapper experimental PASS 2 conserve l'ancienne signature sans `page_width`.

### Artefact
Un artefact partiel d'echec a ete produit uniquement par `if: always()` :
- nom `RAVEMEMS-V2-RCL0193ENG-ZERO-DEFECT-PASS2` ;
- ID `9907375931` ;
- taille environ 5.7 Ko ;
- digest `sha256:7ee751c7a96b8385a1dcecada4340f2c60fb07ac68fdc4bf045cfa1ed56380c6`.
Cet artefact ne contient aucun resultat semantique exploitable et ne doit pas etre interprete comme une validation ou un echec du parseur semantique.

### Verdict
AUCUN NOUVEAU VERDICT SEMANTIQUE. Le dernier verdict exploitable reste PASS 1 : 22 defauts numeriques restants.

### PROCHAINE ACTION EXACTE
Corriger uniquement la signature du wrapper PASS 2 pour accepter `page_width` et `page_height`, conserver les protections et la source exactes, puis relancer uniquement RCL0193ENG. Ne tirer un verdict qu'apres extraction complete et controle des sequences ciblees, du SQLite, des FK, des pages 131-136 et de tous les defauts restants.
