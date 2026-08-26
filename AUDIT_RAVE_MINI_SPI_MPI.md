# AUDIT DOCUMENTAIRE — RAVE / ROVER MINI SPi / MPi

> Branche documentaire : `RAPPORT`  
> Statut : **audit / classement uniquement — aucune intégration runtime encore**  
> Objectif : fournir des faits constructeur traçables aux futures réponses immédiates de IA MEMS.

## Règles de classement

- Ne pas recopier les manuels : conserver uniquement des faits techniques structurés.
- Chaque fait doit garder son document, sa page/section, sa variante et son niveau de preuve.
- Séparer strictement **SPi ancien**, **SPi 1997+**, **MPi** et **cas particuliers (ex. Japon)**.
- Ne jamais transformer une valeur spécifique à un millésime/ECU en valeur universelle.
- En cas de divergence entre deux pages constructeur, conserver le conflit au lieu de choisir arbitrairement.
- Niveau utilisé ici : **`verifie_constructeur`** pour une donnée lue directement dans une publication Rover identifiable ; **`verifie_constructeur_copie_numerisee`** lorsque la publication Rover est identifiable mais consultée via une copie numérisée tierce.

## Sources constructeur identifiées

### SRC-RCL0193
- Document : **Mini Workshop Manual — RCL0193ENG**.
- Couverture : Mini 1997–2000 ; document indiqué comme supplément de AKM7169ENG.
- PDF public consulté : `https://www.uscars.biz/uscars/mini-documents/Rave_CD_wmxn990e.pdf`
- Niveau : `verifie_constructeur`.

### SRC-RCL0213
- Document : **Mini Electrical Reference Library — RCL0213ENG**.
- Publication Rover Technical Communication, ©1997 Rover Group Limited.
- À utiliser avec RCL0193ENG et RCL0194ENG.
- PDF public consulté : `https://www.uscars.biz/uscars/mini-documents/Rave_CD_elxn970e.pdf`
- Niveau : `verifie_constructeur`.

### SRC-RCL0194
- Document : **Mini Electrical Circuit Diagrams — RCL0194ENG**.
- Schémas électriques Mini 97MY et amendements.
- PDF public consulté : `https://www.goclassic.eu/upload/content/e68ce7_9c0fd6a5b3a14a3585ecddb02b3ceb9d.pdf`
- Niveau : `verifie_constructeur`.

### SRC-AKM7169
- Document : **Mini Repair Manual — AKM7169ENG**, Rover Group Limited 1993.
- Couvre les Mini à partir de VIN 049349 et contient la section `MEMS – SPi Fuel Delivery System`.
- Copie numérisée publique consultée via Scribd : `https://www.scribd.com/document/625575179/Mini-Repair-Manual-92-96-Autom`
- Niveau : `verifie_constructeur_copie_numerisee`.

---

# LOT 1 — VALEURS DE RÉFÉRENCE CONSTRUCTEUR

## RAVE-VAL-001 — ralenti SPi ancien, boîte manuelle
- Famille : `SPi_1993_1996`
- Moteur : `12A2DF75 / 1275 cm3`
- Valeur : **850 ± 25 tr/min**.
- Condition : ralenti commandé par ECU.
- Source : `AKM7169ENG`, Engine Tuning Data, modèle 1.3 Mini SPi manual gearbox.
- Preuve : `verifie_constructeur_copie_numerisee`.
- Réponse immédiate visée : « Quelle est la valeur d'origine du ralenti sur cette SPi ? ».

## RAVE-VAL-002 — ralenti SPi ancien, boîte automatique
- Famille : `SPi_1993_1996`
- Moteur : `12A2DF76 / 1275 cm3`
- Valeur : **850 ± 25 tr/min**.
- Source : `AKM7169ENG`, Engine Tuning Data.
- Preuve : `verifie_constructeur_copie_numerisee`.

## RAVE-VAL-003 — ralenti SPi ancien, haute compression
- Famille : `SPi_1993_1996`
- Moteur : `12A2EF77 / 1275 cm3`
- Valeur : **850 ± 25 tr/min**.
- Source : `AKM7169ENG`, Engine Tuning Data.
- Preuve : `verifie_constructeur_copie_numerisee`.

