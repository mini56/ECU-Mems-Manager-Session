# RCL0193FRE - lot 1840 - PRE-POUSSE

Branche documentaire cible : `tmp-rave-visual-backfill`.
Production interdite : `MEMSX64` doit rester sur BUILD #101 / `22dbe75ed14e0a61e694159d505ef72245116b48`.

## Périmètre exact

- Manuel constructeur français `RCL0193FRE`.
- PDF 256 a 278.
- Section : systèmes de protection supplémentaire SRS / airbag / prétensionneurs.
- 20 pages utiles retenues ; pages blanches/intercalaires exclues.
- Captures = vraies pages constructeur rendues à 150 dpi, aucune image générée ou redessinée.

## Validation locale candidate

- `PRAGMA integrity_check = ok`.
- `PRAGMA user_version = 20`.
- Historiques préservés : 93 `mems_rave_fact`, 105 `mems_expert_fact_external`.
- 34 connaissances.
- 19 spécifications / 19 valeurs.
- 4 opérations de réparation constructeur.
- 8 phases de procédure structurées.
- 74 étapes.
- 9 exigences de procédure.
- 15 relations.
- 4 procédures de déploiement manuel conservées textuellement avec ordre constructeur et pages exactes.
- 20 captures constructeur.

## Points importants conservés

- Composants SRS et implantation : module airbag conducteur, prétensionneurs, témoin SRS, DCU, accouplement tournant, faisceau SRS jaune.
- Après déploiement, les composants marqués et les faisceaux jaunes doivent être remplacés.
- DCU non réparable ; surveillance continue et signal de déclenchement airbag/prétensionneurs.
- Airbag : gonflage environ 30 ms ; cycle déploiement/dégonflement environ 0,1 s.
- Témoin SRS : environ 5 s dans Description/Fonctionnement, mais 3 s dans les contrôles après réparation. Les deux valeurs sont conservées et liées par `conflicts_with`, sans arbitrage.
- Avant intervention SRS : batterie déconnectée et attente 10 min pour décharge du circuit de secours.
- Utiliser uniquement un appareil digital ; avertissement constructeur qu'un appareil analogique peut provoquer un déploiement accidentel.
- Remisage airbag : éviter les sources de chaleur de plus de 85 °C.
- Déploiement manuel : outil Rover SMD 4082/1, personnel à au moins 15 m ; attendre 5 min après prétensionneur et 30 min après airbag.
- Prétensionneur de ceinture : remplacement tous les 15 ans.
- Réparations structurées : 76.73.71 airbag conducteur, 76.73.31 prétensionneur avant, 76.73.72 système SRS/DCU, 76.73.73 faisceau SRS.

## Hashes candidats

- SQL SHA-256 : `a6730280d647b68dbdfe46a6b2bc286ea980d600bdc02b22b97f962f1113685a`.
- QZ64 SHA-256 : `7a042d914c76657f0932e2b2f0cf70ba5533ca0e2e6b6dd1433fb167120ffd34`.
- TIFF G4 150 dpi SHA-256 : `b0d1d952b2521d5efa84f96b7388a1137cb0c820674da873a375f7243f5c9bb8`.
- Archive TIFF XZ SHA-256 : `484c776b67f12b3e922df8ad8cdd4c95a2ddbe74500242b7fcf665e80ecea46b`.

PROCHAINE ACTION EXACTE : pousser uniquement le lot documentaire 1840 sur `tmp-rave-visual-backfill`, valider via GitHub Actions, vérifier l'état distant et nettoyer les fichiers temporaires avant de poursuivre le lot 1850.
