# Audit à conserver — Diagnostic automatique ECU MEMS Manager SESSION

**Date de consignation :** 2026-11-22  
**Statut :** DOCUMENTATION UNIQUEMENT — corrections différées, non autorisées dans ce lot  
**Branche existante :** `fix-documentary-additive-session-161-20261118`  
**Sources logicielles examinées :** `diagnosticpanel.cpp`, `rosco.h`, `mainwindow.cpp`, `othercontrolunitstab.h`  
**Référence de départ :** commit `e1822a9a47a4743bf16c4afc39dce1e212c788ee` (correction bobine en source après le run 195)  
**Méthode :** ne jamais promouvoir une hypothèse documentaire en diagnostic de panne sans vérification du protocole, des unités, de la variante ECU et des conditions moteur.

## Décision

**Conserver ces observations pour une correction future. Ne modifier maintenant ni le programme, ni la DLL, ni les packs de données, ni la CI, ni les seuils.** L'utilisateur signale une **erreur sérieuse dans les dernières modifications** ; le détail exact de son signalement reste à obtenir. Ne pas présumer qu'une hypothèse de l'audit explique à elle seule ce signalement.

La présente liste est un **audit du code et des risques**, et **non une certification** de valeurs constructeur. La documentation déjà conservée dans `database/reference/audits/rovermems_live_measurements_audit.md` fait état de conversions et de masques contradictoires entre implémentations : ces conflits doivent rester visibles.

## Tableau de travail — valeurs de Diagnostic automatique

| Contrôle | Règle actuellement trouvée dans `diagnosticpanel.cpp` | Constat / risque | Confiance / traitement futur |
|---|---|---|---|
| DTC ECU | `dtc0 != 0 || dtc1 != 0 || dtc2 != 0` = anomalie | Ne distingue pas toutes les variantes ni les bits documentés, réservés ou incertains. Les masques DTC sont déjà documentés comme conflictuels. | **À vérifier** bit par bit, génération par génération ; aucune modification automatique. |
| Batterie | `battery_voltage/10.0` ; plage `11.5…15.2 V` | Conversion cohérente avec l'affichage actuel ; un seuil unique confond potentiellement moteur arrêté et charge moteur tournant. | **À améliorer** après recoupement références électriques et conditions de mesure. |
| Température liquide | `coolant_temp > 0 && coolant_temp < 250` sur octet brut | Simple test de plausibilité du brut, pas un diagnostic en °C. La conversion candidate et ses offsets doivent être validés selon la famille. | **À corriger après vérification** des unités et des variantes. Ne pas imposer un offset universel sans preuve. |
| Régime | `engine_rpm <= 8000` | Filtre de plausibilité très large, insuffisant pour distinguer ralenti ou état moteur. | **À améliorer** par conditions moteur et variante. |
| MAP | `map_kpa <= 160` | Limite très large : une pression inadaptée au ralenti peut rester « OK ». | **À corriger après vérification** de l'unité et de la plage selon régime, charge et pression atmosphérique ; aucune borne fixe universelle inventée. |
| TPS | `throttle_pot <= 255` | **Erreur logique certaine** : `throttle_pot` est un `uint8_t`, donc ce test est toujours vrai ; l'indicateur peut rester « OK » même si le conseil lit des bits DTC. | **À corriger** : ne pas traiter 0–255 comme diagnostic de santé ; examiner tension, progression, défauts réellement prouvés et statut non évalué. |
| Lambda | `dtc2 & 0x04` ou `dtc2 & 0x08` | Signification des masques à recouper ; rapport affiche des données brutes et l'état de boucle. Risque d'alerte injustifiée si un bit n'est pas un vrai DTC. | **Conflit à vérifier** via tables DTC d'origine et conditions de boucle fermée. |
| Corrections carburant | `short_term_fuel_trim - 100`, `long_term_fuel_trim - 128` ; alerte si écart > 20 | Décodages et seuils pris comme indicatifs, pas comme spécifications générales. | **À vérifier** sur exemples bruts/convertis, variantes et états moteur. |
| Bobine / dwell | `coil_time * 0.002` ms ; à 13.5–14.5 V moteur tournant : OK 2.0–3.5 ; surveiller en dessous ou entre >3.5–4.0 ; anomalie **possible** >4.0 | Correction source introduite au commit `e1822a9a` ; la documentation RoverMEMS conserve un conflit sur un maximum exprimé en µs. Ne pas déclarer une bobine défaillante sur ce seul temps. | **Modifié en source, non validé par le run 195** ; futur test réel et par famille nécessaire. |
| Erreur ralenti chaud | `raw7d1415 = (idle_error2<<8) | uk10`, puis `raw - 32768 - (idle_hot - 35)`, alerte abs > 15 | Sens de deux champs / compensation / signe pas suffisamment recoupés. D'autres conversions sont signalées incertaines dans les audits existants. | **À vérifier** contre les trames et plusieurs relevés réels avant changement. |
| IAC / moteur pas à pas | `iac_position == 0 && idle_error >= 50 && idle_switch == 0 && uk3 != 0` | Détection combinatoire spécifique, mais l'état de commande, l'unité d'erreur, les conditions de ralenti et les bits sont à confirmer. | **À améliorer / vérifier** ; éviter de généraliser. |
| Comparaison avec référence | `RPM - RPM_ref`, `MAP - MAP_ref`, `TPS - TPS_ref` | Les soustractions sont mathématiquement correctes ; une dérive n'est pas à elle seule une panne constructeur. | **Correct comme comparaison arithmétique**, sens mécanique à contextualiser. |

