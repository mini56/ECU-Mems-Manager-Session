# Rapport — perte de couverture multi-calculateurs et remise en conformité
Date : 28 novembre 2026. Base figée : SESSION 231, commit 4b465406cca3a3a619b0396bc8ca85c3af4c9dc0. Branche de travail existante : fix-documentary-additive-session-161-20261118.
## Constat
Les enrichissements successifs n'ont pas conservé une couverture fonctionnelle homogène. Le programme reste multi-protocole, mais les validations antérieures n'établissent pas l'absence de régression par référence. L'audit d'occurrences 46175262722 a traité 1575/1576 fichiers, 103114 occurrences dans 79 chemins (essentiellement documentaires) ; il ne constitue pas un audit fonctionnel. Une archive RAVE externe reste non examinée. La compilation verte 46172627967 ne vaut pas preuve de fonctionnement sur toutes les variantes. Aucune intention malveillante ne peut être déduite du code ; les limitations insuffisamment signalées sont un défaut de méthode et de compte rendu.
## Inventaire initial des restrictions réellement lues dans le code 231
| Fonction | Source | Couverture / restriction | Correction attendue |
|---|---|---|---|
| Mesures complémentaires 1.9 | mems220_measurements.h:7-10 | KBHBG009, identité 900009 | qualification par référence et champ |
| Observations lambda/eau | mems217_passive.h:23-55, mems218_tests.h:106-149 | catalogue 1.6 NA + KBHBG009 ; conditions de validité couplées | séparer les capacités par capteur et les conditions de validité |
| Air/TPS/contact/MAP | mems222_sensor_observation.h:12-30 | catalogue NA + KBHBG009 | profils par mesure, sans verdict inventé |
| Correction injection/ralenti | mems217_passive.h:6-8, mems219_units.h:13-19, mainwindow.cpp:1505-1531 | NA + KBHBG009/KBHBV001 ; ailleurs affichage -- pour correction injection | distinguer capacités, comparer l'affichage historique et restaurer les données justifiées |
| Jauge | mems218_tests.h:6-13 | 15 références NA | qualifier les autres profils ; conserver les sécurités |
| Compte-tours | mems218_tests.h:30-75, memsinterface.cpp:522-527 | 41 références 1.9 ; restriction par famille | qualification physique par référence et protocole |
| Sécurité instruments | mems218_tests.h:77-86 | contrôle supplémentaire 80/27 pour AANMP002 seul, polarité documentée seulement pour lui | profil de sécurité prouvé ; ne pas supprimer ni extrapoler |
| Retour test injecteur | mainwindow.cpp:311-318 ; translations/fr/fr_session222.json | message activation non confirmée uniquement AANMP000/AANMP002 avec réponse 3, alors que l'accusé de réception ne prouve pas l'activation physique pour aucun ECU | message générique non trompeur, conserver octet reçu |
| Test dynamique injection | injectionramtest.cpp:295-303 | catalogue NA seulement | plans de lecture et sécurité par famille vérifiée |
| DTC étendus | memsextendeddtc.h | DTC2 1.9 KBHBG009 seul ; groupes 3-6 par listes partielles | masques par référence, inconnu distinct d'absence de défaut |
| 2J / 3 | mainwindow.cpp:1144-1185, diagnosticpanel.cpp:551-598 | branche séparée, retour anticipé et contexte NA effacé | adapter les vues aux capacités de chaque protocole, pas de commandes ROSCO sur ces ECU |
| Journalisation | logger.cpp:61-96,245-260 | schéma enrichi NA, autres format historique | schéma commun versionné, préserver anciens fichiers et lecteurs |
| Analyse | analysistab_impl_part3.inc:199-234,340-403 | interprétation enrichie NA, autres historique | replay par référence, compatibilité ascendante |
| IA | expert/IaMemsNaContext.h | contexte enrichi NA | observations et règles par capacités, pas de données inventées |
| Toutes données / réglages / diagnostic | mainwindow.cpp:2813+, summarytab.cpp:312-314,470-535, diagnosticpanel.cpp:240+, settingspage.cpp:144 | rendu et règles selon profils incomplets | affichage commun alimenté par données qualifiées |
## Règles de remise en conformité
- Périmètre : MEMS 1.2, 1.3, 1.6 NA, 1.6 Turbo, 1.9, 2J, 3 ; Lucas 5AS selon ses fonctions propres ; Diesel exclu. Ne pas assimiler une priorité à une exclusion.
- Conserver les sources et protocoles, ne pas introduire de patch binaire ni modifier la DLL sans nécessité établie. Branche unique de travail. Aucune suppression de garde de sécurité sans preuve.
- Pour chaque fonction : matrice référence × fonction avec états pris en charge / non applicable démontré / non qualifié / régression avérée ; preuve de firmware, test logiciel, essai physique distincts. Comparer à la dernière version où la fonction était disponible ; ne pas supposer qu'une limitation est automatiquement une régression.
- Catalogue commun des capacités, sans généralisation d'adresses, polarités, échelles ou commandes non prouvées. Conserver les formats anciens et leurs lecteurs.
- Enregistrer résultats et limites avant compte rendu. Tests positifs et négatifs, profils non privilégiés, changements d'identité, déconnexion, défauts présents et absents. Setup pour chaque lot validé ; pas de publication automatique.
## État de départ
Inventaire initial non exhaustif, régressions historiques non encore quantifiées, matériel non testé par ce contrôle. Le premier lot doit traiter une restriction démontrée sans modifier la commande envoyée : interprétation commune des réponses aux tests d'actionneurs, sans conclure à l'activation physique. Les autres restrictions restent ouvertes. Ce rapport n'est pas une validation du logiciel.
