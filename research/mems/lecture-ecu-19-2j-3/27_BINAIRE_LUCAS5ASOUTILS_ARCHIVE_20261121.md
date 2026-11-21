# Rapport 27 — Archive Andrew Revill MEMSTools(5).zip : lecture Lucas 5AS

**21/11/2026 ; étude technique statique**, sur l'unique branche `fix-documentary-additive-session-161-20261118`. **Ni exécutable lancé, ni ECU connecté, ni code de Manager modifié, ni compilation lancée.**

## 1. Contenu authentifié par ouverture directe de l'archive

ZIP téléversé par l'utilisateur : **1 236 fichiers**, environ **393 Mo compressés** et **1,77 Go en taille décompressée**. L'archive contient les vrais moteurs Windows :
- `Executables/Lucas5asTools.64.exe` : **27 860 520 octets** ; SHA256 `b06a1ddd6079d2aac7e36877421f2466fd0a9244d61f1e0e1b16f4900fe1542f`.
- `Executables/Lucas5asTools.32.exe` : **24 500 100 octets** ; SHA256 `48e55f587a57164c7803634e921deafdbcf32ad70dc2f1b991e3683fc6ea1877`.
- Lanceur `Lucas 5AS 10AS Tools - For Lucas 5AS 10AS Immobilisers.exe`.
- Configuration `Executables/Lucas5asTools.set`.
- Autres programmes : `PektronScuTools.64/.32.exe`, `Lucas27vtTools.64/.32.exe`, `MemsMapper.*`, `MemsFlasher.*`, pilotes FTDI/CH340.

L'exécutable Lucas5asTools est un **programme Delphi natif Windows**. Les métadonnées de débogage permettent de retrouver les noms de méthodes. **Aucun fichier `.log` de dialogue réel avec un 5AS n'est présent dans le ZIP.**

## 2. Méthodes de protocole 5AS retrouvées

Noms directement retrouvés dans le programme :
- `TLucas5as.SlowInit`, `TLucas5as.Connect`.
- `TLucas5as.SendByte`, `WriteByte`, `ReadArray`.
- `TLucas5as.SynchronizeRead`, `SynchronizeWrite`.
- `TLucas5as.ReadParameters`, `ReadModelName`, `ReadSerialNumber`, `ReadWeekOfManufacture`.
- Énumération `TLucas5as.TInitialisationMode` : modes séparés `imDiagnostics` et `imProgramming`.
- `TLucas5as.TWriteEvent` : octets, description, indicateur `ASlowInit`.
- `TLucas5as.TReadEvent` : octets, description, indicateurs `AEcho` et `AError`.

Il s'agit du **moteur de communication** et de ses événements de journal, pas seulement d'une interface visuelle.

## 3. Réveil à 5 bauds : octets retrouvés dans le fichier

Dans le moteur 64 bits, deux occurrences littérales **`1C 83 76`** se situent aux **offsets fichier `0x006B2D4C` et `0x006B2D7C`**. On retrouve deux occurrences dans la version 32 bits.

Dans la fonction d'initialisation lente (instructions du moteur x64 vers **`0x008751E0`–`0x0087560A`**) :
1. Choix du profil et de l'adresse lente ; une branche distincte concerne le mode d'édition, que nous n'utilisons pas pour le diagnostic.
2. Transmission de l'adresse à **5 bauds** ; la temporisation de base de **200 000 microsecondes** par bit (200 ms) est visible.
3. Lecture **de trois octets** de synchronisation ; vérification du premier contre **`55`**, du deuxième et du troisième contre la table de famille.
4. Fin de la fonction après vérification. **Aucun octet d'acquittement de type ISO9141 `89` ou `E3` n'a été trouvé après la réponse à l'intérieur de cette fonction**.

Le mode normal du Lucas 5AS suit ainsi le réveil documentaire `1C` → `55 83 76` et **ne semble pas utiliser l'ACK supplémentaire du MEMS 1.9** dans le chemin inspecté. Cette observation est **statique**, pas une preuve de liaison avec un vrai boîtier. D'autres branches de connexion et variantes peuvent exister.