## RAVE-VAL-004 — ralenti SPi 1997+
- Famille : `SPi_1997_plus`
- Moteurs RCL0193 : `12A2EK71` manuel et `12A2EK72` automatique.
- Valeur : **900 ± 50 tr/min**.
- Condition : pas de charge électrique ; ralenti contrôlé par ECM.
- Source : `RCL0193ENG`, pages PDF 38–39, Engine Tuning Data.
- Preuve : `verifie_constructeur`.

## RAVE-VAL-005 — ralenti MPi
- Famille : `MPi_1997_plus`
- Moteur : `12A2LK70 / 1275 cm3`.
- Valeur : **900 ± 50 tr/min**.
- Condition : pas de charge électrique ; ralenti contrôlé par ECM.
- Source : `RCL0193ENG`, page PDF 40, Engine Tuning Data.
- Preuve : `verifie_constructeur`.

### Règle IA issue de VAL-001 à VAL-005
La question « valeur de ralenti d'origine » **ne doit jamais recevoir une seule valeur universelle** : les premiers SPi du manuel AKM7169 sont donnés à 850 ±25 tr/min, alors que RCL0193 donne 900 ±50 tr/min pour SPi 1997+ et MPi. Demander/tenir compte du véhicule, millésime, moteur ou ECU lorsque le contexte n'est pas identifié.

## RAVE-VAL-006 — pression carburant SPi ancien
- Famille : `SPi_1993_1996`
- Valeur : **1,0 bar ± 4 %**, constante.
- Source : `AKM7169ENG`, Engine Tuning Data.
- Preuve : `verifie_constructeur_copie_numerisee`.

## RAVE-VAL-007 — pression carburant SPi 1997+
- Famille : `SPi_1997_plus`
- Valeur : **1,0 ± 0,2 bar**, constante.
- Source : `RCL0193ENG`, pages 38–39.
- Preuve : `verifie_constructeur`.

## RAVE-VAL-008 — pression carburant MPi 1997+
- Famille : `MPi_1997_plus`
- Valeur : **3,0 ± 0,2 bar**, constante (donnée de réglage constructeur).
- Source : `RCL0193ENG`, page 40.
- Preuve : `verifie_constructeur`.

## RAVE-VAL-009 — tension capteur papillon
- Famille : `SPi_1997_plus | MPi_1997_plus`
- Fermé : **0 à 1 V**.
- Ouvert 90° : **4 à 5 V**.
- Ouvert 65° : **3 à 4 V**.
- Source : `RCL0193ENG`, pages 38–40.
- Preuve : `verifie_constructeur`.
- Note : AKM7169 confirme également 0–1 V fermé et 4–5 V ouvert sur les SPi anciens.

## RAVE-VAL-010 — position IACV en fonctionnement
- Famille : `Mini_RCL0193_MEMS`
- Valeur de contrôle : **20 à 40 pas** moteur en fonctionnement.
- Source : `RCL0193ENG`, page PDF 114, Idle air control valve.
- Preuve : `verifie_constructeur`.
- Important : le manuel précise que l'ajustement porte sur la position **apprise**, pas sur la position instantanée de ralenti ; l'ajustement n'est pas recommandé hors procédure TestBook.

## RAVE-VAL-011 — ventilateur MPi 97MY
- Famille : `MPi_97MY`
- Mise en marche : **105 °C**.
- Arrêt : **98 °C**.
- Commande : ECM via relais de ventilateur, retour ECT.
- Source : `RCL0213ENG`, section Cooling Fan Operation, page PDF 48.
- Preuve : `verifie_constructeur`.

## RAVE-VAL-012 — ventilateur SPi Japon 97MY
- Famille : `SPi_Japan_97MY`
- Mise en marche : au-dessus de **98 °C**.
- Arrêt : **93 °C**.
- Commande : contacteur haute température du radiateur ; ventilateur auxiliaire de secours au ventilateur mécanique.
- Source : `RCL0213ENG`, page PDF 48.
- Preuve : `verifie_constructeur`.
- Interdiction de généralisation : cette donnée est explicitement **SPi Japon uniquement**.

---

