# Rapport 24 — Où orienter la recherche du dialogue complet Lucas 5AS ?

**Date : 21 novembre 2026.** Projet : ECU-Mems-Manager-Session. Branche conservée : `fix-documentary-additive-session-161-20261118`.  
**Objet strict :** combler l'intervalle inconnu entre la réception du réveil `55 83 76` et la première réponse au service normal de lecture `83`. **Aucun changement de protocole ni compilation pour cette recherche.**

## PRIORITÉ 1 — Andrew Revill, « Lucas 5AS Tools »

**Source primaire vérifiée :** https://andrewrevill.co.uk/MEMS3LucasImmobiliserSupported.htm  
**Annonce détaillée de l'auteur :** https://www.caterhamlotus7.club/forums/topic/275971-mems3-mapper-support-for-the-lucas-5as-immobiliser-key-fob-pairing-etc/  
**Archive du logiciel annoncée :** https://andrewrevill.co.uk/Downloads/MEMSTools.zip

### Pourquoi cette piste est la plus solide

Andrew Revill explique que son logiciel de diagnostic Lucas 5AS :
- sait communiquer avec un 5AS réel à travers un câble dédié FTDI ou CH340 adapté ;
- possède un bouton **About 5AS** qui se connecte et identifie le boîtier selon sa génération ;
- possède un onglet **Live Diagnostics** lisant les états du boîtier ;
- possède un onglet **Log** montrant les détails des communications **entre le PC et le 5AS** ;
- peut être lancé en application autonome depuis l'installation MEMS Tools.

**Travail ciblé :** retrouver **une copie textuelle ou capture exploitable du Log d'une connexion « About 5AS » puis « Live Diagnostics »** : pour chaque étape, énumérer requête, réponse, échos et délais, seulement si visibles. Un journal réel pourrait résoudre le point inconnu sans devoir supposer la même confirmation que MEMS 1.9.

**Limites :** la présence de l'onglet Log n'établit pas encore son niveau de détail (hexadécimal/temporisations) ; le logiciel distribué n'est pas forcément open source et une analyse statique n'est pas une preuve de dialogue réel. L'auteur propose aussi des fonctions d'écriture, mais **notre première étude concerne uniquement la lecture/identification**, sans écriture de données de sécurité.

**Éventuelle question technique à adresser à l'auteur :** le handshake normal 5AS après `55 83 76` requiert-il un ACK ? Si oui, quel octet, quel délai, et quelle réponse attendue ? Quel est le format exact de la première commande de statut `83` ? L'onglet Log peut-il exporter les échanges en hexadécimal ?

## PRIORITÉ 2 — James Portman, RoverMEMS et code de l'application 5AS

**Source publiée :** https://github.com/james-portman/rover-mems-documentation/blob/master/Immobiliser/5AS/diagnostics%20and%20configuring.md  
**Application :** https://rovermems.com/5as/application.html  
**Code publié de l'agent moteur :** https://github.com/james-portman/rover-mems-agent  
**Ébauche de simulateur :** https://github.com/james-portman/5as/blob/master/5as.py

**Établi :** Portman publie le réveil à 5 bauds `1C`, la réponse `55 83 76`, une vitesse autour de 9600 bauds, le service normal `83`, le format à longueur et la commande `F6`. L'application dispose d'une interface graphique 5AS. L'agent public a des modules 1.9, 2J, 3 et RC5 mais pas de module 5AS complet dans l'arbre inspecté. Le dépôt `james-portman/5as` est un simulateur expérimental côté boîtier, non un pilote client complet, avec un problème de comparaison Python entre entiers et octets.

**Travail ciblé :** rechercher un autre dépôt, une branche publique, une capture brute ou demander à Portman l'implémentation du **client 5AS normal mode**, sans demander ni déduire les commandes d'écriture EEPROM. Vérifier particulièrement les trois premiers octets émis par le PC après la réponse `55 83 76`.

## PRIORITÉ 3 — Pscan / Rover T4 : protocole diagnostique d'origine

**Source :** https://www.the-t-bar.com/forum/pscan-uk-diagnostic-tool-forum/98318-interrogating-lucas-5as

Le développeur Pscan affirme sur ce forum avoir **rétroconçu le fonctionnement de l'outil constructeur MG Rover T4**. Des fonctions de lecture des états et d'essai des entrées 5AS existent, mais la discussion consultée **ne fournit pas de trame hexadécimale de connexion**.

**Travail ciblé :** rechercher une capture de liaison entre T4 ou Pscan et un boîtier 5AS lors d'une **identification simple** ; chercher des manuels T4 précisant initialisation et messages de statut. Les procédures d'essai des sorties ne sont pas nécessaires au premier module.

## PRIORITÉ 4 — Vérification matérielle réelle lorsque disponible

Pour confirmer les commandes extraites des sources :
- un boîtier **Lucas 5AS** fonctionnel, un adaptateur 5AS dont les niveaux/voies RX/TX sont documentés et un journal série ;
- relever uniquement **connexion, identification et lecture des états** ; conserver les octets et le délai entre eux ;
- distinguer les données du PC vers le 5AS des réponses du 5AS vers le PC, ainsi que les éventuels échos propres au câble ;
- ne pas interpréter la détection USB COM comme une connexion réussie ;
- valider sur un vrai boîtier la transition de `55 83 76` vers `83` avant d'annoncer une liaison réelle.

Si la méthode reproduit le dialogue ISO 9141 moteur, l'établir **par traces**, pas par transposition d'adresse.

## PRIORITÉ 5 — Normes de transport, seulement en complément

ISO 9141 peut expliquer la forme des 10 bits à 5 bauds, les octets de synchronisation et les possibilités d'ACK. Les spécifications générales ne prouvent pas que **Lucas 5AS utilise les mêmes compléments de clé et confirmation** que le moteur MEMS 1.9.

## Décision opérationnelle

1. **D'abord** chercher la trace dans **Andrew Revill Lucas 5AS Tools → Log**, lors d'une identification non modifiante.
2. Si le log ne donne pas les octets, passer au **code client RoverMEMS** ou demander la trace exacte à l'auteur.
3. Ensuite seulement comparer avec **Pscan / T4** et la norme ISO 9141.
4. Conserver le décodeur pur `lucas5asprotocol.h` et ses tests déjà enregistrés ; ne toucher ni à RC5, ni aux protocoles moteur, ni aux bases documentaires.
5. **Lacune maintenue** tant qu'aucune trace réelle ne démontre l'acquittement, les délais, la réponse de confirmation et la première commande `83`.

**Statut : les priorités et sources sont vérifiées, mais la trame de confirmation après `55 83 76` n'est pas encore démontrée. Documenter ce manque est préférable à inventer des octets.**
