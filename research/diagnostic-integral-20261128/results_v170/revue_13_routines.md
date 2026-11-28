# V1.70 — Revue directe ASM MEMS 1.3
Sources: KBE6R003 blob 7974a20fbfbee0a799880c591c3364f6cc64395a; MGE7R002 blob 3759ab2513678e0a7e27ce47f13713785c5442ee. Lecture directe des deux blobs.
Les deux variantes présentent les mêmes sites et effets:
- C1 331C -> 336F : remise à zéro de l'index.
- C2 3321 -> 3376 : incrément de l'index.
- C3 3326 -> 337D : décrément de l'index.
- D8 332B -> 3384 : incrément circulaire du sélecteur RAM_C06C, borne RAM_C06E.
- D9 3330 -> 3396 : décrément circulaire du même sélecteur.
- C0 3335, 80 333C, A0 3345 : bornes de plages, non commandes indépendantes à ces sites.
- 3354 : index multiplié par 32, addition du décalage et de RAM_C068; 3365 lecture octet, 33B6/33C0 incrément/décrément et réécriture.
- 3314 : garde INTMEM_9A masque 08; 33CB appel ROM_30B4; 33D1 effacement bit 3.
Différence d'implantation : index INTMEM_7E sur KBE6R003, INTMEM_7C+2 sur MGE7R002.
Bilan: 10 branches à effet documenté et 6 bornes confirmées sur deux références. Ne pas assimiler à des commandes utilisables ni les exécuter: provenance externe, contexte complet, ROM et nature des données restent à établir. DC non approfondi dans ce lot. Aucun changement des 295 candidats ni de Manager/DLL.