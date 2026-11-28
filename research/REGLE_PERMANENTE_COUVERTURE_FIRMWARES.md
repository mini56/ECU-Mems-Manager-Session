# Règle permanente de recherche MEMS — 28 novembre 2026

Instruction explicite du responsable du projet : les recherches concernent TOUS les firmwares disponibles dans les fichiers du dépôt GitHub, et non exclusivement son ECU AANMP002.

- À chaque reprise du chantier, lire cette règle et inventorier le corpus complet (HEX/ROM, ASM, XF1/XF3 et autres preuves disponibles). Inclure les nouvelles références ajoutées au dépôt.
- Pour chaque fonction ou donnée recherchée, étudier toutes les références du corpus, en tenant compte des différences de génération, de variantes, d'adresses, de commandes et de chemins indirects. La priorité 1.6 NA puis 1.9 puis 1.3 définit un ordre, jamais une exclusion du reste du corpus.
- AANMP002 et KBHBG009 ne sont que des exemples de départ ; aucune conclusion globale fondée sur ces deux exemples. Reprendre les résultats antérieurs limités à quelques profils et compléter leur couverture.
- Fournir une matrice par firmware : fonction présente et preuve, absence démontrée, variante, ou non résolu (motif précis). Chiffrer le nombre total, analysé, qualifié et restant. Ne jamais assimiler absence de signature à absence de fonction.
- Comparer chaque résultat à la version actuelle de Manager pour ne retenir que les apports nouveaux ou les corrections nécessaires. Les lectures et tests matériels non validés restent explicitement distingués des preuves statiques.
- Enregistrer les preuves et les résultats à chaque lot dans les rapports FR/EN, le registre des futures intégrations et l'archive cumulative. Ne pas déclarer une recherche terminée tant que des firmwares du périmètre restent à étudier.
- Recherche uniquement : ne modifier ni Manager ni DLL sans demande explicite ; ne pas lancer de tests ECU risqués.

Cette règle est permanente et s'applique aux recherches futures comme aux reprises de résultats incomplets.
