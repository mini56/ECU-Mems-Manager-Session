# Suivi V0.49 — Lot 2B3 : Diagnostic automatique
Base b24cb65b4b774a89c748c25e7e273adfc564949c. V0.43 ch.48,49,61,63,65,66.

- Page et rapport exporté utilisent le même snapshot NA et les mêmes noms/masques/unités que les deux pages de mesures. Sept groupes DTC, défauts mémorisés comptés par bit, indicateurs inconnus distingués, 40 DTC2 informatif. DTC1/04 et /08 selon le profil. Aucune attribution TPS sur DTC2/01 ni lambda sur DTC2/08. Le chauffage lambda n'est nommé que si confirmé; l'état 7D/09 et la régulation 7D/0A sont distincts. Conversion lambda conservée.
- Écart ralenti brut -32768 et correction injection µs sans ancien seuil numérique. L'heuristique IAC dépendant de l'ancien écart est remplacée dans cette branche par des mesures informatives. Critères existants indépendants (batterie, température brute, RPM, MAP, bobine) conservés. Correction court terme indicative conservée hors trois variantes de transmission; aucune nouvelle règle de défaut.
- Export texte reprend les lignes du diagnostic avec leurs détails et identité. Capture/effacement de référence préservés, référence NA isolée par session et firmware (pas de comparaison entre ECU); pas de capture d'un échantillon invalide. En cas de trame non validée, pas de diagnostic normal ni ancienne mesure interprétée. Aucun changement au flux historique non NA.
- Sept clés nouvelles × six langues, total 56; test Qt sans ECU ajouté (AANMP002, EKDMF002, bit 40, défauts multiples, unités, trames invalides, fin session, retour historique). CI privée, pas de publication.
- DLL mise de côté sur instruction utilisateur. Pas de changement de ses sources/ABI/commandes, ni reprise de la branche DLL. À rattraper séparément après stabilisation Manager, en distinguant règles d'interprétation Manager et fonctions réellement nécessaires à la DLL. Aucun changement d'allumage.
- Reste : logger, analyse/relecture, Aide, validation UI et réelle. D1 non ajouté au dialogue automatique. Tests à vérifier après exécution.

## État CI au contrôle
- Code : `0df190e864938b7111834735a3813aa5d5aed41c`.
- Lot 2B2 : [45471374336](https://github.com/mini56/ECU-Mems-Manager-Session/actions/runs/45471374336)
- tests : completed / success
- windows : in_progress / non terminé (Compiler SESSION et tests de mise a jour)
- Lot 2B3 : [45472045837](https://github.com/mini56/ECU-Mems-Manager-Session/actions/runs/45472045837)
- tests : in_progress / non terminé (Valider les trois tests autonomes et les six langues)
- Pas de publication. Aucun résultat Qt/Windows ne doit être supposé avant la fin du run.
