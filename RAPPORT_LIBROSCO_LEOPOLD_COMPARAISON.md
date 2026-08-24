# Comparaison binaire — LeopoldG `librosco.dll`

> Rapport de comparaison uniquement. Cette DLL externe ne remplace pas la DLL validée d’ECU MEMS Manager et ses significations de commandes ne sont pas promues automatiquement.

## Identité

- Source publique : `LeopoldG/mems-rosco/main/librosco.dll`
- Taille : **63161 octets**
- SHA-256 : `8c3087e2ce0c453eb250caff6a2ace1928563d593584dabb255a107bf8195911`
- Architecture : **I386 / x86 32 bits / PE32**
- Nombre d’exports nommés : **45**
- DLL importées : `KERNEL32.dll`, `msvcrt.dll`

## Comparaison avec les 22 exports de notre DLL historique

- Exports communs : **21**
- Exports supplémentaires Leopold : **24**
- Exports historiques absents chez Leopold : **1**

### Exports communs

- `mems_cleanup`
- `mems_clear_faults`
- `mems_connect`
- `mems_disconnect`
- `mems_get_lib_version`
- `mems_heartbeat`
- `mems_init`
- `mems_init_link`
- `mems_is_connected`
- `mems_lock`
- `mems_move_iac`
- `mems_openserial`
- `mems_read`
- `mems_read_iac_position`
- `mems_read_raw`
- `mems_read_serial`
- `mems_reset_ECU`
- `mems_send_command`
- `mems_test_actuator`
- `mems_unlock`
- `mems_write_serial`

### Exports supplémentaires Leopold

- `mems_Boost_Valve_Off`
- `mems_Boost_Valve_On`
- `mems_CloseIAC`
- `mems_Fan1_Off`
- `mems_Fan1_On`
- `mems_Fan2_Off`
- `mems_Fan2_On`
- `mems_O2Heater_Off`
- `mems_O2Heater_On`
- `mems_OpenIAC`
- `mems_Purge_Valve_Off`
- `mems_Purge_Valve_On`
- `mems_Save`
- `mems_fuel_trim_minus`
- `mems_fuel_trim_plus`
- `mems_idle_decay_minus`
- `mems_idle_decay_plus`
- `mems_idle_speed_minus`
- `mems_idle_speed_plus`
- `mems_ignition_advance_minus`
- `mems_ignition_advance_plus`
- `mems_reset_ADJ`
- `mems_reset_EMI`
- `temperature_value_to_degrees_f`

### Exports historiques absents

- `mems_reset_adjustments`

## Table complète des exports Leopold

1. `mems_Boost_Valve_Off`
2. `mems_Boost_Valve_On`
3. `mems_CloseIAC`
4. `mems_Fan1_Off`
5. `mems_Fan1_On`
6. `mems_Fan2_Off`
7. `mems_Fan2_On`
8. `mems_O2Heater_Off`
9. `mems_O2Heater_On`
10. `mems_OpenIAC`
11. `mems_Purge_Valve_Off`
12. `mems_Purge_Valve_On`
13. `mems_Save`
14. `mems_cleanup`
15. `mems_clear_faults`
16. `mems_connect`
17. `mems_disconnect`
18. `mems_fuel_trim_minus`
19. `mems_fuel_trim_plus`
20. `mems_get_lib_version`
21. `mems_heartbeat`
22. `mems_idle_decay_minus`
23. `mems_idle_decay_plus`
24. `mems_idle_speed_minus`
25. `mems_idle_speed_plus`
26. `mems_ignition_advance_minus`
27. `mems_ignition_advance_plus`
28. `mems_init`
29. `mems_init_link`
30. `mems_is_connected`
31. `mems_lock`
32. `mems_move_iac`
33. `mems_openserial`
34. `mems_read`
35. `mems_read_iac_position`
36. `mems_read_raw`
37. `mems_read_serial`
38. `mems_reset_ADJ`
39. `mems_reset_ECU`
40. `mems_reset_EMI`
41. `mems_send_command`
42. `mems_test_actuator`
43. `mems_unlock`
44. `mems_write_serial`
45. `temperature_value_to_degrees_f`

## Interprétation

- Une API plus large ne signifie pas que les sémantiques sont compatibles avec notre binaire Haro. Les conflits `0x0F`, `0xFA` et `0xAF` restent explicitement bloquants pour un remplacement direct.
- Cette comparaison sert à identifier des capacités à préserver dans la cartographie, pas à choisir Leopold 3.0 comme base automatique de la DLL x64.

