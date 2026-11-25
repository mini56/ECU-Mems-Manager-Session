# V0.55 — Préparation du Setup privé SESSION 207 (1.2.7)

Base applicative 1ff42590051f1f0ba6119edbef66c6d0ab424a82. Les deux runs 45566562332 et 45566562425 ont été contrôlés verts. Aucun changement du code applicatif ou de la DLL source. Même branche.

Le workflow .github/workflows/session-207-private-test-setup.yml reprend la CI verte et les étapes éprouvées d'assemblage, installation silencieuse et démarrage Windows, contrôle des 16 pages, puis vérification SHA256 de l'installateur et des traductions installées. Distribution de référence historique conservée ; nouveaux catalogues MEMS 1.6 NA vérifiés dans les six langues. Le Setup ne contient pas la base documentaire RAVE ni la base utilisateur. Ce n'est pas une validation sur véhicule.

Instruction utilisateur : aucun dépôt sur le site de MAJ avant ses propres essais. Le workflow vérifie la visibilité privée du dépôt et crée uniquement une pré-release de test non latest dans ce dépôt, avec un tag propre au run. Aucun jeton MAJ, aucun appel au script de publication publique, aucune modification du manifeste public. Résultat du build et disponibilité de l'installateur à vérifier après exécution.
