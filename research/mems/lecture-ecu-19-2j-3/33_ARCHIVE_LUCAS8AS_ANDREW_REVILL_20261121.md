# Archive Lucas 8AS — Andrew Revill

**21 novembre 2026 — ECU-Mems-Manager-Session**  
**Portée :** recherche documentaire seulement, sans changement du logiciel ni du RUN SESSION 192.

## Source vérifiée

https://andrewrevill.co.uk/MapFirmwareLibrary/Lucas%208AS/

Le répertoire public contient **un fichier**, avec la taille affichée par l'index :

- `Lucas_8AS - 1994_5034274.bin` — **256 octets**.

La mention `1994` et la référence `5034274` proviennent **du nom de fichier**, sans validation indépendante d'un millésime véhicule ou d'une référence fabricant. Le listing ne suffit pas à démontrer la cartographie des octets, la nature exacte des données (EEPROM ou autre), la variante ni une compatibilité avec un calculateur réel.

## Intérêt pour Manager

- Répertorier le **Lucas 8AS** comme famille potentiellement distincte parmi les modules d'alarme / verrouillage / antidémarrage.
- Conserver cet échantillon en **référence externe**, pour de futures comparaisons de structure mémoire en lecture seule lorsque le contenu et les droits seront vérifiés.
- Étudier d'abord les caractéristiques physiques, le câblage et les trames du 8AS dans des sources spécifiques à cette famille.
- Ne pas supposer que le réveil, les commandes 5AS (dont la lecture normale `83` et les commandes PLIP `5F`/`6F`), les images 256 octets du 5AS, ni le mode 10AS soient compatibles avec le 8AS.
- Ne jamais copier des codes de sécurité, identifiants d'alarme, données de télécommandes ou d'appairage dans les journaux, les publications ou les packs documentaires sans cadre explicite.
- N'intégrer un éventuel décodeur de fichiers BIN qu'après validation du format par des sources et des tests indépendants.

## Sources de contexte (pas des preuves de compatibilité)

- Andrew Revill, index global : https://andrewrevill.co.uk/MapFirmwareLibrary/
- Étude 5AS : `31_ARCHIVE_LUCAS5AS_EEPROM_ANDREW_REVILL_20261121.md`
- Étude 10AS : `32_ARCHIVE_LUCAS10AS_EEPROM_ANDREW_REVILL_20261121.md`

## Décision de portée

- **Aucun fichier BIN tiers ajouté** au dépôt, aux Releases, au Setup ou à la base documentaire.
- **Aucun changement** des protocoles et fonctions existants : ROSCO, MEMS 1.9, 2J, 3, RC5, Lucas 5AS, PLIP.
- **Pas de nouvelle branche et pas de compilation** au titre de cette simple référence.
- Le RUN 192 existant n'est pas modifié.

**Statut :** source externe inventoriée ; contenu binaire non analysé et protocole Lucas 8AS non établi.
