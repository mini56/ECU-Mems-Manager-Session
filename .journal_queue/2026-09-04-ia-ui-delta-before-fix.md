## 2026-09-04 — DELTA UI IA IDENTIFIÉ AVANT CORRECTION

### BASE ET COMPARAISON
- HEAD de travail analysé : `35c539bfa7c64d416a202e66dc913904cbe67fd3` sur `tmp-ai-memslibrary-bridge`.
- État restauré de référence comparé : `16b42634377e37010d258e09c0d0a40966721aa3` (`Restore validated IA visual catalog and scroll`).
- Comparaison GitHub `16b426...` -> `35c539...` : 6 commits d'avance, mais aucune modification de `iamemstab.cpp`, `expert/IaMemsDiagramCatalog.cpp` ou `expert/IaMemsDiagramSelfTest.cpp`. Les ajouts intermédiaires sont uniquement des workflows/tests/scripts temporaires. Le défaut UI observé existe donc toujours dans l'état restauré lui-même ; il n'est pas une régression ultérieure de `iamemstab.cpp`.

### DELTA EXACT À CORRIGER
1. SCROLL : le code actuel crée `iaMemsTranscriptScroll`, une barre externe synchronisée uniquement avec le `QTextBrowser` du transcript. La demande utilisateur est un défilement vertical standard de toute la fenêtre IA afin que l'arrivée du texte fasse défiler la vue et que tout l'historique puisse être remonté. Le transcript ne doit donc plus posséder son propre mécanisme de défilement visible ; la page IA entière doit être portée par un scroll vertical unique.
2. BOUTON IMAGE : les tests utilisateur naturels « Quel est le jeu axial du pignon primaire ? » et « Quelle est la procédure de restauration de la batterie ? » disposent d'illustrations RAVEMEMS pertinentes, mais le bouton `Voir le schéma` ne s'affiche pas parce que le routage restauré exige principalement une intention explicite telle que « schéma », « voir », « brochage ». La correction doit permettre une suggestion visuelle implicite uniquement lorsqu'il existe une correspondance RAVEMEMS suffisamment forte et pertinente ; une question diagnostique ordinaire ne doit pas recevoir d'image parasite.

### PÉRIMÈTRE AUTORISÉ
- Correction strictement limitée à l'UI IA et au routage de suggestion visuelle nécessaire au bouton `Voir le schéma`.
- Ne modifier ni MEMSLibrary, ni `MEMSLibrary_Pack_001`, ni les données RAVEMEMS, ni Qwen, ni le protocole, ni `MEMSX64`.
- `MEMSX64` reste BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`; aucun BUILD #104.
- La base verte IA + MEMSLibrary + RAVEMEMS + Qwen reste `33889068203` / `9da599e697aa96d099982a30f5f5c8a562a7c788`.

### MÉTHODE DE POUSSE / TEST
Préparer un seul commit atomique pour la correction afin d'éviter plusieurs déclenchements du workflow `TMP IA MEMSLibrary Bridge`. Une seule exécution de validation applicative est autorisée pour cette correction. Elle doit vérifier au minimum : compilation x64 du code IA UI, scroll de page unique présent dans la source, les deux questions naturelles ci-dessus produisent une suggestion visuelle valide, et une question diagnostique ordinaire reste sans suggestion visuelle. Après le résultat, mettre immédiatement le RAPPORT à jour avant toute autre action.

### PROCHAINE ACTION EXACTE
Appliquer uniquement ce delta UI sur `tmp-ai-memslibrary-bridge` en un commit atomique et lancer une seule validation dédiée, sans toucher aux sous-systèmes interdits. Puis journaliser le commit et le résultat avant toute suite.
