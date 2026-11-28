# Recherche V1.64 — lecteurs mémoire : couverture statique intégrale

Le run V1.63 corrigé (46135232161, succès) a conservé les 345 preuves des générations anciennes et qualifié 134 lecteurs 68K (9 MEMS 2J, 95 MEMS 3, 30 Td5). Erreur corrigée : les sept identifiants de V1.62 sont des SHA-1 de blobs Git, non des MD5.

Les 4 L-Series Di restants sont qualifiés par lecture directe ASM et confrontation binaire : mode 1E, plage de sélecteurs 00–7F, lecteur de mot, commande DC, état 8 et stockage dans la variable utilisée par le lecteur. DRAG0522 possède une variable de bloc différente. Constante de comparaison 001A observée ; ne pas extrapoler les bornes et les conditions de dialogue. Les tableaux de preuve contiennent les octets et adresses. La matrice cumulative conserve chaque référence et son statut.

Total 483/483 lecteurs reliés statiquement : 345 MEMS anciens + 134 service 23 + 4 L-Series Di. Ceci ne signifie ni couverture complète de toutes les fonctions diagnostiques, ni validation du dialogue ou des essais physiques. Prochain chantier : tables de groupes 21, stratégie de secours température et disponibilité des données par firmware. Aucun changement Manager/DLL.
