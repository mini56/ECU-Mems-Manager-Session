# AKM7169FRE — MEMS injection monopoint (SPi), source française

## Origine et limites

- Publication constructeur : **Rover Mini, Manuel de réparation, AKM 7169FRE**, © Rover Group Limited 1993.
- Le numéro **AKM 6921** figure sur la couverture comme renvoi à un *autre* manuel de dépannage mécanique ; ce n'est pas la référence du présent PDF.
- Source exacte : `5292642.pdf`, **20 pages scannées**, SHA-256 `37cd0434347fe4009963d075e4b91b0b21ebc905bae5657519a8080c3026d4f5`.
- Document **français d'origine**, jamais traduit du corpus anglais AKM7169ENG.
- **Ce n'est qu'un extrait MEMS SPi** : l'ensemble des autres chapitres AKM7169FRE reste à rechercher.

## Extraction vérifiée sur images

- Page physique 4 : **19 composants** du MEMS SPi identifiés.
- Page physique 6 : **18 emplacements** de composants.
- Pages physiques 11 à 20 : **20 rubriques de réparation** et références constructeur quand lisibles.
- **9 valeurs** repérées sur les fac-similés : page 12 (1,0 bar et 0,7 bar), pages 15 et 16 (vis de capteur vilebrequin à 6 N.m), page 16 (sonde air 7 N.m), page 17 (sonde oxygène 55 N.m), page 19 (raccords 24 N.m et vis d'injecteur 5 N.m).
- Texte des **20 pages** extrait par reconnaissance en français pour la **recherche uniquement**. Il n'a pas été relu ligne par ligne : il doit toujours être affiché avec `review_required` et ne doit **jamais** servir de procédure approuvée à l'IA.

## Intégration SQLite française

À partir de la base RCL0193FRE existante, une copie autonome `mems_documentary_fr.sqlite` a été construite et contrôlée :

| Indicateur | Avant | Avec AKM7169FRE SPi |
| --- | ---: | ---: |
| Documents français | 1 | 2 |
| Pages identifiées | 371 | 391 |
| Entrées FTS5 | 5 958 | 6 084 |
| Images / pages constructeur conservées | 325 | 345 |

**126 nouvelles entrées** pour AKM7169FRE : 20 index de pages, 20 OCR marqués non vérifiés, 20 liens image, 19 composants, 18 emplacements, 20 rubriques de réparation et 9 valeurs en attente de validation technique.

Contrôles hors ligne réalisés sur les *octets exacts* de la SQLite : `PRAGMA integrity_check=ok`, `foreign_key_check=[]`, 345 SHA-256 d'images vérifiés, FTS5 6 084 entrées, les **5 958 entrées RCL0193FRE conservées sans changement**, aucune entrée anglaise importée.

Les sources originales, l'index XML/JSON et l'outil d'import reproductible sont livrés dans le **ZIP privé de travail** généré avec le PDF fourni. Ils ne sont pas ajoutés à un dépôt public ni publiés comme mise à jour de production.

## Logiciel

Le lecteur C++ de cette branche reconnaît plusieurs documents français d'origine et affiche les pages illustrées, avec un écran de sécurité dans l'IA pour empêcher de fournir les erreurs d'OCR comme instructions. Tests de non-régression en CI : `RAVE_FR_CPP_READER_PASS`, `MEMS_DOCUMENTARY_SELFTEST_OK` et compilation du visualiseur.

**Non activé en production** : ni Setup Windows ni installation par pack chiffré/mot de passe dans le programme installé. Le statut demeure `source_inventory`. Il faudra valider le dispositif de distribution et les droits avant toute publication.
