# Manager — masses calculées d'air et de carburant

Deux symboles PETA déjà recensés au chapitre 151 (RES-064), dont la chaîne de calcul et l'exploitation Manager sont maintenant approfondies.

| Symbole | AANMP002 | KBHBG009 | Type / conversion XF |
|---|---|---|---|
| s_mass_air | 019C | 0252 | W non signé, brut × 0,03906 mg |
| s_mass_fuel | 019A | 0250 | W non signé, brut × 0,03906 mg |
| s_afr_final | 00D3 | 00C4 | B, brut × 0,1 AFR |

AANMP002 : 5894–58BD calcule et stocke la masse d'air; 58C2 multiplie par dix, 58C6 charge AFR brut, 58C9 appelle division 4B80, 58CC–58D7 arrondit/sature et écrit la masse carburant; 58E4 relit cette masse pour la suite du calcul d'injection. KBHBG009 : 7EFE–7F53 calcule et stocke masse air; 7F58–7F6D même chaîne via 5BF6; 7F7A relit masse carburant. Les deux routines de division produisent un quotient et une fraction, traitent le débordement; la relation nominale est masse carburant brute ≈ arrondi(10 × masse air brute / AFR brut), hors saturation. Ce ne sont pas des mesures physiques ni une preuve de masse réellement injectée; ne pas déduire un débit en L/h sans démontrer la cadence des événements et les autres corrections.

Manager 216 (488dc8c) : aucune correspondance trouvée dans summarytab.cpp, mainwindow.cpp, mems16na_analysis.h, mems16na_decoder.h, mappedinjection_profiles.inc, mappedinjection_profiles_mems19.inc, mappedinjection_runtime_part3.inc et part4.inc. Lecture normale 80/7D non identifiée pour ces masses. Lecteur mode 4 existant mappedinjection_helpers.inc 325–348, adresses paires. Proposition deux lignes « Masse d'air calculée (mg) » et « Masse de carburant calculée (mg) » en lecture seule, avec qualité de lecture et invalidation si perte de connexion. Profil strict: KBHBG010 utilise 0252 pour une autre variable (chap.151). Pas d'activation automatique du mode 4, pas d'écriture, pas de nouvelle lecture ECU. Ne pas assimiler la valeur calculée à la quantité réellement injectée. Aucune intégration logicielle ni compilation dans ce lot.
