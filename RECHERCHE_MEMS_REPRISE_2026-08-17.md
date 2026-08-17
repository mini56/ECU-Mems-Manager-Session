# Reprise des recherches MEMS — 17 août 2026

Ce fichier est un point de reprise persistant pour éviter toute perte des recherches en cas de coupure ou changement de discussion.

## Cadre impératif

- Branche de travail : `ui-rebuild` uniquement.
- HEAD observé avant création de ce checkpoint : `2f00bfdca2b7d960e1f45dd3e21ddb0346704ee6`.
- Ne jamais changer de branche sans demande explicite de l’utilisateur.
- Référence fonctionnelle du moteur de recherche : build #518 / commit `9fba41d125030616c9eb35eef07a10a00a90e138`.
- Les recherches ci-dessous ne doivent jamais provoquer de régression du moteur de recherche validé #518.
- Pendant le test utilisateur du build en cours, ne pas modifier le code applicatif/la base sans nécessité ; ce fichier de documentation peut servir de sauvegarde de travail.

## État de la base vérifié pendant cette recherche

La base du package du build #533 a été reconstruite localement depuis les mêmes fichiers `qz64` que le build.

Résultat :

- `PRAGMA integrity_check` : `ok` ;
- ECU : **85** ;
- affectations : **140** ;
- commandes protocole : **91**.

Les candidats listés ci-dessous ont donc été comparés à une base réellement reconstruite, pas seulement aux archives Markdown.

## Sources externes effectivement étudiées

### Colin Bourassa — `librosco`

Fichiers/commits importants :

- `src/protocol.c` ;
- `src/rosco.h` ;
- commit `3a39534e6db18cb94daad8609962099dfc025968` — « Names for more fields in data frames 7D and 80 » ;
- commit historique `63b15e35dd32bbf0120f846090ccc39a953bfa04` — ajout de la trame `0x7D`.

### Andrew Jackson — `rosco`

Fichiers/commits importants :

- `commands.go` ;
- `structures.go` ;
- historique des commandes/réglages ;
- commit `aee42148cec12ef652c13d1e6da540c67547ef4f` — fonctions de réglage ;
- commit `08b204871d09a8b637d3b4c9b5979ea8051a86ba` — tests actionneurs/IAC ;
- commit `18ee97650ffde9b58485644103a431cd6ebedb81` — position vilebrequin passée de booléen à `uint8`.

### James Portman — `rover-mems-documentation`

Arborescence inspectée :

- `ECUs/1.3-ECU` ;
- `ECUs/1.6-ECU` ;
- `ECUs/1.x-ECU-all/diagnostics` ;
- documentation câbles 3 broches / 16 broches ;
- documentation immobiliseur 5AS ;
- documents KWP/ISO présents dans le dépôt.

### Blackbox Solutions

Documentation de diagnostic MEMS 1.6 / 1.9 utilisée comme source indépendante pour des valeurs de service, fonctions de diagnostic et signification de certains paramètres. Quand Blackbox ne donne pas l’offset brut, ne pas inventer cet offset.

## Résultats actuellement suffisamment solides

### Trame `0x7D`

Le code original de Colin et son commit 2022 donnent :

- `7D:02` angle papillon : **brut × 0,6** ;
- `7D:04` air/fuel ratio : **brut / 10** ;
- `7D:06` tension lambda : **brut × 5 mV** ;
- `7D:09` état circuit lambda ;
- `7D:0A` boucle fermée ;
- `7D:0B` long-term fuel trim ;
- `7D:0C` short-term fuel trim ;
- `7D:0D` duty cycle canister ;
- `7D:0E` DTC3 ;
- `7D:0F` idle base position ;
- `7D:11` DTC4 ;
- `7D:12` ignition advance 2 / offset selon implémentation ;
- `7D:13` idle speed offset ;
- `7D:14` idle error 2 dans Colin ;
- `7D:16` DTC5 dans l’implémentation Andrew/MemsFCR ;
- `7D:1F` `JackCount` dans l’implémentation Andrew/MemsFCR.

