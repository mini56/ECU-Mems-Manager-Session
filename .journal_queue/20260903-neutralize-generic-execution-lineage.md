# Neutralisation de la lignée d'exécution générique

Date: 2026-09-03

Contexte validé:
- le SHA 607a861401d057c6084382e40323fd056a19ca44 accepte PDF/dossier/ZIP/TAR et traite RCL0179ENG sans profil spécifique;
- le résultat est vert à audit zéro;
- les logs montrent cependant des résidus de l'ancien prototype RCL0193ENG dans le chemin d'exécution générique: champ manifest prototype, préfixes de diagnostics, nom SQLite et imports de modules RCL spécifiques.

Prochaine action corrective, avant toute nouvelle pousse:
- créer une lignée de modules génériques indépendante des noms de publication;
- conserver les modules historiques RCL0193ENG uniquement comme références de régression;
- le chemin d'entrée générique ne doit plus importer un module nommé RCL0193ENG;
- le nom de base générique doit être neutre;
- les diagnostics génériques doivent être neutres;
- ensuite rerun RCL0179ENG puis régressions RCL0193ENG et RCL0213ENG.

Aucun code production ECU/UI/protocole n'est concerné. MEMSX64 reste BUILD #103 SHA 1d6316bd1746d6f2b4cfb751cab88d18e27ef730.
