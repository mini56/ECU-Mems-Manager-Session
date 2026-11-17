# Recherche externe — LeopoldG MEMS-ROSCO3

Date de vérification : 17 novembre 2026. **Recherche seule**, aucun programme, protocole ni Setup SESSION modifié.

## Identification du dépôt

- L'URL saisie `https://github.com/leopoldG/mems-rosco-3` ne correspond pas à un dépôt public accessible (404).
- Dépôt public retrouvé par recherche GitHub : **https://github.com/LeopoldG/mems-rosco3**
- Branche principale : `main`.
- Historique public du 15 novembre 2026 : commit `0f58ea5e235e` (« Add files via upload »), puis `7cbad08cfcf9` (« Create README.md »).
- Contenu actuel de la racine : `README.md` (193 octets) et `mems-rosco-3.rar` (**12 087 551 octets**).
- README complet : « the new mems-rosco can read nearly all logfiles of the known diagnostic tools, as well you can expand the gui. But the best known tool you may find at andrew-revill.co.uk called mems-tools.zip »

## Niveau de preuve

**CONFIRMÉ PAR README** : le développeur revendique la lecture de fichiers de journaux produits par différents outils et la possibilité d'étendre l'interface GUI.

**NON CONFIRMÉ** : connexion directe ECU, type de transport (ROSCO/K-Line/KWP), commandes effectivement envoyées, prise en charge réelle 1.6/1.9/2J/3, conversion des mesures, lecture des défauts, disponibilité de code source, licence d'utilisation.

**BLOCAGE D'INSPECTION** : le seul autre fichier est une archive RAR binaire d'environ 12 Mo. Les outils GitHub utilisés ici exposent son entrée de répertoire et sa taille, mais ne fournissent pas le contenu RAR décompressé. Son inventaire interne et ses éventuels fichiers de log **n'ont donc pas été examinés**. Il serait trompeur d'attribuer à cette archive des fonctionnalités non démontrées.

## Sources complémentaires pour comparaison

- Ancien dépôt identifié par indexation web : `https://github.com/LeopoldG/mems-rosco` — description « diagnostic and analysing programm for Rover mems up to mems 1.6 » ; anciennes ressources publiques mentionnées : `mems-rosco.exe`, `readmems.exe`, `librosco.dll`, répertoire `logs`, PDF. **L'accès direct au dépôt n'a pas été confirmé lors de cette session**, donc ne pas déclarer ces fichiers actuellement accessibles.
- Andrew Jackson, wiki ROSCO : `https://github-wiki-see.page/m/andrewdjackson/memsfcr/wiki/Rover-Modular-Engine-Management-System-%28MEMS%29-ROSCO-Protocol`. Le document cite LeopoldG parmi les auteurs d'outils ayant alimenté les connaissances sur les commandes ROSCO 1.6/1.9.

## Décision de recherche

1. Ne pas importer d'exécutable, DLL, journaux ou fragments de code tant que le contenu exact et la licence ne sont pas inspectés.
2. Lorsque l'archive sera accessible/décompressée, inventorier les fichiers, chercher des captures brutes de communications et leurs descriptions et comparer aux `0x7D` / `0x80` documentés.
3. Vérifier séparément si ROSCO3 **diagnostique en temps réel** ou **analyse des fichiers**, ainsi que les ECU réellement couverts.
4. Si des journaux sont présents, extraire des exemples de tests **hors-ligne en lecture seule** avant de modifier SESSION.
5. La recherche sur ce dépôt n'entraîne aucun build.
