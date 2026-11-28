# Règle permanente de recherche MEMS — 28 novembre 2026

Instruction explicite du responsable du projet : les recherches Manager concernent TOUS les firmwares essence compatibles avec le périmètre MEMS du dépôt GitHub, et non exclusivement son ECU AANMP002. Les calculateurs diesel sont définitivement exclus des recherches Manager, y compris des inventaires de commandes, des matrices de compatibilité et des futures intégrations. Cette instruction remplace le périmètre historique de 483 images.

- Périmètre actuel : 449 firmwares essence (MEMS 1.3 : 3 ; 1.6 NA : 123 ; 1.6 Turbo : 4 ; 1.9 : 215 ; 2J : 9 ; MEMS 3 EU3 : 83 ; EU4 : 12).
- Hors périmètre définitif : Land Rover Td5 MSB (15), Td5 NNN (15), Rover L-Series Di (4), et toute nouvelle référence diesel ajoutée au dépôt. Ne pas les inclure dans les dénominateurs de couverture Manager.
- Conserver les fichiers diesel et les preuves historiques V1.64 et antérieures sans les modifier ni les supprimer ; ils restent des archives hors périmètre.
- À chaque reprise, lire cette règle, inventorier les références essence (HEX/ROM/BIN, ASM, XF1/XF3 et autres preuves disponibles), inclure les nouvelles références essence et signaler les familles non classées avant toute inclusion.
- Pour chaque fonction ou donnée, étudier toutes les références du périmètre, en tenant compte des différences de génération, de variantes, d'adresses, de commandes et de chemins indirects. La priorité 1.6 NA puis 1.9 puis 1.3 définit un ordre, jamais une exclusion du reste du corpus essence.
- AANMP002 et KBHBG009 ne sont que des exemples de départ ; aucune conclusion globale fondée sur ces deux exemples. Reprendre les résultats antérieurs limités à quelques profils et compléter leur couverture.
- Fournir une matrice par firmware : fonction présente et preuve, absence démontrée, variante, ou non résolu (motif précis). Chiffrer le nombre total, analysé, qualifié et restant. Ne jamais assimiler absence de signature à absence de fonction.
- Comparer chaque résultat à la version actuelle de Manager pour ne retenir que les apports nouveaux ou les corrections nécessaires. Les lectures et tests matériels non validés restent explicitement distingués des preuves statiques.
- Enregistrer les preuves et les résultats à chaque lot dans les rapports FR/EN, le registre des futures intégrations et l'archive cumulative. Inclure le répertoire des signatures de commandes dans le rapport. Ne pas déclarer une recherche terminée tant que des firmwares du périmètre restent à étudier.
- Recherche uniquement : ne modifier ni Manager ni DLL sans demande explicite ; ne pas lancer de tests ECU risqués.

Cette règle est permanente et s'applique aux recherches futures comme aux reprises de résultats incomplets.

## Règle impérative : sauvegarde avant toute restitution

- Avant toute réponse présentant des résultats de recherche, même partiels, enregistrer les résultats et leurs preuves dans le dépôt GitHub, dans les compléments du rapport FR/EN et, lorsque disponibles, dans le registre et l'archive cumulative. Vérifier la présence effective des fichiers enregistrés et conserver le lien vers le commit ou la version.
- Les workflows doivent sauvegarder les résultats automatiquement dans le dépôt, indépendamment de la poursuite de la conversation. Un run vert ne suffit pas : vérifier que les résultats ont effectivement été publiés.
- En cas d'interruption de ChatGPT, la reprise doit pouvoir se faire à partir des fichiers sauvegardés, sans dépendre des messages du chat. Si une sauvegarde ou une exécution est bloquée, consigner ce blocage et ne pas annoncer des résultats non sauvegardés comme acquis. Ne jamais attendre une réponse de l'utilisateur pour sauvegarder des résultats déjà obtenus.
