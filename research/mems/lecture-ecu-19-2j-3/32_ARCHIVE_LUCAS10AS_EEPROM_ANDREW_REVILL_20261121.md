# Archive Andrew Revill — Lucas 10AS

**21 novembre 2026** — documentation de référence pour `ECU-Mems-Manager-Session` (branche unique `fix-documentary-additive-session-161-20261118`).

## Répertoire observé

https://andrewrevill.co.uk/MapFirmwareLibrary/Lucas%2010AS/

Le listing public consulté le 21/11/2026 présente **un seul fichier** :
- `Lucas_10AS - 0.bin` — **256 octets** (taille affichée par le listing).

Il s'agit d'un petit échantillon binaire utile pour l'étude de la mémoire du boîtier **Lucas 10AS**, et non d'une cartographie moteur MEMS. Le format exact et la correspondance des octets ne sont **pas** démontrés par le seul listing. Aucun contenu sensible du fichier n'est copié dans ce dépôt.

## Points acquis dans nos recherches antérieures

- L'audit de l'application Andrew Revill `Lucas5asTools` (rapport 27) montre une branche de famille différente du 5AS et une commande `9B` dans le chemin de lecture correspondant, sans avoir établi à ce stade **une séquence complète de dialogue Lucas 10AS**.
- Les configurations et câblages 5AS et 10AS doivent être distingués. Le rapport 27 signale un adaptateur KKL avec une permutation 7/8 pour les Land Rover étudiés, tandis que le 5AS nécessite un autre câblage. **Vérifier le véhicule précis avant d'utiliser ces informations ; ne pas fusionner les deux guides.**
- Le rapport 19 constate que le protocole, les réponses, les lectures et l'apprentissage de télécommandes 10AS nécessitent leur propre étude.
- Documentation externe des familles antidémarrage Rover/Land Rover : https://pscan.uk/features/10as.html
- Documentation Andrew Revill sur ses outils : https://andrewrevill.co.uk/MEMSToolsIndex.htm

## Utilité prévue pour Manager

1. Conserver une référence d'échantillon 10AS afin de comparer ultérieurement plusieurs sauvegardes, si d'autres sources légitimes sont disponibles.
2. Étudier les structures mémoire en **lecture seule** : zones communes et différences avec le 5AS **uniquement lorsqu'elles auront été démontrées**.
3. Concevoir, plus tard, une session diagnostic 10AS autonome, avec identification fondée sur les réponses effectives du boîtier.
4. Prévoir une interface séparée pour les fonctions de lecture et de télécommandes propres au 10AS, après vérification de leurs commandes et conséquences.
5. Ne pas publier d'identifiants de télécommandes, de données d'appairage, de codes EKA/EMS ou d'informations sensibles.

## Périmètre

- **Référence externe uniquement** : aucun fichier `.bin` tiers n'est versé au dépôt, dans le Setup ni dans les bases documentaires.
- **Aucune modification du programme, des protocoles, de la connexion, de la traduction ou du RUN 192**.
- Aucun protocole ni aucune commande d'apprentissage 10AS ne sont inférés à partir du 5AS.
- Pas de nouvelle branche, de nouveau build ou de validation sur calculateur physique à l'occasion de cet inventaire.

**Statut :** référence archivée, contenu binaire et protocole Lucas 10AS non validés.
