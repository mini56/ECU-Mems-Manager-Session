# Intégration Manager — diagnostic apprentissage papillon

Profil initial : AANMP002 (MEMS 1.6). Ne pas généraliser.
- PETA m_thr_z_error, RAM 015F, octet. ASM 4A7E/546F écrivent le résultat, 4B4E le remet à zéro après succès.
- Dispatcher normal : entrée 4150, commande 70 -> 3EF3 (demande de scan, état AD/80, sous-état 0D); 4154 -> 3EFA (sous-état, ne pas confondre avec Fan1Off 0D du mode normal). Aucune émission ECU à ce stade.
- Prévoir test utilisateur explicite, affichage du résultat et code détaillé; priorité à lecture seule du résultat et vérification du dialogue/annulation avant toute activation. Codes 10/20/30/40/50 : interprétation exacte des branches à confirmer avant libellés définitifs.
- Ne pas modifier Manager ni DLL ni lancer compilation dans ce lot de recherche. Référence rapport V1.17 chapitre 178.