# LOT 2 — CAPTEURS : RÔLE / MESURE / CONSÉQUENCES LOGIQUES DOCUMENTÉES

## RAVE-SEN-001 — CKP / capteur vilebrequin
- Famille : `MEMS_RCL0193_97MY`
- Mesure : position vilebrequin + régime moteur.
- Principe : capteur inductif face à la couronne/reluctor du volant moteur.
- Couronne : 32 pôles espacés de 10°, avec 4 pôles manquants aux positions 30°, 60°, 210° et 250°.
- Importance : le manuel le décrit comme **entrée primaire nécessaire au fonctionnement du moteur**.
- Source : `RCL0213ENG`, page PDF 34 ; `RCL0193ENG`, page PDF 108.
- Preuve : `verifie_constructeur`.
- Réponse immédiate possible : rôle, principe, pourquoi une absence de signal régime/CKP est critique.

## RAVE-SEN-002 — CMP / capteur arbre à cames
- Famille : `MPi_RCL0193`
- Fonctions : permet le fonctionnement en injection séquentielle et mesure la période d'arbre à cames.
- Défaillance moteur déjà en marche : le manuel indique que le moteur continue à fonctionner.
- Défaillance avant démarrage : démarrage possible mais avec limite de régime réduite par rapport à la limite normale de **6500 tr/min**.
- Source : `RCL0193ENG`, page PDF 108.
- Preuve : `verifie_constructeur`.
- Note : ne pas appliquer cette réponse au SPi sans preuve spécifique.

## RAVE-SEN-003 — MAP
- Famille : `Mini_97MY_MEMS`
- Emplacement : directement sur le collecteur d'admission.
- Mesure : pression absolue du collecteur ; représentation de la charge moteur.
- Utilisation ECU : ajuste quantité de carburant injectée et avance à l'allumage.
- Principe : variations de pression converties en signal électrique gradué.
- Source : `RCL0213ENG`, page PDF 34.
- Preuve : `verifie_constructeur`.

## RAVE-SEN-004 — ECT / température liquide
- Famille : `Mini_97MY_MEMS`
- Emplacement : coude/sortie de liquide de refroidissement.
- Type : thermistance.
- Signal : varie inversement avec la température.
- Utilisation ECU : notamment durée d'ouverture des injecteurs ; la documentation MEMS décrit aussi une compensation d'avance selon température.
- Source : `RCL0213ENG`, page PDF 35 ; `RCL0193ENG`, page PDF 109.
- Preuve : `verifie_constructeur`.

## RAVE-SEN-005 — TP / capteur position papillon
- Famille : `Mini_97MY_MEMS`
- Type : potentiomètre couplé directement au papillon.
- Alimentation : 5 V depuis ECM dans RCL0213.
- Rôle : position papillon ; détection papillon fermé ; déclenche la régulation de ralenti via IACV.
- La vitesse de variation du signal sert à l'enrichissement accélération, gestion décélération et coupure d'injection en décélération.
- Source : `RCL0213ENG`, page PDF 35 ; `RCL0193ENG`, pages PDF 113–114.
- Preuve : `verifie_constructeur`.

## RAVE-SEN-006 — interdiction réglage butée papillon
- Famille : `Mini_RCL0193_MEMS`
- Fait : la position du papillon est préréglée en fabrication ; la vis de butée **ne doit pas être ajustée** pour régler le ralenti.
- Source : `RCL0193ENG`, pages PDF 114 et 118.
- Preuve : `verifie_constructeur`.
- Réponse immédiate visée : « Je règle le ralenti avec la vis du papillon ? » → non, avec avertissement constructeur.

## RAVE-SEN-007 — IAT / température air admission
- Famille : `Mini_97MY_MEMS`
- Emplacement : côté du collecteur d'admission.
- Type : NTC, résistance diminue lorsque température augmente.
- Utilisation ECU : combinée au MAP pour estimer la quantité/volume d'oxygène entrant et corriger finement l'injection.
- Source : `RCL0213ENG`, page PDF 37 ; `RCL0193ENG`, page PDF 112.
- Preuve : `verifie_constructeur`.

