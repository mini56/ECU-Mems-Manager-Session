# CAHIER DES CHARGES RAVEMEMS
## Nouvelle extraction documentaire structurée pour ECU MEMS Manager

**Statut : BROUILLON À RELIRE ET COMPLÉTER PAR L’UTILISATEUR**  
**Date : 2 septembre 2026**  
**Projet : ECU MEMS Manager**  
**Fonction : RAVEMEMS**

---

## 1. OBJECTIF

RAVEMEMS doit être le moteur générique d’ingestion documentaire de MEMS Manager.

Le nom **RAVEMEMS** est conservé. Il vient du travail réalisé sur les manuels RAVE, mais la fonction ne doit pas être limitée aux manuels RAVE, à Rover/MG/Mini, à l’anglais ou à un format documentaire particulier.

RAVEMEMS doit pouvoir traiter à terme tout document technique pertinent, quelle que soit sa langue, afin d’enrichir la base de données MEMS Manager et l’IA MEMS avec procédures, spécifications, schémas, illustrations, tableaux et informations de diagnostic.

Le corpus RAVE actuel sert de **premier corpus de validation complet** de cette fonction.

---

## 2. PRINCIPE GÉNÉRAL

Le document d’origine est extrait **une seule fois dans sa langue source**.

Chaîne cible :

**Document original → détection de langue → extraction structurée source → relations → visuels → traductions → base MEMS Manager → IA MEMS**

Une traduction ne doit jamais obliger à refaire l’extraction du document.

Le même principe doit fonctionner avec un document dont la langue source est l’anglais, le français, l’allemand, l’italien, l’espagnol, le portugais ou toute autre langue exploitable.

---

## 3. LE PDF ORIGINAL RESTE LA SOURCE PRIMAIRE

Le PDF original est conservé afin de permettre une éventuelle ré-extraction, le contrôle d’un élément, l’import d’une édition plus récente ou la reconstruction d’un visuel.

Il n’est pas nécessaire de conserver en parallèle une copie texte complète de toutes les pages uniquement comme preuve.

---

## 4. LA PAGE PDF N’EST PLUS L’UNITÉ DE CONNAISSANCE

La page PDF devient principalement une **provenance interne**.

RAVEMEMS doit reconstruire la structure réelle du manuel :

**Document → Chapitre → Section / sous-section → Opération constructeur → Phase → Étapes ordonnées → Informations associées → Visuels associés**

Exemples de phases : Dépose/Remove, Repose/Refit, Contrôle/Test, Réglage/Adjust, Inspection, Vidange, Remplissage, Diagnostic et autres phases réellement présentes.

---

## 5. PROCÉDURES SUR PLUSIEURS PAGES

Une procédure qui traverse plusieurs pages doit rester **une seule procédure**.

Exemple :

Page 133 : Dépose 1 à 8.  
Page 134 : Dépose 9 à 21.

RAVEMEMS doit reconstruire : **Dépose : étapes 1 à 21**.

La coupure physique entre deux pages PDF ne doit jamais faire croire à l’IA que l’étape 9 est le début de la procédure.

---

## 6. DÉPOSE ET REPOSE RESTENT DES PHASES DISTINCTES

La numérotation constructeur réelle doit être conservée.

Exemple :

**Dépose** : 1, 2, 3…  
**Repose** : 1, 2, 3…

Il ne faut jamais transformer artificiellement `Repose 1` en étape 9, 15 ou 22 simplement parce qu’une phase Dépose la précède.

---

## 7. NUMÉROS ET RÉFÉRENCES CONSTRUCTEUR

Conserver lorsqu’ils existent : numéro d’opération, numéro de procédure, numéro de figure, référence de connecteur, référence de composant, référence d’outil, référence de pièce et autre identifiant constructeur utile.

Exemples : `12.29.02`, `19.22.61`, `C159-30`, `C158-27`.

