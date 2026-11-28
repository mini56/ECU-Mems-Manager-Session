# V1.69 — Inventaire des sélecteurs 80196, contrôle binaire direct

V1.68 : run 46137949056 vert, 449 firmwares essence, 30 839 entrées, 2 nouveaux sites dans KMJ7V001, aucun SERVICE_COMPARE_CANDIDATE 68K restant dans le périmètre des comparaisons déjà recensées. La couverture de tous les services et leur sémantique ne sont pas démontrées.

V1.69 : lecture directe des sept ROM, vérification des 295 occurrences V1.68 par octets 99 xx 47 suivis d'un saut conditionnel, calcul de la cible relative et de la continuation. Les sept bornes de listes de 18 pointeurs en mode 1E et les répartiteurs sont confirmés par les octets ; 272 sites distincts par ROM/adresse, dont 209 comparaisons d'égalité et 63 comparaisons d'ordre. 23 doublons de fenêtres. Une comparaison d'ordre n'est pas nécessairement un seuil de plage externe ; l'égalité ne prouve pas une commande indépendante.

| ROM | Occurrences | Sites | Doublons | Égalité | Ordre |
|---|---:|---:|---:|---:|---:|
| ABEMR002 | 43 | 40 | 3 | 31 | 9 |
| KBE6R003 | 43 | 40 | 3 | 31 | 9 |
| MGE7R002 | 43 | 40 | 3 | 31 | 9 |
| TCF7X005 | 42 | 38 | 4 | 29 | 9 |
| TCF8X005 | 42 | 38 | 4 | 29 | 9 |
| TFF8X000 | 41 | 38 | 3 | 29 | 9 |
| TFFSX000 | 41 | 38 | 3 | 29 | 9 |
| Total | 295 | 272 | 23 | 209 | 63 |

Les sept ROMs ont la même architecture de liste indirecte : 18 pointeurs, lecture et branchement indirect, avec répartiteur 2E12 (1.3), 374B (TCF), 374D (TFF). Vérification individuelle des octets, adresses, cibles et références dans preuves_selecteurs_80196.json.

Limite : le balayage initial à 96 octets n'établit pas le chemin depuis le pointeur. Des comparaisons peuvent être hors du gestionnaire d'origine (exemple ABEMR002, 30CA après le retour à 3095). Prochaine qualification : parcours réel des branches, séparation des chemins accessibles, comparaisons internes, plages et identifiants, puis rapprochement Manager. Les 295 occurrences ont été examinées, mais les 272 sites ne sont pas tous qualifiés sémantiquement. Aucun essai physique, aucun diesel, aucun changement Manager/DLL. PDF et ZIP cumulatifs non actualisés.