## RAVE-SEN-008 — HO2S / lambda
- Famille : `Mini_97MY_MEMS`
- Système : boucle fermée.
- Mélange pauvre : plus d'oxygène dans l'échappement → tension capteur plus faible.
- Mélange riche : moins d'oxygène → tension plus élevée.
- L'ECM utilise le signal pour corriger la quantité de carburant.
- Chauffage : relais commandé pour accélérer l'atteinte de la température de fonctionnement.
- Source : `RCL0213ENG`, pages PDF 37–38.
- Preuve : `verifie_constructeur`.

---

# LOT 3 — ACTIONNEURS / TESTS : CE QUE LE SYSTÈME DOIT FAIRE

## RAVE-ACT-001 — IACV / moteur pas à pas
- Famille : `Mini_97MY_MEMS`
- Emplacement : collecteur d'admission.
- Fonction : ouvre un passage de dérivation autour du papillon ; contrôle l'air de ralenti et le débit supplémentaire au démarrage à froid.
- À froid : l'ECM ouvre davantage l'IACV pour créer un ralenti accéléré ; cette ouverture diminue à mesure que le liquide chauffe.
- Commande électrique RCL0213 : phases 1 à 4 contrôlées par ECM.
- Source : `RCL0213ENG`, page PDF 37 ; `RCL0193ENG`, page PDF 114.
- Preuve : `verifie_constructeur`.
- Pour futur test actionneur : si la commande est envoyée et qu'aucun mouvement/variation n'apparaît, le diagnostic doit distinguer actionneur, alimentation, masse, phases/câblage et problème mécanique du passage d'air ; ne pas condamner l'IACV sans contrôle.

## RAVE-ACT-002 — injecteur(s)
- Famille : `SPi_97MY | MPi_97MY`
- Architecture RCL0213 : **1 injecteur SPi**, **2 injecteurs MPi**.
- Type : électrovannes/solénoïdes.
- Grandeur de commande principale : temps d'ouverture, ou **pulse width**.
- L'ECM détermine la quantité de carburant par la durée d'ouverture.
- Entrées documentées : CKP, CMP selon version, MAP, IAT, ECT, TP, tension batterie, HO2S.
- Source : `RCL0213ENG`, page PDF 36 ; `RCL0193ENG`, page PDF 112.
- Preuve : `verifie_constructeur`.

## RAVE-ACT-003 — pompe à carburant
- Famille : `Mini_RCL0193_MEMS`
- Emplacement : réservoir.
- Commande : ECM via relais de pompe et coupe-circuit/inertia fuel cut-off switch.
- Le relais de pompe est activé brièvement à la mise du contact, pendant le démarrage et moteur en marche.
- Source : `RCL0193ENG`, pages PDF 114–115.
- Preuve : `verifie_constructeur`.
- Réponse immédiate possible : lors d'un test, expliquer ce que l'utilisateur doit normalement entendre/mesurer et les contrôles alimentation/relais/coupe-circuit/pompe si elle ne tourne pas.

## RAVE-ACT-004 — purge canister
- Famille : `Mini_97MY_MEMS`
- Fonction : ouvre le circuit du canister vers l'admission pour brûler les vapeurs d'essence.
- RCL0213 : la vanne reste fermée moteur froid et au ralenti afin de protéger le réglage moteur et le catalyseur.
- RCL0193 / système EVAP : ouverture lorsque les conditions moteur le permettent ; sur la description EVAP, la purge est inhibée jusqu'à ce que la température moteur dépasse environ **70 °C**.
- Source : `RCL0213ENG`, page PDF 38 ; `RCL0193ENG`, pages PDF 98 et 114.
- Preuve : `verifie_constructeur`.
- Note : conserver la condition 70 °C avec son contexte de section/millésime, ne pas la généraliser hors de ce corpus sans vérification.

## RAVE-ACT-005 — ventilateur de refroidissement MPi
- Famille : `MPi_97MY`
- Commande : ECM via relais.
- Référence : ON 105 °C / OFF 98 °C.
- Source : `RCL0213ENG`, page PDF 48.
- Preuve : `verifie_constructeur`.

---

# LOT 4 — ADAPTATION / RÉGLAGES