## Portée MEMS 2J / MEMS 3

`DiagnosticPanel::update2JData` utilise une présentation distincte : données / défauts structurés, essentiellement informatifs. **Ne pas transposer automatiquement** les seuils MEMS 1.x au MEMS 2J ou 3. Les libellés et commentaires restant dans cette méthode exigent également une vérification des variantes et des traductions avant toute extension.

## Sources documentaires à recouper

1. `database/reference/audits/rovermems_live_measurements_audit.md` : provenance James Portman/RoverMEMS, 57 champs, 26 bits et cinq conflits identifiés (lambda 7D:06, TPS 80:09, ralenti 80:0A, avance 80:16, dwell 80:17–18).
2. `database/reference/audits/rovermems_archive_386_audit.md` : archive RoverMEMS 386, ancien décodage et limites documentaires, distinctions entre versions de parseur.
3. `research/mems/ETAT_RECHERCHE.md` et `MEMS_RESEARCH_RULES.md` : conserver les conflits, variantes, sources et niveaux de confiance.
4. Manuel Rover AKM7169 français, volume 05, page 119 : résistance primaire bobine **0,71 à 0,81 Ω à 20 °C** (ce n'est PAS un seuil de temps de charge).
5. Documentation / fichier linguistique `transfile_fr.ts`, notice du temps de charge : valeur **environ 2–3 ms à 14 V** (indication générale et non preuve de panne).
6. RoverMEMS MPi/SPi, chapitre allumage : indication de dwell **environ 3,0–3,5 ms** selon conditions, à resituer dans sa génération exacte avant d'en déduire des limites logicielles.

Les points 4–6 sont des **indications documentaires** repérées au cours de l'étude. Toute nouvelle valeur de coupure, notamment au-delà de 4 ms, exige un recoupement avec le calculateur ciblé et des essais fiables.

## Alerte indépendante : dernières modifications d'affichage

Contrôle de source dans `othercontrolunitstab.h` : le commit du run 195 a inséré `root->addSpacing(17)` **après** `root->addLayout(learningControls)`. Cet espace apparaît donc **après la rangée des boutons Démarrer / Terminer l'apprentissage**, et ne peut pas, à lui seul, abaisser ces deux boutons pour les aligner sur « Enregistrer le journal ». Il décale surtout le contenu suivant. **Erreur de positionnement constatée dans la source ; à traiter séparément uniquement sur instruction de l'utilisateur.**

## Alerte indépendante : run / version

Le run **SESSION 195** déclaré vert correspond au commit **`f0767179dab6f6b0504d808bda2bc75890c30e2c`**. La modification des seuils bobine date du commit **`e1822a9a47a4743bf16c4afc39dce1e212c788ee`**, **postérieur**. Il serait faux de dire que le run vert 195 teste cette modification. **Pas de nouvelle compilation ou publication demandée ici.**

## Conditions obligatoires avant toute correction future

- Confirmer avec l'utilisateur l'erreur sérieuse signalée ; ne pas la supposer résolue par cet audit.
- Comparer les règles avec les documents constructeur et les audits existants ; conserver les désaccords non résolus.
- Définir pour chaque mesure la génération, l'unité, le contexte moteur/charge/température et les cas non évaluables.
- Tester des cas limite, valeur brute, décodage et rendu multilingue ; séparer alerte possible et panne confirmée.
- Préserver le code / les comportements hors du périmètre autorisé ; **zéro patch**, branche existante, pas de modification implicite du programme ni de la DLL.
- Après décision explicite de correction : build + test sur Setup installé + vérification de la publication MAJ, sur un **nouveau numéro**. Ne jamais confondre un run vert précédent avec la validation de nouveaux commits.

**Statut à la clôture de cette note :** audit archivé, aucune correction appliquée par cette note.
