# Suivi V0.51 — Lot 2D : Analyse
Base dff2edb5a76f8e27d1ec0f65e5c3afe85d4f9018. Run 45474093612 vert (tests et Windows). Rapport V0.43 chap.47–49 et V0.50.

- Voies de mesure V2 issues du même LogV2 et du décodeur commun pour le direct et la relecture. Libellés, unités et disponibilités traduits, quatre références TPS numériques distinctes. DTC brut reste numérique dans les courbes, détail des circuits et états dans Colonnes. Les données indisponibles ne produisent pas de point zéro. Les cellules textuelles n'ont pas de courbe fictive.
- V2 : contrôle exact des en-têtes et identité firmware/ID, reconstitution locale du contexte depuis l'identité enregistrée, validation des deux trames complètes. Recalcul exclusivement depuis les octets d'origine, pas depuis les valeurs déjà converties. Horodatage ISO absolu, passages de minuit corrects. Une trame invalide devient une ligne non validée ; une date invalide ou non monotone est ignorée. Aucun fichier d'origine modifié.
- Anciens TXT/CSV : lecteur et valeurs historiques conservés. Indication en infobulle si format historique MEMS ; sans nom firmware vérifié, pas d'interprétation NA ni de correction rétroactive automatique. Les clés historiques et les fichiers Communications ECU restent inchangés.
- En direct : changement d'identité ou reconnexion NA réinitialise les séries afin de ne pas mélanger les profils; historique ordinaire et injection live conservent leurs voies. Pas de changement de dialogue, DLL, réglage ou commandes. Le D1 existant reste nécessaire.
- 17 clés ajoutées dans les six langues (73 total). Test Qt Widgets isolé (traductions I18n et relecture sans écran) : identité exacte, trames, unités, quatre TPS, DTC2/40, absence et libellés. Compile-only CI privée, pas de Setup ni publication. Validation graphique/lecture complète sur Windows et contrôle véhicule restent à faire. Aide au lot suivant.

## Validation
- Code : `fe820fc3a2d1ead9d3a2ae5c00ab2e347fddfe18` ; correction des dépendances Qt et exclusion des champs texte des courbes `6467953a9b64715acca0af3ba7ab4b9795638b65` ; CI hors écran `be4f33c5e80dec4d62588b464b064d1f57079ff9`.
- Run de référence [45476618873](https://github.com/mini56/ECU-Mems-Manager-Session/actions/runs/45476618873).
- tests : in_progress / non terminé (Valider les trois tests autonomes et les six langues)
- Pas de workflow de publication déclenché pour ce lot au contrôle. Tests de relecture et compilation Windows en attente de résultat. Aucun test sur véhicule.

## Correction compilation — 25 novembre 2026
- Runs 45476587409, 45476618873 et 45476769392 : tests autonomes réussis ; échec Windows à l'étape « Tester la relecture et les voies MEMS 1.6 NA ». C3493/C2668 dans mems16na_analysis.h:60 : argument de QStringLiteral contenant une concaténation avec la variable unit. QStringLiteral doit recevoir uniquement un littéral. Construction du suffixe dynamique en QString, contrôle de doublon préservé. Tests renforcés sur V, µs, TPS, pas et °C.
- Run 45476769392 : C1083 QAbstractButton dans i18n.cpp ; la branche actuelle contient déjà find_package(Qt5 COMPONENTS Widgets) et target_link_libraries(... Qt5::Widgets), vérifiés dans le CMake du test. Ne pas réintroduire la configuration Core seule.
- Correction directe des sources, sans nouveau lot, sans changement DLL/commandes ECU, sans publication. Ne pas considérer le lot 2D validé tant que le test Qt et la compilation Windows complète ne sont pas verts.

- Validation finale : [45563391687](https://github.com/mini56/ECU-Mems-Manager-Session/actions/runs/45563391687) vert, tests autonomes, test Qt diagnostic, test Qt Analyse et compilation Windows complète réussis. Commit 74b58f333625943063bb1f8e7c0d2847bcd38542. Validation réelle et visuelle encore à faire.
