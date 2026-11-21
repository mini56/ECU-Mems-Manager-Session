# Rapport 22 — Fabrication personnelle des câbles RC5 / Lucas 5AS / PLIP

**21 novembre 2026.** Projet ECU MEMS Manager SESSION, page « Autres calculateurs ».  
**Principe utilisateur : aucun achat obligatoire.** Dans les trois boutons d'aide, indiquer **deux possibilités** : acheter éventuellement un câble adapté, **ou fabriquer / modifier soi-même** un câble à partir d'un KKL 409.1 et d'une rallonge OBD, quand la documentation technique le permet.  
**Recherche et documentation seulement.** Ne modifier ni ECU, ni anti-démarrage, ni programmation de clés. Le BUILD 188 déjà lancé utilise son propre instantané de sources et ne recevra pas rétroactivement ces textes.

## 1. Airbag RC5 : adaptateur OBD de la Mini MPI

La ligne moteur MEMS 2J sur Mini MPI arrive à l'**OBD 7** ; le calculateur Airbag RC5 utilise l'**OBD 13**, suivant les données pscan. Les deux broches sont des **lignes de diagnostic différentes**, pas des fils à mettre en parallèle.

**Solution à fabriquer :** une rallonge/adaptateur OBD à 16 contacts réservée au RC5. Avec les deux extrémités débranchées, amener la **ligne K du câble diagnostique (contact 7 côté câble)** au **contact 13 côté prise véhicule**. Conserver uniquement les raccordements utiles et confirmés pour l'alimentation/masse de l'interface. **Ne jamais établir de pont permanent 7–13 sur le véhicule.** Étiqueter clairement « RC5 SRS ». Vérifier la continuité 7→13 et l'absence de contact indésirable avec les autres broches avant branchement. Cette méthode n'est pas à généraliser à une Mini JDM, MGF, MG TF ou une autre Rover sans vérifier le schéma spécifique.

Le montage ne concerne que la **ligne diagnostique**. Ne jamais toucher au câblage pyrotechnique des airbags/prétensionneurs.

## 1 bis. Ne pas confondre le câble FTDI de MEMS 1.6 et celui du Lucas 5AS

La mention **« FTDI FT232R » décrit le convertisseur USB–série, pas le câblage ni les niveaux électriques du câble complet**.

| Équipement | Interface de diagnostic publiée | Contact véhicule |
|---|---|---|
| MEMS 1.6 moteur | Adaptateur **USB–TTL 5 V** (par exemple FTDI TTL-232R-5V), émission/réception logique directe et masse | **Prise moteur à trois broches** |
| Lucas 5AS | Adaptateur **automobile KKL 409.1**, lignes K et L adaptées aux signaux autour de **0–12 V**, réception inversée | Sur modèles concernés à prise 16 voies : **OBD 3 (TX outil)** et **OBD 1 (RX outil)** après modification |
| MEMS 1.9 / 2J moteur | KKL automobile sur ligne moteur, avec connecteur adapté au modèle | Prise diagnostic moteur ; souvent **OBD 7** pour la ligne K |

**Conséquence essentielle : il ne suffit JAMAIS de cocher « Invert RXD » dans FT_PROG sur le câble MEMS 1.6 USB–TTL 5 V pour en faire un câble 5AS.** Il manquerait l'interface électronique automobile 12 V, la connexion des lignes K/L et le bon connecteur. Un câble basé sur FT232R peut exister dans chacune de ces catégories, mais leurs circuits et leurs usages sont différents.

Pour un câble fabriqué maison, partir d'un **KKL automobile 409.1 compatible**, et non du seul adaptateur USB–TTL MEMS 1.6, puis effectuer les modifications vérifiées au chapitre 2.

**Sources distinctes :**
- Colin Bourassa, procédure MEMS 1.6, recommandation FTDI TTL-232R-5V et prise trois broches : https://colinbourassa.github.io/car_stuff/mems_interface/
- Documentation MEMSdiag, MEMS 1.6 USB–TTL 5 V contre MEMS 1.9 câble KKL 12 V : https://memsdiag.blogspot.com/2015/01/memsdiag-what-is-memsdiag-memsdiag-is.html
- Andrew Revill, classification des câbles 3 broches ROSCO et OBD KKL : https://andrewrevill.co.uk/MEMS3FamiliesSupported.htm
- Andrew Revill, câble KKL 409.1 à adapter au 5AS : https://andrewrevill.co.uk/MEMS3LucasImmobiliserSupported.htm

## 2. Lucas 5AS : méthode KKL 409.1 documentée par Andrew Revill

La fabrication d'un câble 5AS comporte **deux modifications indépendantes**, sans quoi le câble peut être reconnu comme COM mais ne pas dialoguer.

### A. Inverser la réception RXD

**FTDI FT232R authentique** (méthode logicielle) :
1. Déconnecter le câble du véhicule ; ne laisser connecté au PC que le KKL choisi.
2. Lancer l'utilitaire officiel FTDI **FT_PROG**.
3. Dans **DEVICES → Scan & Parse**, vérifier le bon périphérique.
4. Dans **Hardware Specific → Invert RS232 Signals**, cocher **Invert RXD**.
5. Dans **DEVICES → Program**, enregistrer ; débrancher et rebrancher l'USB.
6. Relire la configuration pour confirmer que l'inversion a bien été mémorisée. Des clones FT232R ne permettent pas toujours cette écriture ; ne pas leur supposer les mêmes possibilités.