## 4. Lecture normale du diagnostic : service 83

La fonction de lecture, vers **`0x008760EF`–`0x0087624B`** du moteur 64 bits :
- transmet la **commande `83`** dans la branche correspondant au 5AS ;
- transmet **`9B`** dans une branche de famille différente (logiciel aussi compatible 10AS) ;
- lit un **octet initial de longueur** ;
- lit les **longueur moins un** octets restants ;
- conserve ensuite l'ensemble pour décoder les paramètres.

Les trois libellés exacts retrouvés dans le binaire sont « diagnostic data request », « diagnostic data length » et « diagnostic data response ».

Reconstruction du dialogue **pour le diagnostic normal 5AS**, à valider par un vrai boîtier :

```text
OUTIL -> 5AS : 1C (initialisation à 5 bauds)
5AS -> OUTIL : 55 83 76
                [pas d'ACK supplémentaire identifié dans SlowInit]
OUTIL -> 5AS : 83 (lecture, transport 9600 bauds)
5AS -> OUTIL : [LONGUEUR] [LONGUEUR-1 octets]
```

Ne pas confondre cette construction avec le protocole du MEMS 1.9, avec RC5 ou avec le mode de programmation 5AS.

## 5. Le Log 5AS peut être enregistré — confirmation dans le binaire

Chaînes présentes dans **Lucas5asTools.64.exe** :
- `Log Files (*.log)|*.log`
- `Save Log File ...`
- `Clear the communications log?`

Ces éléments confirment **une fonctionnalité de sauvegarde du journal vers `.log`** dans Lucas 5AS Tools. Le rapport 25 avait seulement confirmé cette possibilité pour l'autre application MEMS Mapper ; la distribution 5AS permet maintenant de lever ce doute. L'emplacement précis du bouton et la présence de toutes les étapes de réveil dans le fichier enregistré restent à vérifier par l'interface en fonctionnement.

## 6. Brochages et variantes trouvés dans les messages internes

- **Lucas 5AS** : câble KKL 409 **RXD inversé**, permutation des contacts **1 avec 15** et **3 avec 7**, ou variante à prise Rover 3 broches avec alimentation 12 V dédiée.
- **Lucas 10AS** : **autre montage**, KKL 409 avec permutation **7 avec 8** sur les véhicules Land Rover décrits. Ne pas appliquer ce câblage au 5AS.
- **Pektron SCU** : programme indépendant présent dans la distribution ; ne pas réutiliser les commandes 5AS sans preuve.
- Fichier `Lucas5asTools.set` : `Family=mfLucas10as`, `Connection=mcComPort`, `Device=COM1`. Ce sont **les valeurs de configuration fournies**, nullement la preuve de l'identification d'un boîtier réel.

## 7. Conclusion et conséquences pour Manager

**Démontré par le contenu de l'archive :** moteur 5AS/10AS complet, routines de communication, vecteur `1C 83 76`, validation des 3 octets de synchronisation, demande de lecture `83`, réponse encadrée par sa longueur et existence d'une fonction d'export du journal `.log`.

**Fortement suggéré par le flux désassemblé :** aucun acquittement ISO9141 additionnel de type MEMS 1.9 n'est nécessaire dans la routine de réveil normal analysée. À **confirmer physiquement** sur 5AS si possible, sans imposer un ACK arbitraire.

**Prochaine étape source Manager :** session 5AS indépendante, réveil lent, validation du `55 83 76`, lecture `83` avec longueur complète, conservation du brut, journal TX/RX, identification réelle avant affichage « connecté », tests synthétiques et non-régression. **Aucune fonction d'EEPROM, de télécommandes ou d'écriture à brancher au diagnostic initial.**

**Licence et méthode :** conserver seulement ces constats techniques et programmer notre propre implémentation ; **ne pas déposer les exécutables tiers ni copier du code propriétaire dans le dépôt**.
