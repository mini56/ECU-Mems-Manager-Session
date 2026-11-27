# SESSION 218 — Tests température / suite V1.30
Base validée : SESSION 217, commit documentaire 35228361411c12b13568938412b806cdbe18fa62, run 45981906933. Branche unique fix-documentary-additive-session-161-20261118.
## Intégrations et limites
| Fonction | Statut source | Validation |
|---|---|---|
| ACT-001 test manuel de sonde température moteur | Démarrer/arrêter, initiale/actuelle/variation et courbe, rapport, données fraîches, moteur tournant, profils AANMP002/KBHBG009 déjà utilisés par le diagnostic passif 217 | CI à exécuter, véhicule non testé |
| ACT-009 emplacement jauge | Colonne droite après pas-à-pas, avant État système compact ; gauche préservée ; symbole sans aiguille ; boutons avec commentaire permanent | CI à exécuter, contrôle visuel 1200/1366/1920 restant |
| ACT-009 commande réelle | VERROUILLÉE, aucun transport actif raccordé ; six chemins firmware documentés (V1.30 ch. 169–173 : AANMP002, AANMP000, ABNMF032, ABNMP005, ACNMF024, ACNMF026). Autres profils non confirmés, pas déclarés absents. Régime inconnu / moteur tournant interdits. | Banc requis pour qualification de la sortie, restauration F4 (générale), délai réel, perte de lien et absence de forçage résiduel. Aucun ordre ne doit partir en 218. |
| ACT-004 lambda passif | Conservé de 217 | CI non-régression ; véhicule à tester |
| ACT-008 lambda actif et CB | Non intégrés | Banc et autorisation |
## Décisions de réalisation
- Nouvelle observation dédiée température indépendante de l'état lambda ; seuil indicatif de réchauffement 3 °C (déjà utilisé pour l'observation 217), non seuil constructeur ; aucune déclaration de capteur HS/OK. Défaut mémorisé ≠ défaut actuel ni preuve de substitution ; résultat non concluant si présent. Courbe de mesures ECU, pas mesure indépendante. Acquisition interrompue si donnée invalide, moteur arrêté, perte liaison ou délai interface >3 s (seuil logiciel de fraîcheur, pas temporisation ECU). Export du résultat dans le rapport existant. 
- Jauge : 00/10 et retour F4 documentés par analyse statique ; pas d'envoi, pas de nouvelle commande, pas de modification DLL. Les trois boutons restent désactivés même moteur arrêté sur les profils statiquement documentés tant que qualification physique non acquise. Explication exacte visible, régime en marche explicitement signalé. Cette version ne permet pas de faire le test actif de la jauge sur véhicule. Étape de qualification dédiée nécessaire avant activation.
- Six langues, aucun nouveau dépôt. Setup privé à générer par CI. Le registre V130 complet et les autres intégrations restent inchangés.
