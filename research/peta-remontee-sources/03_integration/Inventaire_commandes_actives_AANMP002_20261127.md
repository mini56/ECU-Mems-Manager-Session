# Inventaire initial des commandes actives — AANMP002

27 novembre 2026 — recherche statique uniquement, sans modification de Manager ou DLL, ni envoi de commande. Base documentaire V1.30, comparaison source SESSION 215 (475243db18c3099595e1a5928b6916185380a808). La version 217 n'a pas été localisée dans cette vérification : ne pas affirmer son état d'intégration.

## Sources primaires
- `firmwares-extraits/Firmware Archive/Rover MEMS 1.6/NA/AANMP002.asm` : bloc de données désassemblé à 407C, entrées de quatre octets (borne haute, borne basse, adresse little-endian), routines 3CCA–3EF9. Les entrées appartiennent à plusieurs tables contiguës et ne doivent pas être amalgamées.
- `firmwares-extraits/Firmware Archive/Rover MEMS 1.6/NA/AANMP002.xf1` : symboles et drapeaux Rover PETA.
- `rosco.h`, `memsinterface.cpp` au commit 475243db.

## Résultats de tri
| Commande | Preuve AANMP002 | Classement |
|---|---|---|
| CC | 3E20–3E2A efface les octets RAM 54,50,55,52,46 (dont io_tripped_ops/log, flagbyte2/3). MEMS_ClearFaults dans rosco.h ; interface utilise guardedClearFaults. | Déjà intégrée, pas un nouvel actionneur. |
| EF/F7 | Même cible 3CCA dans le bloc de dispatch ; contrôle 5E nul avant sous-routines. rosco.h déclare MEMS_TestInjectorsMPi=EF et MEMS_TestInjectors=F7. | Alias de routine pour ce profil, EF déclaré dans source 215 ; ne pas compter comme nouvel actionneur ni conclure sur son accès UI. |
| 65/6D | 3D40/3D3B : bit 1 de AD effacé/posé, bit 0 posé dans les deux cas. XF1 flagbyte8, d_thr_sw_force/onoff. | Forçage contact papillon, déjà identifié comme non retenu dans V1.30. |
| D2/D3/CD | 3E2D, 3E6A, 3EC6 ; accès à RAM 142 b_code_timer, 15E b_data_result, 148 b_code, 140 b_start_code, 155 b_ecu_type. | Fonctions liées à la sécurité, hors inventaire des tests moteur, ne pas intégrer dans Actionneurs. Aucun test ni procédure proposé. |
| 7E/7F | 3DF7/3DEF : valeurs constantes dans 45 i_setpt_rosco / 44 ign_offs_rosco. | Réglages existants, déjà documentés V1.30 ch.168. |
| 82 | 3D8E : contrôle 5E nul, modifie état de protocole 98, compteur 36, réponse. | Fonction non caractérisée ; pas de preuve de nouveau test. Ne pas envoyer. |
| 00/10, ED/EE, CB, 70 | Déjà recensés dans V1.30. | Pas une nouvelle découverte de ce lot. |

Conclusion : aucun nouveau test moteur confirmé par ce premier lot. La présence d'une entrée ne démontre ni une sortie physique ni une compatibilité générale. Prochaine étape : extraire les tables avec bornes de dispatch exactes sur tous les profils NA, identifier les effets des handlers non classés, et comparer au commit exact de SESSION 217. Aucun changement programme ni essai véhicule. Ce fichier est une note de reprise indépendante, pas une nouvelle version du rapport cumulatif.
