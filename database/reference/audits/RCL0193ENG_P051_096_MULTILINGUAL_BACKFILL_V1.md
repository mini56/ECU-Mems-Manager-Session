# RCL0193ENG — pages physiques 51–96 — ENGINE — backfill multilingue V1

## Source exacte

- Document : **RCL0193ENG Mini Workshop Manual, 5th Edition**
- Fichier : `rave/xn/wmxn990e.pdf`
- Taille : **4 744 911 octets**
- Pages physiques : **372**
- SHA-256 : `c050a3eebe50c5a85bf8a69b7722bd2052079944e09d58578a498984ecf06715`
- Lot : pages physiques **51–96**.

## Contrôle d’exhaustivité / déduplication

La comparaison page à page avec le manuel français RCL0193FRE confirme un alignement exact sur ce bloc :

- **RCL0193ENG p051–096 ↔ RCL0193FRE p050–095** ;
- **RCL0193ENG p052** est blanche et correspond à **RCL0193FRE p051**, également blanche ;
- le sommaire ENGINE source contient **27 opérations nommées** ; les 27 intitulés et numéros `Service repair no` sont présents dans le lot anglais ;
- le lot historique français `research_enrichment_1800.qz64` couvre déjà la mécanique de RCL0193FRE p050–095 (procédures/étapes/valeurs/illustrations), mais cela n’est pas utilisé comme motif pour ignorer la source anglaise.

### Règle appliquée

**Aucune information utile à l’utilisateur final n’est écartée au motif qu’une donnée française existe déjà.**

Le présent lot conserve donc :
- le **texte source anglais intégral des 45 pages non blanches** ;
- les **27 opérations constructeur** avec leur numéro immuable de réparation, utilisé comme ancre de rapprochement inter-langue ;
- les valeurs/couples/tolérances utiles explicitement structurés ;
- les outils spéciaux Rover ;
- les avertissements, précautions et notes critiques ;
- les marquages physiques importants (`TOP`, `FRONT`, `FLYWHEEL SIDE`, `F`) avec règle de non-altération du marquage réel.

La page 52 seule est `out_of_scope / not_required` car elle est **visuellement confirmée blanche**.

## Préparation traduction et images

Le lot ne limite **jamais** l’architecture aux six langues actuellement actives dans MEMS Manager. La table `mems_doc_locale` est dynamique : les futurs japonais, chinois, hindi/autres langues indiennes, etc. peuvent être ajoutés sans changer le schéma documentaire.

Pour **chaque page utile 51–96**, un `visual_candidate` est créé avec la stratégie suivante :

1. conserver la page constructeur originale comme preuve/source ;
2. extraire ultérieurement les dessins techniques en visuels autonomes ;
3. conserver les numéros de repère, numéros d’outil Rover, identifiants de figure et marquages physiques comme ancres **non traduites** ;
4. stocker les légendes/captions/avertissements humains dans `mems_doc_text` et, lors de la matérialisation du visuel, dans `mems_doc_visual_region` ;
5. ne jamais fabriquer six, neuf ou N copies raster figées d’un même dessin : **un visuel source + N couches de texte localisées**.

Cette règle prépare directement l’ajout futur de nouvelles langues sans refaire les images.

## Valeurs et données structurées remarquables

Le lot conserve notamment :
- jeu axial pignon primaire : **0,089–0,165 mm** ; rondelles **2,79–3,04 mm**, pas **0,05 mm** ;
- jeu culbuteurs à froid : **0,30 mm** ;
- plaque de butée arbre à cames : **11 N.m** ;
- poulie vilebrequin : **150 N.m** ;
- couvercle distribution : **16 N.m** ; tendeur courroie auxiliaire : **22 N.m** ;
- culasse : **34 N.m puis 68 N.m**, ordre illustré ;
- collecteurs : **22 N.m** ; bougies : **0,85 mm / 25 N.m** ;
- nombreux couples moteur/boîte/supports/freins explicitement structurés ;
- filtre à huile : serrage manuel au contact + **1/2 tour**, huile **10W/40** ;
- volant moteur : **150 N.m** ; CKP **6 N.m** ;
- pressostat huile : **25 N.m + Loctite 572** ;
- ressort soupape de décharge : renouveler sous **72,5 mm**, bouchon **60 N.m** ;
- distribution : cale **0,152 mm**, écrou pignon AAC **90 N.m**, tendeur chaîne **22 N.m**.

## Outils spéciaux explicitement indexés

`18G 134 BD`, `18G 134`, `18G 1044`, `18G 1519`, `18G 1240`, `18G 1584`, `18G 1068B`, `18G 1043`, `18G 1068`, `18G 134BC`, `18G 1303`, `18G 1381`, `18G 98 A`, plus l’outil de blocage du volant lorsqu’il n’est pas numéroté sur la page.

## Validation locale

- `integrity_check = ok`
- `foreign_key_check = 0` erreur(s)
- `user_version = 21`
- unités 51–96 : **46 / 46**
- pages exactes 51..96 : **True**
- pages source texte : **45 / 45**
- candidats visuels : **45 / 45**
- ancres d’opérations : **27 / 27**, numéros uniques = **True**
- valeurs structurées du lot : **61**
- outils spéciaux : **14**
- avertissements/notes/exigences : **23**
- marquages physiques : **4**
- seconde application du lot : **sans doublon**
- QZ64 → SQL : **byte pour byte = True**

## Périmètre de pousse prévu

Uniquement :
- `database/reference/prototypes/rcl0193eng_p051_096_multilingual_v1.sql`
- `database/reference/prototypes/rcl0193eng_p051_096_multilingual_v1.qz64`
- `database/reference/audits/RCL0193ENG_P051_096_MULTILINGUAL_BACKFILL_V1.md`

Pas de changement de `manifest.json`, pas de modification de tables historiques, pas de protocole/ECU/UI/IA/ONNX et aucun changement de `MEMSX64`.
