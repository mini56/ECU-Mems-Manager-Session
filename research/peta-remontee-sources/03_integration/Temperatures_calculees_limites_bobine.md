# Manager — températures internes calculées et limites de bobine (AANMP002)
Neuf propositions de nouvelles lignes d'affichage MEMS 1.6 NA; ne pas confondre avec les températures mesurées et le temps de charge déjà affichés.

| Symbole XF1 | RAM | Type | Conversion | Libellé |
|---|---|---|---|---|
| m_oiladj_clt_te | 01C7 | B | -40+0.705882*x °C | Température moteur corrigée pour la stratégie thermique |
| m_oil_pred_cnt | 01C8 | B | 512*x tours | Objectif de tours du modèle de réchauffement |
| m_oil_wrmup_cnt | 03CB | B | 512*x tours | Tours écoulés dans la phase de réchauffement (modèle) |
| m_strat_iat | 01CA | B | -40+0.705882*x °C | Température d'air retenue par la stratégie |
| s_std_iat | 01CC | B | -40+0.705882*x °C | Température d'air de référence cartographiée |
| s_charg_iat_fct | 01CB | B | 0.01*x | Coefficient de correction de température de charge |
| s_charg_iat_tem | 018E | W | -273+0.705882*x °C | Température de charge d'air calculée |
| ign_max_charg_b | 0146 | W | 0.001333*x ms | Durée maximale de charge bobine issue de la tension batterie |
| ign_max_charg_t | 0144 | W | 0.001333*x ms | Durée maximale de charge bobine appliquée |

Preuves XF1 AANMP002 lignes 475–476, 537–545, 607–608, 629. ASM 5521–554D: carte 14 indexée par 01AD, branche de reconnaissance du démarrage F7/02, moyenne avec 0046 selon comparaison; stockage 01C8. 5557–5563 mémorise l'octet haut de 0076 en 01C9 et active FA/08 selon seuil thermique; 5568–557D calcule différence modulo 256, stocke 03CB, soustrait à l'objectif. 5582–5592 interpolation ou correction maximale calibrée; 5597–55A1 somme saturée de 01AD et correction; consommateurs 5774, 6122, 6147, 6895. 01C7 n'est PAS une mesure de température d'huile. 03CB n'est ni un compteur total moteur ni une mesure physique; n'afficher la paire 01C8/03CB comme progression que durant phase active FA/08 et non terminée F7/80, sinon indisponible; valeur 01C8 valable après initialisation FA/10. Ne pas calculer de pourcentage si objectif nul.
55A6–55B5 carte 15 produit 01CB, 55BA–55C1 carte 16 produit 01CC. 55C6–55D3 sélectionne 01F7 ou référence 01CC (55/02). 55D8–560F construit charge thermique par écart signé à la référence multiplié par facteur /100, saturation, ajout 330; 58A1 lit 018E dans calcul de masse d'air. 01CA n'est pas nécessairement la température effectivement mesurée par le capteur; 018E n'est pas un capteur supplémentaire.
2F71–2F8A utilise carte 0 indexée par 01F5 (batterie), effectue la conversion interne et écrit 0146; 2F92 copie vers 0144 dans la branche F5/10, 2364–2369 recopie la limite batterie dans 0144, 22F2–2303, 238D–2399 et 28D2–28DE comparent/limitent la durée à 0144. Ce sont des plafonds et non la mesure de charge existante. Ne pas conclure à un défaut bobine sur la seule valeur du plafond.

Comparaison ciblée Manager 216 488dc8cd7dc6a8ec6e25bf8905597fb36e644006 : summarytab.cpp lignes 99–155, 408–410, 523–580; mainwindow.cpp, mems16na_analysis.h, mems16na_decoder.h, mappedinjection_profiles.inc, mappedinjection_profiles_mems19.inc, mappedinjection_runtime_part1–4.inc, translations/fr/fr.json. Les lignes oil_c pour MEMS 2J/3 ne sont pas assimilées au modèle 1.6; coil_time déjà affiché, exclu des nouveautés; purge duty déjà affiché, exclu. Aucun des neuf champs identifié pour AANMP002 dans la recherche ciblée. Aucun de ces neuf symboles dans les fiches textuelles CSV/JSON/MD/TXT du dossier 03_integration de V123. Accès candidat en lecture seule via readMappedByteInBlock/readMappedWordInBlock (helpers 325–348), blocs 01/03. Identité firmware stricte, pas d'activation automatique mode 4, pas d'écriture; invalider si session/lecture indisponible. Aucune lecture physique, modification Manager/DLL, ni compilation.
