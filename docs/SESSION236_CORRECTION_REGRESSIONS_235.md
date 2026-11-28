# SESSION 236 — correction des régressions constatées sur 235
Base : 1d022c1cc0f1892c771496d7d80356b8121a9cd7. Références utilisateur : 211, 215, 219, 228 ; rapport CONTROLES_UTILISATEUR_NON_REGRESSION_SESSION235_20261128.md.
## Échec constaté
Run 235 (46180459246), job Windows 141088675481, étape diagnostic Qt : FAIL 197, vérification du texte DTC2 inconnu et 0xFF. Le formatteur mask() mettait aussi le préfixe en majuscule (0XFF) ; normalisation en 0x + chiffres hexadécimaux majuscules. Le test initial n'est ni supprimé ni assoupli. Cause déduite de la source et du log, à confirmer par exécution.
## Corrections
- Libellés 17/18 des trois 1.3 documentés maintenus même en l'absence de défaut, comme avant 235.
- DTC2 de profils qualifiés sans trame : valeur -- et texte explicite de trame non disponible/non validée, dans diagnostic, Toutes données et Trames. La lecture historique DLL reste conservée pour les profils non qualifiés ; elle est signalée comme non confirmée en l'absence de trame. Pas de nouvelle interprétation de défaut.
- Suppression des masques hexadécimaux dans les nouvelles bulles d'aide introduites par 235.
- Ajout des assertions Qt de disponibilité dans diagnostic et Toutes données, six langues pour la seconde. Aucun changement de DLL, de commande, de sécurité ni de protocole.
## Limites
Le contrôle des libellés est statique, pas un test graphique de la fenêtre principale. La compilation/validation Windows 236 et le Setup ne sont pas encore acquis à la rédaction. Les autres contrôles utilisateur, les régressions historiques et l'ensemble de la couverture des calculateurs restent à valider. Aucun Setup intermédiaire demandé à l'utilisateur.
