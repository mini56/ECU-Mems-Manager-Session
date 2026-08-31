# RCL0193ENG — pages physiques 105–136 — ENGINE MANAGEMENT SYSTEM - MEMS — backfill multilingue V1

## Source
- `rave/xn/wmxn990e.pdf` — RCL0193ENG 5th Edition
- 4 744 911 octets — 372 pages — SHA-256 `c050a3eebe50c5a85bf8a69b7722bd2052079944e09d58578a498984ecf06715`
- bloc exact : pages physiques **105–136** ; p106 et p120 visuellement blanches ; p137 ouvre `FUEL DELIVERY SYSTEM`.

## Correspondance française / déduplication
Le rapport historique V5 du lot français 1790 confirme explicitement :
- `Gestion moteur MEMS : PDF 104-135` ;
- portée MPi ; `mems_family` volontairement NULL/UNKNOWN, aucune génération 1.6/1.9 inventée.

Alignement utilisé : **ENG p105–136 ↔ FRE p104–135**. Le lot `research_enrichment_1790.qz64` sert à éviter les doublons mécaniques, jamais à supprimer le texte ou les visuels anglais.

## Exhaustivité utilisateur final
- 32/32 unités physiques ; p106/p120 blanches ;
- texte source anglais intégral : **30/30 pages non blanches** ;
- candidats visuels : **30/30** ;
- **20 numéros constructeur** conservés comme ancres inter-langues ;
- **32 faits MEMS** structurés ;
- **37 valeurs** structurées ;
- **29 avertissements/cautions/notes/exigences** ;
- outil 22 mm HO2S conservé comme outillage et non comme spécification véhicule ;
- p107 : figure `18M0222`, **14 repères numériques + 14 libellés localisables** ;
- p112 : tableau ECM, **12 entrées + 10 sorties localisables**.

## Données MEMS remarquables
- CKP : 32 pôles espacés de 10°, manquants 30°/60°/210°/250° ;
- limite normale citée 6500 rpm ;
- bobine primaire 0,63–0,77 ohm à 20°C ;
- enrichissement démarrage sous ~400 rev/min ;
- TP alimenté en 5 V ; IACV 20–40 pas ;
- coupure décélération >1600 rev/min ; coupure surrégime ~6500 rev/min ;
- après coupure contact : relais principal ~30 s, surveillance LDR minimum 2 min, logique ventilateurs jusqu'à 8 min ;
- couples/jeux de réparation conservés de 1,5 à 55 N.m et bougies 0,85 mm.

## Traduction des images
Architecture **N langues** : un visuel constructeur source + couches de texte localisées. Les numéros de repère, figure IDs, géométrie, flèches et ancres techniques ne sont pas traduits. Les libellés, titres, notes, cautions et explications sont séparés pour FR/EN/ES/IT/PT/DE et futures locales japonais/chinois/hindi/autres.

## Périmètre
Aucun `manifest.json`, aucune table historique, aucun protocole/ECU/UI/IA/ONNX, aucun `MEMSX64`.
