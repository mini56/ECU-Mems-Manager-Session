## 2026-10-20 — CORRECTION DE TRANSPORT — QUOTA GITHUB ACTIONS
- Run de préflight 39384726710 : extraction V2 47 PDF, 1359 pages, 1667 visuels, erreurs extraction 0, erreurs replay 0.
- Echec réel à l'étape d'upload : « Failed to CreateArtifact: Artifact storage quota has been hit ». Aucune erreur constatée dans RAVEMEMS.
- Décision technique : ne supprimer aucun artefact/release existant et ne modifier aucune donnée validée. Transférer les résultats du pipeline via des assets d'une préversion GitHub Releases privée nommée par run; contrôler le contenu et les hashes et fournir Setup de test seulement après smoke tests.
- Branche isolée existante conservée : tmp-ia-clarification-test-1b106ee. Production MEMSX64 inchangée.