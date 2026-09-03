## 2026-09-03 — RAVEMEMS V2 RCL0193ENG — ZERO DEFAUT PASS 2 — AVANT POUSSE PARSEUR

- Perimetre inchange : `RCL0193ENG` uniquement sur `tmp-ravemems-v2-foundation`; `MEMSX64` reste BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`; aucun #104.
- Le journal de la pousse lecture deux-colonnes est vert : run `33790022321` SUCCESS.
- La correction parseur qui suit est strictement generique et structurelle :
  1. rejeter tout marqueur d'etape numerique inferieur a 1 afin qu'une valeur telle qu'une decimale ne cree jamais une etape 0 ;
  2. reconnaitre un titre de phase `Remove/Refit/Adjust/Inspection/...` seulement s'il est aligne sur la marge structurelle de sa colonne, afin de ne pas confondre les fragments de corps indentes `assembly.` / `remove.` avec des titres ;
  3. lorsqu'une operation contient directement une vraie sequence numerotee commencant a 1 sans titre de phase explicite, creer une phase implicite `procedure` au lieu de perdre les etapes ;
  4. a une frontiere de page, si une phase deja numerotee voit une page ulterieure redemarrer generiquement a 1 sans nouvelle structure, fermer le contexte de phase et bloquer la capture implicite de cette liste afin d'empecher les fuites vers les pages/operations suivantes ;
  5. valider les numeros constructeur entre phases semantiques consecutives : une phase peut legitimement commencer a N>1 seulement si la phase numerique precedente de la meme operation se termine a N-1 et que sa propre suite est strictement contigue ;
  6. conserver les phases semantiques reelles et ne supprimer aucun controle d'audit.
- Transport technique : une action temporaire auto-supprimee appliquera ces remplacements textuels au fichier existant puis ne laissera dans l'arbre final que les fichiers `ravemems/v2/**` autorises. La garde de perimetre finale doit donc rester identique.
- Apres pousse : compilation Python, socle SQLite/C++, extraction complete des 372 pages, audit, comparaison SQLite et verification pages 133-135. Objectif exige : `numeric_phase_defect_count=0` reel.
