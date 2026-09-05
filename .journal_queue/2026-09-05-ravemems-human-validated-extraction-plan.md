## 2026-09-05 — Nouveau planning officiel : extraction RAVE validée sémantiquement avant SQLite/DLL

Décision utilisateur : abandonner la logique où RAVEMEMS V2 tente de décider seul, par heuristiques, de la structure sémantique et des visuels pertinents. La nouvelle méthode fait de RAVEMEMS V2 un moteur d’extraction fidèle et reproductible, tandis que l’analyse sémantique des procédures/illustrations est validée explicitement avant assemblage en base.

Baseline applicatif confirmé : BUILD #101 `22dbe75e...`. Les BUILD #102 à #105 ne sont pas utilisés comme fondation documentaire. Aucun BUILD #106 ne doit être lancé pendant cette reconstruction.

### Planning officiel

1. Figer les sources et le périmètre
- Source documentaire : exactement les 47 PDF RAVE originaux du corpus canonique.
- Inventaire SHA/blob/nom/page-count avant traitement.
- Aucun réemploi de SQLite, DLL ou visuels dérivés des chaînes #102+ comme source de vérité.

2. Corriger RAVEMEMS V2 comme moteur d’extraction brute fidèle
- Conserver lecture texte native, géométrie de spans, numéros de pages, métadonnées, provenance.
- Pour les images uniquement : exclure la zone d’en-tête ; ne rien exclure de cette zone pour le texte ou la structure documentaire.
- Réduire la marge de capture actuelle (`24 pt` gauche/haut/droite et `32.4 pt` bas) après mesure sur pages tests ; la marge ne doit jamais remonter dans l’en-tête.
- Corriger le regroupement des tracés vectoriels afin d’éviter les unions artificielles de zones sans rapport.
- Préserver rectangle source exact et rectangle final de crop séparément.
- Rejeter pictogrammes/icônes/logos décoratifs non utiles comme preuves techniques.

3. Construire une couche intermédiaire auditable par document/page
Pour chaque page utile, produire des enregistrements structurés avec : document, révision, langue, page physique, section/titre, opération constructeur, phase, étape, texte exact, avertissement/note/spécification, références croisées, rectangles source, visuels retenus, visuels rejetés et motif, SHA/provenance.

4. Validation sémantique contrôlée
- Commencer par `RCL0193ENG` comme document pilote.
- Vérifier page par page les opérations/procédures/illustrations réelles contre le PDF source.
- Cas de référence obligatoire : p.53, opération `12.21.28`, `ENGINE CRANKSHAFT PRIMARY GEAR END-FLOAT`, valeur `0.089–0.165 mm`, contrôle par feeler gauges, vraie illustration mécanique retenue, icône d’en-tête rejetée.
- Aucun passage au corpus complet tant que le document pilote n’est pas propre.

5. Étendre par lots au corpus complet
- Traiter les 47 PDF par lots contrôlés.
- Après chaque lot : vérifier pages, texte, opérations, étapes, visuels, exclusions, géométrie, provenance et absence de mélange entre documents.
- Toute anomalie de moteur est corrigée puis le lot est rejoué avant acceptation.

6. Assemblage SQLite seulement après validation de l’extraction
- Construire une nouvelle base à partir de la couche intermédiaire validée.
- Liens explicites : document -> révision -> langue -> page -> opération -> phase -> étape -> texte -> visuels associés.
- Contrôles : `PRAGMA integrity_check=ok`, `foreign_key_check=0`, unicité des clés, absence de collisions document/page/langue, conformité des SHA et reproductibilité byte-identical si possible.

7. Audit des images avant DLL
- Générer inventaire visuel global avec dimensions, bbox source, crop bbox, page, opération/étape liée et statut retenu/rejeté.
- Contrôle visuel par échantillons + cas critiques.
- Aucun pictogramme d’en-tête ne doit être présent comme illustration technique.

8. Reconstruire la DLL après validation SQLite
- Nouveau contrat orienté provenance structurée.
- La DLL doit retourner directement document, révision, langue, page, opération/section, extrait, IDs de visuels pertinents et provenance.
- Pas de reconstruction de provenance dans MEMS Manager à partir de chaînes texte.
- Tests DLL indépendants de Qwen/MEMS Manager sur plusieurs questions réelles.

9. Validation documentaire sans IA
- Question -> résultat base/DLL -> bonne page/opération/valeur/image, sans Qwen.
- Cas pignon primaire obligatoire, plus batterie, papillon et plusieurs cas hors RCL0193ENG.
- Aucun faux positif de sous-chaîne type `axial` -> `coaxial`.

10. Retour dans MEMS Manager uniquement après tout ce qui précède
- Repartir du baseline applicatif BUILD #101.
- Intégrer la nouvelle DLL + nouvelle SQLite + nouveau catalogue visuel validé sur branche temporaire.
- Tests IA et UI seulement après validation de la couche documentaire.
- Aucun build production suivant sans validation et autorisation explicites.

### Jalons de validation
J0 sources figées ; J1 moteur RAVEMEMS V2 corrigé ; J2 RCL0193ENG pilote validé ; J3 corpus complet extrait et audité ; J4 SQLite validée ; J5 images validées ; J6 DLL validée sans IA ; J7 intégration temporaire MEMS Manager ; J8 seulement alors décision de nouveau build production.

### PROCHAINE ACTION EXACTE
Créer une branche temporaire dédiée à cette nouvelle fondation RAVEMEMS V2 à partir du moteur de référence, sans toucher MEMSX64. Première tâche : produire un audit géométrique sur plusieurs pages RAVE afin de fixer précisément la frontière d’en-tête image, la nouvelle marge de crop et les règles de regroupement vectoriel, puis journaliser ces paramètres avant toute pousse du moteur corrigé.
