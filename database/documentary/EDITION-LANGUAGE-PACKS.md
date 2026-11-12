# RAVE — bases documentaires séparées par langue

## Décision (12 novembre 2026)

La source Rover Mini **AKM 7169** existe dans plusieurs éditions linguistiques du constructeur.
La base française doit être construite **à partir du document français AKM 7169FRE**,
et non par traduction automatique de la base anglaise **AKM 7169ENG**.

Le moteur de traduction de masse OPUS-MT et le comparatif Hy-MT2 ne sont plus
la chaîne de production documentaire. Leurs workflows de traitement sont
désactivés et leurs résultats ne peuvent pas être publiés.

## Sources réellement disponibles

| Langue | Édition | Fichier source vérifié dans le dépôt | État |
| --- | --- | --- | --- |
| Anglais | AKM 7169ENG | Oui — MEMS Forge, révision ed82bcb4db6c742f8e8a66ca5db81202ad18da78 | Corpus et lecteur existants |
| Français | AKM 7169FRE | Non — édition référencée publiquement, PDF constructeur non présent dans les dépôts examinés | **Attente du PDF exact et de son empreinte** |
| Portugais | À inventorier | Non vérifié | En attente |
| Espagnol | À inventorier | Non vérifié | En attente |
| Italien | À inventorier | Non vérifié | En attente |
| Allemand | À inventorier | Non vérifié | En attente |

**Attention :** une édition portant le même numéro de publication peut comporter
des différences de pagination, de mise en page ou de révision. La correspondance
des opérations, références et véhicules doit être auditée et non supposée.

## Contrat de publication

1. Une base SQLite documentaire **complète et autonome par langue** avec
   un schéma de consultation commun à l'IA MEMS et à la page Base de données.
2. Chaque base conserve sa propre provenance : code de langue, édition,
   SHA-256 du PDF, date/révision, pagination et contrôle d'intégrité.
3. Le français n'est proposé que si le pack français est **entièrement**
   construit, audité et disponible. Aucun mélange de procédures anglaises et
   françaises n'est autorisé dans une réponse française.
4. L'anglais demeure le corpus documentaire de repli pour les langues
   dont la base complète n'est pas encore disponible, en l'annonçant clairement.
5. Le changement de langue sélectionne un **fichier SQLite**, pas une substitution
   de mots, une traduction Qwen à l'affichage ou un croisement avec l'autre langue.
6. Le programme doit rester utilisable hors ligne : les packs validés sont
   installables localement. Un téléchargement facultatif peut être proposé
   lorsqu'une connexion est disponible, avec SHA-256/signature et contrôle de version.
7. Le code anglais actuel et le Setup de production restent intacts jusqu'à
   l'intégration et la vérification du pack français. Les sources constructeur
   ne sont jamais réécrites par MEMS Manager SESSION.
8. Avant de publier ou de redistribuer un PDF ou une base intégrant sa
   transcription, les droits d'utilisation et de redistribution doivent être vérifiés.

## Avancement vérifiable

- Corpus anglais Rover 482 pages : présent et contrôlé.
- Édition française AKM 7169FRE : identifiée, **fichier source pas encore récupéré**.
- Extraction française à partir du PDF constructeur : **non commencée**.
- Couverture française validée de la future base française : **0 %**.
- Aucun nouveau Setup autorisé à ce stade.

Le pourcentage français futur sera calculé sur le nombre réel d'éléments
extraits du document français et validés, pas sur les 11 079 unités du corpus
anglais, qui peuvent ne pas être identiques après extraction.
