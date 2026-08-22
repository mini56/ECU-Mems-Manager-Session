# RAPPORT COMPLET DE REPRISE — ACTIVATION DU PROTOCOLE MEMS 1.9

**Projet : ECU MEMS Manager**  
**Dépôt : `mini56/ECU-Mems-Manager-Session`**  
**Branche de travail : `ui-rebuild` uniquement**  
**Date : 22 août 2026**  
**But du document : permettre une reprise complète du travail après une coupure de conversation, sans perdre les recherches, décisions, règles ni l’état technique atteint.**

---

# 1. ÉTAT DE RÉFÉRENCE AU MOMENT DU RAPPORT

Le travail actuel concerne l’activation et la validation réelle du support **Rover MEMS 1.9 MPI** dans ECU MEMS Manager, tout en préservant intégralement le fonctionnement existant des ECU SPI/MEMS déjà pris en charge.

Dernier HEAD au moment de la rédaction :

`de40f276523eeb896707b660d5db74a3ca235302`  
Commit : **Guide the MEMS 1.9 validation test**

Parent direct :

`48f5a9eda2cc0eecefe9eae5ba1d043d29449386`  
Commit : **Add passive MEMS 1.9 ECU test recorder**

Ce parent correspond au **build #752 explicitement confirmé vert**.

La capture utilisateur fournie après `de40f276...` montre que le nouvel onglet **Test ECU 1.9** s’affiche désormais correctement :

- titre correct ;
- texte d’aide visible ;
- état général visible ;
- consigne courante visible ;
- bouton **Démarrer le test** ;
- bouton **Arrêter le test** ;
- accès au dossier de journal ;
- liste complète des étapes de validation ;
- interface cohérente avec le style sombre actuel.

Le problème précédent de collision des traductions `7930–7933` avec l’onglet Injection a été corrigé en utilisant une plage dédiée `19000+` pour le test 1.9.

---

# 2. RÈGLES ABSOLUES DU PROJET

Ces règles doivent être respectées lors de toute reprise :

1. **Ne jamais changer de branche.** La branche de travail est `ui-rebuild`.
2. **Compilation uniquement via GitHub Actions.** L’utilisateur n’utilise pas Qt Creator.
3. **Ne modifier que ce qui est demandé explicitement.**
4. **Ne pas refaire l’interface graphique globale.** Le style dark actuel est conservé.
5. **Tout doit rester responsive.**
6. **Aucune régression SPI n’est acceptable.**
7. Ne jamais confondre :
   - information recherchée/documentée ;
   - fonction implémentée ;
   - compilation CI réussie ;
   - validation sur ECU réel.
8. Une cartographie RAM injection ne doit être utilisée que pour un **firmware exact et validé**.
9. Ne jamais déduire une cartographie uniquement parce que deux noms de firmware se ressemblent.
10. Si le firmware n’est pas cartographié, la connexion normale et les mesures normales doivent continuer à fonctionner ; seule la lecture injection RAM mappée doit être indisponible.
11. `memsinterface.h` doit toujours conserver :

   `void onProtocolCommandRequested(quint8 command);`

12. `MainWindow::onProtocolResponse(quint8, QByteArray)` doit rester implémenté.
13. La correction du ralenti chaud ne doit jamais être figée à `-3`. Elle doit utiliser la correction configurée dans l’onglet Réglage.
14. Les DLL de packaging déjà corrigées ne doivent pas être retirées :
   - `Qt5SerialPort.dll`
   - `Qt5Charts.dll`
15. Les fonctions SPI existantes doivent rester utilisables même après ajout du MPI 1.9.

---

# 3. POINT FONDAMENTAL : PASSAGE SPI → MPI 1.9

Le passage au MEMS 1.9 n’est **pas** un simple changement de protocole sur le même calculateur.

La situation réelle est :

