## RAVEMEMS — planning maître révisé

Décision utilisateur : avant toute nouvelle extraction, ne pas supposer que les 47 PDF sont 47 contenus techniques indépendants. Plusieurs peuvent être le même manuel dans plusieurs langues. Il faut éviter d'extraire 2, 3 ou 5 fois le même contenu et les mêmes images.

### Baseline applicatif
- Reprise future de MEMS Manager depuis BUILD #101, dernier socle avant l'intégration documentaire massive de #102+.
- Aucun BUILD #106 pendant la reconstruction documentaire.

### Planning officiel

1. INVENTAIRE ET REGROUPEMENT DES PDF RAVE
- Examiner les PDF présents dans le corpus RAVE.
- Regrouper les documents qui représentent le même contenu technique dans des langues différentes.
- Distinguer au contraire les éditions, années, variantes, bulletins et manuels réellement différents.
- Pour chaque groupe multilingue, choisir un PDF maître pour l'extraction technique complète, probablement l'anglais quand c'est le meilleur document, mais le choix doit être vérifié groupe par groupe.
- Ne pas extraire plusieurs fois les mêmes images et procédures uniquement parce qu'elles existent dans plusieurs langues.
- Une autre langue n'est extraite comme contenu technique distinct que si elle contient réellement une procédure, une variante, une page, un schéma, une valeur ou une révision différente.

2. CORRIGER RAVEMEMS V2 AVANT L'EXTRACTION
- RAVEMEMS V2 devient d'abord un moteur d'extraction fidèle, pas un moteur qui devine seul le sens documentaire.
- Conserver l'extraction du texte, des pages et de la géométrie.
- Pour la capture d'images uniquement : exclure la zone d'en-tête, car les icônes inutiles sont en haut de page. Cette exclusion ne doit pas supprimer le texte d'en-tête du reste de l'extraction.
- Réduire la marge de crop actuellement trop grande.
- Empêcher le regroupement abusif de dessins vectoriels distincts.
- Conserver séparément la zone source et la zone de crop finale.

3. PREMIER DOCUMENT PILOTE
- Commencer par RCL0193ENG ou par le PDF maître correspondant après regroupement.
- Contrôler directement le texte, les opérations, procédures, valeurs et images utiles.
- Test obligatoire : procédure du jeu axial du pignon primaire, opération 12.21.28, valeur 0.089–0.165 mm, contrôle aux cales, vraie illustration mécanique conservée, icône d'en-tête rejetée.

4. COUCHE INTERMÉDIAIRE VALIDÉE
- Avant SQLite, produire une représentation structurée et lisible du contenu réellement retenu : section/opération, procédure, étapes, valeurs, notes et images utiles.
- Le but est de pouvoir vérifier le résultat avant de l'enfermer dans la base.

5. EXTRACTION DES DOCUMENTS TECHNIQUEMENT UNIQUES
- Appliquer la méthode validée au reste des PDF maîtres et aux documents réellement distincts.
- Ne pas répéter le travail pour de simples traductions du même manuel.
- Vérifier chaque lot avant le suivant.

6. CONTRÔLE DES IMAGES
- Vérifier que les icônes d'en-tête ne sont plus capturées.
- Vérifier que les vraies illustrations ne sont ni coupées ni entourées d'une marge excessive.
- Vérifier les schémas vectoriels et leur regroupement.
- Une illustration technique identique entre plusieurs langues doit être stockée une seule fois.

7. CONSTRUCTION SQLITE
- Construire SQLite seulement après validation de l'extraction et des images.
- La base doit contenir uniquement les informations nécessaires pour retrouver correctement procédures, valeurs et visuels utiles.
- Éviter les duplications linguistiques inutiles.

8. CONSTRUCTION DE LA DLL
- Construire la DLL à partir de cette nouvelle base propre.
- La DLL doit retrouver directement les bonnes informations et les bons visuels sans heuristiques spécifiques dans MEMS Manager.

9. TESTS SANS QWEN / SANS MEMS MANAGER
- Tester d'abord SQLite + DLL seules.
- Une question doit retrouver la bonne procédure, la bonne valeur et le bon visuel avant toute reformulation IA.
- Utiliser plusieurs cas de référence sur plusieurs documents.

10. RETOUR DANS MEMS MANAGER
- Repartir du BUILD #101.
- Intégrer la nouvelle base, la DLL et les visuels sur une branche temporaire.
- Tester les réponses utilisateur finales dans les langues de MEMS Manager.
- La langue de réponse est gérée à la fin ; elle ne doit pas imposer de dupliquer toute l'extraction technique.
- Aucun build production suivant sans validation complète et autorisation explicite.

### PROCHAINE ACTION EXACTE
Faire l'inventaire du corpus RAVE et identifier les groupes de PDF correspondant au même contenu technique dans des langues différentes, afin de déterminer quels PDF maîtres devront réellement être extraits avant de modifier RAVEMEMS V2.
