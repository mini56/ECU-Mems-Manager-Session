# ECU MEMS Manager — fonctions futures issues des recherches

Dernière consolidation : build #500 / v1.5.0

Ce fichier conserve les pistes de fonctions trouvées pendant l'enrichissement MEMS 1.2 / 1.3 / 1.6 / 1.9. Il ne signifie pas que ces fonctions sont déjà actives.

## Règles

- Une fonction déjà présente dans ECU MEMS Manager reste attribuée en priorité à ECU MEMS Manager ; les projets externes ne servent que de recoupement.
- Le décodage 7D octets 14–15 / uk10 et sa correction de ralenti chaud restent une découverte/définition ECU MEMS Manager.
- Aucune commande potentiellement destructive n'est activée sur la seule base d'une source externe.
- Les différences de transport restent séparées : ROSCO/UART 3 broches pour 1.2/1.3/1.6, K-Line pour 1.9.

## Priorité haute

### Lecture et sauvegarde ROM complète

Andrew Revill documente la lecture/sauvegarde de ROM complète sur MEMS 1.2, 1.3, 1.6 et 1.9. `ECUManager::readROM()` existe actuellement mais n'est qu'un stub ; il s'agit donc d'une vraie fonction à implémenter.

Ordre recommandé : lecture seule -> contrôle taille/checksum -> identification firmware/calibration -> sauvegarde. Aucun écriture stock avant validation indépendante de la procédure et des risques.

### Identification firmware / calibration

Exploiter D0/D1, les identifiants ASCII, les références ECU, les calibrations et la bibliothèque ROM pour reconnaître plus précisément le calculateur et le véhicule. Cette fonction viendra après constitution suffisante de la base.

### Matrice de compatibilité des commandes

Pour chaque commande, conserver : générations supportées, ECU/firmwares testés, réponse attendue, conditions, niveau de risque et source. Une commande ne devra être proposée que si son contexte est compatible.

### Recherche tolérante aux fautes

La recherche actuelle normalise déjà casse, accents et ponctuation, mais reste essentiellement basée sur les sous-chaînes. Ajouter un score de proximité/fuzzy pour éviter un résultat vide sur une faute simple d'orthographe.

### Diagnostic assisté par la base

Créer à terme un chemin : symptôme -> DTC -> paramètres à observer -> câblage/broches -> composants -> tests -> pannes connues -> causes plausibles, en affichant toujours le niveau de confiance et la source.

## Priorité moyenne

### Profils physiques de communication

Séparer explicitement le noyau protocole commun et la couche physique : 1.2/1.3/1.6 en ROSCO/UART 5 V 3 broches ; 1.9 en K-Line avec réveil spécifique. Empêcher les choix de câble incompatibles.

### Plages normales issues de traces réelles

Exploiter des logs ECU réels pour construire des plages de référence par ECU, moteur, injection et conditions de fonctionnement. Ne jamais transformer une plage observée sur un véhicule en valeur universelle.

### Navigateur firmware / calibration

Afficher les correspondances véhicule, moteur, boîte, calibration, référence ECU, ROM connue et provenance. Permettre la recherche par référence, calibration ou identifiant D1.

### Antidémarrage et réglages de service

Les familles 1.2/1.3/1.6/1.9 sont documentées comme capables de fonctions d'appairage/statut antidémarrage, effacement adaptations et réglages de service. Avant activation, vérifier les commandes exactes et les conditions par génération/firmware.

## Recherche avancée / désactivée tant que non maîtrisée

### Modes diagnostic usine / lecture RAM-ROM

Des sources publiques décrivent plusieurs modes diagnostic et commandes mémoire. Les conserver dans la base avec niveau de confiance, mais ne pas les activer avant compréhension complète des transitions de mode et des effets secondaires.

### Comparaison librosco

Comparer la `librosco.dll` de 63 161 octets trouvée dans `LeopoldG/mems-rosco` avec la DLL de 49 672 octets actuellement embarquée par ECU MEMS Manager. Examiner exports, fonctions réellement supplémentaires et compatibilité ABI avant toute substitution.

### Écriture ROM / remapping

MEMS 1.9 stock utilise une ROM OTP/interne : la lecture est possible, l'écriture stock n'est pas une opération normale. Toute fonction d'écriture/remapping doit rester hors interface standard tant que le matériel, le protocole et les sécurités ne sont pas parfaitement définis.