## RAVE-SET-001 — apprentissage IAC
- Famille : `Mini_RCL0193_MEMS`
- À température normale, l'ECM apprend la position IAC nécessaire pour atteindre le ralenti spécifié et s'en sert comme référence lorsque la charge change.
- Source : `RCL0193ENG`, page PDF 111 ; `RCL0213ENG`, page PDF 32.
- Preuve : `verifie_constructeur`.
- Réponse immédiate : oui, l'ECU apprend/adapte une référence IAC ; ne pas présenter un réglage manuel comme une position de ralenti absolue permanente.

## RAVE-SET-002 — apprentissage correction carburant
- Famille : `Mini_RCL0193_MEMS`
- À température normale, l'ECM apprend un décalage de richesse nécessaire pour atteindre la cible de tension de sonde oxygène.
- But documenté : éviter des corrections excessives susceptibles de dégrader émissions et agrément.
- Source : `RCL0193ENG`, page PDF 111.
- Preuve : `verifie_constructeur`.

## RAVE-SET-003 — stabilisation du ralenti par avance + IAC
- Famille : `Mini_97MY_MEMS`
- L'ECM utilise simultanément l'IACV et des corrections rapides d'avance/retard à l'allumage pour maintenir le ralenti lorsque les charges changent.
- Conséquence normale : l'avance affichée peut varier continuellement au ralenti ; ce n'est pas automatiquement un défaut.
- Source : `RCL0213ENG`, page PDF 35 ; `RCL0193ENG`, page PDF 109.
- Preuve : `verifie_constructeur`.

---

# LOT 5 — CONFLITS / POINTS À NE PAS RÉPONDRE COMME UNE CONSTANTE UNIQUE

## RAVE-CONFLICT-001 — valeur de ralenti d'origine
- AKM7169 SPi ancien : **850 ±25 tr/min**.
- RCL0193 SPi 1997+ : **900 ±50 tr/min**.
- RCL0193 MPi : **900 ±50 tr/min**.
- Décision audit : indexer par moteur/millésime/ECU et demander le contexte si inconnu.

## RAVE-CONFLICT-002 — résistance primaire de bobine
- AKM7169 SPi ancien : **0,71 à 0,81 Ω à 20 °C**.
- RCL0193, tableaux Engine Tuning Data pages 38–40 : **0,41 à 0,61 Ω à 20 °C**, bobine NEC1000710.
- RCL0193, description MEMS page 110 : **0,63 à 0,77 Ω à 20 °C**.
- Décision audit : **ne pas fusionner** ces trois plages. Elles doivent rester liées à leur source, variante/référence de bobine et section jusqu'à clarification complète.

## RAVE-CONFLICT-003 — ventilateur SPi
- La valeur 98/93 °C de RCL0213 concerne **SPi Japon uniquement** et un contacteur haute température dans le radiateur.
- Décision audit : ne pas l'utiliser comme seuil SPi général.

---

# LOT 6 — PANNES / STRATÉGIES DE SECOURS / CONSÉQUENCES DOCUMENTÉES

## RAVE-FAIL-001 — stratégie de secours ECM générale
- Famille : `Mini_RCL0193_MEMS`
- Fait constructeur : pour **certaines** entrées système défaillantes, l'ECM met en œuvre une stratégie de secours permettant de continuer à fonctionner, mais avec un niveau de performance réduit.
- Source : `RCL0193ENG`, page PDF 107, description générale MEMS.
- Preuve : `verifie_constructeur`.
- Règle IA : ne jamais affirmer qu'un défaut de capteur entraîne nécessairement l'arrêt moteur ; vérifier s'il existe une stratégie spécifique documentée. Inversement, ne pas inventer la valeur de substitution utilisée par l'ECM si le manuel ne la donne pas.

## RAVE-FAIL-002 — absence de signal CKP
- Famille : `Mini_RCL0193_MEMS`
- Le CKP fournit position vilebrequin et régime ; le manuel le qualifie d'entrée primaire nécessaire au fonctionnement du moteur.
- Source : `RCL0193ENG`, page PDF 108.
- Preuve : `verifie_constructeur`.
- Réponse immédiate visée : une anomalie CKP est **critique** car l'ECM dépend de ce signal pour connaître vitesse/position moteur. Pour un cas réel, contrôler d'abord présence du signal, capteur, entrefer critique, connecteur et câblage avant de condamner l'ECM.
- Prudence : la conséquence exacte « jamais de démarrage dans tous les cas » ne doit être formulée qu'en restant dans le contexte couvert par le manuel.