Ces informations ne doivent pas être traduites.

---

## 8. TEXTE SOURCE À CONSERVER

Ne pas conserver systématiquement une copie texte entière de chaque page.

Conserver le texte source original des éléments techniques réellement structurés : titre, chapitre, section, opération, phase, étape, avertissement, précaution, remarque, condition, spécification, couple, valeur, tableau, renvoi, légende utile et autre contenu technique.

Ce texte source sera la base des traductions futures.

---

## 9. RELATIONS ENTRE OPÉRATIONS

Les renvois du manuel doivent devenir autant que possible des relations internes réelles :

- `Voir…` / `Se reporter à…` ;
- opération préalable ;
- opération nécessaire après remplacement ;
- contrôle ou réglage consécutif ;
- avertissement ou précaution liée ;
- valeur ou spécification liée ;
- illustration, schéma ou tableau lié.

Exemples logiques :

**Opération A → nécessite l’opération B**  
**Opération A → après repose, effectuer le réglage C**  
**Opération A → voir également l’opération D**  
**Opération A → illustration F**

Ces relations doivent aider l’IA à guider l’utilisateur.

---

## 10. COMPORTEMENT DE L’IA — QUESTION GÉNÉRALE

Une question très générale ne doit pas provoquer une réponse énorme.

Exemple :

**Utilisateur :** `alternateur`

Comportement attendu :

**Que voulez-vous concernant l’alternateur ?**
1. Procédure de dépose
2. Contrôle de l’alternateur
3. Réglage de la courroie d’alternateur

Ces choix doivent être générés à partir des rubriques réellement présentes dans la base. Ils ne doivent pas être codés en dur pour le mot `alternateur`.

---

## 11. COMPORTEMENT DE L’IA — QUESTION PRÉCISE

Lorsque la question est déjà suffisamment précise, l’IA répond directement.

Exemples :

`dépose alternateur` → procédure de dépose.  
`couple vis pompe à eau` → valeur et unité.  
`broche MAP` → connecteur + broche + fonction.  
`où est le capteur vilebrequin` → localisation / implantation.

L’IA ne doit pas demander une clarification inutile lorsque la cible est déjà déterminée.

---

## 12. SUGGESTIONS APRÈS LA RÉPONSE

Après avoir répondu à la demande principale, l’IA peut proposer une information connexe réellement pertinente.

Exemple :

**Je vous conseille également de consulter le réglage de la courroie d’alternateur après repose.**

Cette proposition doit être fondée sur une relation constructeur ou une relation technique enregistrée. L’IA ne doit pas inventer un lien simplement parce que deux sujets semblent proches.

---

## 13. LIMITER LES PROPOSITIONS

Le premier menu doit rester court et utile : **3 choix maximum**, puis une quatrième option **`4. Autre chose ?`**. L’utilisateur peut donc répondre `1`, `2`, `3` ou `4`. S’il choisit `4`, l’IA lui demande simplement ce qu’il souhaite savoir d’autre afin qu’il puisse formuler librement une autre demande.

La base peut contenir beaucoup d’informations sans obliger l’utilisateur à tout voir.

---

## 14. CONTEXTE VÉHICULE / ECU / VARIANTE

Une même opération peut varier selon modèle, moteur, SPi/MPi, famille MEMS, version ECU, année, marché, boîte, climatisation, équipement ou autre variante.

RAVEMEMS doit conserver ce contexte lorsqu’il est présent dans le document.

Si un véhicule / ECU est actuellement connecté, l’IA ne doit pas supposer automatiquement que la question concerne ce véhicule. Elle doit demander à l’utilisateur si sa question concerne le véhicule actuellement connecté. L’utilisateur peut très bien demander une information sur un autre véhicule.

Si aucun véhicule / ECU n’est connecté, l’IA ne connaît pas le véhicule concerné et ne doit pas en inventer un. Elle doit utiliser les informations données dans la question ou demander le véhicule / ECU / variante nécessaire.