**ECU SPI branché → déconnexion physique → ECU SPI retiré → ECU MPI MEMS 1.9 branché → contact → nouvelle connexion MEMS 1.9.**

Le logiciel ne doit donc jamais être conçu comme s’il devait convertir une session SPI active en session MPI sur le même ECU.

Le principe futur doit être :

- ECU SPI détecté → interface SPI actuelle inchangée ;
- ECU MPI MEMS 1.9 détecté → interface et fonctions adaptées au MPI.

Cette séparation est essentielle pour éviter toute régression.

---

# 4. CONNEXION MEMS 1.9 DÉJÀ IMPLÉMENTÉE

La connexion MEMS 1.9 est déjà codée. Le test réel n’a pas pour but de redécouvrir le protocole mais de **valider l’implémentation existante sur un vrai ECU**.

Fichier principal :

`memsinterface_dispatch.cpp`

## 4.1 Stratégie de connexion

`connectToECU()` procède ainsi :

1. énumération des interfaces série détectées ;
2. essai du chemin ROSCO historique sur toutes les interfaces ;
3. si un ECU historique répond, le comportement MEMS 1.3 / 1.6 est conservé ;
4. si aucun ECU ne répond au chemin historique, tentative de réveil MEMS 1.9 ;
5. après handshake 1.9 valide, reprise du chemin série historique à 9600 bauds ;
6. émission de `serialInterfaceDetected(..., "MEMS 1.9 K-Line")` sur succès.

Ce choix protège le comportement des ECU SPI existants.

## 4.2 Initialisation lente ISO 9141 MEMS 1.9

La séquence implémentée est :

- port série configuré à 9600 bauds ;
- 8 bits ;
- pas de parité ;
- 1 stop bit ;
- pas de flow control ;
- slow-init ISO 9141 à **5 bauds** ;
- adresse ECU : **`0x16`** ;
- durée d’un bit à 5 bauds : **200 ms** ;
- ligne K pilotée via BREAK ;
- attente du synchronisme ECU **`0x55`** ;
- lecture des deux key bytes ;
- envoi du complément du deuxième key byte ;
- tolérance d’un éventuel écho du câble K-Line ;
- recherche de la confirmation ECU **`0xE9`**, complément de l’adresse `0x16` ;
- fermeture de la phase wake-up ;
- reprise de la connexion série normale.

Le code n’impose pas une paire de key bytes spécifique ; il impose le `0x55`, récupère les key bytes et effectue l’opération ISO 9141 attendue.

**Si ce chemin fonctionne sur le vrai ECU, ne pas le modifier ensuite sans raison mesurée.**

---

# 5. CÂBLE, PORT COM ET DÉTECTION

Le programme possède déjà une détection des adaptateurs série dans `serialadapterdetector.h`.

Informations récupérées :

- nom du port, par exemple `COM4` ;
- nom Qt du port ;
- chemin périphérique ;
- famille de l’adaptateur ;
- description ;
- fabricant ;
- VID/PID USB ;
- état de détection système.

Familles identifiées :

- FTDI / FT232 ;
- Prolific / PL2303 ;
- WCH CH340 / CH341 ;
- Silicon Labs CP210x ;
- USB Serial générique.

Un câble portant la mention **Proscan-Automotive** a été évoqué.

Important : une puce FT232 seule ne garantit pas que l’électronique du câble soit une véritable interface KKL/K-Line adaptée. Pour MEMS 1.9, il faut une interface électrique ISO9141/K-Line correcte.

Le problème d’affichage instantané du COM dans la barre supérieure est un sujet séparé du protocole 1.9. Il ne doit pas être mélangé à la validation actuelle.

---

# 6. COMMANDES ECU DÉJÀ EXPOSÉES

L’onglet ECU/ROSCO permet déjà les commandes de lecture/session utiles :

