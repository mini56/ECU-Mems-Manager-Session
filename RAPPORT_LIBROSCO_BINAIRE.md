# Audit binaire de `prebuilt-librosco/librosco.dll`

> Généré automatiquement sur la branche `RAPPORT` afin de conserver un état lisible et transmissible entre discussions.

## Identité du binaire

- Taille : **49 672 octets**
- SHA-256 : `a5ad466f8f1a198a6f8259a5ea5ab59775815c8cbc9637aa584db9e9c811d38f`
- SHA-1 : `70b8bf3265510fe06cf9ac8921b3a07c344d9132`
- MD5 : `b03655b490239a9d44c45babfa7de239`
- Machine PE : **IMAGE_FILE_MACHINE_I386 (x86 / 32 bits)**
- Optional Header : **PE32 (32 bits)**
- ImageBase : `0x6EC80000`
- EntryPoint RVA : `0x1410`
- Horodatage COFF brut interprété UTC : `2018-04-07T08:26:27+00:00`
- Flag DLL : **oui**
- Section relocations présente : **oui**
- Signature Authenticode embarquée : **non**

## Correspondance API `rosco.h` / exports DLL

- Fonctions `mems_*` déclarées dans le header : **35**
- Noms exportés par la DLL : **22**
- Noms communs header + DLL : **16**
- Exports DLL absents du header : **6**
- Déclarations du header absentes des exports : **19**

### Exports présents dans la DLL mais absents de `rosco.h`

- `mems_lock`
- `mems_openserial`
- `mems_read_serial`
- `mems_send_command`
- `mems_unlock`
- `mems_write_serial`

### Fonctions déclarées dans `rosco.h` mais non exportées

- `mems_Boost_Valve_Off`
- `mems_Boost_Valve_On`
- `mems_Fan1_Off`
- `mems_Fan1_On`
- `mems_Fan2_Off`
- `mems_Fan2_On`
- `mems_O2Heater_Off`
- `mems_O2Heater_On`
- `mems_Purge_Valve_Off`
- `mems_Purge_Valve_On`
- `mems_fuel_trim_minus`
- `mems_fuel_trim_plus`
- `mems_idle_decay_minus`
- `mems_idle_decay_plus`
- `mems_idle_speed_minus`
- `mems_idle_speed_plus`
- `mems_ignition_advance_minus`
- `mems_ignition_advance_plus`
- `mems_interactive_mode`

## Table exacte des exports

| Ordinal | RVA | Nom | Forwarder |
|---:|---:|---|---|
| 1 | `0x000015BB` | `mems_cleanup` | `` |
| 2 | `0x000021E4` | `mems_clear_faults` | `` |
| 3 | `0x0000168E` | `mems_connect` | `` |
| 4 | `0x00001629` | `mems_disconnect` | `` |
| 5 | `0x00001601` | `mems_get_lib_version` | `` |
| 6 | `0x00002358` | `mems_heartbeat` | `` |
| 7 | `0x00001580` | `mems_init` | `` |
| 8 | `0x00001A86` | `mems_init_link` | `` |
| 9 | `0x00001864` | `mems_is_connected` | `` |
| 10 | `0x00001BFA` | `mems_lock` | `` |
| 11 | `0x000020BD` | `mems_move_iac` | `` |
| 12 | `0x00001708` | `mems_openserial` | `` |
| 13 | `0x00001D17` | `mems_read` | `` |
| 14 | `0x00002049` | `mems_read_iac_position` | `` |
| 15 | `0x00001C49` | `mems_read_raw` | `` |
| 16 | `0x00001874` | `mems_read_serial` | `` |
| 17 | `0x00002260` | `mems_reset_ECU` | `` |
| 18 | `0x000022DC` | `mems_reset_adjustments` | `` |
| 19 | `0x000019C6` | `mems_send_command` | `` |
| 20 | `0x0000216A` | `mems_test_actuator` | `` |
| 21 | `0x00001C2D` | `mems_unlock` | `` |
| 22 | `0x00001949` | `mems_write_serial` | `` |

## DLL importées et fonctions utilisées

### `KERNEL32.dll` — 33 imports

- `CloseHandle`
- `CreateFileA`
- `CreateMutexA`
- `DeleteCriticalSection`
- `EnterCriticalSection`
- `FreeLibrary`
- `GetCommState`
- `GetCommTimeouts`
- `GetCurrentProcess`
- `GetCurrentProcessId`
- `GetCurrentThreadId`
- `GetLastError`
- `GetModuleHandleA`
- `GetProcAddress`
- `GetSystemTimeAsFileTime`
- `GetTickCount`
- `InitializeCriticalSection`
- `LeaveCriticalSection`
- `LoadLibraryA`
- `QueryPerformanceCounter`
- `ReadFile`
- `ReleaseMutex`
- `SetCommState`
- `SetCommTimeouts`
- `SetUnhandledExceptionFilter`
- `Sleep`
- `TerminateProcess`
- `TlsGetValue`
- `UnhandledExceptionFilter`
- `VirtualProtect`
- `VirtualQuery`
- `WaitForSingleObject`
- `WriteFile`

### `msvcrt.dll` — 18 imports

- `__dllonexit`
- `_amsg_exit`
- `_initterm`
- `_iob`
- `_lock`
- `_onexit`
- `_unlock`
- `abort`
- `calloc`
- `free`
- `fwrite`
- `malloc`
- `memset`
- `printf`
- `puts`
- `strlen`
- `strncmp`
- `vfprintf`

## Sections PE

| Section | RVA | VirtualSize | RawSize | Characteristics |
|---|---:|---:|---:|---:|
| `.text` | `0x00001000` | 8868 | 9216 | `0x60500060` |
| `.data` | `0x00004000` | 32 | 512 | `0xC0300040` |
| `.rdata` | `0x00005000` | 2252 | 2560 | `0x40300040` |
| `/4` | `0x00006000` | 2796 | 3072 | `0x40300040` |
| `.bss` | `0x00007000` | 1064 | 0 | `0xC0700080` |
| `.edata` | `0x00008000` | 621 | 1024 | `0x40300040` |
| `.idata` | `0x00009000` | 1524 | 1536 | `0xC0300040` |
| `.CRT` | `0x0000A000` | 44 | 512 | `0xC0300040` |
| `.tls` | `0x0000B000` | 32 | 512 | `0xC0300040` |
| `.reloc` | `0x0000C000` | 612 | 1024 | `0x42300040` |
| `/14` | `0x0000D000` | 56 | 512 | `0x42400040` |
| `/29` | `0x0000E000` | 3559 | 3584 | `0x42100040` |
| `/41` | `0x0000F000` | 137 | 512 | `0x42100040` |
| `/55` | `0x00010000` | 297 | 512 | `0x42100040` |
| `/67` | `0x00011000` | 56 | 512 | `0x42300040` |

## Indices de chaîne de compilation

- Sections de type `/NN` typiques d’un binaire GNU/MinGW avec chaînes/constantes fractionnées.
- Import de `msvcrt.dll` sans VCRUNTIME/MSVCP : cohérent avec un build MinGW/GCC ancien ou statiquement lié côté libgcc.

## Conclusion automatique

- **La DLL analysée est bien une DLL Windows x86 32 bits (PE32).** Elle ne peut pas être chargée directement par un processus Windows x64 natif.
- Le header et le binaire ne sont pas parfaitement alignés : les écarts listés ci-dessus doivent être traités avant migration x64.
- Cet audit décrit le contrat binaire. La signification protocolaire des commandes reste à comparer aux sources Colin/Haro/Leopold avant de reconstruire la DLL x64.

