# Étude complémentaire — télécommandes de portail 433 MHz et Lucas 5AS

**21 novembre 2026 — documentation uniquement.**  
**Projet :** ECU MEMS Manager SESSION, troisième colonne « Programmation des clés ».  
**Aucune modification d'interface, de protocole ou de fonction d'appairage. Aucun nouveau run.**

## Question

Une télécommande de portail à 433 MHz, parfois duplicable avec un boîtier universel, pourrait-elle servir de base pour reproduire une télécommande de Rover Mini utilisant le Lucas 5AS ?

**Réponse : la fréquence commune n'établit pas la compatibilité.** La fréquence est une caractéristique de transport radio ; il faut aussi que l'encodage, la modulation, le protocole, l'identité et la synchronisation soient compatibles avec le récepteur du 5AS.

## Informations confirmées

1. **Fréquence** : de nombreux télécommandes Lucas 5AS européennes fonctionnent autour de **433,92 MHz** ; des variantes destinées à d'autres marchés utilisent **315 MHz**. Les 315 MHz ne sont pas compatibles par une simple reprogrammation d'un émetteur radio 433 MHz.  
   Sources : https://donnlee.com/2007/08/30/rover-lucas-5as-immobilizer-a-alarm-system-info-rover-mini-cooper/ et https://store.rover-coupe.com/services/56-lucas-5as-ecu-fob-postal-matching-service.html

2. **Code variable** : le Lucas 5AS utilise un mécanisme de code tournant/synchronisation. Un signal radio capturé à un instant donné ne constitue pas une copie durable de l'identité et de l'état de synchronisation de la télécommande.  
   Source : https://donnlee.com/2007/08/30/rover-lucas-5as-immobilizer-a-alarm-system-info-rover-mini-cooper/

3. **Portails électriques** : certains anciens systèmes à **code fixe** acceptent une télécommande duplicatrice ; d'autres, également à 433,92 MHz, utilisent un **code tournant** et doivent être associés au récepteur. Deux appareils réglés sur 433,92 MHz ne sont donc pas nécessairement interopérables.  
   Source : https://www.1control.eu/blog/en/fixed-code-vs-rolling-code

4. **Le Lucas 5AS connaît les identités des PLIP** : la documentation professionnelle Faultmate SM034 indique que le boîtier peut mémoriser jusqu'à **quatre télécommandes**, et que l'apprentissage est une opération distincte de la reproduction radio. Elle documente la lecture d'informations associées aux télécommandes, notamment leur code d'identification.  
   Source : https://www.blackbox-solutions.com/site/product?sku=SM034

5. **La voie de travail la plus fiable est le diagnostic du 5AS** : RoverMEMS documente la synchronisation de télécommandes via l'interface diagnostique et précise que cette opération n'est **pas** le jumelage du 5AS à l'ECU moteur.  
   Source : https://rovermems.com/5as/index.html

6. **Les télécommandes déjà associées peuvent être perdues lors d'un nouvel apprentissage** : l'outil de diagnostic d'Andrew Revill précise qu'une procédure de programmation remplace les associations précédentes. Il faut identifier toutes les télécommandes légitimes avant d'engager une telle opération.  
   Source : https://andrewrevill.co.uk/MEMS3LucasImmobiliserSupported.htm

## Conséquences pour le futur bloc « Programmation des clés »

- Conserver les trois notions **télécommande PLIP**, **antidémarrage 5AS** et **clé mécanique** comme objets différents.
- Documenter la compatibilité d'une télécommande de remplacement **par modèle de PLIP, version du 5AS et marché 433/315 MHz**, et non par la seule fréquence.
- Prévoir une consultation **en lecture** du nombre de PLIP appris et de l'état de synchronisation si la variante de calculateur le permet.
- Prévoir ultérieurement, séparément, une fonction d'**apprentissage d'une télécommande compatible** via diagnostic 5AS, avec vérification du nombre et de la validité des télécommandes autorisées après programmation.
- Une télécommande générique de portail à 433,92 MHz **ne peut pas être déclarée compatible avec le 5AS** sans preuve du protocole radio et de son mécanisme de synchronisation. Ne pas présenter un duplicateur ordinaire comme une solution sûre ou universelle.
- Les réécritures de données de sécurité ou de synchronisation ne sont **pas** couvertes par une simple découverte de fréquence : elles nécessitent une conception et une validation spécifiques.
- L'étude ne justifie **aucun changement** du module RC5 de SESSION 187 ; le RC5 Airbag est un calculateur différent du Lucas 5AS.

## Conclusion

**Oui**, 433,92 MHz est une piste radio techniquement intéressante. **Non**, ce seul point commun ne permet pas de copier à l'identique une télécommande Mini 5AS avec un duplicateur de portail. La méthode connue et documentée pour un remplacement fiable est **l'association d'un PLIP compatible avec le 5AS via les fonctions de diagnostic**, une opération que SESSION pourra étudier plus tard dans sa troisième colonne.