- `D0` — identification ECU ;
- `D1` — firmware / identifiant étendu ;
- `D2` — statut sécurité ;
- `F0` — statut de session ;
- `F2` — mode 6 ;
- `F3` — mode 4 ;
- `F4` — session normale ;
- `F5` — mode 3.

`D3` reste volontairement désactivé car il concerne le recodage.

Exemples déjà observés pendant les recherches/tests antérieurs :

- `D0 → D0 98 00 02 02`
- `D1 → AANMP002`
- `D2 → 00 01`
- `F0 → 50`
- `F4 → 00`

Dans le test 1.9, le décodage de `F0` est :

- `0x14` → `mode3`
- `0x1E` → `injection`
- `0x50` → `diagnostic`

---

# 7. ARCHITECTURE DE LECTURE INJECTION

La recherche a établi que le temps d’injection final ne doit **pas** être calculé à partir du polling diagnostic normal `0x7D / 0x80`.

Ces trames normales exposent différentes données moteur, mais elles ne fournissent pas directement les mots RAM nécessaires au calcul exact du temps d’injection final.

Architecture retenue :

1. connexion normale ;
2. lecture D1 du firmware ;
3. recherche d’un profil injection exact ;
4. si profil validé : entrée en Mode 4 ;
5. lecture RAM directe ;
6. calcul du temps d’injection ;
7. affichage ;
8. retour automatique à la session diagnostic en quittant le mode injection.

Un firmware inconnu doit continuer à fonctionner normalement, sans lecture RAM injection mappée.

---

# 8. CARTOGRAPHIES INJECTION MEMS 1.9 INTÉGRÉES

Au total, **164 firmwares MEMS 1.9 corrélés ont été intégrés** :

- groupe principal : 100 ;
- batch 5 : 25 ;
- batch 6 : 25 ;
- batch 7 : 13 ;
- profil spécial TBP : 1.

## 8.1 Profil MEMS 1.9 normal

Pour la majorité des profils MEMS 1.9 validés :

- RAM scheduler : **`0x004E`** ;
- nombre scheduler : **8** ;
- RAM de compensation exacte selon firmware :
  - `0x0148`, ou
  - `0x014A`, ou
  - `0x014C` ;
- facteur principal : **0,008 ms/raw**.

## 8.2 Famille KLH4 / KLQ4

Profil de sortie doublée :

- même logique générale ;
- facteur de sortie : **0,016 ms/raw** ;
- gestion du chemin grand pulse dans le calcul avant doublement.

## 8.3 Profil spécial TBP7F015

Profil indépendant :

- scheduler RAM : **`0x0056`** ;
- scheduler count : 8 ;
- compensation : **`0x0040`** ;
- centre de compensation : **`0x8000`** ;
- base : **0,008 ms/raw** ;
- compensation : **0,001 ms/raw**.

Calcul :

`compensation = transientRaw - 0x8000`

`outputRaw = baseRaw * 8 + compensation`

`finalMs = outputRaw * 0.001`

Ce profil ne doit pas être généralisé aux autres firmwares.

---

# 9. ONGLET « TEST ECU 1.9 »

L’onglet a été créé pour sécuriser la seule occasion de test réel et éviter de perdre des informations.

Fichier :

`mems19testtab.h`

Objet :

`mems19_test_tab`

Le test ne remplace pas les autres fonctions du programme. Il utilise les mécanismes existants et journalise la validation.

## 9.1 État visuel actuel

La capture utilisateur du 22 août 2026 confirme que l’onglet affiche correctement :

- titre **Validation MEMS 1.9 / KKL** ;
- description du test ;
- rappel qu’il s’agit d’une procédure guidée ;
- état général ;
- consigne actuelle ;
- tableau des étapes ;
- bouton **Démarrer le test** ;
- bouton **Arrêter le test** ;
- bouton pour ouvrir le dossier du journal ;
- zone d’événements du test.

## 9.2 Étapes suivies par l’onglet

La procédure guidée est unique.

Elle couvre :

