# ECU MEMS Manager SESSION — audit des correctifs empiles

Etat de travail : 9 novembre 2026. Branche de nettoyage : `cleanup-session-source-consolidation-20261109`.

## Protection

- Reference fonctionnelle et installable conservee : BUILD 134, commit `e59247313a18cdc171e6fe0fe332218422ac6940`, branche `dev-complete-build117-14-pages-20261109`.
- Les changements de nettoyage ne sont pas publies comme installateur et ne sont pas reintegres automatiquement dans cette reference.
- Ne pas modifier les 14 pages, leurs dispositions approuvees (BUILD 117), le stockage de donnees, RAVE/MEMSLibrary, ROSCO ou l'IA ONNX hors demande explicite.
- Nom visible unique : **ECU MEMS Manager SESSION**. Les identifiants internes CMake, Qt et la base existante restent inchanges pour compatibilite.

## Suppressions et reintegrations deja effectuees

| Ancien fichier | Etat | Nouvelle source de responsabilite |
|---|---|---|
| `settingsvisualpatch.cpp` | Retire avant la creation de cette branche | `settingstabdark.cpp` (dimensions, cadrans, boutons) |
| `splashtextfix.cpp` | Retire de la compilation et du depot | `main.cpp` (splash, textes, progression, identite BUILD) |
| `taborderpatch.cpp` | Supprime ; etait deja non compile | La navigation active reste dans `navigationorderpatch.cpp` |
| `sidebarbubblecleaner.cpp` | Retire de la compilation et du depot | `summarytab.cpp`, `summarytab.h`, `tooltipbubblemanager.cpp`, `darkstyle.cpp`, `uirebuild.cpp` |
| `readabilityfix.cpp` | Retire de la compilation et du depot | `analysistab_impl_part1.inc`, `summarytab.cpp`, `diagnosticpanel.cpp`, `dedicatedpagesrebuild.cpp` |

Transferts effectues sans retirer les donnees ou les fonctions ECU. Compilation Windows reussie pour les premiers lots ; tout lot non encore valide demeure en test.

## Modules encore presents a examiner, sans suppression aveugle

- **Presentation globale conflictuelle :** `strictvisualrebuild.cpp`, `visualcompletion.cpp`, `darkfixes.cpp`, `darkstyle.cpp`, `uirebuild.cpp`.
- **Constructeurs/pages indispensables a consolider :** `overviewrebuild.cpp` (Apercu), `majorpagesrebuild.cpp` (Reglages de base, Erreurs, Actionneurs), `dedicatedpagesrebuild.cpp` (pages dediees), `errorsvisualpatch.cpp` (Erreurs), `analysismodernizer.cpp` (Analyse).
- **Fonctions metier et navigation a proteger :** `navigationorderpatch.cpp` (14 pages et commandes), `mainwindowtitlebarfix.cpp` (instance unique, confirmation de fermeture et commandes), `systemstateconnectionfix.cpp` (etats ECU deconnecte).
- **Source consolidee a garder :** `settingstabdark.cpp` (Reglages).
- **Fichiers de base documentaire avec "Patch" dans le nom :** `database/IaDocumentaryLockPatch.cpp`, `database/MemsDatabaseDiagramSearchPatch.cpp`, `database/MemsSearchCompletenessPatch.cpp`. Ils contiennent des fonctions de recherche documentaire et ne doivent pas etre detruits sur le seul critere de leur nom.

## Regle de nettoyage

1. Identifier ce qu'un module rend reellement, quels objets il cree et quels signaux il connecte.
2. Transferer ses fonctions dans la source qui possede le composant, sans changement visible ou fonctionnel.
3. Eliminer le traitement concurrent (`Q_COREAPP_STARTUP_FUNCTION`, filtres globaux, timers de retouche) quand sa responsabilite a ete transferee.
4. Supprimer l'ancien fichier de CMake, puis du depot.
5. Compiler et tester ; conserver les dispositions et fonctions precedemment validees. Une compilation verte n'est pas une validation du rendu graphique.

## Livraison

La derniere version de test installee et publiee reste la BUILD 134. Les prochaines sorties utiliseront le nom de produit et d'artefact **ECU MEMS Manager SESSION**, sans les suffixes `COMPLETE` ou `TEST` ; version/build sont suivis dans les metadonnees techniques. Aucune nouvelle Release n'est creee par le workflow de consolidation.
