# Retour utilisateur — recette visuelle SESSION 219, à traiter après 220
27/11/2026. Captures 1073 (Actionneurs) et 1072 (Réglages), version affichée 1.2.19, déconnectée, thème sombre. SESSION 220 (51f9c8cb31a0e20ef8249ec1784253a9a8197318) : run 45995383334 success ; ne pas confondre CI et recette visuelle utilisateur.
## Actionneurs
- A01 : espace vertical excessif avant et autour du titre moteur pas-à-pas, et entre titre et commandes. Recomposer la colonne : titre en haut, commandes groupées, jauge immédiatement dessous, État système en bas. Distribuer l'espace utile plutôt que de laisser de grands blancs.
- A02 : voyant État système trop réduit, texte minuscule. Réduire uniquement la hauteur par rapport à la disposition antérieure ; conserver largeur utile et taille de texte cohérente avec les autres cartes. Ne pas imposer une miniature à l'aide d'une mise à l'échelle uniforme.
- A03 : libellé Chauffage sonde O2/lambda tronqué. Assurer largeur suffisante ou retour à la ligne, sans collision avec bulle ni boutons.
- A04 : bulles des boutons de jauge grises, non conformes au style orange des autres aides. Conserver le commentaire visible de désactivation. Pas de nouvelle commande ni activation physique de la jauge.
## Réglages
- R01 : titre Correction instantanée lambda réduit isolément. Harmoniser taille/alignement des cinq titres ; autoriser deux lignes si nécessaire et valider toutes les langues.
- R02 : double bulle d'aide sur Correction d'injection. Regrouper explication de l'unité et de l'adaptation dans une seule bulle.
- R03 : unité affichée -- à côté de valeur --. Montrer une indication explicite de l'unité conditionnelle (µs pour profils confirmés ; pas d'unité physique inventée ailleurs). Choix exact du libellé à valider, ne pas changer les conversions validées.
- R04 : cadrans hors connexion incohérents : quatre zéros avec aiguilles, correction lambda -- sans aiguille. Distinguer données absentes et zéro mesuré ; appliquer une présentation homogène sans supprimer graduations.
## Vérification source
ecupages.cpp 162–218, 460–617 : carte système compacte et disposition des actionneurs ; settingspage.cpp 83–88 : réduction individuelle de police, 192–219 : indisponibilité uniquement FuelTrim, 475–477 : tooltip ajouté à l'unité et au libellé. uishellsmoketest.cpp 200–212 et 1050–1063 : contrôles de géométrie/ordre et absence de chevauchement ne suffisent pas pour recette visuelle. SESSION 220 ne modifie ni ecupages.cpp ni settingspage.cpp : les défauts observés persistent dans ces sources.
## Critères du prochain lot
- Contrôles de taille de police, largeur de libellé, absence de blancs anormaux, rendu des bulles et cohérence des états indisponibles ; captures réelles de l'application installée, aux résolutions 1200/1366/1920 et thèmes clair/sombre. Vérifier FR et six catalogues, préserver les autres pages, aucun scroll vertical principal.
- Reprendre d'abord l'affichage avant de poursuivre les nouvelles fonctionnalités. Cette note documente la demande et les constats ; corrections non effectuées. Pas de relance de 219 ou 220, aucun changement fonctionnel ni DLL.

## Décisions utilisateur complémentaires — impératives pour SESSION 221
- Tous les cadrans conservent leur aiguille visible, placée à zéro hors connexion, comme dans la présentation d'origine. Ne pas masquer les aiguilles ni imposer un rendu différent au cadran de correction lambda. Reprendre également les changements de masquage de la 220 dans Aperçu. Ce zéro est l'état de repos de l'affichage, non une mesure ECU reçue.
- Même police et même taille de titre pour les cadrans de Réglages ; raccourcir le libellé si nécessaire plutôt que réduire isolément la police.
- Actionneurs : agrandir nettement la représentation de la jauge et réduire les trois boutons disproportionnés. Conserver les protections et le commentaire explicatif ; ne pas assimiler le dessin à une mesure de position physique.
- Ces décisions remplacent R04 et toute proposition antérieure de masquer les aiguilles hors connexion. Regrouper toutes les corrections dans un seul prochain lot ; aucun run de correction isolée.

## Décision utilisateur : affichages numériques hors connexion
- Rétablir zéro pour tous les cadrans et les quatre afficheurs numériques de Réglages ECU, au démarrage comme à la déconnexion. Ne pas afficher -- en remplacement de zéro dans ces instruments. Conserver les unités visibles et les boutons désactivés hors connexion. Pour la correction d'injection, conserver l'unité µs du réglage documenté, avec précision de son périmètre dans l'unique bulle du libellé ; ne pas appliquer de conversion non qualifiée à un ECU connecté. Le zéro hors connexion est seulement un état de repos, jamais une donnée de diagnostic ou de journal. Cette décision remplace les propositions antérieures d'affichage -- hors connexion pour ces instruments, sans modifier les règles de disponibilité des tableaux de données et des diagnostics.
- Aucun changement de code ni run isolé pour cette consigne ; intégrer au lot graphique SESSION 221.

## Consigne permanente utilisateur — validation graphique à partir de SESSION 222
Supprimer des prochains workflows de compilation les contrôles de conformité esthétique (proportions, tailles, alignements, couleurs et décisions de présentation) et la génération automatique de captures de recette visuelle. Ne pas substituer des critères esthétiques décidés par l'assistant à l'appréciation de l'utilisateur sur son PC. Conserver les tests techniques et fonctionnels indispensables, la compilation et la génération du Setup. Seul l'utilisateur valide le rendu graphique après installation sur son PC. Un run vert atteste les contrôles techniques exécutés, jamais l'approbation graphique. Laisser la SESSION 221 terminer son run existant ; aucun run supplémentaire pour cette modification de procédure. Appliquer la simplification dans le prochain lot source.
