## 2026-09-05 — POUSSE TECHNIQUE — NETTOYAGE CLARIFICATION IA

Branche temporaire conservée : `tmp-ia-clarification-test-1b106ee`.
Base fonctionnelle testée PC avant correction : build temporaire vert sur commit `8149a9e631669f76244a378fa6e55dcf35e34cf9`.

Commit de correction : `7b2899c5093d34f94ed5d221427b3e057b58f419` — `Refine IA clarification choices and hide internal grounding`.

Modification technique limitée à `database/MemsSearchCompletenessPatch.cpp` :
- clarification existante conservée ;
- extraction de vrais intitulés de sections/sujets depuis les résultats au lieu de proposer `PDF p.xxx` ;
- rejet des choix génériques/traductions non discriminantes comme `embrayage — clutch` ;
- sélection numérique convertie en sujet précis avant la recherche suivante ;
- interception de la réponse documentaire avant affichage afin de masquer les instructions internes RAVEMEMS/MEMSLibrary et les métadonnées de provenance dans les réponses normales ;
- pour les demandes de couple/jeu précises, conservation de la ligne technique la plus pertinente au lieu du déversement de la page entière ;
- provenance conservée en interne et affichable sur demande explicite.

Aucun changement Qwen/ONNX, ECU/protocole, RAVEMEMS/Pack001, visuels ou autre UI.

Build isolé déclenché automatiquement : run `33981075657` (`TMP IA Clarification Test from BUILD105`).
Statut au moment de cette entrée : **IN_PROGRESS**. Aucun verdict n'est inscrit avant la conclusion réelle du run.

PROCHAINE ACTION EXACTE : attendre uniquement le résultat de `33981075657`. S'il est vert, faire tester sur PC `FREIN`, un choix de couple précis, `embrayage`, un vrai sous-thème, puis une question précise sans ambiguïté. S'il est rouge, corriger uniquement l'erreur réelle du run.