1. connexion et identification de l’interface KKL / MEMS 1.9 ;
2. récupération D0 ;
3. lecture firmware D1 ;
4. lecture sécurité D2 ;
5. lecture session F0 ;
6. validation du polling normal ;
7. attente du moteur tournant ;
8. possibilité de poursuivre sans moteur en marquant le test partiel ;
9. passage en Mode 4 pour lecture injection ;
10. acquisition de plusieurs échantillons injection ;
11. sortie du Mode 4 ;
12. retour en diagnostic ;
13. déconnexion ;
14. reconnexion MEMS 1.9 ;
15. résultat final.

Résultats possibles :

- **TEST TERMINÉ — TOUT EST OK** ;
- **PARTIEL** ;
- **ÉCHEC**.

Il n’existe pas une série de tests différents cachés après celui-ci.

**Si ce test passe entièrement, la validation protocole 1.9 prévue est terminée.**

Si une étape échoue, on corrige cette étape précise et on relance **le même test**.

---

# 10. ENCHAÎNEMENT DE PLUSIEURS PASSAGES

Le même test peut être relancé plusieurs fois.

À chaque nouveau lancement :

- l’état du test est remis à zéro ;
- un nouveau jeu de journaux est créé ;
- les journaux précédents sont conservés ;
- la procédure repart depuis la connexion.

L’intérêt principal d’un second passage est la confirmation ou la reprise après correction. Un premier passage complet et réussi suffit à la validation prévue.

---

# 11. JOURNALISATION AUTOMATIQUE

Répertoire local :

`Documents/ECU Mems Manager/MEMS19-tests`

Chaque session crée trois fichiers distincts :

- `MEMS19_test_<timestamp>_events.log`
- `MEMS19_test_<timestamp>_data.csv`
- `MEMS19_test_<timestamp>_injection.csv`

## 11.1 Journal événements

Contient notamment :

- version logiciel ;
- build ;
- SHA du commit ;
- date/heure ;
- interface détectée ;
- protocole détecté ;
- connexion/déconnexion ;
- erreurs de lecture ;
- erreurs d’envoi ;
- réponses protocolaires ;
- changements de mode ;
- étapes du test ;
- résultat final.

## 11.2 CSV données normales

Enregistre les échantillons déjà produits par le polling normal, sans créer de polling supplémentaire.

Colonnes principales :

- RPM ;
- température eau ;
- température air ;
- MAP ;
- batterie ;
- papillon ;
- idle switch ;
- position IAC ;
- erreur ralenti ;
- avance ;
- dwell ;
- lambda ;
- closed loop ;
- trims ;
- DTC ;
- conversions physiques utiles.

## 11.3 CSV injection

Enregistre les échantillons `injectionLiveSample` :

- timestamp ;
- read mode ;
- firmware ;
- injection finale ms ;
- base ms ;
- compensation/transitoire ms ;
- base raw ;
- source raw ;
- compteur.

---

# 12. INTÉGRATION DU NOUVEL ONGLET SANS RÉGRESSION

Le nouvel onglet est ajouté en dernière position logique de la navigation afin de ne pas déplacer les indices physiques des onglets existants.

`navigationorderpatch.cpp` gère désormais 13 onglets.

Ordre logique :

0. Aperçu
1. Injection
2. Réglages
3. Actionneurs
4. Erreurs
5. Diagnostic automatique
6. Analyse
7. Toutes les mesures
8. ECU/ROSCO
9. Toutes les données
10. Base de données
11. Interactif
12. Test ECU 1.9

Le code de navigation existant n’a pas été réordonné arbitrairement.

---

# 13. CORRECTION DE LA COLLISION DE TRADUCTIONS

La première version du test utilisait les clés `7930–7954`.

Problème : l’onglet Injection utilisait déjà notamment :

- `7930` → Système connecté ;
- `7931` → Injection : %1 ms ;
- `7932` → Firmware : %1 ;
- `7933` → Fichier CSV/TXT.

