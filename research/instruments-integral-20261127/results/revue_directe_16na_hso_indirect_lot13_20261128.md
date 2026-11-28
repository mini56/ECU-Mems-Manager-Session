# MEMS 1.6 NA — Lot 13 : réexamen des quatre profils sans piste et audit ASM complet
28/11/2026. Suite lot 12. Aucun changement Manager/DLL, aucun accès ECU.
Les quatre références dépourvues de piste V2 ont été relues dans leur ASM, XF1 et ROM. Leurs écritures dynamiques vers HSO_COMMAND (06h) proviennent de tables indexées dont les quatre octets ont été lus directement dans les ROM, et non déduits du nom des variables.

| Firmware | ASM | Chargement de table | Table ROM | Écriture HSO | Quatre octets |
|---|---|---|---|---|---|
| EKDMF008 | 0324fad6f9f120f2e60341b1824c587f1596dee7 | 3500 | 3582 | 350B | 01 01 01 01 |
| KBACF001 | c3753560177244e2ec4152e58b6940ac62aa8f69 | 30AA | 312F | 30B5 | 01 01 01 01 |
| KCE8F000 | 5dc8daf526fb6549b14b28d55b9e79655748353e | 308E | 3113 | 3099 | 01 01 01 01 |
| KDE6F015 | 5595f0782b4d57748aabacfb013ee516995f3187 | 322C | 32B1 | 3237 | 01 01 01 01 |

Les routines bornent l'index par comparaison à 4, puis chargent le canal, appliquent éventuellement 20h et programment l'événement. Pour cette voie indexée et pour un index 0–3, canal 1 uniquement : le pilotage HSO2 de jauge des témoins n'est pas caché dans ces tables. Aucun test autonome de compte-tours n'est démontré par cette constatation ; autres sorties et chemins non exclus. Les fourchettes de code, références d'allumage et autres candidats restent à qualifier.
ABNMP000 et ABNMP003, témoins supplémentaires ASM seuls, ont été relus pour établir les contrôles positifs de l'audit.
Nouvelle méthode : audit_16na_asm_complete.py, inventaire à partir des 125 ASM (pas de filtre ROM/XF), extraction des octets DB y compris les tables répétées sous la même adresse IDA, recoupement ROM quand disponible, masquage des tables diagnostiques identifiées comme données, inventaire des écritures HSO fixes/dynamiques et tables indexées, des additions à retenue à deux ou trois opérandes, des écritures de ports et des sélecteurs 0/FF liés à un accumulateur, sans filtre de nom XF ni numéro de commande pour ces motifs. Les 15 témoins intégrés et deux ASM orphelins doivent être retrouvés ou le run échoue après avoir enregistré les preuves. Les nouveaux résultats restent des candidats à examiner, pas des autorisations de tests. Aucun verdict d'absence globale. Les chemins indirects non locaux, les autres formes de génération de signal et le câblage ne sont pas résolus par ce seul audit.