**CH340G à 16 broches** (méthode de soudure selon l'exemple d'Andrew Revill) :
1. Tout débrancher, ouvrir le boîtier du KKL et vérifier la référence **CH340G**, la disposition des 16 pattes et le repère de patte 1.
2. Vérifier que la **patte 15 (R232)** du **circuit intégré CH340G** n'est pas utilisée par la carte ; la patte **16 est VCC** selon sa documentation.
3. Sur la carte correspondant au montage documenté, réaliser un pont **entre les pattes 15 et 16 du CH340G** pour obtenir l'inversion RXD. Si la patte 15 est déjà connectée, **ne pas réaliser le pont à l'aveugle** ; une adaptation matérielle doit être étudiée par une personne compétente.
4. Contrôler soigneusement les courts-circuits et refermer la carte avant usage.

**À ne jamais confondre :** « 15 et 16 » sont les **pattes du CH340G**, **pas les broches de la prise OBD**. Une jonction OBD 15–16 serait une erreur électrique dangereuse. La procédure ne s'applique pas automatiquement aux CH340C, CH341 ou à n'importe quel adaptateur USB-série.

### B. Adapter les broches OBD

Schéma **pour les véhicules Lucas 5AS à OBD 16 contacts documentés** :

| Liaison | Sortie côté KKL | Entrée/sortie côté véhicule |
|---|---|---|
| Commande de diagnostic du PC | **KKL OBD 7 (K, émission)** | **OBD 3 (réception 5AS)** |
| Réponse diagnostique du 5AS | **KKL OBD 15 (L, réception)** | **OBD 1 (émission 5AS inversée)** |

Andrew Revill préconise une **rallonge ou répartiteur OBD** avec connecteurs accessibles :
1. Débrancher toutes les prises et repérer la **numérotation des contacts moulée** (attention aux vues de face et vues côté fils qui sont en miroir).
2. Sur **la sortie femelle réservée au 5AS**, échanger les contacts **3 et 7**, puis les contacts **1 et 15**. Une simple rallonge réservée au 5AS convient aussi ; un répartiteur à deux sorties n'est pas obligatoire.
3. À l'ohmmètre, vérifier **7 du câble → 3 du véhicule** et **15 du câble ← 1 du véhicule**, et l'absence de pont parasite. Ne pas changer alimentation et masse sans schéma.
4. Marquer **MOTEUR** et **5AS** clairement, avec des câbles et sorties identifiables ; un câble RXD inversé et permuté ne doit pas être pris pour un KKL moteur habituel.

Cette adaptation doit être réalisée **sur le câble ou la rallonge**, **pas sur le faisceau du véhicule**. Les signaux du 5AS sont autour de 0–12 V avec polarité particulière ; ne pas raccorder directement une UART TTL 3,3/5 V ou une RS232 de PC. Utiliser le circuit automobile approprié du KKL modifié. 

**Mini SPI et autres prises vertes à 3 broches :** câblage différent. La documentation Portman ne fournit pas de repérage universel fiable des trois contacts de la prise verte. Il faut les schémas précis du véhicule avant fabrication ; ne pas inventer des numéros.

**Contrôle de fonctionnement :** la détection Windows COM n'est qu'une preuve USB. Le branchement 5AS n'est confirmé que par une **réponse réelle du boîtier** dans un logiciel qui prend déjà en charge 5AS. La SESSION 188 ne possède pas encore de fonction de lecture 5AS.

## 3. Programmation des clés / télécommandes

**Lucas 5AS :** les télécommandes compatibles seront programmées via **le même câble 5AS** que pour la lecture du module. Les opérations de fabrication et vérification sont celles du chapitre 2. Le 433,92 MHz (ou 315 MHz sur certains marchés) concerne la radio, **pas** la ligne de diagnostic OBD.

**Pektron SCU et Lucas 10AS :** protocoles et boîtiers distincts ; **aucune procédure de fabrication universelle de câble compatible Manager n'est actuellement confirmée**. Ne pas suggérer qu'un simple câble 5AS est forcément compatible. Aucun essai de programmation ne doit être réalisé pendant un contrôle matériel de câble.

## 4. Sources exactes

- Andrew Revill, **description illustrée et originale** de FT_PROG, FT232R, CH340G et permutation des contacts : https://andrewrevill.co.uk/MEMS3LucasImmobiliserSupported.htm (section « Diagnostic Cable Required », voir notamment ses photographies).
- Andrew Revill, câble préparé facultatif, et distinction câble moteur / 5AS : https://andrewrevill.co.uk/shop.htm
- James Portman, OBD 1 = 5AS C61-24 TX, OBD 3 = C61-16 RX : https://github.com/james-portman/rover-mems-documentation/blob/master/Cables/16-pin-5AS-cable.md
- James Portman, connecteur 5AS vert trois voies, absence de repère standard : https://github.com/james-portman/rover-mems-documentation/blob/master/Cables/3-pin-5AS-cable.md
- FTDI, utilitaire officiel FT_PROG : https://ftdichip.com/utilities/#ft_prog
- Fiche du composant CH340G, repère R232 sur patte 15 et VCC sur patte 16 : https://docs.sparkfun.com/SparkFun_RTK_Facet_mosaic/assets/component_documentation/CH340DS1.PDF
- Pscan Mini (différence K-Line moteur/RC5) : https://pscan.uk/features/mini.html

**Statut des preuves :** dispositions KKL et modifications 5AS **confirmées par la documentation et les photographies du développeur** ; compatibilité d'un exemplaire précis de câble et fonctionnement avec Manager **à valider physiquement**. Ce guide sert à enrichir les aides hors ligne dans une prochaine version, sans modifier les protocoles moteur ni la version 188 déjà compilée.