Cela provoquait l’affichage erroné vu sur la première capture du test 1.9.

La correction a déplacé les textes du test vers une plage dédiée `19000+`, dans les six langues :

- français ;
- anglais ;
- espagnol ;
- italien ;
- portugais ;
- allemand.

---

# 14. COMMITS IMPORTANTS À CONSERVER COMME POINTS DE REPÈRE

## Build #752 — dernier explicitement confirmé vert avant test guidé

`48f5a9eda2cc0eecefe9eae5ba1d043d29449386`

Message :

**Add passive MEMS 1.9 ECU test recorder**

## Test guidé

`de40f276523eeb896707b660d5db74a3ca235302`

Message :

**Guide the MEMS 1.9 validation test**

Diff volontairement limité à :

- `mems19testtab.h` ;
- les six fichiers de traduction `*_mems19test.json`.

Aucune modification dans :

- `mainwindow.cpp` ;
- `mainwindow.h` ;
- `memsinterface.cpp` ;
- `memsinterface_dispatch.cpp` ;
- calculs injection mappés ;
- CMake ;
- workflow packaging.

## Autres points importants

`4928e76016e1f48a42407fc67d2ece7a975f6744` — Enable MEMS 1.9 injection profiles batch 7

`ac6b243ff7c88e8d1c98d06a033e4065e65147b0` — Add special MEMS 1.9 TBP7F015 injection profile

`7ef62e399a006f92b7cc6d0c97cc63f9fcb94d4d` — Preserve workflow formatting

---

# 15. APRÈS VALIDATION RÉELLE : ÉVOLUTION MPI DES ONGLETS

Une fois le vrai MEMS 1.9 validé, le travail suivant ne consistera pas à remplacer l’interface SPI par une interface MPI unique.

Il faudra mettre en place une adaptation dynamique selon le type d’ECU connecté.

Principe :

**SPI → affichage SPI actuel exactement conservé**  
**MPI MEMS 1.9 → affichage/fonctions propres au MPI**

Les zones potentiellement concernées seront :

- Aperçu ;
- Injection ;
- Réglage ;
- Actionneurs ;
- Erreurs ;
- Diagnostic automatique ;
- Analyse ;
- Toutes les mesures ;
- ECU/ROSCO ;
- Toutes les données.

Le MPI peut apporter :

- plusieurs voies d’injection ;
- davantage de sondes ;
- davantage de mesures ;
- davantage de cadrans utiles ;
- davantage de tests actionneurs ;
- davantage d’états ECU.

L’utilisateur a rappelé que le MPI concerné possède **deux injecteurs/voies d’injection à prendre en compte**.

Il ne faut cependant **rien inventer** : les nouveaux cadrans, sondes, tests et valeurs doivent être basés sur ce que le vrai ECU renvoie et sur les données documentées/validées.

---

# 16. CE QU’IL FAUT FAIRE LE JOUR DU TEST

Préparation :

1. conserver le programme dans l’état actuel ;
2. brancher le câble KKL adapté ;
3. retirer l’ECU SPI ;
4. brancher physiquement l’ECU MPI MEMS 1.9 ;
5. mettre le contact ;
6. ouvrir ECU MEMS Manager ;
7. ouvrir **Test ECU 1.9** ;
8. cliquer **Démarrer le test** ;
9. suivre uniquement la consigne affichée à l’écran ;
10. démarrer le moteur lorsque l’onglet le demande si possible ;
11. attendre le résultat final.

À la fin :

- si **TOUT EST OK** → la validation prévue du protocole 1.9 est terminée ;
- si **PARTIEL** → exploiter les journaux pour identifier ce qui manque ;
- si **ÉCHEC** → corriger précisément l’étape fautive et relancer la même procédure.

Après le test, conserver les trois fichiers de journal générés.

---

