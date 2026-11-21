# SESSION 192 — Apprentissage des télécommandes Lucas 5AS

**Sources vérifiées le 21/11/2026**

- James Portman — https://rovermems.com/5as/index.html : commandes normales `5F` pour entrer en apprentissage et `6F` pour en sortir.
- Andrew Revill — https://andrewrevill.co.uk/MEMS3LucasImmobiliserSupported.htm : l'apprentissage efface les associations précédentes, jusqu'à quatre télécommandes, généralement environ huit appuis répétés sur LOCK.
- Blackbox Solutions — https://www.blackbox-solutions.com/site/product?sku=SM034 : contact sur ON pour apprendre ; disponibilité selon la variante 5AS.

## Intégration source

Le programme conserve la connexion Lucas 5AS actuelle (réveil à 5 bauds et lecture `83` ayant répondu). La troisième colonne propose deux boutons Démarrer/Terminer, inactifs sans connexion. Le bouton Démarrer affiche d'abord un dialogue de confirmation, avec Annuler par défaut, qui prévient de l'effacement possible de toutes les télécommandes précédemment associées. Aucun démarrage automatique et aucun envoi pendant une connexion.

En mode apprentissage, le programme arrête le suivi automatique des données du 5AS ; la commande `5F` est transmise uniquement après validation de l'utilisateur. La commande `6F` clôt le mode ; lors d'une déconnexion avec un apprentissage encore actif, le transport tente également `6F` avant de fermer le port.

La réponse affichée « commande envoyée » ne constitue **pas** une confirmation de l'apprentissage réel. La réception radio, le nombre de télécommandes effectivement enregistrées et la réussite du dialogue doivent être vérifiés avec un 5AS physique. Un échec de transmission de la commande de fin n'est pas traité comme un arrêt réussi.

## Non-régression et limites

- Six langues pour les nouveaux textes et confirmation du risque.
- Pas de commandes de programmation EEPROM, de lecture du code EKA ou d'association ECU moteur.
- Aucun changement aux couches MEMS 1.2/1.3/1.6, 1.9, 2J, 3, ROSCO ou RC5.
- Test de protocole 5F/6F et test de démarrage UI hors connexion, sans émission réelle de commande.
- Les essais sur boîtier réel restent nécessaires avant de déclarer cette programmation validée.

