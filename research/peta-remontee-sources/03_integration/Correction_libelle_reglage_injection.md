# Correction du libellé et de l’unité du réglage d’injection

Décision utilisateur du 27 novembre 2026, complément au chapitre 189 (V1.29) et au registre des intégrations.

- MEMS 1.6 NA et MEMS 1.9 : renommer le réglage existant « Correction carburant » en « Correction d’injection » dans la page Réglages, son aide et les libellés associés à ce même paramètre.
- Afficher la correction temporelle en µs, conversion (brut − 128) × 8, pour les profils où f_adpt_offset et la correspondance diagnostique sont établis. Conserver le signe et le zéro neutre. Ne pas appliquer cette conversion aux corrections lambda en pourcentage ni à un autre paramètre de carburant.
- Les boutons +/− et commandes 79/7A restent ceux du réglage existant. Aucun réglage supplémentaire en doublon. Respecter les variantes et les traductions, FR prioritaire.
- Recette : brut 127 → −8 µs ; 128 → 0 µs ; 129 → +8 µs. Vérifier les libellés, l’aide, les unités et les données fraîches, sans régression des réglages existants.

Référence : Reglages_MEMS16_19_V129.md, Manager 216. Consigne d’intégration future ; aucune modification du code Manager ou de la DLL ni compilation effectuée par cette note.
