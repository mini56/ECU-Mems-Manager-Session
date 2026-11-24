# MEMS 1.6 NA - suivi V0.45, lot 1 (24 novembre 2026)

Base SESSION 206: 643d944b8663f6390fbdaa3add2879932c259683; branche fix-documentary-additive-session-161-20261118. Cahier des charges V0.43, chapitres 21, 44-47, 51-52 et 65-67.

- Catalogue source: 123 profils HEX (107 ID PETA distincts), 36 températures carburant présentes, 16 références papillon répétées. Les deux ASM sans HEX ne sont pas intégrés.
- DTC1: 51 PTC en 04, 1 PTC en 08 (EKDMF002), 13 ventilateurs, 56 auxiliaires en 04 et 2 en 08. La désignation physique des 58 sorties auxiliaires reste non confirmée.
- Décodeur isolé, sans I/O ni Qt: conversions validées, valeurs brutes conservées, états / défauts / indicateurs séparés. DTC2/40 n'est jamais une panne. DTC constants FF non utilisés.
- Identification: exige preuve NA et nom complet, contrôle PETA si disponible. Pas de sélection sur ID numérique seul. Les contacts P/N, climatisation, contact coupé restent non vérifiés hors AANMP002 faute de preuve de polarité de sortie. DTC2/04 hors AANMP002 reste défaut non identifié tant que sa fonction n'est pas confirmée par profil.
- Allumage non modifié; lambda x5 maintenue. Aucun changement de DLL, de commandes, de branche ni d'affichage des générations existantes.
- Le décodeur n'est pas encore raccordé aux écrans: il faut établir un contexte de session fiable et des longueurs réelles avec les informations déjà disponibles. La structure mems_data seule ne transporte pas les longueurs. Ne pas créer de nouvelle commande ECU pour résoudre ce point.
- Test autonome: `c++ -std=c++11 -Wall -Wextra -Werror -pedantic -I. tools/mems16na_decoder_selftest.cpp -o mems16na_test && ./mems16na_test`. Workflow de test isolé, aucun Setup ni publication à ce stade. Les résultats CI doivent être vérifiés avant le lot 2.
