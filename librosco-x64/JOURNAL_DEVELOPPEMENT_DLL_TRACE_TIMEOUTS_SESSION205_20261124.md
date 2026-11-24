# Journal DLL — traçage legacy et délais — 24 novembre 2026

Branche unique dll-manager ; Manager 205 inchangé. Run adaptateurs 45389288786 VERT (12/12 CTest, 133 exports, 15 imports, 16 pages, Setup isolé). Ajout de deux exports 148–149, les 147 précédents restent conservés.

- 148 `mems_manager_set_session_timeouts`
- 149 `mems_manager_get_session_timeouts`

Traçage legacy : les octets réels des ReadFile/WriteFile historiques et du réveil 1.9 sont transmis au journal uniquement dans un scope interne, associé au handle et au thread de l'auto-session. Hors scope, pas de trace ni modification du protocole historique. Les transitions électriques BREAK ne sont pas des octets. Ni changement de structure mems_info ni export interne. Tests de correspondance TX/RX et absence d'effet hors scope, réveil 1.9. Pas de fichier créé par la DLL.

Délais : lecture et configuration des cinq paramètres COMMTIMEOUTS réels et, pour 2J/3, délai de réponse optionnel par transaction. Zéro conserve le délai spécifique historique du protocole encadré ; 1.x/1.9 ne supporte pas un délai global distinct. Configuration après connexion seulement. Les timings de réveil ne changent pas. Pas de garantie de délai global strict, les appels Win32 peuvent attendre selon leur configuration. Tests génériques lecture/écriture/retour par défaut. Validation intégrale 149 exports et 13 CTests à lancer ; aucun test physique ECU. Pas de modification du programme ni de publication officielle.