### Point important : lambda

Une ancienne page publique mentionne `0,5 mV/LSB`, mais le code original actuel de Colin applique explicitement `lambda_voltage * 5`, et le commit 2022 documente lui aussi « multiply by 5 for voltage ».

Conclusion de recherche actuelle : **retenir ×5 mV**, tout en conservant la trace du conflit documentaire.

### `7D:1F` — Jack Count

- Andrew/MemsFCR le place à `7D:1F`.
- Blackbox confirme indépendamment la **signification fonctionnelle** du Jack Count : nombre d’interventions/corrections du système de ralenti/stepper, utile pour diagnostiquer stepper, câble/papillon/réglage.
- Blackbox ne confirme pas directement l’offset `1F`.

Conclusion : signification fonctionnelle bien recoupée ; **offset `7D:1F` encore dépendant des implémentations Andrew/MemsFCR**.

### Trame `0x80`

Le code Colin 2022 nomme notamment :

- `80:07` MAP ;
- `80:08` tension batterie /10 ;
- `80:09` TPS ×0,02 V ;
- `80:0F` idle setpoint ;
- `80:10` idle hot, correction `-35` ;
- `80:12` IAC position ;
- `80:13-14` idle error ;
- `80:15` ignition advance offset ;
- `80:16` avance ;
- `80:17-18` coil time ×0,002 ;
- `80:19` `crankshaft_pos`.

Andrew Jackson a explicitement corrigé historiquement `80:19` de booléen vers `uint8` (commit `18ee976...`).

Conclusion : **ne pas réduire `80:19` à un simple booléen sans preuve supplémentaire**.

## Commandes/réglages retrouvés dans les implémentations

Andrew Jackson documente :

- `0x0F` : reset/clear adjustments ;
- `0xFA` : reset ECU / valeurs calculées-apprises ;
- `0xCC` : clear faults ;
- `0xFB` : lecture position IAC ;
- `0x79` / `0x7A` : réglage fuel trim court terme + / - ;
- `0x7B` / `0x7C` : réglage fuel trim long terme + / - ;
- `0x89` / `0x8A` : idle decay + / - ;
- `0x91` / `0x92` : idle speed + / - ;
- `0x93` / `0x94` : ignition advance offset + / - ;
- `0xFD` / `0xFE` : mouvement/réglage IAC suivant l’implémentation.

Ces commandes doivent être comparées à la base existante avant toute insertion et les conflits de sens doivent rester explicitement marqués.

## Actionneurs : prudence obligatoire

Dans `librosco` de Colin, plusieurs commandes sont présentes mais **désactivées par `#if 0` car non testées** : purge valve, O2 heater, boost valve, fan 1, fan 2.

Ne jamais transformer leur présence dans le code en « fonction confirmée » sans autre source/essai.

## DTC : conflit détecté, ne pas importer les masques aveuglément

Les forks/implémentations externes ne sont pas tous cohérents sur certains masques DTC.

Exemple : le code Colin utilise pour la faute potentiomètre papillon un masque différent de certaines structures Andrew.

Règle de reprise :

- ne pas importer les masques DTC d’un fork comme vérité ;
- rechercher une source constructeur / documentation indépendante / plusieurs implémentations concordantes ;
- conserver les conflits dans la base de recherche si utile, mais ne pas les rendre exécutables comme diagnostic certain.

## Seuils/procédures Blackbox trouvés mais à recouper avant intégration

Candidats absents de la base structurée #533 au moment de la comparaison :

- MAP au ralenti : environ **25–40 kPa** ;
- position IAC moteur chaud : environ **10–50 pas** ;
- erreur/déviation de ralenti importante : **>100 tr/min** ;
- réglage de service du ralenti : ordre de grandeur **49/50 tr/min par pas/offset selon contexte Blackbox** — formulation exacte à revérifier avant insertion ;
- réapprentissage papillon : **5 appuis complets sur accélérateur en ≤10 s après mise du contact, puis attendre environ 20 s**.

