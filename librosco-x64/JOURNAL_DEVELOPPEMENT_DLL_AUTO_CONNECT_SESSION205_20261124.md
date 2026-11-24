# Journal DLL — auto-connexion moteur — 24 novembre 2026

Branche unique dll-manager. Référence immuable Manager 205 b5cd9b61bbaed6251f17559885481b1bd03f4380. Aucun changement du programme, de son Setup officiel ou du site MAJ. Cinq exports supplémentaires (T04–T08) :

- 134 `mems_manager_auto_connect`
- 135 `mems_manager_get_session_identity`
- 136 `mems_manager_get_session_family`
- 137 `mems_manager_get_session_capabilities`
- 138 `mems_manager_disconnect_auto`

Reprise de l'ordre exact de memsinterface_dispatch.cpp : indice KKL 1A86:7523 = 2J, 3, 1.9, ROSCO ; autres = ROSCO, 2J, 3, 1.9. Énumération native et port préféré en premier. La connexion n'est retournée qu'après dialogue ECU, pas à l'ouverture COM. Le sous-type historique 1.2/1.3/1.6 n'est pas supposé sans preuve ; D0 brut conservé pour identification ultérieure. 2J/3 identifiés par leur dialogue propre, sans inventer un identifiant D0. Fermeture des essais en échec, propriété unique du transport retenu. Les API de capacités décrivent les implémentations disponibles, non les permissions ou capteurs exacts d'un ECU. Aucun RC5/5AS dans la découverte moteur, aucun effacement ou réglage implicite.

Tests déterministes : ordre, fallback même câble et autre port, absence d'ECU, annulation, ressources libérées, identité et capacités. Le test substitue toutes les fonctions d'accès matériel. Pas de matériel physique. Limites : annulation legacy seulement entre appels bloquants ; trace byte TX/RX actuellement 2J/3 uniquement ; les prochaines API génériques d'acquisition doivent exploiter le handle opaque. T09–T15 encore à développer. CI du lot adaptateurs lancée avant ce commit : ne pas l'annuler ; nouvelle validation 138 exports/13 CTest à déclencher après son résultat. Ce commit ne modifie pas le workflow.
