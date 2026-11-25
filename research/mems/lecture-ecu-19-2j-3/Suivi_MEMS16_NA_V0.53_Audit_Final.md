# V0.53 — Audit final de conformité, première passe en lecture seule
Référence V0.43, p.42–45, 67–68 ; sources 7b224dc8326f16ee3b2629cbd37e251508d59289. Lot 2D vert, lot 2E : tests: completed / success; windows: in_progress / non terminé. Pas de modification de code pendant la compilation du lot 2E.

## Matrice des 23 choix
Chaque ligne correspond aux choix effectivement retenus, y compris les précisions 28–32. Constat statique : les 23 choix ont un chemin dans le décodeur commun, les deux pages et le schéma V2. Ce n'est pas une validation physique ni une validation de l'interface complète.
| Choix | Position | Résultat attendu et présent dans le décodeur | Réserve |
|---|---|---|---|
| 1 | 80/04 | Température ambiante | FF non disponible (28) |
| 2 | 80/06 | Température carburant | 36 profils avec capteur, 87 sans (29) |
| 3 | 80/0B | P/N : 0 P/N, 32 hors P/N | AANMP002 seulement ; sinon brut/non confirmé |
| 4 | 80/0C | Climatisation : 0 demande, 1 absence | AANMP002 seulement |
| 5 | 80/0F | r × 6,25 RPM, signe cohérent 7D/13 | Rendu direct de 7D/13 à harmoniser (F1) |
| 6 | 80/10 | Position chaude en pas | Réglages distincts des mesures (F4) |
| 7 | 80/11 | r × 0,1 V | 134 → 13,4 |
| 8 | 80/09 | r × 5/255 V | 38 → 0,75 |
| 11 | 80/1B | Contact coupé 1 oui/0 non | AANMP002 seulement |
| 15 | 7D/0B | (r − 128) × 8 µs | Réglages distincts des mesures (F4) |
| 17 | 7D/0F | Position de base en pas | 107 → 107 |
| 20 | 7D/14–15 | W − 32768 RPM | 32767 → −1 ; contrôler cellule ajoutée (F5) |
| 22 | 7D/17–18 | W / 256 TPS | 1B00 → 27 |
| 23 | 7D/19–1A | W / 256 TPS | 1FC0 → 31,75 |
| 24 | 7D/1B–1C | W / 256 TPS | 27C0 → 39,75 |
| 25 | 7D/1D–1E | W / 256 TPS | 3780 → 55,50 |
| 26 | 7D/1F | Compteur d'apprentissages | 1 → 1 |
| 27 | 7D/02 | r × 0,5° | 19 → 9,5° |
| 28 | 80/04 | FF = non disponible | Pas 200 °C |
| 29 | 80/06 | FF constant si profil sans capteur | Ne pas masquer un capteur pris en charge |
| 30 | 7D/17–1E | Quatre références TPS, huit octets accessibles | Contrôle visuel restant |
| 31 | 80/11 | Unité V | 13,4 V |
| 32 | 80/10 | Pas, sans −35 pour la mesure | Ne pas modifier les commandes d'ajustement |

Les neuf choix non retenus (9,10,12,13,14,16,18,19,21) ne sont pas renommés dans les adaptations NA ; 7D/0E,10,11,16 reçoivent seulement une interprétation DTC. Vérification de la matrice DTC : 123 profils et 107 IDs, 51+1+13+56+2 variantes, bit 40 informatif, aucun défaut en FF constant, masques inconnus prudents. Les libellés FR 29006–29018 contiennent déjà « Défaut ». Les sept groupes ont une aide dans les deux pages. Les tests C++ et Qt précédemment verts ne remplacent pas les tests d'affichage du cahier des charges DTC-22.

## Écarts réels à corriger avant validation finale
- F1 — Cohérence des vues : SummaryTab::applyNaPresentation ne remplace pas m_rowIdleSpeedOffset ; MainWindow::showNaPages ne remplace pas 7D/13, 7D/09, 7D/0A et 7D/06. Les voies V2 utilisent le décodeur commun. En cas d'incohérence/échantillon non validé, certaines vues continuent à montrer une ancienne valeur calculée. La correction signée combinée doit rester conditionnée à la cohérence, l'indicateur lambda séparé de la régulation.
- F2 — Données invalides : updateData et onDataReady exécutent d'abord le rendu historique, puis les adaptations NA. Si profil identifié mais paire invalide, plusieurs champs inchangés restent alimentés depuis la structure DLL alors que V2 refuse la paire. Vérifier leur provenance ; ne pas produire de faux état « OK » sur une trame non validée. DiagnosticPanel::rebuildNa et LogV2 ont déjà un traitement explicite.
- F3 — Transition de contexte : restoreNaPages remet m_naFaultHistoryActive=false avant le test de transition dans onDataReady ; retour NA → historique peut conserver le masque précédent. Invalider la comparaison d'historique à la sortie du profil, sans effacer les défauts du calculateur. Comparer identité complète pour une référence du diagnostic, et non seulement le pointeur de profil (contrôle supplémentaire).
- F4 — Hors des deux pages, les afficheurs d'ajustement e_FuelTrim_lcd et e_IdleDecay_lcd continuent d'afficher les anciennes échelles. Distinguer paramètres de service et mesures avant toute correction : aucune modification de la commande, du paramètre envoyé, de la sauvegarde, des tests d'actionneurs ou des valeurs d'ajustement autorisée. Les heuristiques existantes de l'onglet Erreurs (lambda, IAC) restent à confronter à la spécification, sans inventer de seuil.
- F5 — Vérifier géométrie réelle des cellules de calcul ajoutées dans showNaPages (extra() retourne nullptr si la colonne 3 est déjà occupée), présence d'une bulle permanente, textes non tronqués et thème clair/sombre aux trois tailles. Le test Qt Analyse ne construit pas MainWindow. Aucun contrôle visuel réalisé à ce stade.
- F6 — Identification réelle : pas de commande D1 automatique supplémentaire. La nouvelle interprétation dépend du nom complet issu du dialogue D1 existant, du D0 concordant et de la paire brute du journal TX/RX. Une session sans D1 reste historique ; l'utilisateur doit pouvoir distinguer clairement ce cas. Vérifier le flux réel sur véhicule. Le test sur les 30 paires capturées valide le rejeu, pas l'acquisition physique du Manager compilé.

## Périmètre et non-régression
Comparaison depuis SESSION206 643d944b8663f6390fbdaa3add2879932c259683 : 59 fichiers modifiés/ajoutés, aucun fichier du code DLL, rosco.h ou implémentation ECU dédiée modifié. Les fonctions d'acquisition observées utilisent mems_read/mems_read_kline19 existants et une observation passive du journal. Les anciens workflows de publication 202,203,204,206 sont passés en déclenchement manuel. Aucun Setup public demandé. Vérification statique des fichiers modifiés ne prouve pas l'absence de toute régression d'exécution.
Le rapport de suivi V0.52 conserve les captures d'interface anciennes et les six guides mis à jour ; nouvelles captures et validation véhicule encore à faire.
Priorité suivante : F1–F3 après CI du lot 2E verte, puis tests de non-régression ciblés et audit des afficheurs d'ajustement sans changement de commande. Ne pas déclarer l'intégration terminée avant les vérifications.