Si plusieurs réponses incompatibles restent possibles, l’IA doit demander la variante nécessaire avant de répondre.

---

## 15. SUPPRESSION DE LA NOTION DE « PREUVE » CÔTÉ UTILISATEUR

Décision retenue : **ne plus encombrer les réponses de l’IA avec une preuve documentaire systématique**.

À supprimer de la présentation normale :
- `preuve constructeur` ;
- référence de page affichée systématiquement ;
- référence PDF affichée systématiquement ;
- bouton `Voir la source` systématique.

L’utilisateur demande une information technique, pas un dossier de justification documentaire.

---

## 16. PROVENANCE INTERNE MINIMALE

RAVEMEMS conserve le minimum nécessaire au fonctionnement interne : document, numéro de page PDF source, opération, phase, identifiant du contenu et identifiant du visuel.

Cette provenance sert à raccorder les procédures multi-pages, éviter les mélanges, reconstruire les relations, réimporter une nouvelle édition et retrouver le bon visuel. Elle reste invisible dans la réponse normale.

---

## 17. NOUVELLE EXTRACTION DES IMAGES OBLIGATOIRE

Les images du corpus actuel doivent être réextraites.

L’extraction directe d’un objet bitmap PDF peut perdre la rotation appliquée dans la page, le rendu de masque, la polarité ou certaines caractéristiques de composition.

RAVEMEMS ne doit donc plus considérer automatiquement `extract_image()` comme le visuel utilisateur final.

---

## 18. RENDU FIDÈLE DES VISUELS

Le visuel destiné à MEMS Manager doit être généré depuis un rendu fidèle du PDF, ou une méthode donnant le même résultat.

Le rendu doit conserver : orientation réelle, transformations PDF, couleurs, fond, masque et disposition réelle.

Objectif : **ce que voit l’utilisateur dans MEMS Manager doit correspondre à ce que le manuel affiche réellement.**

---

## 19. RECADRAGE DES IMAGES

Lorsque la page est grande mais que l’illustration utile est petite, RAVEMEMS doit privilégier un recadrage propre de la zone technique utile.

Bénéfices : aperçu plus lisible, image utile plus grande, moins besoin de zoomer, stockage réduit et moins de texte parasite.

---

## 20. SUPPRESSION DES EN-TÊTES ET PIEDS DE PAGE DANS LES VISUELS

Pour les pages ou zones graphiques conservées, supprimer lorsque cela est possible sans perte technique :
- en-têtes répétitifs ;
- pieds de page ;
- numéros de page imprimés ;
- marges inutiles ;
- mentions éditoriales répétitives.

Cela évite également de traduire du texte inutile.

---

## 21. CE QUI NE DOIT JAMAIS ÊTRE SUPPRIMÉ D’UN VISUEL

Ne jamais retirer : numéro de repère utile, légende technique, numéro de broche, numéro de connecteur, valeur, unité, avertissement, référence indispensable à la compréhension, flèche, symbole ou texte intégré nécessaire au schéma.

Le nettoyage doit améliorer la lecture, jamais dégrader l’information.

---

## 22. TYPES DE VISUELS

Catégories initiales :
- schéma électrique ;
- illustration mécanique ;
- implantation / localisation ;
- vue de connecteur ;
- tableau ;
- photo / vue technique ;
- autre visuel technique.

Cette classification doit aider l’IA à choisir le bon visuel selon la question.

---

## 23. CHOIX DU VISUEL SELON LA QUESTION

`broche MAP` → priorité au schéma électrique / connecteur.  
`où est le MAP` → priorité à une implantation.  
`dépose injecteur` → priorité à l’illustration mécanique associée à la dépose.  
`couple pompe à eau` → priorité au tableau ou à la procédure contenant la valeur si un visuel apporte quelque chose.

