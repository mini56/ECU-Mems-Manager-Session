# MEMS 1.6 NA — contrôle du lot 02 et correction du détecteur
Run 46127179199 : 123/123 ROM, 10/15 témoins historiques retrouvés, zéro nouveau candidat par le détecteur, zéro symbole tacho. Ce résultat ne permet pas de conclure à l'absence de nouveaux profils.

## Cinq faux négatifs expliqués sur ASM
| Firmware | Accumulateur de PWM | Mise à 1 du bit 6 stratégie | Fusion stratégie | Écriture de sortie |
|---|---|---|---|---|
| ABNMP005 | 2833 : A5 + 01AC | 283A : A3 OR 40h | 28AD | 28C0 : 1FFE |
| ABNMP006 | à confirmer par lot 03 | à confirmer par lot 03 | 28AD | 28C0 : 1FFE |
| ACNMP003 | 2831 : A5 + 01AD | 2838 : A3 OR 40h | 28AB | 28BE : 1FFE |
| ACNMP004 | 2833 : A5 + 01AD | 283A : A3 OR 40h | 28AD | 28C0 : 1FFE |
| ACNMP005 | à confirmer par lot 03 | à confirmer par lot 03 | 28AD | 28C0 : 1FFE |

Les cinq firmwares ont effectivement une écriture vers 1FFE, et non 0401, avec fusion de la stratégie A3 et masque de préservation A0. Le détecteur lot 02 recherchait uniquement 0401 dans la branche PWM par port, alors que les témoins AANMP002 (HSO) et EKDMF009 (0401) n'exerçaient pas cette variante. Correction 6ff7a1a2cea5aa60d4afdd09c873844d59a6d78e : reconnaissance 0401 ou 1FFE et assertion obligatoire des 15 témoins. Le lot 03 ne remplace pas les preuves du lot 02.

## Limites restantes
La reconnaissance de cette structure n'est pas une preuve d'exhaustivité des autres mécanismes (notamment compte-tours sans symbole ou accès indirect). Ne pas qualifier automatiquement les 108 autres firmwares comme incompatibles. Pas de commande envoyée à un ECU. Manager et DLL inchangés.