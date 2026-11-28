# V1.71 — Contexte des sélecteurs MEMS 1.3
Lecture directe ASM KBE6R003 (7974a20fbfbee0a799880c591c3364f6cc64395a) et MGE7R002 (3759ab2513678e0a7e27ce47f13713785c5442ee).
Résultats identiques sur les deux variantes :
- DC : 2EE2 compare le sélecteur, 2EE7 efface le bit 3 de INTMEM_9A, 2EEA restaure INTMEM_3C depuis INTMEM_48, 2EED sauvegarde INTMEM_46 dans INTMEM_4E (octet), 2EF0 charge 31h dans INTMEM_46. Transition interne d'état probable; accessibilité externe et rôle exact non établis.
- C5 : 31D6 -> 31F1, sélection de la base RAM_C064 vers RAM_C068 si bit 5 de INTMEM_9A actif (31FE).
- C6 : 31DB -> 3205, sélection de la base RAM_C066 vers RAM_C068 si bit 6 actif (3212).
- C7/C8/C9 : comparaisons 31E0/31E5/31EA, destinations 3219/3239/325F. Effets complets non qualifiés.
- RAM_C068 : base de table dynamique (3141/3146, 31FE, 3212), et non adresse fixe de paramètre moteur.
- RAM_C06E : quotient calculé par division non signée à 3AB4, stocké à 3AB7; RAM_C082 reçoit le produit à 3ABF. Confirme une logique de dimensionnement du parcours circulaire D8/D9; nature des éléments non déterminée.
- D7 : comparaison 39F4, puis calcul d'adresse à partir du sélecteur RAM_C06C, facteur RAM_C080 et base RAM_C076. Effet final non encore qualifié.
Bilan du lot : 6 occurrences avec effet local documenté (DC/C5/C6 sur 2 références), 8 comparaisons supplémentaires observées (C7/C8/C9/D7 sur 2 références), sans prétendre qu'elles sont nouvelles dans le corpus global. Aucun changement du compteur 295 : rapprochement avec inventaire et preuves binaires en attente. Aucun changement Manager/DLL. Diesel exclu. Pas de run. Rapport cumulatif FR/EN et ZIP non actualisés.