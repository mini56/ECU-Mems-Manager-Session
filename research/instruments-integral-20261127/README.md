# Recherche intégrale — tests instruments, 27/11/2026

Corpus : 483 binaires/ROM/HEX, dont 449 Rover MEMS et 34 autres calculateurs. Cette analyse est indépendante de Manager. Elle ne déclenche aucune compilation ni commande ECU.

Le script lit chaque fichier, calcule son SHA-256, ouvre les ASM et XF associés, repère les tables de commandes 80196 lorsqu'elles sont structurées et recherche les symboles et calibrations Rover relatifs aux instruments. Les preuves individuelles comportent les adresses, extraits de code, données de calibration et égalités exactes de code. Les 56 profils 227 restent distingués des candidats. Les autres architectures sont inventoriées mais pas assimilées au protocole 80196.

Un candidat n'est pas une intégration validée : il reste à remonter la commande jusqu'au consommateur et à la sortie physique, puis l'annulation et l'identification D0/D1 (ou protocole correspondant), et à confirmer l'absence de danger. Les profils sans preuve ne sont pas déclarés incompatibles. Aucun ajout automatique à Manager.
