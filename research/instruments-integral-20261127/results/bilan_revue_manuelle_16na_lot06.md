# MEMS 1.6 NA — revue manuelle lot 06, 28 novembre 2026
Périmètre exclusif : jauge de température et compte-tours. 123 ROM, 15 témoins intégrés, 104 pistes V2 et 4 sans piste. Aucun changement Manager/DLL.
## Dix pistes de sortie lot 05
Les ASM complets des dix références ont été lus (API blob pour les fichiers > 1 Mo). Toutes ont une fusion de port 0401 avec stratégie A4 et masque de préservation A0, ainsi qu'un balayage A6 (décalage et comparaison 40h) lié à la surveillance de sorties. Aucun des sites « producteurs » relevés par lot 05 n'est une modification démontrée du bit 6 de A4 : les LDB/CLRB sur A3 sont des opérations sur l'octet voisin et non des écritures mot sur A4. Les pistes de sortie restent documentées, mais ne constituent pas un test d'instrument.
| Firmware | Fusion | Table générique (ROM) |
|---|---|---|
| KBP8F064 | 291E | 3680 |
| KBP8F065 | 271C | 3474 |
| KBP8F066 | 271C | 3474 |
| KCE6F018 | 28C1 | 3668 |
| KCE8F007 | 28C1 | 3668 |
| KCE8F009 | 28C1 | 3668 |
| KDE6F022 | 2A32 | 3734 |
| KGM6F004 | 28C1 | 3668 |
| KGM6F007 | 28CF | 3676 |
| KGM6F008 | 28CF | 3676 |
Pour toutes les dix tables, AND 0Fh du numéro de commande puis lecture de la table et application du masque aux registres de forçage ; octets lus dans chaque ROM : 00 01 04 02 00 00 00 00 08 10 30 00 1C 00 23 00. Union 3Fh, aucun bit 6. Cette voie générique ne peut pas sélectionner ce bit.
## Trois références sans piste
KBACF001 : 00/10 écrivent AD/01 et AD/02 ; table 34CE. KCE8F000 : idem, table 34B2. KDE6F015 : idem, table 364E. Les trois tables ont les mêmes 16 octets, union 3Fh. Absence de forçage du bit 6 par cette voie. EKDMF008 : voir revue directe lot 03 (pas de commandes 00/10, préservation E0).
## Limite et suite
Ces conclusions éliminent des mécanismes précis, pas des firmwares entiers. Les commandes alternatives, transferts calculés, signaux compte-tours et sorties indirectes doivent encore être examinés. Lot 07 : comparaison sémantique de chaque témoin V2, tables de forçage réellement indexées par les commandes, recherche de basculements de sortie et paires HSO. Les preuves individuelles sont dans les fichiers revue_manuelle_*_lot06.*. Pas d'intégration supplémentaire validée, pas de test physique.