Ces éléments ne doivent être ajoutés qu’après comparaison avec le code existant et la génération concernée.

## Référence ECU MEMS 1.2 candidate

Une source de catalogue/remanufacturation classe :

- `MNE10050` — Metro GTi 1.4 16V, environ 1991–1993 — MEMS 1.2.

Cette référence était absente de la base #533 lors de la recherche.

**Ne pas l’intégrer comme confirmée tant qu’une deuxième source indépendante n’a pas été trouvée.**

Les références ECU qui restent `A_DETERMINER` doivent rester ainsi si aucune source fiable ne permet de les attribuer.

## Problèmes découverts dans les fiches XML du package #533

### MEMS 1.3

Le SVG/connecteur visuel a été corrigé, mais l’ancienne fiche XML contient encore un tableau hérité intitulé **« connecteur ECU 25 broches »**.

Or la règle de projet actuelle est : **MEMS 1.3 = 36 + 18 voies**.

Le tableau 25 broches doit être **reconstruit/remplacé à partir de sources vérifiées**, pas simplement renommé.

### MEMS 1.6

Même problème : ancienne fiche XML avec tableau 25 broches, alors que la documentation externe consultée utilise un connecteur principal allant jusqu’à la broche **36**, avec second connecteur suivant version.

Le tableau doit être remplacé, pas renommé.

### MEMS 1.9

Le tableau actuel de la fiche XML présente des divergences avec le brochage MGF/MEMS 1.9 documenté sur plusieurs broches, notamment autour des pins **1, 5, 10, 13, 31 et 32**.

Conclusion : **revalider tout le tableau 1.9 ligne par ligne** avant correction.

### MEMS 1.2

La fiche 1.2 reste la fiche récemment validée visuellement avec **36 voies** et la règle de statut :

- information confirmée ; ou
- **« À vérifier suivant modèle du véhicule »**.

Jamais « probable ».

## Ce qui reste à résoudre en priorité

1. Brochage MEMS **1.3** complet et fiable, source par source.
2. Brochage MEMS **1.6** complet, distinction connecteur principal / second connecteur / variantes véhicule.
3. Brochage MEMS **1.9** complet, comparaison ligne par ligne avec la fiche actuelle.
4. Conflit exact autour de la commande `0x0F` et des fonctions de reset/réglage.
5. Confirmation indépendante de l’offset `7D:1F` pour Jack Count.
6. Validation finale du sens de `80:19` et de ses plages/usage diagnostic.
7. Masques DTC : résoudre les contradictions avant intégration.
8. Modes diagnostic et commandes encore ambiguës (`F0`, `F4`, autres séquences de mode/service) : ne rien figer tant que les sources divergent.
9. Deuxième source pour `MNE10050` avant ajout.
10. Comparer les seuils Blackbox au code d’analyse de MEMS Manager avant ajout en base.

## Ordre exact de reprise si la discussion est coupée

1. Ouvrir ce fichier et `REPRISE_UI_REBUILD.md`.
2. Vérifier le HEAD courant de `ui-rebuild` ; **ne pas changer de branche**.
3. Reprendre la recherche au point **brochage 1.3 / 1.6 / 1.9**, sans modifier immédiatement les XML.
4. Construire un tableau de comparaison par génération : broche actuelle / source A / source B / statut / correction proposée.
5. Résoudre ensuite les conflits protocole (`0x0F`, `7D:1F`, `80:19`, DTC, modes diagnostic).
6. Seulement après validation des sources : préparer le nouveau lot d’enrichissement et les corrections XML.
7. Faire passer les validations DB + self-test MEMS sans toucher au comportement de recherche validé #518.
8. L’utilisateur teste le nouvel artefact Windows avant de considérer le lot fonctionnellement validé.

## Interdiction de perte de contexte

En cas de nouvelle discussion, ne pas demander à l’utilisateur de répéter ces recherches. Le présent fichier est la référence de reprise persistante pour ce lot.
