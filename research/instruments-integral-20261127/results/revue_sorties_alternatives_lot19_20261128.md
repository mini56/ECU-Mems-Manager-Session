# Lot 19 — tri des sorties alternatives et correction des six identités
28/11/2026. Périmètre 125 ASM 1.6 NA et 217 ASM 1.9. Aucune modification Manager/DLL.
## Run 18
46137389051 : success, 342 profils, 336 enregistrements D0/D1 reconnus automatiquement, 33 groupes de collisions D0 nominales, 1063 entrées de pistes sur les 242 profils hors des 100 précédents. Entrées et non mécanismes distincts.
## Six D1 absents du bilan : données brutes du désassemblage
Les six fichiers ASM ont été lus directement, DB reconstruits en poursuivant les offsets pour les lignes répétées. La routine D0 pointe vers le champ identité du bloc, D1 commence dans une zone représentée en DB et non en instructions. Encodage D1 commun B1 07 rr A1 ll hh 38 11 36 B1 0B 3E; le chargement immédiat A1 ll hh 38 pointe au début du nom ASCII (D0-8). Cette reconstruction statique n'est pas un contrôle de la réponse sur véhicule.
| Firmware | Blob ASM | Entrée D0 | Pointeur D0 | Entrée D1 | Pointeur D1 (DB) | Identité statique |
|---|---|---|---|---|---|---|
| EKDMF004 | 411a984a3e5e3af9e4f11114ed12b5a7d453223f | 385F | 3418 | 39F3 | 3410 | 90000414 |
| KAE6P000 | d2e929fbff3f4dce4efb84e7c68a8ca4f3ea3e2d | 390D | 3460 | 3AA1 | 3458 | A1000014 |
| KBP8P000 | 87159ed457105fd191f7bed09759daa36a387c99 | 391D | 3464 | 3AB1 | 345C | 93000014 |
| KBP8P002 | 80bbf02d77a6755bcbd16d827dfe81aed0e215dc | 3923 | 3466 | 3AB7 | 345E | 93000214 |
| TAP8P000 | c442e216d0b369f13aa80fdcf399170e339d2cbf | 3A82 | 358C | 3C16 | 3584 | A8000014 |
| TAP8P002 | e49d4dd847c0c2ae65bc49a0669f67d295387cd8 | 3A88 | 358E | 3C1C | 3586 | A8000214 |
Les six noms ASCII correspondent au fichier. 342/342 enregistrements statiques retrouvés (336 lot18, six revue manuelle); le fichier de bilan lot18 original reste inchangé.
## HSO0 : huit MEMS 1.9 examinés individuellement
| Firmware | Blob ASM | Blob XF | Entrée EC | Table EC | HSO0 | Période |
|---|---|---|---|---|---|---|
| SELFG007 | ba0b59ad4f97c51045829f0ee9dcc3f9190f8ac8 | 5c3c6c38c156256ce9585febdf8e34a5282f78c1 | 54BF | 587A | 54CD | 12h[40] |
| TAH7V000 | 71d5320da54c38c562e948d41be25adc13178af6 | ebdb64f9b756df9b0dc369aef3797654f4ad92c1 | 4EEF | 52A8 | 4EFD | 12h[40] |
| TAH7V001 | f86d7758a6a27d59c3ace35b0ff0a1e762c25f79 | 3745f8c236b2f49b385f6ff96342a602fdf3e488 | 531F | 5698 | 532D | 12h[40] |
| TAH8V000 | a659f05ffdcff9d994900e2f50eeeb01837d3020 | 7d56692e4871be94151a0d4ea537e07755719463 | 531F | 5698 | 532D | 12h[40] |
| TBH7V000 | 6b2d57c11fff3c4cc185325beeb1202b6ac1cd61 | 41c4fb3682ee9d44186f8badd2df3bbaece3dea5 | 4EEF | 52A8 | 4EFD | 12h[40] |
| TBH7V002 | 5531cdd08c1074386e3f0ac0a59ff445dbf3a66a | 842398b4e8c3078bc24ae1e3bc4ff8478d632a03 | 531F | 5698 | 532D | 12h[40] |
| TBP7F015 | b085f990e237531cf4e88c9af4918c27b77c7a1d | 0941fa0df403d3226b18a4abd978f2cbfb78c4a0 | 40CD | 43BC | 40E5 | 12h[36] |
| TFH7V000 | adeae82b66aff17597918b9187eadf2cf19c5c94 | 0fb0b6e6e49059cc1942e65c468ca91e84c62bf6 | 4F25 | 531E | 4F33 | 12h[40] |
XF : dknk_rosco_time offset 18 décimal (XF1), 0012 hex (XF3 TBP7F015). EC est la commande du test cliquetis, pas instrument. 8/8 chemins écartés individuellement, pas les firmwares.
## Tri de toutes les 1063 entrées sauvegardées (registre_tri_sorties_lot19.json)
242 HSO dynamiques présentent la même topologie de sélection indexée (signature d'ordonnancement déjà démontrée pour des profils 1.6); 2 autres à 9402, KLHBV001/002, sont des données de calibration décodées comme instructions. 349 HSO3 présentent une impulsion unique, deux écritures 23h/03h, ajout d'une durée à l'échéance et retour; 241 profils, pas de boucle périodique démontrée par ce motif. 428 entrées de port représentent 214 ancres distinctes sur 107 profils 1.6 (doublons de détection); toutes comportent la réinitialisation de masques d'interruption, HSO 18h, succession de mises à zéro/à un bit 0 puis restauration de l'ombre. Ports 401/402 ou 1FFE/1FFF. Ce motif est une séquence d'initialisation, non une modulation instrument. 34 HSO0 1.6 restent à qualifier individuellement (signature similaire au cliquetis, déjà démontrée pour plusieurs profils en lot08). 8 HSO0 1.9 confirmés ci-dessus. Le tri est une classification des fenêtres ASM, pas une preuve sémantique exhaustive des 242 firmwares.
## Vérifications ponctuelles HSO3
EKDMF000 (ASM ea215aeb8979eb4442cdb2259b95e039f3d70ad5, XF afb34455942042ac0a138b4567b1a5a73e26cba5) : 3819/3827, échéances 381C/382A, retour 3830. FTST0152 (ASM 97754771fdb3c788b32dd00fe9169fec8a1614ae, XF e9e670c5fd5af00736894ffd8bc57514af926d1c) : 49B9/49C7, 49BC/49CA, retour 49CE. SELFG007 : 48F1/48FF, deuxième canal HSO4 4912/4920, deux impulsions séparées; XF contient inj_pw_rosco (offset 14 décimal) et variables d'ordonnancement d'allumage. Ne pas assimiler ces impulsions à un compte-tours sans consommateur périodique et preuve matérielle. HSO4 n'était pas pris en compte par la condition canal <=3 du lot18 : limite réelle à lever dans une recherche ultérieure.
## Bilan
Aucune nouvelle intégration ni nouveau profil qualifié. 56 intégrés, 40 chaînes statiques supplémentaires, quatre ASM incomplets, 242 autres profils. Les signatures trouvées ne doivent pas servir d'exclusion globale. Prochaine direction : tous canaux HSO, calcul d'échéance/rechargement hors XOR/retour de retenue, provenance des durées et des commandes, sans traverser les zones DB/calibration. Ne pas présenter les 1063 entrées comme 1063 fonctions distinctes.
