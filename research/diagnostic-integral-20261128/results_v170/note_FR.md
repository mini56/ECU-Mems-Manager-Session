# V1.70 — Qualification par chemins et tables alternatives (80196)

Périmètre: sept ROMs essence (trois MEMS 1.3, quatre 1.6 Turbo), 272 sites distincts de V1.69. Les 23 doublons de fenêtres ne sont pas des sites supplémentaires. Aucun diesel, aucune modification Manager/DLL.

Le décodeur binaire borné part des 18 pointeurs de chacune des sept tables 1E et suit les instructions reconnues et les deux branches conditionnelles. 217 sites (31 par ROM) ont un chemin de préfixe documenté. Ce résultat ne démontre ni la faisabilité des conditions ni l'origine série du sélecteur. Sept comparaisons supplémentaires 7F (une par ROM) sont retrouvées à l'entrée du lecteur de mots; elles constituent des bornes, non sept commandes nouvelles.

Les 55 autres sites ont été recherchés en dehors de la table 1E:
- 49 sites (sept par ROM: 79, 7C, 89, 94, 7E, 7F, CB) appartiennent à un bloc dont le point d'entrée est référencé par une seconde liste bornée et un interpréteur indirect distinct, vérifiés dans chaque ROM. Les comparaisons 79/7C et 89/94 participent à des tests d'intervalle; 7E/7F/CB sont des tests d'égalité. Le contexte d'entrée externe de cette seconde liste n'est pas établi. Ne pas les présenter comme commandes 1E indépendantes.
- Trois comparaisons F5 (une par MEMS 1.3) sont rattachées à une liste de cinq pointeurs chargée après comparaison de l'état INTMEM_46 à C8. L'autre site F5 de chacun des trois firmwares appartient au groupe 1E. C8 peut être un état interne, pas une commande série démontrée.
- Trois comparaisons C4 à 30CA (MEMS 1.3) restent sans rattachement démontré. Dans ABEMR002, la fenêtre du pointeur 307E dépasse le retour à 3095; le gestionnaire à 30C4 est distinct. Ne pas déclarer C4 absent du firmware.

| Famille | Sites | Préfixe 1E | Table alternative | État C8 | Non résolus |
|---|---:|---:|---:|---:|---:|
| MEMS 1.3 | 120 | 93 | 21 | 3 | 3 |
| MEMS 1.6 Turbo | 152 | 124 | 28 | 0 | 0 |
| Total | 272 | 217 | 49 | 3 | 3 |

269 sites ont ainsi un rattachement statique de niveau différent, pas 269 fonctions validées. Les tables alternatives, les préfixes, les adresses et les octets sont sauvegardés dans les trois fichiers JSON du lot. Le décodeur s'arrête aux instructions non reconnues; il ne remplace pas un désassemblage complet. Il reste à démontrer la provenance du sélecteur, les conditions et les effets des gestionnaires, puis comparer aux sources Manager. Les 449 ROMs restent le périmètre global; ce lot traite seulement les sept précédemment en attente. Aucun essai physique. PDF/ZIP cumulatifs non actualisés.
