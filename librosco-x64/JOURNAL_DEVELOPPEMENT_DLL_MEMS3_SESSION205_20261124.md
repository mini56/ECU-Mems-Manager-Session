# Journal DLL — MEMS 3 — 24 novembre 2026

Branche unique dll-manager, référence Manager 205 immuable b5cd9b61bbaed6251f17559885481b1bd03f4380. Aucun changement de Manager, de son Setup officiel ni de son site MAJ.

Les exports 1–114 sont préservés. Neuf nouveaux exports 115–123 :

- 115 `mems_manager_mems3_connect`
- 116 `mems_manager_mems3_read_measurement`
- 117 `mems_manager_mems3_read_faults`
- 118 `mems_manager_mems3_clear_faults`
- 119 `mems_manager_mems3_keepalive`
- 120 `mems_manager_mems3_read_cycle`
- 121 `mems_manager_mems3_last_transaction`
- 122 `mems_manager_mems3_is_open`
- 123 `mems_manager_mems3_close`

Transport Win32 9600 8E1, réveil encadré 1A 9A / 5A 9A, session 10 A0, seed/key 27 (seed zéro : pas de clé), ping 3E/7E. Trames B8 13 F7 LEN PAYLOAD XOR ; réponses avec adresse ECU variable. Lecture mesures 21 XX et cycle DTC + 00/06/0A/0B/21 ; DTC 18 00 00 00 / 58, effacement explicite 14 00 00 / 54 00 00. Aucun effacement automatique. Traces TX/RX, réponses négatives, pending, timeout, annulation. Codecs MEMS3 existants réutilisés, aucune dépendance Qt. Sources de référence : mems3protocol.h de Session 205 et protocol_catalogue.c.

Test Win32 simulé avec réponses XOR indépendantes, écho fragmenté/complet ou brut, seed zéro, erreurs, DTC et fermeture. Validation CI prévue : compilation DLL, 10 CTest, 123 exports exacts, 15 imports Manager, 16 pages, Setup isolé. **Résultat CI non encore constaté ; aucun test sur ECU physique.**