Le fait qu’un composant apparaisse quelque part dans une grande page ne suffit pas pour considérer cette page comme le meilleur visuel.

---

## 24. BOUTONS VISUELS CÔTÉ UTILISATEUR

Le bouton doit correspondre à ce qui est réellement proposé :
- **Voir le schéma** ;
- **Voir l’illustration** ;
- **Voir le tableau** ;
- autre libellé adapté si nécessaire.

La référence interne au document ou à la page n’a pas besoin d’être affichée.

---

## 25. TABLEAUX

Les tableaux ne doivent pas être réduits à un gros bloc de texte lorsque leur structure peut être récupérée.

Extraire autant que possible colonnes, lignes, libellés, valeurs, unités et conditions, particulièrement pour les couples de serrage, résistances, tensions, jeux, pressions, températures, valeurs de contrôle et caractéristiques.

---

## 26. VALEURS ET SPÉCIFICATIONS

Les spécifications doivent être exploitables directement par l’IA.

Exemple : **Pompe à eau — boulons : 22 N·m** plutôt que de forcer l’IA à rechercher cette valeur dans un paragraphe de plusieurs dizaines de lignes.

Le texte constructeur original reste conservé dans l’élément source correspondant.

---

## 27. DÉDUPLICATION

Si une même image ou une même information apparaît plusieurs fois, RAVEMEMS peut éviter de stocker plusieurs copies identiques tout en conservant plusieurs relations vers leurs contextes d’utilisation.

---

## 28. IDENTIFIANTS STABLES

Chaque connaissance importante doit disposer d’un identifiant interne stable.

Une traduction ne crée pas une nouvelle opération. La même opération peut avoir son texte source et plusieurs traductions.

Même principe pour étape, valeur, renvoi, visuel et tableau.

---

## 29. MISE À JOUR D’UN DOCUMENT

RAVEMEMS doit pouvoir gérer une nouvelle version d’un document et mettre à jour le document concerné sans reconstruire toute la base documentaire.

---

## 30. CONSERVATION TEMPORAIRE DU CORPUS ACTUEL

Le corpus RAVEMEMS actuel ne doit pas être supprimé avant validation du nouveau pipeline.

Il sert de contrôle de non-régression pour vérifier qu’aucune page, procédure, valeur, image, information OCR ou connaissance déjà présente n’est perdue.

---

## 31. STRATÉGIE DE DÉVELOPPEMENT

Ne pas lancer immédiatement une nouvelle extraction des 47 PDF.

Commencer par un **prototype sur un seul manuel représentatif** afin de valider la méthode avant de multiplier le travail.

---

## 32. CAS OBLIGATOIRES À VALIDER SUR LE PROTOTYPE

Le premier prototype doit tester au minimum :
1. une procédure qui traverse plusieurs pages ;
2. une opération avec Dépose + Repose ;
3. un renvoi `voir…` vers une autre opération ;
4. une image actuellement extraite blanc sur noir ;
5. une image actuellement tournée à 90° ;
6. un schéma électrique ;
7. une illustration mécanique ;
8. un tableau ;
9. une valeur de couple ;
10. une variante véhicule / ECU ;
11. une relation correcte entre une procédure et son illustration ;
12. une question générale donnant un menu court ;
13. une question précise donnant directement la réponse.

---

## 33. CRITÈRES DE RÉUSSITE AVANT LE CORPUS COMPLET

Le prototype doit démontrer que les étapes multi-pages restent continues, les phases restent distinctes, les bonnes images sont associées, les images ont le bon sens et le bon fond, les visuels sont lisibles, les informations parasites sont supprimées sans perte technique, les renvois sont exploitables, les variantes ne sont pas mélangées et l’IA sait distinguer demande générale et demande précise.

Seulement après cette validation, le pipeline pourra être appliqué au corpus complet.

---

## 34. PRINCIPE DE FONCTIONNEMENT POUR L’IA

