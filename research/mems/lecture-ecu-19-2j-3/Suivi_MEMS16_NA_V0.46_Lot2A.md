# Suivi V0.46 - Lot 2A : preuve de profil et rejeu des communications (24 novembre 2026)
Base lot 1: 525d54a07359bccf5b334a02e9787e8e7582a78e. Branche unique: fix-documentary-additive-session-161-20261118.
Cahier des charges V0.43, ch. 42, 47, 49, 53, 65-67.

## Preuve réelle
Journal fourni du 24/11/2026 13:13:38 : D0 = 98 00 02 02; D1 = écho D1, puis trois répétitions de AANMP002 + 98 00 02 02. 31 couples 80/7D, dont 30 après D1. Longueurs réelles 28/32, échos conservés. Fixture privée réduite aux octets TX/RX, sans horodatage ni texte du journal.

## Source ajoutée
- mems16na_session_evidence.h : observation passive des transactions terminées, identification par nom exact du catalogue NA et concordance D0/D1, rejet des réponses contradictoires, contrôle longueur annoncée et réelle, association 80/7D consécutive, reset du contexte local sans action ECU.
- Aucun D1 ajouté à la connexion. D1 est déjà disponible dans l'outil de protocole existant. Sans réponse D1 complète, le profil reste non identifié. Ne pas déduire AANMP002 du seul ID.
- Tests synthétiques et rejeu réel; les 30 couples après D1 sont à vérifier en CI. Les réponses D1 à une ou deux répétitions sont acceptées seulement si les enregistrements sont complets et concordants; seule la forme à trois répétitions est attestée sur ce véhicule.
- Pas de modification des sources historiques, DLL, commandes, traduction ou autres générations. Aucun Setup. Raccordement aux écrans et aux fichiers d'analyse encore à faire. Il faut relier le contexte à des transactions réellement achevées, vérifier les frontières et l'absence d'impact sur les temps de polling. Ne pas déclencher l'ancien workflow de publication 206 en modifiant ses chemins surveillés sans préparer une CI de test.
