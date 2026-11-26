# Manager — diagnostic du démarrage et de la pompe (AANMP002)
Trois nouvelles valeurs d'affichage, pas des corrections de champs existants.

| Symbole | RAM | Format et conversion | Affichage |
|---|---|---|---|
| m_startup_temp | 01AE | B, -40 + 0.705882 × brut °C | Température mémorisée au démarrage |
| m_cycs_to_start | 00CB | B, 2 × brut tours (échelle XF1) | Tours restants avant reconnaissance du démarrage |
| m_fuel_pump_tmr | 00B5 | B, 0.064 × brut s (échelle XF1) | Temporisation restante de pompe |

Sources XF1 AANMP002 lignes 530–534, 544, 799–804. ASM 4E68–4EAC : si régime inférieur au premier seuil, effacement CB ; au-dessus, comparaison à un second seuil puis chargement CB à 4E8F, test zéro 4E94, ou reconnaissance directe si régime dépasse le second seuil. 20B2–20D4 décrémente CB avec saturation à zéro. Ne pas interpréter CB comme nombre de tours déjà effectués ni comme durée réelle de lancement. 4E99 active F6/40 (m_eng_started), 4E9C–4EA1 copie la température de liquide 01AD dans 01AE ; 6122–6133 et 6342–6350 réutilisent 01AE pour les cartes 37 et 39 (XF1 lignes 316/318). N'afficher 01AE comme valide qu'après reconnaissance du démarrage. 48A4 charge B5 depuis la calibration ; 2844–2864 décrémente B5 sans sous-débordement ; 4EAF–4EC2 coupe la demande F7/04 et A4/01 lorsque l'état moteur arrêté F6/80 est actif et le compteur vaut zéro, sinon maintient la demande. Il s'agit d'une temporisation logicielle et non d'une mesure de pression ou de tension de la pompe.

Comparaison Manager 216 488dc8cd7dc6a8ec6e25bf8905597fb36e644006 : summarytab.cpp, mainwindow.cpp, mems16na_analysis.h, mems16na_decoder.h, mappedinjection_profiles.inc, mappedinjection_runtime_part3.inc, mappedinjection_runtime_part4.inc ne contiennent pas les symboles, adresses ni libellés anglais recherchés. Comparaison ciblée, pas audit exhaustif. Lecteur existant readMappedByteInBlock (mappedinjection_helpers.inc 335–348) extrait l'octet pair de poids faible, impair de poids fort ; blocs 01 et 00 ; lecture candidate avec session mode 4 déjà autorisée, non vérifiée sur ECU. Pas d'activation automatique, pas d'écriture. Ne pas afficher 0 comme valeur de remplacement en cas d'échec. Aucune généralisation MEMS 1.9. Aucun changement logiciel ni compilation.
