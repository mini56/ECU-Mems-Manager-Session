# Rapport 25 — Recherche ciblée sur l'onglet Log de Lucas 5AS Tools (Andrew Revill)

**21 novembre 2026 — ECU MEMS Manager SESSION.**  
Branche unique : `fix-documentary-additive-session-161-20261118`. **Recherche uniquement : aucun changement de code, aucune compilation, aucune connexion matérielle réalisée ici.**

## 1. Sources primaires consultées et visualisées

- Page Andrew Revill : https://andrewrevill.co.uk/MEMS3LucasImmobiliserSupported.htm (Lucas 5AS Tools version 7.91, programme autonome, About 5AS, Log, Live Diagnostics et fabrication du câble).
- **Capture originale de l'onglet Log 5AS**, consultée : https://andrewrevill.co.uk/MEMS3LucasImmobiliserSupported_files/image012.png
- **Capture originale de l'identification About 5AS**, consultée : https://andrewrevill.co.uk/MEMS3LucasImmobiliserSupported_files/image006.png
- **Capture originale Live Diagnostics**, consultée : https://andrewrevill.co.uk/MEMS3LucasImmobiliserSupported_files/image001.png
- **Documentation d'une AUTRE application de la même suite, MEMS Mapper EU3, section The Log Tab** : https://andrewrevill.co.uk/MEMSMapperEU3.htm
- Présentation par l'auteur sur Caterham / Lotus Seven Club (10 juillet 2024) : https://www.caterhamlotus7.club/forums/topic/275971-mems3-mapper-support-for-the-lucas-5as-immobiliser-key-fob-pairing-etc/

## 2. Ce que montre exactement l'image012.png (la bonne piste, mais pas encore le bon extrait)

La capture originale porte le titre **Lucas 5AS Tools Version 7.91 Release (64-Bit Version)**, indique **COM9** et montre l'onglet **Log**.

Chaque ligne affichée comporte :
- un nombre temporel croissant (ex. 527751 puis 527890) ;
- une direction **Write** ou **Read** ;
- la vitesse **9600** ;
- un libellé de fonction **Eeprom Data** ;
- des octets de requête/réponse en hexadécimal.

Une barre de défilement verticale apparaît sur le côté ; l'exemple se situe vers la fin d'une longue série. Les lignes photographiées concernent **des transactions EEPROM**, et non le mode diagnostic normal. Leurs commandes hexadécimales n'autorisent donc **aucune conclusion** sur l'acquittement du réveil 5AS.

**La capture NE montre PAS :** l'envoi initial du `1C` à 5 bauds, la réponse `55 83 76`, la présence ou l'absence d'un ACK envoyé ensuite par le PC, puis la première demande normale `83`. Elle ne permet donc pas encore de résoudre le point manquant dans Manager.

L'auteur confirme textuellement que **l'onglet Log de Lucas 5AS Tools présente les détails des communications PC ↔ 5AS**, sans préciser sur cette page si toutes les étapes d'initialisation à 5 bauds y figurent.

## 3. Ce que les autres captures prouvent

- **About 5AS — image006.png :** le logiciel se connecte et affiche le modèle et le numéro de série du boîtier, sans exposer les échanges bruts.
- **Live Diagnostics — image001.png :** l'outil affiche de nombreuses entrées (capot, portes, coffre, contact, état de la clé/télécommande) ; la capture n'est pas une preuve de séquence du protocole. Certains champs privés du boîtier y figurent et ne sont pas nécessaires à l'étude du handshake.

**Recommandation de recherche :** demander les **premières lignes du Log immédiatement après le clic About 5AS**, puis éventuellement une courte lecture **Live Diagnostics**, plutôt qu'une capture après lecture EEPROM.

## 4. Fonctionnalités du journal vérifiées dans MEMS Mapper EU3 (application distincte)

Dans https://andrewrevill.co.uk/MEMSMapperEU3.htm, la section **The Log Tab** décrit :
- horodatage en **millisecondes** ;
- direction **Write / Echo / Read** ;
- vitesse de communication, description du message, octets effectivement transmis ;
- possibilités de **Copy**, **Clear** et **Save the log to a text file**.

