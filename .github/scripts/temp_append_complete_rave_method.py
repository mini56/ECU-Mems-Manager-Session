from pathlib import Path

REPORT = r'''

## 2026-08-31 - METHODE RAVE COMPLETE UTILISATEUR FINAL - REGLE CANONIQUE

### CONSTAT ET CORRECTION OBLIGATOIRE
Les lots RCL0193ENG p193-234 generes par la methode directe GitHub ont conserve le texte source integral et les principales donnees techniques, mais la presence d'un `visual_candidate` ne garantit pas un asset image local reel et exploitable par ECU MEMS Manager. La conformite utilisateur final exige egalement les tableaux structures, les procedures entierement structurees et les portees vehicule/moteur/variante/marche lorsqu'elles sont explicites dans la source.

Les lots p193-202 MANUAL GEARBOX, p203-210 DRIVE SHAFTS, p211-226 STEERING et p227-234 SUSPENSION doivent donc etre repris/completes avant de poursuivre p235 BRAKES. Les donnees textuelles deja exactes sont conservees ; on complete ce qui manque, sans perte ni reinterpretation.

### CONTENU OBLIGATOIRE POUR QU'UNE PAGE RAVE SOIT COMPLETE
Pour chaque page utile, conserver tous les elements applicables :
- texte source integral, document, edition, page physique/logique et provenance exacte ;
- faits, valeurs, couples, reglages, warnings, cautions, notes, outils, consommables et remplacements ;
- procedures entierement structurees, dans l'ordre constructeur, avec etapes, avertissements, outils, couples, remplacements et controles associes ;
- tableaux structures en lignes/colonnes/cellules, tout en conservant leur preuve visuelle ;
- portee explicite vehicule, moteur, variante, marche, annee, transmission, induction et VIN/plage lorsqu'elle existe ; une portee absente reste inconnue et ne doit jamais etre inventee ;
- tous les schemas, illustrations, vues, tableaux graphiques et autres supports visuels utiles sous forme d'assets locaux reels avec chemin, SHA-256, dimensions et liaison exacte a la page/figure/connaissance.

Si le schema courant ne sait pas representer une information utile, ajouter de maniere additive la structure necessaire au lieu d'abandonner l'information.

### REGLE UNIQUE POUR LES TEXTES DANS LES IMAGES
Une seule methode d'affichage est autorisee afin de limiter les risques de regression : **IMAGE CONSTRUCTEUR CONSERVEE + MASQUE NUMEROTE + LEGENDE TRADUITE DANS LE MEME RENDU IMAGE**.

- Conserver l'image constructeur originale intacte comme preuve.
- Detecter les zones contenant du texte humain, en priorite a partir des coordonnees texte natives du PDF afin d'eviter l'OCR lorsque ce n'est pas necessaire.
- Chaque zone traduisible recoit une cle stable, par exemple `RCL0193ENG_P193_T01`, et un numero stable.
- Dans le rendu utilisateur, le texte humain d'origine est masque/remplace par son numero ; la legende `1. texte traduit`, `2. texte traduit`, etc. apparait **dans le meme rendu image**.
- Ne jamais masquer ou modifier les ancres techniques immuables : reperes constructeur, references Rover et outils, references de pieces, valeurs, unites, dimensions, connecteurs, numeros de procedure, fleches, geometrie et autres identifiants techniques.
- Stocker une seule image source et les zones/masques/cles ; ne pas dupliquer une image raster complete par langue.

Locales actuelles du programme : `fr`, `en`, `es`, `it`, `pt`, `de`. L'architecture doit etre Unicode et extensible sans refonte a `ja` (japonais), `zh` (chinois), `hi` (hindi/Devanagari) et autres scripts futurs.

### PIPELINE GITHUB CANONIQUE A AUTOMATISER
Le traitement doit lire directement le corpus canonique `main/rave/` et, pour chaque lot :
1. verifier fichier source, SHA-256, taille, edition et nombre de pages ;
2. extraire texte et coordonnees natives ;
3. rendre les pages et extraire les assets visuels reels ;
4. classifier chaque page `blank`, `text`, `visual`, `table`, `procedure` ou `mixed` ;
5. detecter les zones traduisibles et creer masque numerote, cles stables et donnees de traduction ;
6. structurer les tableaux et conserver aussi leur image/preuve ;
7. structurer les procedures completes dans l'ordre constructeur ;
8. extraire uniquement les portees explicites sans generalisation inventee ;
9. generer SQL, QZ64, audit et assets ;
10. executer un garde de completude page par page : aucun element applicable ne doit rester silencieusement absent ; toute incertitude devient `needs_review` et est revue manuellement ;
11. appliquer deux fois le SQL et verifier idempotence, `PRAGMA integrity_check=ok`, `PRAGMA foreign_key_check=0`, `user_version` attendu et roundtrip QZ64->SQL ;
12. verifier le perimetre final et que `MEMSX64` reste strictement BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`.

### OBJECTIF DE PERFORMANCE
Automatiser tout ce qui peut l'etre a partir du PDF natif et des coordonnees de texte. La revue humaine n'intervient que pour les zones incertaines. Une page ne doit jamais etre declaree complete parce que le texte existe seulement : image, tableau, procedure et portee applicables font partie de l'information utilisateur finale au meme titre que le texte.

### REPRISE OBLIGATOIRE
Reprendre avec cette methode complete, dans l'ordre :
- p193-202 MANUAL GEARBOX ;
- p203-210 DRIVE SHAFTS ;
- p211-226 STEERING ;
- p227-234 SUSPENSION.

Ne pas poursuivre p235 BRAKES avant validation du pilote p193-202 et preuve que la fonction complete est reproductible sur les lots suivants.

### PROCHAINE ACTION EXACTE
Construire et valider sur p193-202 un pilote GitHub reproductible produisant effectivement : assets visuels locaux reels, zones/masques numerotes et cles multilingues, legendes rendables dans l'image, tableaux structures, procedures entierement structurees, portees explicites et texte source integral. Ajouter un garde automatique qui refuse le lot si un element applicable est seulement `candidate` au lieu d'etre integre ou explicitement `needs_review`. Une fois le pilote vert et inspecte, appliquer exactement la meme fonction a p203-210, p211-226 et p227-234 avant p235 BRAKES.
'''

p = Path('RAPPORT_SUIVI_ECU_MEMS_MANAGER.md')
text = p.read_text(encoding='utf-8')
marker = '## 2026-08-31 - METHODE RAVE COMPLETE UTILISATEUR FINAL - REGLE CANONIQUE'
if marker not in text:
    if text and not text.endswith('\n'):
        text += '\n'
    text += REPORT
    p.write_text(text, encoding='utf-8', newline='')