# 17. CE QU’IL NE FAUT PAS FAIRE LE JOUR DU TEST

- Ne pas modifier le code avant d’avoir lu les journaux si une anomalie apparaît.
- Ne pas créer un nouveau protocole parallèle sans preuve.
- Ne pas modifier la connexion 1.3/1.6 pour forcer le 1.9.
- Ne pas supprimer ou remplacer les fonctions SPI.
- Ne pas déduire une cartographie injection approximative.
- Ne pas transformer un test partiel en validation complète.
- Ne pas multiplier les tests différents : il existe une seule procédure guidée.

---

# 18. POINTS CONNUS HORS DU TEST 1.9

Ces sujets existent mais ne doivent pas être mélangés avec la validation 1.9 actuelle :

## Affichage du port COM

Un problème antérieur concernait la barre supérieure : le numéro du port attendu sous « Port » n’était pas affiché. La détection série existe, mais l’actualisation visuelle immédiate/hotplug n’est pas considérée comme définitivement résolue dans ce rapport.

## Détection hotplug

La détection des ports existe lors de la connexion. Un watcher USB continu permettant de mettre à jour l’affichage dès l’insertion du câble n’est pas établi comme fonction finalisée.

Ces deux sujets ne doivent pas entraîner une modification du protocole 1.9 lui-même.

---

# 19. FICHIERS DE RECHERCHE / MATÉRIEL TECHNIQUE DISPONIBLES

Des fichiers issus de MEMSTools ont été fournis dans les échanges :

- `Rover K-Series MKC-EU2 VVC.mm2`
- `Rover K-Series NNN-EU3 MPI.mm3`
- `Rover K-Series NNN-EU3 VVC.mm3`
- `Rover Mini MKC-EU2 MPI.mm2`
- `MEMSTools(1).zip`

Ils servent de matière de recherche lorsque des informations de protocole, RAM ou firmware doivent encore être vérifiées.

Ne pas supposer qu’un chemin local temporaire existe encore après une nouvelle session : demander ou rechercher le fichier dans les uploads si nécessaire.

---

# 20. SYNTHÈSE DE REPRISE IMMÉDIATE

Si ce document est lu après une coupure de conversation, reprendre exactement ainsi :

1. **ne pas revenir à une ancienne branche ou un ancien build ;**
2. vérifier le HEAD actuel de `ui-rebuild` ;
3. considérer `de40f276523eeb896707b660d5db74a3ca235302` comme le commit de référence du test guidé décrit ici, sauf commit plus récent explicitement validé ensuite ;
4. ne pas toucher au protocole avant le test réel ;
5. effectuer la procédure unique **Test ECU 1.9** sur le véritable ECU MPI ;
6. récupérer les journaux ;
7. si tout est OK, passer à la phase suivante : **adaptation dynamique des onglets MPI tout en laissant le SPI intact** ;
8. si une étape échoue, corriger seulement cette étape et relancer la même procédure.

---

# 21. CONCLUSION

Le projet n’est plus au stade de la simple recherche théorique du protocole MEMS 1.9.

À ce stade :

- le chemin de connexion K-Line/ISO9141 1.9 est codé ;
- l’identification D0/D1/D2/F0 est prise en compte ;
- les lectures normales sont conservées ;
- les profils injection RAM MEMS 1.9 corrélés sont intégrés ;
- le Mode 4 injection est implémenté ;
- le retour diagnostic est implémenté ;
- la reconnexion est incluse dans le test ;
- la journalisation détaillée est intégrée ;
- l’onglet de test est maintenant guidé et visuellement exploitable.

**La prochaine opération déterminante est donc le test réel sur l’ECU MPI MEMS 1.9.**

Une réussite complète de cette procédure clôt la validation prévue du protocole 1.9. La phase suivante sera l’adaptation fonctionnelle de l’interface au MPI, basée uniquement sur les capacités réellement observées, sans régression du mode SPI.
