# Suivi V0.47 - Lot 2B1 - Raccordement live et Toutes les mesures

Base ab283dd618e1f0aa606191e77b7b1f9104e014f0, branche unique fix-documentary-additive-session-161-20261118. V0.43 ch.44–47, 49, 65–67.

- MEMSInterface observe la réponse D1 existante et D0 de l'initialisation; aucune commande ajoutée. Le journal TX/RX existant est lu uniquement sur le segment de chaque mems_read réussi, après identification NA. Contrôle de longueur et de plusieurs valeurs brutes communes au résultat DLL. Snapshot atomique avec mems_data; invalidation si segment absent, incomplet ou incohérent. Aucun changement de DLL ni des autres familles. Seul le chemin ROSCO identifié NA active la présentation.
- Toutes les mesures : labels conditionnels, conversions communes, quatre mots TPS, sept groupes DTC expliqués par bulle, détail des défauts multiples, états distincts et valeurs absentes. Restauration des noms et bulles précédents à la déconnexion/changement de famille.
- 49 nouvelles clés numériques dans les six langues; contrôle d'unicité des identifiants. Les états non prouvés hors AANMP002 restent non confirmés.
- D1 doit être reçu par le mécanisme existant; aucun envoi automatique ajouté. La lecture du journal dépend de sa disponibilité sous Windows; sans preuve, aucun calcul NA n'est produit. Le premier couple avant D1 reste historique. Les limites du mécanisme de journal et son impact temporel sont à vérifier sur Windows.
- Pas de publication, à la demande de l'utilisateur. Déclenchements push des workflows de publication 204 et 206 retirés; déclenchement manuel historique préservé. Nouveau workflow CI privé en lecture seule (tests autonomes puis compilation Windows 207 sans Setup ni release).
- Les pages Toutes les données, Erreurs, diagnostic automatique et journaux/Analyse restent à raccorder. Ne pas considérer le lot 2B complet avant ce travail. Tests Windows et visuels en attente.
