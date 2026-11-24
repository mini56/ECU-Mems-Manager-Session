# Journal DLL — RC5 / EC5 Airbag — 24 novembre 2026

Branche unique dll-manager ; Manager Session 205 figé b5cd9b61bbaed6251f17559885481b1bd03f4380. Le lot MEMS3 précédent est vert (run 45367580697). Aucun changement du programme ni de son installateur officiel, ni MAJ.

Les 123 premiers exports et leurs signatures restent inchangés. Sept nouveaux exports :

- 124 `mems_manager_rc5_open_transport`
- 125 `mems_manager_rc5_wakeup`
- 126 `mems_manager_rc5_ping`
- 127 `mems_manager_rc5_read_faults`
- 128 `mems_manager_rc5_decode_fault_detail`
- 129 `mems_manager_rc5_clear_faults`
- 130 `mems_manager_rc5_close_session`

Source : rc5protocol.h de Session 205, codecs RC5 historiques de la DLL. Transport dédié 2400 8N1, réveil BREAK et validation réelle de réponse, ping avant lecture. Lecture de trame DTC intacte et décodage par table de 24 identifiants connus (clé de traduction, présent/historique et inconnu conservés). La commande standard d'effacement DTC nécessite une invocation explicite confirmée et un acquittement spécifique. Elle ne répare pas un défaut, ne garantit pas sa disparition et ne concerne pas les données de choc, la programmation ou le déclenchement d'airbag. Aucun envoi implicite d'effacement. Aucune découverte RC5 dans le moteur MEMS. Interface électrique adaptée nécessaire.

Le CTest simule le port et l'horloge Win32 ; tests de rythme du réveil, réponse ping, écho fragmenté, défauts présents/historiques/inconnus, zéro défaut, checksum, confirmation, refus et annulation. Pas de matériel connecté. Validation attendue : 11 tests, 130 exports PE, 15 imports de Manager, 16 pages et Setup séparé. **CI à confirmer.**

