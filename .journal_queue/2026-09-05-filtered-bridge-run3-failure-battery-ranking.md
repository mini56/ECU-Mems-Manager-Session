## 2026-09-05 — BRIDGE IA / MEMSLibrary — RUN 3 ROUGE APRÈS ISOLATION STRICTE DE PAGE

Branche : `tmp-ai-memslibrary-filtered-bridge`
Base protégée : BUILD #104 `a55427affeec84643f916621df6247adf29e80fb`
HEAD testé : `16c679ad3730da2ad92967216e37a855616584d7`

Run : `33955644103`
Job : `101278376491`
Conclusion : **FAILURE**.

Étapes avant le test documentaire :
- ascendance/périmètre : PASS ;
- DLL corrigée + Pack001 épinglés et SHA vérifiés : PASS ;
- visuels p53 `VIS_P0053_001` / `VIS_P0053_002` : PASS ;
- configuration du self-test : PASS ;
- compilation du self-test : PASS.

Échec réel :
`FAIL battery regression failed doc=DOC_RCL0193ENG page=136`
Self-test exit 17.

Le durcissement à la page sélectionnée compile donc correctement, mais le classement général des groupes de preuve peut maintenant sélectionner un mauvais groupe documentaire pour la question batterie. La correction ne doit pas être contournée en relâchant l'isolation de page, ni en hardcodant `DOC_RCL0221ENG:p20`.

Aucun fichier de production protégé n'a été poussé vers MEMSX64 et aucun BUILD #105 n'a été lancé.

### PROCHAINE ACTION EXACTE
Analyser les résultats Pack001 exacts produits pour les requêtes batterie/restoration afin d'identifier pourquoi `DOC_RCL0193ENG:p136` bat la preuve attendue `DOC_RCL0221ENG:p20`, puis corriger uniquement la logique générale de ranking/couverture sémantique du bridge et son self-test. Conserver l'isolation stricte document+révision+langue+page. Relancer le workflow temporaire, puis journaliser le résultat. Aucun BUILD #105.