# RCL0193ENG — pages physiques 97–104 — EMISSION CONTROL — backfill multilingue V1

## Source exacte

- Document : **RCL0193ENG Mini Workshop Manual, 5th Edition**
- Fichier : `rave/xn/wmxn990e.pdf`
- Taille : **4 744 911 octets**
- Pages physiques : **372**
- SHA-256 : `c050a3eebe50c5a85bf8a69b7722bd2052079944e09d58578a498984ecf06715`
- Lot : pages physiques **97–104**.

## Contrôle anglais / français

- RCL0193ENG p097–104 correspond à RCL0193FRE p096–103.
- RCL0193ENG p098 est visuellement blanche ; son équivalent français RCL0193FRE p097 est également blanc.
- Le lot historique français `research_enrichment_1800.qz64` couvre déjà la mécanique/dépollution de ce bloc, mais n'est jamais utilisé comme motif pour supprimer la couche source anglaise.

## Contenu utilisateur conservé

- texte anglais intégral des **7 pages non blanches** ;
- **5 opérations constructeur** ancrées par `Service repair no` : 17.10.03, 17.15.13, 17.15.39, 17.50.01, 17.50.05 ;
- **9 faits** de fonctionnement/architecture antipollution ;
- **4 valeurs** directement exploitables : seuil purge >70 °C, 18 N.m, 45 N.m, 9 N.m ;
- **9 avertissements/cautions/exigences** ;
- **2 renvois constructeur** vers ENGINE MANAGEMENT SYSTEM - MEMS et MANIFOLD & EXHAUST SYSTEMS ;
- **7 figures techniques** préparées comme candidats visuels avec identifiants constructeur 17M0112, 17M0105, 17M0102, 17M0103, 17M0101, 17M0100, 17M0117 ;
- légende de la figure 17M0112 structurée en **10 ancres numériques + 10 libellés anglais localisables**.

## Préparation traduction des images

Le lot est volontairement **N-langues** :
- une image/dessin constructeur source ;
- identifiant de figure, numéros de repère et flèches conservés comme ancres immuables ;
- titres, légendes, avertissements et explications stockés séparément ;
- futures traductions ajoutées par locale, sans recréer N rasters différents.

Cela permet d'ajouter ultérieurement japonais, chinois, hindi/autres langues indiennes ou toute autre locale à `mems_doc_locale` sans changer le schéma documentaire.

## Données notables

- trois familles de contrôle : émissions échappement, pertes de vapeur carburant, émissions carter ;
- catalyseur entre tuyau avant et tuyau arrière ; réduit CO, NOx et hydrocarbures ; palladium et rhodium comme constituants actifs ;
- sonde à oxygène chauffée dans le collecteur d'échappement, retour ECM et correction de carburant ;
- EVAP sous la joue d'aile intérieure gauche, charbon actif ; purge commandée par ECM ;
- soupape de purge fermée jusqu'à température moteur **supérieure à 70 °C** ;
- reniflard/séparateur d'huile : **18 N.m** ;
- catalyseur/tuyau avant : **45 N.m** ; tuyau arrière : **9 N.m** ;
- carburant plombé : risque de dommage grave au catalyseur ;
- catalyseur céramique fragile : éviter les chocs importants ;
- véhicule levé : chandelles de sécurité obligatoires.

## Périmètre

Le lot ne modifie ni `manifest.json`, ni tables historiques, ni protocole, ECU, UI, IA, ONNX ou `MEMSX64`.

## Validation locale finale

Application du socle multilingue + lot 1–50 corrigé + lot 51–96 + lot 97–104 appliqué deux fois :

- `PRAGMA integrity_check = ok`
- `PRAGMA foreign_key_check = 0`
- `PRAGMA user_version = 21`
- unités 97–104 : **8/8**
- page 98 : **blanche / out_of_scope / not_required**
- pages source non blanches : **7/7**
- candidats visuels de page : **7/7**
- figures techniques granulaires : **7**
- opérations constructeur : **5/5**
- faits structurés : **9**
- valeurs structurées : **4**
- avertissements/cautions/exigences : **9**
- renvois constructeur : **2**
- légende figure 17M0112 : **1 table, 20 cellules (10 ancres + 10 libellés)**
- seconde application : **sans doublon**
- QZ64 → SQL : **byte pour byte exact**

Fichiers candidat :
- SQL : **190681 octets**, SHA-256 `51dfb695c19fe450a7ea0db7fa23b93344c93f792c44eadca76405fb8458a961`
- QZ64 : **15061 octets**, SHA-256 `8390eb844369f77ace3d7ed88d25c206c02c32f4eabcedb5296ba50688959d5a`
