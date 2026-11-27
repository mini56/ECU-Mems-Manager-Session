# Reprise — intégration des recherches MEMS dans Manager (27 novembre 2026)

## Objectif
Passer des recherches à l'intégration concrète dans ECU MEMS Manager. Priorité MEMS 1.6 NA (véhicule d'essai Mini AANMP002, ID diagnostic 98000202), puis 1.9 et couverture des autres variantes selon preuves. Ne pas refaire les recherches déjà documentées ni compter comme nouveautés les fonctions présentes. 

## Références
- Dépôt application : https://github.com/mini56/ECU-Mems-Manager-Session
- Dernière référence auditée : SESSION 216, commit 488dc8cd7dc6a8ec6e25bf8905597fb36e644006 ; capture utilisateur SESSION 215. Vérifier le HEAD et la branche de développement effective avant toute modification ; la branche de recherche n'est pas la branche de compilation.
- Branche documentaire : research/mems-firmwares-20261126
- Dossier : https://github.com/mini56/ECU-Mems-Manager-Session/tree/research/mems-firmwares-20261126/research/peta-remontee-sources/03_integration
- Registre : https://github.com/mini56/ECU-Mems-Manager-Session/blob/research/mems-firmwares-20261126/research/peta-remontee-sources/03_integration/Registre_operationnel_complement_V128.md
- Décision réglages : https://github.com/mini56/ECU-Mems-Manager-Session/blob/research/mems-firmwares-20261126/research/peta-remontee-sources/03_integration/Decision_UI_adaptation_injection_V130.md

## Documents sauvegardés dans la bibliothèque
Dossier /ECU MEMS MANAGER/Recherche technique :
- Manager_Integrations_Registre_detaille_V128_complement.docx et Registre_actions_Manager_V128_complement.csv : 111 actions historiques (71 candidats, 1 correction certaine, 29 audits/extensions, 8 recherches, 2 validations). Ce n'est pas 111 nouveautés certaines. Ajouter les décisions V129–V130 sans doublon.
- Manager_Integrations_Restantes_V128_2026-11-27.docx : plan général, à utiliser avec le registre détaillé.
- Rapport_MEMS_Trames_v1.30_FR_2026-11-27.docx et Rapport_MEMS_Trames_v1.30_EN_2026-11-27.docx : 190 chapitres, dernières recherches.
- Recherche_MEMS_Manager_V130.zip : 29 989 092 octets ; preuves, fiches, archives, tests.
- Dictionnaire_PETA_Rover_V128_FR.xlsx et .docx, Dictionnaire_PETA_Rover_V128.csv : 1344 noms distincts, 2042 entrées par firmware, 22 profils dans cet inventaire ; certaines traductions et associations restent à préciser.
- Lot_Integration_Manager_216_2026-11-26.zip : lot antérieur C++ et règles DTC, à comparer à l'existant, ne pas importer aveuglément.

## Ordre de travail proposé (à valider)
1. Lire le registre et comparer chaque entrée au HEAD actuel ; établir un tableau intégré / correction / absent / accès à vérifier. Ne pas reprendre en bloc les 601 règles DTC : audit V128 = 407 couvertes, 17 différences, 177 règles explicites non retrouvées ; ces comptes de masques ne constituent pas une validation de tous les diagnostics.
2. Premier lot : corrections certaines d'interface et de diagnostic sur 1.6/1.9. Réglage manuel « Correction d'injection » au lieu de « Correction carburant », valeur (brut−128)×8 µs sur profils prouvés ; garder les commandes 79/7A. Cadran instantané lambda en % distinct, renommer si source confirmée. Bulle selon décision V130. Corriger unité correction régime (brut−128)×25 tr/min là où le décodage est établi. TPS : diagnosticpanel.cpp (216, ligne 202) uint8_t <=255 toujours vrai ; ne pas afficher OK sur cette seule base. Vérifier état des corrections connexion et logs, déjà demandées.
3. Nouvelles données en priorité 1.6 NA, puis 1.9 : exploiter en premier les trames ordinaires 80/7D, DTC 3/4/5 conditionnels selon firmware, contacts, références papillon et ralenti. 1.3 : 7D courte selon profil ; 1.6 NA : FF constants non interprétables sur profils concernés ; 1.9 : masques spécifiques. Vérifier les 23 changements NA annoncés. Ne pas inventer de panne à partir d'un bit réservé.
4. Tests et réglages : lambda (passif et ED/EE/F4 sous conditions validées ; pas de verdict HS automatique), sortie jauge température (distincte de sonde, profils équipés), contrôle température moteur/IAT avec valeurs de secours, MAP, TPS et contacteur. CB capture position ralenti chaud confirmée statiquement 10 profils 1.6 NA et 4 profils 1.9 ; écrire uniquement sur action explicite, profil reconnu, moteur chaud stabilisé et position 10–60 pas ; contrôle avant/après et persistance séparée. Les quatre réglages existants ne sont pas des nouveautés. Aucun cinquième paramètre indépendant démontré.
5. Enrichir mesures détaillées par lots : consigne et vitesse pas à pas, apprentissages, masses air/carburant, débit de référence injecteur, corrections thermiques, temps injection, stratégies de démarrage et ralenti. Ne pas confondre données calculées et mesures physiques. Adaptation injection V130 : pour AANMP002 et KBHBG009, délai nominal 22,528 s sous conditions, 10 basculements/évaluation, max 5 évaluations, pas 8 µs, limites automatiques ±240 µs ; pas de durée totale garantie. Progression candidate RAM B3/8D, 0271/029E, 0272/029F : accès réel à confirmer.
6. Mode 4 : distinguer le lecteur existant et les nouvelles lectures RAM proposées ; ne pas activer automatiquement un mode ni envoyer d'écritures/programmer. Valider profil, lecture, qualité et invalidation de données. Fonctions actives non éprouvées à maintenir désactivées tant que conditions et sécurité non validées.

## Règles de développement
Correction du code source, une seule branche de développement existante, pas de patch ni changements hors périmètre ; préserver fonctions existantes, Windows 7/10/11, FR prioritaire, thèmes clair/sombre et mise en page sans scroll vertical principal. Pas de modification DLL implicite (chantier distinct). Numéro de version supérieur après vérification du dernier build. Tests unitaires, non-régression et compilation du Setup pour chaque lot validé ; contrôle run et essai utilisateur réel avant validation terrain. Aucun changement programme/DLL ni compilation effectué au cours de cette reprise documentaire. Ne pas déclarer une fonction validée sur ECU à partir d'une seule analyse statique. Enregistrer les résultats et statuts avant de quitter chaque discussion ; conserver les anciens documents.

## Fiches récentes à lire en priorité
- Reglages_MEMS16_19_V129.md
- Correction_libelle_reglage_injection.md
- Temporisation_adaptation_injection_MEMS16_19.md
- Decision_UI_adaptation_injection_V130.md
- Diagnostic_actuel_capteurs_MAP_lambda.md
- Tests_sondes_temperature_contacteur_papillon.md
- Registre_operationnel_complement_V128.md

Le dossier est une base de reprise ; l'inventaire n'est pas exhaustif et le dernier rendu Word n'a pas été intégralement revérifié. Le choix de l'ordre des lots est proposé, non une décision utilisateur supplémentaire.