L’IA ne doit plus fonctionner principalement comme :

**question → recherche de quelques fragments de pages → assemblage d’une réponse**

Elle doit fonctionner comme :

**question → compréhension de l’intention → sélection du type de connaissance → navigation dans la structure RAVEMEMS → réponse ciblée → visuel pertinent si utile → suggestion connexe justifiée si nécessaire**

---

## 35. EXEMPLES DE TYPES D’INTENTION

Première liste à prévoir : sujet général, procédure, dépose, repose, contrôle, réglage, diagnostic, spécification, couple, valeur, pinout/broche, câblage, localisation, fonctionnement, identification, avertissement/précaution.

Cette liste n’est pas nécessairement définitive.

---

## 36. PAS DE LOGIQUE BASÉE SUR UNE LANGUE PARTICULIÈRE

Les comportements ne doivent pas dépendre de mots français ou de mots d’une autre langue codés en dur.

La structure et les intentions doivent pouvoir fonctionner quelle que soit la langue de la question et quelle que soit la langue source du document.

---

## 37. TRADUCTIONS FUTURES

Les traductions doivent se rattacher à la connaissance structurée.

On traduit ce que l’utilisateur doit lire : titres, procédures, étapes, avertissements, descriptions, légendes textuelles utiles et contenu de tableaux si nécessaire.

On évite de traduire identifiants internes, numéros de broche, numéros de connecteur, références, valeurs numériques, unités universelles et numéros d’opération.

---

## 38. VISUELS ET TRADUCTION

Un visuel est extrait une seule fois. Il ne faut pas réextraire la même image pour chaque langue.

Si un schéma contient du texte source intégré, la stratégie de traduction de ce texte pourra être traitée séparément. Le visuel technique source doit rester stable.

---

## 39. PÉRIMÈTRE ACTUEL DU CHANTIER

Ce cahier des charges concerne RAVEMEMS, la nouvelle extraction documentaire, la structuration de la connaissance, la relation entre connaissance et visuels et la préparation de l’exploitation future par l’IA.

Il ne doit pas entraîner de modification non autorisée du protocole ECU, de l’acquisition, de la communication série, du 32 bits, de l’UI générale ou de la version de production.

---

## 40. GARDE-FOUS PROJET

- `MEMSX64` reste sur BUILD #103.
- Commit de production protégé : `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`.
- Aucun BUILD #104 sans autorisation explicite.
- 32 bits intouché.
- Aucun changement protocole dans ce chantier.
- Aucun patch / aucune rustine.
- Nouvelle extraction développée d’abord sur branche de test.
- Rapport maître mis à jour avant chaque pousse technique.

---

## 41. POINTS À RELIRE / COMPLÉTER PAR L’UTILISATEUR

Avant développement, vérifier notamment :
- quantité maximale de choix proposés pour une demande générale ;
- comportement lorsque deux variantes sont possibles ;
- catégories de visuels à ajouter ou retirer ;
- niveau de nettoyage acceptable des images ;
- types de tableaux prioritaires ;
- comportement des suggestions `Je vous conseille aussi de voir…` ;
- langues de traduction prioritaires ;
- manuel à choisir pour le premier prototype ;
- toute autre règle utilisateur à ajouter avant développement.

---

## 42. PROCHAINE ACTION

**Ne lancer aucune nouvelle extraction avant validation explicite de ce cahier des charges.**

Après relecture utilisateur :
1. intégrer les corrections et ajouts ;
2. figer une version validée du cahier des charges ;
3. inscrire cette validation dans le rapport maître ;
4. choisir le manuel prototype ;
5. concevoir le nouvel extracteur RAVEMEMS ;
6. tester le prototype ;
7. seulement ensuite envisager le corpus complet.

---

# FIN DU BROUILLON

**Ce document est volontairement un cahier des charges à relire. Il peut être corrigé, complété ou simplifié avant toute implémentation.**
