# Journal DLL — opérations et traçage auto-session — 24 novembre 2026

Branche unique dll-manager. Manager 205 inchangé. Les 138 exports précédents sont conservés. Nouveaux exports :

- 139 `mems_manager_cancel_current`
- 140 `mems_manager_get_transport_error`
- 141 `mems_manager_get_next_trace_event`
- 142 `mems_manager_clear_trace`
- 143 `mems_manager_get_response_timing`
- 144 `mems_manager_auto_read_legacy`
- 145 `mems_manager_auto_read_measurement`
- 146 `mems_manager_auto_read_faults`
- 147 `mems_manager_auto_keepalive`

T09, T12–T15 : annulation coopérative, état de dernière opération, FIFO TX/RX bornée (256 événements, séquence et pertes signalées), vidage, durée mesurée côté hôte. Le journal est en mémoire, sans fichier créé par la DLL. Les callbacks natifs 2J/3 alimentent la FIFO avec famille et port de chaque essai. Les trames legacy ne sont pas encore tracées. Annulation de la connexion par le callback existant ; après connexion demande atomique pendant opération active, sans fermer le port pendant un appel. Les appels historiques bloquants ne sont pas interrompus de force. Aucun résultat de mesure n'est retourné comme réussi après annulation.

Quatre exports d'acquisition générique ajoutés pour que le handle opaque de l'auto-connexion soit réellement exploitable : lectures legacy (structure historique, y compris DTC), mesures et DTC encadrés 2J/3, maintien de session. Réutilisation des implémentations existantes, aucun réglage ou effacement automatique. Les délais T10/T11 ne sont pas encore paramétrables. Durée mesurée = opération hôte, pas latence ECU isolée ; retour négatif natif conservé lorsqu'il existe. Le total 175 est une projection initiale, les quatre wrappers supplémentaires peuvent modifier le découpage final.

Test hors matériel : contrôles d'ordre précédents, lecture sur session retenue, erreur native, annulation sans écraser résultats, reprise, débordement FIFO et vidage. CI non encore constatée. Run adaptateurs 45389288786 encore en cours lors de la préparation ; pas de nouveau lancement avant son résultat pour éviter son annulation. Aucun changement de programme, Setup officiel ou MAJ.
