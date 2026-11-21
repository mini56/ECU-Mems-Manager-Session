# Rapport 28 — SESSION 189, première implémentation de lecture Lucas 5AS

**21 novembre 2026.** Projet ECU-Mems-Manager-Session, branche unique `fix-documentary-additive-session-161-20261118`.

## Base documentaire et limites

La séquence retenue pour **le diagnostic normal** vient de l'audit de l'archive MEMSTools(5).zip d'Andrew Revill (rapport 27) et de RoverMEMS : `1C` à 5 bauds, réponse `55 83 76`, demande d'état `83` à 9600 bauds, réponse dont le premier octet représente la longueur complète. Aucun ACK ISO 9141 supplémentaire n'a été identifié dans la routine SlowInit du binaire examiné, sans garantir toutes les variantes.

**Cette phase est de la lecture seule** : ne pas confondre le mode de diagnostic avec EEPROM, PLIP/apprentissage, gestion EKA ou programmation. La validation physique avec un 5AS et un câble KKL adapté reste à réaliser.

## Sources modifiées pour la préparation de la version 189

- `lucas5asprotocol.h` : vecteur à 5 bauds, reconnaissance du réveil normal, décodage prudent des entrées et du statut de longueur déclarée.
- `lucas5asprotocol_test.cpp` : tests synthétiques du réveil et de la trame complète, rejet des paquets corrompus/tronqués et filtrage des échos.
- **Nouveau `lucas5asreader.h`** : session Qt `QSerialPort`, réveil à 5 bauds par BREAK, lecture `83`, conservation d'une trace TX/RX et gestion d'annulation. Travailleur `Lucas5AsWorker` séparé du RC5.
- `othercontrolunitstab.h` : activation de la deuxième colonne Antidémarrage avec **Connecter / Déconnecter / Lire les états**, tableau et journal défilants ; les colonnes RC5 et programmation des clés sont conservées.
- `CMakeLists.txt` : déclaration du worker 5AS dans `SOURCES` pour AUTOMOC ; autotest indépendant enregistré.
- `translations/{fr,en,de,es,it,pt}/*_other_ecus.json` : clés 16900–16923 dans les six langues.
- `tools/audit_session_ui_i18n.py` : contrôle de ces traductions et du module Lucas.
- `uishellsmoketest.cpp` : 10 boutons attendus (4 RC5, 3 lecture Lucas, 3 aides câble), état initial sans connexion automatique, troisième colonne réservée, scroll et journal.
- `.github/workflows/session-189-lucas5as-readonly-setup.yml` : **une seule compilation Windows prévue**, autotests de lecture, contrôles des anciennes fonctions et livraison Setup uniquement si tous les contrôles réussissent.

## Séquence d'utilisation attendue

1. Brancher une interface compatible et sélectionner simplement **Connecter** : Manager recherche lui-même les ports COM disponibles.
2. Lorsqu'une réponse réelle `55 83 76` est reçue, Manager émet `83` et attend un paquet de réponse complet. Le port détecté seul ne suffit pas à annoncer « Connecté ».
3. Afficher la description ASCII de modèle, le numéro de série en octets, les bits 7 bruts de contact/portes/capot/coffre éventuel et les quatre compteurs PLIP bruts. Ne pas supposer de polarité « ouvert/fermé » sans mesure réelle.
4. **Lire les états** répète seulement la demande normale `83`. **Déconnecter** libère le port.
5. Aucun traitement de clés, EEPROM ou défauts SRS ne doit être exécuté depuis la deuxième colonne.

## Vérifications

Les vérifications de structure et de traduction du workflow sont déjà passées. **Compilation Windows et test d'installation 189 : en cours au moment où ce rapport est rédigé ; ne pas annoncer le Setup comme validé avant succès du RUN.**

Workflow de contrôle unique : https://github.com/mini56/ECU-Mems-Manager-Session/actions/runs/44910030425

**Les essais sur un véritable Lucas 5AS n'ont pas été effectués**. La future vérification matérielle doit contrôler les délais de la liaison, les octets réellement retournés et les variantes de câble FT232R/CH340G, sans blocage artificiel des modèles reconnus.
