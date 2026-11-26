# Manager — neuf données nouvelles de commande de ralenti (AANMP002)
Comparaison avec Manager 216 et archive V124 : exclure les consignes position/vitesse E7/E8 déjà recensées en V118, le ralenti de base, la position réelle et les consignes de régime déjà affichés. Neuf symboles ci-dessous absents des fiches textuelles 03_integration V124 (recherche mot entier). Ne pas confondre pas nominaux et position physique mesurée.

| Symbole | RAM | Type | Conversion | Libellé |
|---|---|---|---|---|
| i_known_load_st | 00E1 | B | x pas | Compensation de charges connues |
| i_unknown_steps | 0275 | B | x pas nominaux | Compensation apprise de charges non identifiées |
| i_runline_off | 03CF | B | x pas | Décalage de base de ralenti issu de la carte thermique |
| i_return_offs | 03D0 | B | x pas | Décalage temporaire de retour au ralenti |
| i_total_rtn_pos | 0276 | B | x pas | Position cible de retour au ralenti |
| i_crankg_steps | 03D2 | B | x pas | Position de démarrage avant adaptation |
| i_mdmp_step_pos | 00EA | B | x pas | Position de maintien d'ouverture en décélération |
| i_mdmp_target | 03D3 | B | x pas | Cible du maintien d'ouverture |
| i_mdmp_max_pos | 03D4 | B | x pas | Position maximale du maintien d'ouverture |

XF1 AANMP002 lignes 419,431,435–437,446,450,460,462 ; carte 37 ligne 316, 39 ligne 318, 45 ligne 324, 46 ligne 325. ASM 6120 remet E1 à zéro ; 61D3–61DC et 623C–6245 accumulent la différence de position de base lors des branches climatisation/compensation basse tension (FD bits 0/1, XF1 flagbyte19). 65AA–65B2 soustrait E1 de 8E avant apprentissage. 0275 : 65D7–662B, conditions stabilité/temporisation, différence non négative 8E-F3, filtre 3472 et plafond 13E, stockage ; 637D–638A applique un coefficient avant somme cible. Ne pas présenter comme mesure de charge électrique. 6122–613E calcule carte 37 (01C7/01AE), stocke 03CF avant adaptation 4CC2. 6342–636E calcule carte 39 avec modulation ou zéro ; 6373–639B combine F3, compensation inconnue et plafond calibré, stocke 0276 ; 653B–6540 consomme 0276 comme demande E7. 6444–6458 calcule carte 45 (01AF) plus offset 12A, stocke 03D2 avant adaptation 4CC2 et demande 6482. 03D2 n'est ni position réellement atteinte ni valide hors branche démarrage. 664E–6660 carte 46 indexée par régime 01AC, adaptation 4CC2, stocke 03D4. 6665–668E calcule cible via écart TPS au point de référence 027D et position 027E, comparaison au maximum, stocke 03D3. 6693–66D2 charge ou décrémente EA avec tempo C6; 66D5–66DD relève la demande E7 si EA est supérieure. Le maintien d'ouverture est une stratégie logicielle, pas un amortisseur physique mesuré. Afficher les intermédiaires uniquement après exécution de leurs branches; aucune valeur périmée présentée comme actuelle. Conversion XF octet non signé. La fiche ne présume pas l'universalité des adresses.

Manager 216 (488dc8cd7dc6a8ec6e25bf8905597fb36e644006) : comparaison ciblée summarytab.cpp, mainwindow.cpp, mems16na_analysis.h, mems16na_decoder.h, mappedinjection_profiles.inc, mappedinjection_profiles_mems19.inc, mappedinjection_runtime_part1–4.inc et translations/fr/fr.json. Aucun champ identifié par les symboles, adresses et libellés recherchés; recherche non exhaustive. Lecture candidate via readMappedByteInBlock (helpers 335–348), blocs 00/02/03, profil strict. Pas d'activation automatique du mode 4 ni d'écriture. Accès réel non testé, aucune modification Manager/DLL, aucune compilation.