## RAVE-FAIL-003 — défaillance CMP
- Famille : `MPi_RCL0193`
- Si le CMP tombe en panne alors que le moteur tourne : le manuel indique que le moteur continue à fonctionner.
- Si le défaut est présent avant démarrage : le moteur peut démarrer mais avec une limite de régime réduite par rapport à la limite normale de **6500 tr/min**.
- Identification du défaut : TestBook selon le manuel.
- Source : `RCL0193ENG`, page PDF 108.
- Preuve : `verifie_constructeur`.

## RAVE-FAIL-004 — antidémarrage / absence d'autorisation
- Famille : `Mini_RCL0193_MEMS`
- Fait constructeur : l'ECM est immobilisé électroniquement et empêche le démarrage s'il ne reçoit pas le signal codé de l'unité antivol.
- Après remplacement de l'ECM, le code antivol doit être programmé avec TestBook avant démarrage.
- Source : `RCL0193ENG`, pages PDF 107 et 127.
- Preuve : `verifie_constructeur`.
- Réponse immédiate visée : pour un défaut d'antidémarrage, distinguer le système d'autorisation de démarrage des défauts de carburant/allumage ordinaires.

## RAVE-FAIL-005 — coupe-circuit à inertie / pompe
- Famille : `Mini_RCL0193_MEMS`
- Fait constructeur : lors d'une décélération brutale, l'IFS coupe le circuit de pompe afin d'empêcher l'alimentation carburant du moteur.
- Le manuel exige de vérifier fuites et intégrité des connexions carburant avant de réarmer le contacteur.
- Source : `RCL0193ENG`, page PDF 116.
- Preuve : `verifie_constructeur`.
- Réponse immédiate visée : si la pompe ne fonctionne pas après choc/forte décélération, inclure l'IFS dans les contrôles avant de condamner la pompe.

## RAVE-FAIL-006 — HO2S / lambda non opérationnelle
- Famille : `Mini_97MY_MEMS`
- Le manuel électrique indique qu'une sonde oxygène ne fonctionnera pas si son alimentation est absente ; chute/choc ou produits de nettoyage peuvent également l'endommager.
- Source : `RCL0213ENG`, pages PDF 37–38.
- Preuve : `verifie_constructeur`.
- Réponse immédiate visée : pour une erreur lambda/chauffage lambda, contrôler alimentation/chauffage, câblage et état physique avant de conclure que le signal traduit réellement un mélange moteur anormal.

## RAVE-FAIL-007 — purge ouverte dans de mauvaises conditions
- Famille : `Mini_97MY_MEMS`
- RCL0193 explique que l'ouverture de purge moteur froid ou au ralenti enrichirait le mélange, retarderait la mise en température efficace du catalyseur et pourrait provoquer un ralenti irrégulier.
- RCL0213 confirme que la vanne reste normalement fermée moteur froid et au ralenti.
- Source : `RCL0193ENG`, page PDF 98 ; `RCL0213ENG`, page PDF 38.
- Preuve : `verifie_constructeur`.
- Réponse immédiate visée : une purge qui reste ouverte peut être reliée à un ralenti perturbé et à une gestion mélange/catalyseur anormale, sans présenter cela comme la seule cause possible.

## RAVE-FAIL-008 — IAC hors plage de référence
- Famille : `Mini_RCL0193_MEMS`
- Référence : 20–40 pas moteur en fonctionnement dans ce corpus.
- Si l'IAC est identifié hors plage, le manuel prévoit une correction de la **position apprise** via TestBook et déconseille un réglage arbitraire.
- La butée papillon est préréglée en usine et ne doit pas servir au réglage du ralenti.
- Source : `RCL0193ENG`, page PDF 114.
- Preuve : `verifie_constructeur`.
- Réponse immédiate visée : une position IAC hors plage doit conduire à vérifier contexte/charge/admission et apprentissage ; ne pas conseiller de toucher la vis de butée.

