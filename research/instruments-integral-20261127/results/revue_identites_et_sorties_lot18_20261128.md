# Lot 18 — identification D0/D1 et reprise des sorties alternatives
28/11/2026. Exclusivement MEMS 1.6 NA et 1.9. Aucune modification Manager/DLL.
## Preuves directes de collision sur quatre octets
| Profil | Blob ASM | Entrée D0 | Pointeur D0 | 4 octets statiques | Nom ASCII (pointeur -8) |
|---|---|---|---|---|---|
| KLH3G015 | fee5c0a8e7477a3e59486ec62af7222ebc6f0825 | 4DB8 | 3CF4 | 90001514 | KLH3G015 |
| KRHBG015 | 1ff4dfd0213f244126bee934974fe714918d1751 | 4CC6 | 3C4C | 90001514 | KRHBG015 |
| KTHBG015 | 39758e699853203a0f6718768655ec7bf0007bdc | 4EE5 | 3C4C | 90001514 | KTHBG015 |
| TBP7F015 | b085f990e237531cf4e88c9af4918c27b77c7a1d | 3CDC | 36F4 | 90001514 | TBP7F015 |
| KLH3G019 | e746b3c31b1ef8fff230c4c49d2db819fef28b85 | 4F9D | 3CB2 | 90001914 | KLH3G019 |
| TAHHG019 | f358d095a855d1ec6bd39b706e75a2d4f7c71738 | 50FE | 3E04 | 90001914 | TAHHG019 |
| TBH7G019 | 04e91ced15de68b9b0f3ecba47dcaf8858828125 | 5110 | 3E16 | 90001914 | TBH7G019 |

D0 statique ne distingue pas les architectures de fréquence fixe, simulation de régime et profils non qualifiés. Ne pas utiliser les quatre octets statiques seuls. Le code Manager mems16na_session_evidence.h au commit ac19137f2e8dd58c30b80352b9652fbc1145f4d3 vérifie déjà les enregistrements D1 (nom huit caractères et identité quatre octets identique au D0 reçu). Le quatrième octet de la réponse réelle peut différer de la valeur statique de l'image, notamment selon calibration/version; ne pas imposer 14h sans preuve de réponse réelle. Ne pas confondre les données nominales et les octets observés sur banc.
## Quatre ASM sans ROM/XF
| Profil | D0 entrée / pointeur | D1 entrée / pointeur | 12 octets statiques |
|---|---|---|---|
| ABNMP000 | 3924 / 3474 | 3AF1 / 346C | ABNMP000 99 00 00 14 |
| ABNMP003 | 3AF0 / 35BA | 3CBD / 35B2 | ABNMP003 99 00 03 14 |
| KLH3V004 | 4D38 / 3C74 | 4F47 / 3C6C | KLH3V004 C7 00 04 14 |
| TFH7G031 | 51AE / 3EB4 | 53BD / 3EAC | TFH7G031 90 00 31 14 |

Les DB ont été reconstruits à partir des lignes répétées au même offset et lus exactement aux pointeurs. Ces valeurs corrigent toute estimation antérieure du quatrième octet (03 pour ABNMP000/003 non démontré par ces DB). Ne pas déduire d'identité live ni de calibrations absentes. TFH7G031 : écriture 030E/20 après chargement de consigne, le contrôle lot17 a manqué le OR parce que son analyse locale imposait une distance trop courte entre chargement, OR et store. La chaîne manuelle lot16 demeure valable.
## Revue de profils non qualifiés
KRHBG015, KTHBG015, KLH4G019, KBHHG008 : XF contient iq_tacho_fmsk et m_pbu_rev_stat, mais ni tacho_sw_time ni tacho_display_speed. Aucun XOR direct 22/20 suivi de port 1FFE retrouvé dans les ASM lus. Cette observation exclut seulement le mécanisme direct recherché.
TBP7F015 : 10 ->3F23 (OR 8B/80), 00 ->3F28 (AND 8B/7F), les deux ->3F2B (OR 8B/40), restauration 42F4 (AND 8B/BF). Revue des références explicites de 8B : aucun JBC/JBS du bit 6/7 retrouvé. Les ports 401h à 2910 et 2953 utilisent les ombres 15F et 15D, masque E0 préserve bit6; HSO écrit notamment 20/0 (canal 0), 23/3 (canal 3), pas de couple fixe 02/22. Ne pas conclure à une absence globale: indirects, alias et autres chemins non résolus. Aucun nouveau candidat qualifié dans ces cinq revues.
