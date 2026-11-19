# Illustrations de l'Aide — ECU MEMS Manager SESSION

Ce dossier contient **13 captures d'écran fournies et validées** pour les guides intégrés à SESSION.

## Correspondance captures / rubriques

| Capture | Rubrique |
| --- | --- |
| Capture d’écran (896).png | Réglages ECU |
| Capture d’écran (897).png | Erreurs et anomalies en direct |
| Capture d’écran (898).png | Diagnostic automatique |
| Capture d’écran (899).png | Toutes les mesures |
| Capture d’écran (900).png | Toutes les données / trames |
| Capture d’écran (901).png | Aperçu |
| Capture d’écran (905).png | Test câble USB / reconnaissance du port COM |
| Capture d’écran (907).png | Actionneurs |
| Capture d’écran (910).png | Injection |
| Capture d’écran (911).png | Analyse des journaux |
| Capture d’écran (912).png | Base de données documentaire |
| Capture d’écran (914).png | IA MEMS |
| Capture d’écran (915).png | Écran de démarrage |

## Intégration

Les six guides sources sont à la racine du dépôt : `help.html.in` (français), `help_en.html.in`, `help_es.html.in`, `help_it.html.in`, `help_pt.html.in` et `help_de.html.in`.

La configuration CMake repère automatiquement les fichiers PNG/JPG/JPEG de ce dossier et les embarque dans le Setup Windows comme ressources Qt sous `:/help/images/<nom du fichier>`. Les six guides utilisent ces captures **hors ligne**, sans dépendance à un hébergement externe.

Les captures proviennent de l'interface française. Le texte explicatif des cinq autres guides est traduit, mais le texte faisant partie d'une image reste celui de la capture d'origine.

Pour ajouter une nouvelle capture, la déposer ici, puis la référencer dans la rubrique appropriée du guide. Ne pas ajouter de données personnelles ou de secrets dans les captures.