## RAVE-FAIL-009 — stockage de défauts intermittents
- Famille : `Mini_RCL0193_MEMS`
- L'ECM possède une protection contre les courts-circuits et peut mémoriser certains défauts intermittents d'entrées ; TestBook peut interroger ces défauts stockés.
- Source : `RCL0193ENG`, page PDF 107.
- Preuve : `verifie_constructeur`.
- Impact IA : distinguer une erreur mémorisée d'un défaut nécessairement actif au moment de la lecture.

## RAVE-DTC-STATUS-001 — codes numériques 01–24 de MEMS Manager
- Les recherches dans `RCL0193ENG` et `RCL0213ENG` n'ont pas retrouvé de table littérale `fault code` / `diagnostic trouble code` reliant directement les numéros **01–24** affichés aujourd'hui par MEMS Manager aux libellés du logiciel.
- Décision : **ne pas marquer ces numéros comme « vérifiés RAVE » à ce stade**.
- Les fonctions des organes, leurs conséquences documentées et leurs circuits peuvent déjà être enrichis avec RAVE ; la correspondance numérique doit être recoupée séparément avec documentation TestBook/ROSCO/MEMS dédiée avant classement constructeur.

---

# IMPACT SUR LES QUESTIONS IA

Ce premier ensemble permet déjà des réponses déterministes immédiates à des questions comme :

- « Quel est le ralenti d'origine de ma SPi ? » → réponse variant selon génération/ECU, pas valeur universelle.
- « Quelle pression d'essence sur SPi / MPi ? » → 1 bar SPi, 3 bar MPi avec tolérances et contexte source.
- « À quoi sert le MAP ? » → charge moteur, injection + avance.
- « Que fait le capteur ECT ? » → information température pour injection et compensation de gestion moteur.
- « Que fait l'IAT ? » → estimation de l'oxygène avec MAP et correction injection.
- « Quelle position IAC est normale ? » → 20–40 pas dans le contexte RCL0193 moteur en fonctionnement, avec mise en garde sur l'apprentissage.
- « Est-ce normal que l'avance bouge au ralenti ? » → oui, l'ECM l'utilise avec IAC pour stabiliser le régime.
- « Que fait le test pompe à essence ? » → permet de vérifier l'action de la pompe/relais/circuit ; absence de réaction doit lancer un contrôle structuré plutôt que condamner la pompe.
- « Pourquoi la purge canister est fermée au ralenti ? » → protection du réglage moteur/catalyseur selon documentation.
- « Ma purge bloquée ouverte peut faire quoi ? » → ralenti irrégulier et perturbation du mélange/catalyseur sont documentés comme conséquences possibles dans ce contexte.
- « Mon CMP est en défaut, le moteur peut-il tourner ? » → réponse spécifique MPi/RCL0193 avec distinction panne avant/après démarrage.
- « Une erreur capteur veut-elle dire que le moteur s'arrête ? » → non systématiquement ; RAVE documente une stratégie de secours pour certaines entrées, avec performances réduites.
- « Pourquoi la pompe ne tourne plus après un choc ? » → contrôler le coupe-circuit à inertie et l'intégrité du circuit avant réarmement.
- « À combien démarre le ventilateur MPi ? » → 105 °C / arrêt 98 °C pour MPi 97MY.

# PROCHAINE PHASE DOCUMENTAIRE

1. compléter les **connecteurs/pinouts** utiles à MAP, ECT, IAT, TP, IACV, HO2S, purge et pompe ;
2. rechercher davantage de **stratégies de panne spécifiques** capteur par capteur sans déduire les valeurs de substitution ;
3. rechercher les publications **TestBook / diagnostic MEMS** susceptibles de documenter les codes numériques de défaut ;
4. recouper les DTC/erreurs déjà affichés dans MEMS Manager avec les sources Rover, ROSCO et protocoles déjà validés ;
5. rechercher les valeurs constructeur manquantes pour actionneurs et réglages ;
6. seulement après validation utilisateur : transformer ces faits en entrées structurées de la base experte et en routage de réponses immédiates.
