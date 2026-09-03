## RAVEMEMS V2 — correction avant pousse : supprimer le verrou publication du profil

Le troisième test RAVE brut sur `rave/xn/hr01795e.pdf` (94 pages, Owner's Handbook, publication source `RCL 0179ENG`) a échoué avant extraction avec `Expected publication code RCL0193ENG not found in source PDF`.

Cause vérifiée : le seul profil disponible `ravemems/v2/profiles/rcl0193eng.json` contient un `publication_code_regex` codé sur `RCL0193ENG`, et le prototype exige ce motif avant toute extraction.

Correction autorisée et strictement générique :
- ne créer aucun profil spécifique `RCL0179ENG` ;
- ajouter un profil RAVE anglais générique réutilisable par plusieurs publications ;
- faire détecter au moteur générique le code publication présent dans le PDF ;
- conserver les anciens chemins/profils RCL0193ENG seulement pour la régression historique tant qu'ils sont encore nécessaires ;
- retester `RCL 0179ENG` sans adaptation spécifique ;
- après succès, refaire les régressions complètes `RCL0193ENG` et `RCL0213ENG` ;
- `MEMSX64` doit rester exactement BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`.

Aucune modification UI, protocole, IA ou production.
