# Décision utilisateur — page Réglages, 27 novembre 2026

À intégrer ultérieurement pour MEMS 1.6 et 1.9 selon profils vérifiés.

1. Renommer le réglage manuel « Correction carburant » en « Correction d’injection » et afficher (brut − 128) × 8 µs lorsque la correspondance est établie.
2. Remplacer la bulle d’aide : « Permet d’ajuster le temps d’injection par incréments de 8 µs. Sur les moteurs équipés d’une régulation lambda, le calculateur peut adapter automatiquement cette correction lorsque les conditions d’apprentissage sont réunies. Sur les profils étudiés, une temporisation nominale d’environ 23 secondes précède la collecte de 10 basculements lambda par évaluation. La durée totale dépend des conditions et de la réaction de la sonde. » Ne pas présenter ce délai comme universel ni comme une durée garantie de fin d’apprentissage.
3. Conserver le cadran de correction instantanée lambda en %, distinct du réglage en µs. Clarifier son libellé « Correction instantanée lambda » si sa source est bien la correction de boucle fermée.
4. Prévoir l’affichage de progression (temporisation, basculements, évaluations) sous réserve de confirmation de l’accès diagnostique aux adresses spécifiques : voir Temporisation_adaptation_injection_MEMS16_19.md. Ne pas présenter des valeurs non lues comme valides.
5. Mettre à jour aides et traductions associées, sans modifier les commandes de réglage existantes.

Références : rapport V1.30 chapitre 190, Correction_libelle_reglage_injection.md, Temporisation_adaptation_injection_MEMS16_19.md. Accord de principe utilisateur pour modification future, pas une autorisation de modifier maintenant Manager/DLL ou de compiler. Aucun test ECU effectué.
