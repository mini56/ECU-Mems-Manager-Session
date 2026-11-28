# V1.72 — Revue directe ASM
Sources : KBE6R003 blob 7974a20fbfbee0a799880c591c3364f6cc64395a ; MGE7R002 blob 3759ab2513678e0a7e27ce47f13713785c5442ee.
Sur les deux références, les chemins suivants sont identiques :
- C7 : 3219–3237, sélection de la référence de table de type 0, via ROM_373A ou RAM_C06A.
- C8 : 3239–325D, sélection de type 1 via ROM_373A ou RAM_C064, avec garde de disponibilité.
- C9 : 325F–3283, sélection de type 2 via ROM_373A ou RAM_C066, avec garde de disponibilité.
- Sortie commune 3285–3297 : préparation de deux octets à partir de la référence 16 bits, appel ROM_30B4, mise à jour état. Ne démontre pas le format de réponse externe.
- D7 : 39F4–3A32, calcul d'une adresse de bloc avec correction circulaire, préparation d'un état de traitement et de la demi-taille de bloc. Ne démontre pas la lecture complète ni sa finalité.
Sur KBE6R003 seulement, ROM_373A (373A–377D) confirme les trois types de références: type 0 vers ROM ou pointeur lu en ROM; types 1 et 2 vers bases RAM conditionnelles. ROM_3F40 commence par des vérifications de structure et de débordement. La finalité moteur de ces tables reste inconnue.
Bilan: 8 occurrences approfondies, 6 chemins de sélection de référence et 2 chemins de préparation de bloc. Pas de nouvelle commande externe validée; compteur global 295 inchangé, rapprochement binaire et inventaire en attente. Aucun changement Manager/DLL, aucun run, diesel exclu. FR/EN et ZIP cumulatifs non actualisés.