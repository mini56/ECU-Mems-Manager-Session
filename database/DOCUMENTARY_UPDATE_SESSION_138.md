# SESSION 138 — test du téléchargement documentaire indépendant

## Changements effectivement codés

- La page **Base de données** reste accessible même sans SQLite documentaire installée.
- Bouton `documentaryUpdateButton` dans l'en-tête de cette page.
- Consultation réseau HTTPS asynchrone de :
  `https://raw.githubusercontent.com/mini56/ECU-Mems-Manager-Session-MAJ/main/manifest.json`.
- Validation du schéma `mems-documentary-multisource-v1`, de la langue sélectionnée et de la disponibilité.
- Si `available:false`, message explicite « aucune base publiée », sans télécharger ni écrire.
- Si un pack est réellement publié : URL HTTPS de Release du dépôt MAJ uniquement, pack obligatoirement chiffré, SHA-256 obligatoire, téléchargement dans `AppLocalDataLocation/documentary/downloads/<langue>/` avec écriture atomique et vérification SHA-256. Un échec réseau ou de hash ne touche aucun fichier existant.
- Les fichiers de base documentaire, **uniquement eux**, sont absents du nouveau Setup de test. Le lecteur cherche une base linguistique externe dans `AppLocalDataLocation/documentary/bases/<langue>/`, pas dans le répertoire de l'exécutable.

## Bases et fonctions préservées

Ne pas confondre les bases documentaires optionnelles avec :
- `AppLocalDataLocation/database/ecu_mems_manager.sqlite` : données ECU de l'utilisateur, jamais distribuées ni supprimées.
- `database/expert/ia_mems_reference_r20.sqlite` et leur moteur expert : références IA / diagnostic.
- `database/reference/`, ses fiches MEMS et `AppLocalDataLocation/reference/` : diagnostic, schémas et index.
- `MEMSLibrary_Pack_001/knowledge.sqlite` : bibliothèque legacy encore utilisée par des chemins de l'IA, conservée par sécurité à ce stade.
- `mems_manager_x64.dll`, Qt, ONNX : pilotage ROSCO / moteur IA inchangés.

## Limites volontaires du test

**Ce build ne sait pas encore installer/déchiffrer un pack avec mot de passe.**
Le bouton vérifie le manifeste et peut télécharger un futur fichier chiffré ; il
ne remplace aucun SQLite. Aucune base n'est actuellement publiée dans MAJ.
Le lecteur documentaire historique doit encore évoluer pour accepter le nouveau
schéma multi-documents. Ce test vérifie la préparation, la présence du bouton,
l'absence du nouveau corpus RAVE et la non-régression des dépendances ECU/IA.

Aucun nom de programme n'est modifié : `ECU MEMS Manager SESSION`.
Les tests ont été ajoutés directement aux sources Qt et CMake, sans scripts de
réécriture à l'exécution et sans installer une source non validée.

Workflow de compilation et d'installation Windows :
`.github/workflows/session-documentary-update-test.yml`.
