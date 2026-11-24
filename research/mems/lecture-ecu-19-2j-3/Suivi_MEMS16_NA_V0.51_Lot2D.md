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
