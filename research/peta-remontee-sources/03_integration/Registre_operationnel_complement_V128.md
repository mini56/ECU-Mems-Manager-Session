# Registre opérationnel complémentaire V1.28 — 27 novembre 2026

Référence SESSION 216 : 488dc8cd7dc6a8ec6e25bf8905597fb36e644006.
Les onze fiches V117–V127 ne sont pas onze intégrations. Le plan directeur V1.28 regroupait les résultats historiques trop largement. Le nouveau registre Word et CSV sauvegardé dans la bibliothèque ECU MEMS MANAGER/Recherche technique décompose 111 actions de suivi, dont 71 candidats documentés, 1 correction source certaine, 29 audits/couvertures, 8 recherches préalables et 2 validations de fonctions existantes. Ces chiffres ne constituent ni 111 fonctions absentes démontrées ni 71 fonctions prêtes à livrer. L'audit séparé des 601 règles DTC n'est pas ajouté à ce total.

## Index des actions
- ACT-001 à 015 : tests et services. Sondes température moteur et air, MAP, lambda passive, progression TPS, contacteur, défaut courant/mémorisé, lambda active, jauge, lecture résultat apprentissage et scan, intégrité ROM D4, capture ralenti chaud CB, référence papillon transitoire et limite de pas régulation pression (V110, mot déjà reçu).
- ACT-016 à 052 : 37 valeurs récentes (V118–125). 2 consignes moteur pas à pas ; 4 états de reconnaissance lambda ; 31 valeurs détaillées V120–125. Les fichiers individuels restent les preuves de référence.
- ACT-053 à 080 : 28 compléments historiques, détaillés ci-dessous.
- ACT-081 à 089 : 9 valeurs d'injection 1.9, détails ci-dessous.
- ACT-090 à 111 : correction TPS toujours vrai, DTC par profil, DTC Turbo et disponibilité, DTC5, huit règles de diagnostic automatique, compteur Jack déjà présent, compteur arrêts ECU non établi, validation des 23 modifications NA déjà annoncées, sauvegarde ROM, matrice commandes, plages de référence, aide diagnostic et navigateur firmware.

## Compléments historiques à ne plus perdre
| ID | Fonction | Profil / adresse | Référence |
|---|---|---|---|
|053|AFR de base|AANMP002 03CC ; KBHBG009 03D2|V096|
|054|AFR intermédiaire|KBHBG009 0291|V096|
|055|Débit de référence injecteur extension 1.9|KBHBG009 024C ; KBHBG010 0252|V096/V121|
|056|Angle injection calculé|AANMP002 01A8 ; KBHBG009 0260|V080|
|057|Stratégie de tir|AANMP002 01D4|V081, recherche préalable|
|058|Intervalle de tir voie 1|KBHBG009 0065|V080|
|059|Intervalle de tir voie 2|KBHBG009 0066|V080|
|060|Horloge interne|KBHBG009 02DE, 4 ms|V073|
|061|MAP de secours|KBHBG009 0173|V116|
|062|État secours MAP|KBHBG009 RAM 00D4/08|V116|
|063|Température stratégie ralenti|AANMP002 01AF|V127|
|064|Demande ventilateur|KBHBG009 7D/20, 001B/08|V089|
|065|Contact P/N|KBHBG009 80/0B|V089, audit|
|066|Climatisation|KBHBG009 80/0C|V089, audit|
|067|Contact allumage|KBHBG009 80/1B et 7D/01|V089, audit|
|068|Info profil E8|KBHBG009|V075, recherche|
|069|Disponibilité température ambiante|KLH3G015 80/04|V099, audit|
|070|Disponibilité température carburant|TAH7G018, TBH7G016, TFH7G025 80/06|V099, audit|
|071|Références papillon cumulatives|TAH7G018, TBH7G016, TFH7G025 0360/62/64/66|V099, audit|
|072|Référence unique répétée|KBHBG009/010, KBHBV001/002 7D/17–1E|V092, audit|
|073|Écart signé ralenti|KBHBG009 7D/14–15|V091, audit|
|074|Correction consigne ralenti|KBHBG009 7D/13|V091, audit|
|075|Consigne ralenti travail|KBHBG009 00C2|V091|
|076|Seconde durée charge bobine|KBHBG009 0388|V093|
|077|Compteur trame 1.3|80/1A–1B|V094–097, événement exact non établi|
|078|Analogique de secours|ABEMR002 7D/03|V095, audit|
|079|Fournisseur indirect 1.3|ABEMR002/KBE6R003/MGE7R002 80/11|V070–071, recherche|
|080|Identification ECU/calibration D0|15 IDs V115|audit, 31 non récupérés|

## Détail injection 1.9 (V069/V095)
|ID|Champ|KBHBG009|KBHBG010|
|---|---|---|---|
|081|Base voie 1|014C|014E|
|082|Base voie 2|014E|0150|
|083|Électrique voie 1|02D8|02DC|
|084|Électrique voie 2|02DC|02E0|
|085|En attente voie 1|004A|004A|
|086|En attente voie 2|004C|004C|
|087|Tir mémorisé voie 1|03C2|03C2|
|088|Tir mémorisé voie 2|03C4|03C4|
|089|Temps mort|014A|014C|

Mot ×0,001 ms, sauf temps mort biaisé (x-32768)×0,001 ms. Comparer en priorité les valeurs existantes de base et finale et la compensation déjà utilisée par Manager ; accès normal direct non démontré. Ne pas assigner les cylindres. Pour ACT-053/054, AFR ×0,1 ; ne pas confondre avec AFR final déjà affiché. ACT-055 ne pas extrapoler la chaîne AANMP002. ACT-064 correspondance statique, polarité physique à valider. ACT-090 : diagnosticpanel.cpp 202–204, uint8_t <=255 toujours vrai. Correction certaine, pas encore effectuée. Les comparaisons actuelles restent ciblées, pas exhaustives pour les autres fonctions.

Fichiers complémentaires sauvegardés en bibliothèque :
- Manager_Integrations_Registre_detaille_V128_complement.docx (111 lignes, statut, profil, adresse, preuve, action)
- Registre_actions_Manager_V128_complement.csv (mêmes 111 lignes, identifiants ACT-001..111).
Les anciens documents sont conservés. Ce complément ne remplace pas les preuves, le registre historique ni le rapport général. Aucune nouvelle commande envoyée, aucun changement au programme ni à la DLL, aucune compilation.