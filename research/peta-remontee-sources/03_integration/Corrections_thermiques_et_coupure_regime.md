# Manager — sept nouvelles mesures (AANMP002)
Nouvelles propositions d'affichage, pas des corrections de valeurs existantes. Définitions XF1 et producteurs/consommateurs ASM remontés.

| Symbole | RAM | Type | Conversion | Affichage |
|---|---|---|---|---|
| s_clnt_pw_mult | 01CF | B | 0.8+0.01*x | Coefficient injection lié à la température moteur |
| s_iatemp_pw_mul | 01D0 | B | 0.8+0.01*x | Coefficient injection lié à la température air |
| s_ase_pw_mult | 0194 | W | 0.005*x | Coefficient enrichissement après démarrage |
| ign_offset_clnt | 03D7 | B | 64-0.5*x deg | Correction allumage température moteur |
| ign_offset_iat | 03D8 | B | 64-0.5*x deg | Correction allumage température air |
| s_hspfco_thres | 01A2 | W | x rpm | Seuil interne de sortie de coupure haut régime |
| s_hspfco_revs | 01A0 | W | 2*x revs | Tours restants du maintien/de la rampe haut régime |

XF1 lignes 490–491, 604, 610, 616–618. Injection: 5774–578A appelle carte 21 ou 22 selon FA/20 et écrit 01CF; 578F–57B0 carte 23 ou neutre brut 20; 5907–5932 multiplie (80+01CF)*(80+01D0)*0194 avec divisions/arrondis. 57DD–5802 produit 0194 = 200+2*carte24 au démarrage; 5809–5868 état 2 puis décroissance vers 200, état 3. Neutres 20,20,200. Après démarrage, ne pas interpréter coefficient comme correction lambda. Le calcul a des effets d'arrondi et saturation, pas simple égalité à la durée finale d'injection.

Allumage: 6895–68A4 carte 49 (0x31), liquide corrigé 01C7 et pression DF, stocke 03D7; 68B3–68BF carte 50 (0x32), air stratégie 01CA; 68E1–694A applique neutralisation/rampe et écrit 03D8; 694F puis 69EC–6A12 additionnent les corrections codées autour de 128, limitent et combinent avec avance de base et ralenti. 03D8 est la correction traitée, pas le résultat brut de carte 03D9; 128 = 0°, positif = avance selon convention XF. Ne pas assimiler ces deux valeurs à l'avance finale.

Coupure haut régime: 48E3–48E8 initialise 01A2; 5C31–5C41 restaure seuil; 5C49–5C7E charge 01A0 selon température/calibration; 5C81–5CA4 interpole seuil pendant rampe; 5CA9–5CBA décrémente 01A0 à zéro. 5CBF–5CED compare régime au seuil: reprise au seuil ou au-dessus, déclenchement au-dessus du seuil augmenté de l'écart calibré; 3A30–3A3C efface 0066 et 0078. 01A2 n'est pas le régime maximal fixe ni le seuil unique de déclenchement; le compteur n'est pertinent qu'en phase de maintien/rampe (FB/08 et FB/10). XF1 flagbyte17 lignes 838–846: hispfco FB/04. Ne pas provoquer une coupure pour tester.

Comparaison ciblée Manager 216 488dc8cd7dc6a8ec6e25bf8905597fb36e644006: summarytab.cpp, mainwindow.cpp, mems16na_analysis.h, mems16na_decoder.h, mappedinjection_profiles.inc, mappedinjection_profiles_mems19.inc, runtime part1/2/3/4 et translations/fr/fr.json. Aucun des sept champs identifié avec les symboles/adresses/libellés recherchés pour AANMP002. 03D8 existe dans les profils KDE6F015/KDE6F022 pour une autre fonction, interdit de généraliser. Comparaison ciblée non exhaustive. Accès candidat via lecteurs existants readMappedByteInBlock et readMappedWordInBlock (mappedinjection_helpers.inc 325–348), blocs 01/03, profil strict. Aucun test physique, aucune activation automatique mode 4, aucune écriture ni modification Manager. Invalider si lecture échoue. Les sept valeurs sont documentées statiquement et proposées pour intégration, pas encore disponibles sur un ECU réel.
