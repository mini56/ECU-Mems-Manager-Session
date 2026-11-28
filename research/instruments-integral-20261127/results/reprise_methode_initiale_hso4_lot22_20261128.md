# Lot 22 — réexamen de la méthode initiale et première vérification HSO4
28/11/2026. Périmètre 1.6 NA/1.9. Aucun run ni modification Manager/DLL.
## Confrontation de l'inventaire initial et des 242 ASM non qualifiés
Jointure par famille et référence entre inventaire_individuel.csv et registre_sorties_physiques_lot16.json. Les 108 références 1.6 restantes comprennent 107 images avec commandes 00/10 dans l'inventaire initial et deux symboles instrument, plus EKDMF008 (aucune table reconnue dans cet inventaire, XF absent). Les 134 références 1.9 restantes comprennent 133 images sans 00/10 ni 63/6B reconnues dans l'inventaire initial, et TBP7F015 avec 00/10 seulement. Ceci ne prouve pas l'absence d'autres commandes. La méthode initiale audit.py exige les consignes 0/3000 et les commandes 63/6B pour le compte-tours; elle ne peut donc pas identifier une architecture différente.
## HSO4 sur cinq 1.9 hors des 100 références déjà recensées
| Profil | Blob ASM | Blob XF | Deux fronts HSO4 | Autre écriture HSO4 |
|---|---|---|---|---|
| KBHBG009 | fb54ca321b3c19b064f677fdc579b93e2906639f | 018f487f102e4031b5914a3ec515d4c556a018e4 | 486E/487C | 4D3F |
| KBHBG010 | ba8877559863009a08a24492710e677305e4bf71 | dc9bfba9c9dee033288ee3cb2265807c38fe0836 | 48AA/48B8 | 4D7B |
| KBHBV001 | 4b0f57ae57125d3918b7fe6255b369ded4e30240 | 0c86b11d084ed5e63f3cd25066ffd6989883be64 | 4647/4655 | 4B18 |
| KFACV000 | 13955d6c94a2c4b5565bc834d8671f305b67d1e1 | 755a4cf1866deec6b0aaa7b6fc2755d871258383 | 4647/4655 | 4B18 |
| KFHBG007 | d4b09d9d032e1e682ff55f719876c7e71bc79bb4 | 0de0a0eabf35ed592626ec7c5403e035ae4979c5 | 46C5/46D3 | 4B96 |
ASM complets obtenus via blob, car fetch_file renvoyait un contenu vide pour les fichiers volumineux. Pour KBHBG009 et KFHBG007, la sous-routine programme deux fronts à intervalle fourni par l'appelant et retourne; stocke INTMEM_4C dans 03C4, nommé inj2_pw_fired dans le XF3 KBHBG009. L'autre écriture remet les sorties HSO3/4 à zéro (KBHBG009 4D23–4D48, KFHBG007 4B7A–4B9F). XF1 de KFHBG007 identifie les variables de temps d'injection 014E/0150. Les trois autres présentent les mêmes paires d'écritures et les XF portent les variables d'injection; leur chaîne complète reste à confirmer. Pas de preuve de test instrument dans ces chemins. KBHBG009 : port 1FFE 4C62 fusion de masques (4C3F–4C67); 4CC6 initialisation. Ne pas exclure globalement ces firmwares.
## EKDMF008
ASM blob 0324fad6f9f120f2e60341b1824c587f1596dee7, quatre écritures explicites INTMEM_6 : 21E4=19h, 2C22=18h, 350B valeur dynamique F6+1, 3D52=18h. Chemin dynamique à étudier; aucune nouvelle qualification.
## Bilan
0 nouveau candidat qualifié. 5 profils 1.9 relus sur HSO4, dont 2 avec contexte complet de la paire de fronts et de l'annulation; 1 ASM 1.6 examiné pour écritures HSO. La méthode initiale à signatures fixes est saturée sur les données déjà inventoriées, mais ne constitue pas une recherche exhaustive d'autres architectures. Les 242 références restent ouvertes.