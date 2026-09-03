## 2026-09-03 — Résultat test complet RCL 0213ENG / Electrical Reference Library (212 pages)

Test demandé sur un autre PDF anglais RAVE d'au moins 30 pages.

Source figée : `rave/xn/elxn970e.pdf`, commit source `643de091b474f4e27917a065bdf46d5a0c764276`, blob Git `337f957045cb9ea79f38a1a5e3ddb01f194c7156`. Préflight : 212 pages, titre `ELECTRICAL REFERENCE LIBRARY`, publication `RCL 0213ENG`.

Pipeline testé sans changement d'algorithme : SHA exact `a9e47832a2fbd3bf365a32e00d374ed7c70fba5c`.

Run GitHub Actions complet : `33799187283`, job `100794129887`, conclusion FAILURE uniquement au garde strict final d'identité/catégorisation. L'extraction des 212 pages, la construction SQLite et le replay visuel ont terminé correctement avant ce verdict.

Résultats bruts :
- 212/212 pages traitées.
- 0 opération, 0 phase, 0 étape extraites avec le parseur actuel de manuel d'atelier.
- 0 défaut numérique de séquence.
- 0 review flag ouvert.
- 395 visuels extraits.
- replay visuel exact : 395/395 fidélités vérifiées, 0 échec, 0 manquant, 0 inattendu.
- 0 lien visuel, donc 0 échec de lien.
- audit après replay : 0.
- SQLite `integrity_check=ok`.
- 0 FK cassée.

Défauts réels révélés par le garde strict :
1. `document_key` reste codé en dur avec l'identité RCL0193ENG au lieu d'être dérivé de la publication courante.
2. `revision_key` reste codé en dur avec l'identité RCL0193ENG.
3. `document_kind` est `workshop_manual` alors que le document est une `Electrical Reference Library`; la classification générique ne connaît encore que les bulletins techniques versus manuel d'atelier.
4. Le manifest conserve également `prototype=RCL0193ENG`, autre trace d'identité codée en dur.
5. Couverture sémantique non démontrée pour ce type documentaire : l'Electrical Reference Library contient des descriptions et schémas électriques, mais le parseur orienté opérations/phases/étapes de manuel d'atelier produit 0/0/0. Ce n'est pas un défaut de fidélité visuelle, mais un manque de modèle/type documentaire à traiter avant de considérer ce PDF correctement intégré à la base.

Artefact : `RAVEMEMS-V2-RCL0213ENG-212PAGE-FULL-TEST`, ID `9910387693`, taille 28 286 187 octets, SHA256 ZIP `aefc802f0c9983b13e00a5efc185f8685f0625118cd9fa0a960b6fbd2dd58530`.

Verdict : le test est diagnostiquement réussi mais le document n'est PAS validé zéro défaut global. Le chemin visuel généralise correctement; l'identité, la catégorisation et la couverture sémantique des Electrical Reference Libraries doivent être généralisées avant validation de ce type de PDF.

Aucune correction supplémentaire n'est lancée sans nouvelle instruction utilisateur. Production protégée : `MEMSX64` reste BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`; aucun BUILD #104.