Pour **MEMS Mapper EU3**, le développeur explique qu'une communication K-Line monofilaire peut produire la séquence **Write → Echo → Read**, et que l'écho provient de l'électronique du câble, pas du calculateur.

**NE PAS transposer ces fonctions comme acquises à Lucas 5AS Tools.** Cette seconde application est séparée. Elle utilise la **ligne K pour transmettre et la ligne L inversée pour recevoir**, et la capture 5AS montre **Write/Read**, sans ligne Echo visible. Les fonctions Copier/Vider/Enregistrer le journal **ne sont explicitement documentées que pour MEMS Mapper EU3**, même si une fonction équivalente est possible pour 5AS Tools. La signification et la résolution exacte du compteur temporel du journal 5AS restent à vérifier.

## 5. Recherche d'autres logs et du logiciel

Les recherches ciblées sur les termes « Lucas 5AS Tools Log », « About 5AS Log », « 55 83 76 », « 5AS wake » et les publications Andrew Revill/Caterham n'ont pas livré de **capture publique confirmée du début d'un dialogue diagnostic normal**.

L'auteur publie la distribution `https://andrewrevill.co.uk/Downloads/MEMSTools.zip`, mais le lecteur web ne sait pas ouvrir les archives ZIP binaires et la tentative de téléchargement dans le conteneur a échoué. **Il ne faut pas prétendre avoir analysé ni exécuté le binaire**, ni avoir vérifié son interface d'export.

## 6. La procédure de collecte à rechercher

Avec un boîtier Lucas 5AS légitimement accessible, alimenté, et un câble adapté :
1. Lancer **Lucas 5AS Tools — For Lucas 5AS Immobilisers Only.exe** depuis la distribution de l'auteur ; sélectionner le port **COM ou FTDI**.
2. Ouvrir l'onglet **Log** pour voir s'il conserve les événements depuis le début.
3. Cliquer uniquement sur **About 5AS** (identification non modifiante).
4. Revenir dans **Log**, remonter **tout en haut** et conserver les échanges dans l'ordre, avec éventuelles lignes préliminaires de BREAK, vitesses, octets, sens et valeurs temporelles.
5. Si les premières étapes ne sont pas visibles, essayer une lecture courte dans **Live Diagnostics**, arrêter avec **Done**, et examiner le Log (sans lancer les opérations d'écriture).
6. Vérifier SI cette version de 5AS Tools permet de copier ou enregistrer le journal ; si elle ne le permet pas, une capture des premières lignes pourra servir.
7. Pour partager le journal, **masquer les codes EKA, EMS, identifiants de télécommandes et données personnelles**, inutiles pour l'analyse du démarrage.

**Ne pas utiliser pendant ce test :** `Read` EEPROM, `Write`, `Program Key Fobs`, `EKA Code`, `EMS Code`, `Test Outputs` ou les fonctions modifiantes.

## 7. Question technique précise à adresser à Andrew Revill si aucune trace publique n'est disponible

> Could you provide the **first lines of a read-only Lucas 5AS Tools Log** after pressing **About 5AS**, from the `0x1C` five-baud wake to the reply `55 83 76` and the first normal data request `0x83`? Does the host send an ACK after `55 83 76`, and what are the byte values and expected timings? Does the 5AS Tools Log record slow wake-up events? Can it be copied or saved to a text file? No EEPROM contents, EKA or key-fob data are needed.

Il s'agit d'une **question préparée pour la recherche**, pas d'un courriel envoyé.

## 8. Conclusion pour Manager

**Acquis :** le Lucas 5AS Tools d'Andrew Revill montre bien un journal de communication horodaté avec directions, débits et octets. La fonction **About 5AS** existe et permet de déclencher une identification simple.

**Non acquis :** la trame complète entre `55 83 76` et `83`, la présence d'un ACK et ses timings, les lignes de réveil dans l'historique et le mécanisme d'export du journal de **cette application 5AS**.

**Prochaine preuve recherchée : une vraie capture du haut du journal d'About 5AS**, ou une confirmation écrite du développeur. Tant que cette preuve n'existe pas, ne pas copier par hypothèse la confirmation ISO 9141 du moteur MEMS 1.9.

**Aucune modification du code ni nouvelle compilation pour ce rapport.**
