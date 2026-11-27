# SESSION 220 — mesures ordinaires vérifiées
Base 219 verte : 0cc7d4f16b8f1dce70bfc2e797661d30e43c0c72. Branche de recherche vérifiée 99bdd57a1f8392453d1d74d23829139e8d020cb7.
## Périmètre
Comparaison du registre V128/V130 et de la source 219 : le décodeur NA couvre déjà 80/10, 80/11, 7D/13, 7D/14-15, références et compteur. Ne pas recompter ces fonctions. Pour MEMS 1.9 KBHBG009, la source précédente utilise encore les champs historiques et l'écart ralenti mélange pas et tr/min. Extension ciblée de l'interprétation ordinaire, sans nouveau service ECU.
| Champ | Preuve ROM KBHBG009 (table 3BEA) | Unité |
|---|---|---|
| 80/10 | 3C54 : 20 00 30 00 ; XF3 i_hotdbpos 0030 | pas absolus |
| 80/11 | 3C58 : 20 00 31 00 ; XF3 i_dbpos_max_bv 0031 | brut ×0,1 V |
| 7D/13 | 3CFE : 20 00 2F 00 ; XF3 i_setpt_rosco 002F | (brut−128)×25 tr/min |
| 7D/14–15 | 3D02 : 23 00 B8 00 ; XF3 i_error_speed 00B8 | mot −32768 tr/min |
| 7D/17–1E | 3D0A/0E/12/16 : 23 00 2A 00 ; XF3 m_thr_shut_bits 002A | une référence répétée, mot/256 points ADC ; quatre copies comparées |
| 7D/1F | 3D1A : 20 00 32 00 ; XF3 m_thr_jack_cnt 0032 B | compteur, pas une mesure physique |
Identité exacte D0 90 00 09 + D1 KBHBG009, trames ordinaires cohérentes de 28 et 33 octets (longueurs incluses). Décodage partagé et testé, présenté dans Toutes les données, Toutes les mesures et Diagnostic automatique/rapport. Les données brutes restent distinctes ; aucune interprétation si copie incohérente ou trame absente. Ne pas généraliser aux autres MEMS 1.9. Aucun seuil de défaut ajouté.
## Fiabilisation 219
Aperçu : intitulé lambda instantanée distinct ; aiguille et valeurs de température/correction masquées quand invalide ou déconnecté, historique effacé ; C/F conservé. Pas de modification du schéma historique des journaux, ni du mode 4. Les autres voies de l'audit global restent à traiter. 6 langues, aucun ajout de ligne dans les tableaux, contrôle visuel 218 conservé, tests de transition et absence de données.
## Restant
ACT-064/065/066/067 : correspondances bit/table identifiées, sens physique et conditions encore à qualifier, non activées. ACT-016/017 et états courants 0055 etc. : accès mémoire non démontré en usage réel, pas d'intégration aveugle. Jauge active, ED/EE, CB restent verrouillés. Aucune modification DLL, aucune écriture ECU, pas de publication publique. CI et essai physique distincts.
