# Désassemblage ciblé de la `librosco.dll` 32 bits

> Rapport de vérification binaire. Il sert à contrôler ce que fait réellement la DLL, indépendamment des commentaires et des headers des différents forks.

## Fonctions critiques

### `mems_get_lib_version`

```asm
6EC81601: push     ebp
6EC81602: mov      ebp, esp
6EC81604: sub      esp, 0x10
6EC81607: mov      byte ptr [ebp - 3], 0
6EC8160B: mov      byte ptr [ebp - 2], 1
6EC8160F: mov      byte ptr [ebp - 1], 0xc
6EC81613: mov      eax, dword ptr [ebp + 8]
6EC81616: movzx    edx, word ptr [ebp - 3]
6EC8161A: mov      word ptr [eax], dx
6EC8161D: movzx    edx, byte ptr [ebp - 1]
6EC81621: mov      byte ptr [eax + 2], dl
6EC81624: mov      eax, dword ptr [ebp + 8]
6EC81627: leave    
6EC81628: ret      
```

### `mems_init_link`

```asm
6EC81A86: push     ebp
6EC81A87: mov      ebp, esp
6EC81A89: sub      esp, 0x28
6EC81A8C: mov      byte ptr [ebp - 9], 0xca
6EC81A90: mov      byte ptr [ebp - 0xa], 0x75
6EC81A94: mov      byte ptr [ebp - 0xb], 0xf4
6EC81A98: mov      byte ptr [ebp - 0xc], 0xd0
6EC81A9C: mov      byte ptr [ebp - 0xd], 0
6EC81AA0: movzx    eax, byte ptr [ebp - 9]
6EC81AA4: mov      dword ptr [esp + 4], eax
6EC81AA8: mov      eax, dword ptr [ebp + 8]
6EC81AAB: mov      dword ptr [esp], eax
6EC81AAE: call     0x6ec819c6 ; -> mems_send_command
6EC81AB3: xor      eax, 1
6EC81AB6: test     al, al
6EC81AB8: je       0x6ec81ad8
6EC81ABA: movzx    eax, byte ptr [ebp - 9]
6EC81ABE: mov      dword ptr [esp + 4], eax
6EC81AC2: mov      dword ptr [esp], 0x6ec85158
6EC81AC9: call     0x6ec83220
6EC81ACE: mov      eax, 0
6EC81AD3: jmp      0x6ec81bf8
6EC81AD8: movzx    eax, byte ptr [ebp - 0xa]
6EC81ADC: mov      dword ptr [esp + 4], eax
6EC81AE0: mov      eax, dword ptr [ebp + 8]
6EC81AE3: mov      dword ptr [esp], eax
6EC81AE6: call     0x6ec819c6 ; -> mems_send_command
6EC81AEB: xor      eax, 1
6EC81AEE: test     al, al
6EC81AF0: je       0x6ec81b10
6EC81AF2: movzx    eax, byte ptr [ebp - 0xa]
6EC81AF6: mov      dword ptr [esp + 4], eax
6EC81AFA: mov      dword ptr [esp], 0x6ec85158
6EC81B01: call     0x6ec83220
6EC81B06: mov      eax, 0
6EC81B0B: jmp      0x6ec81bf8
6EC81B10: movzx    eax, byte ptr [ebp - 0xb]
6EC81B14: mov      dword ptr [esp + 4], eax
6EC81B18: mov      eax, dword ptr [ebp + 8]
6EC81B1B: mov      dword ptr [esp], eax
6EC81B1E: call     0x6ec819c6 ; -> mems_send_command
6EC81B23: xor      eax, 1
6EC81B26: test     al, al
6EC81B28: je       0x6ec81b48
6EC81B2A: movzx    eax, byte ptr [ebp - 0xb]
6EC81B2E: mov      dword ptr [esp + 4], eax
6EC81B32: mov      dword ptr [esp], 0x6ec85158
6EC81B39: call     0x6ec83220
6EC81B3E: mov      eax, 0
6EC81B43: jmp      0x6ec81bf8
6EC81B48: mov      dword ptr [esp + 8], 1
6EC81B50: lea      eax, [ebp - 0xd]
6EC81B53: mov      dword ptr [esp + 4], eax
6EC81B57: mov      eax, dword ptr [ebp + 8]
6EC81B5A: mov      dword ptr [esp], eax
6EC81B5D: call     0x6ec81874 ; -> mems_read_serial
6EC81B62: cmp      ax, 1
6EC81B66: je       0x6ec81b83
6EC81B68: movzx    eax, byte ptr [ebp - 0xb]
6EC81B6C: mov      dword ptr [esp + 4], eax
6EC81B70: mov      dword ptr [esp], 0x6ec8518c
6EC81B77: call     0x6ec83220
6EC81B7C: mov      eax, 0
6EC81B81: jmp      0x6ec81bf8
```

### `mems_reset_ECU`

```asm
6EC82260: push     ebp
6EC82261: mov      ebp, esp
6EC82263: sub      esp, 0x28
6EC82266: mov      byte ptr [ebp - 9], 0
6EC8226A: mov      byte ptr [ebp - 0xa], 0xff
6EC8226E: mov      eax, dword ptr [ebp + 8]
6EC82271: mov      dword ptr [esp], eax
6EC82274: call     0x6ec81bfa ; -> mems_lock
6EC82279: test     al, al
6EC8227B: je       0x6ec822d6
6EC8227D: mov      dword ptr [esp + 4], 0xfa
6EC82285: mov      eax, dword ptr [ebp + 8]
6EC82288: mov      dword ptr [esp], eax
6EC8228B: call     0x6ec819c6 ; -> mems_send_command
6EC82290: test     al, al
6EC82292: je       0x6ec822bb
6EC82294: mov      dword ptr [esp + 8], 1
6EC8229C: lea      eax, [ebp - 0xa]
6EC8229F: mov      dword ptr [esp + 4], eax
6EC822A3: mov      eax, dword ptr [ebp + 8]
6EC822A6: mov      dword ptr [esp], eax
6EC822A9: call     0x6ec81874 ; -> mems_read_serial
6EC822AE: cmp      ax, 1
6EC822B2: jne      0x6ec822bb
6EC822B4: mov      eax, 1
6EC822B9: jmp      0x6ec822c0
6EC822BB: mov      eax, 0
6EC822C0: mov      byte ptr [ebp - 9], al
6EC822C3: and      byte ptr [ebp - 9], 1
6EC822C7: mov      byte ptr [ebp - 9], 1
6EC822CB: mov      eax, dword ptr [ebp + 8]
6EC822CE: mov      dword ptr [esp], eax
6EC822D1: call     0x6ec81c2d ; -> mems_unlock
6EC822D6: movzx    eax, byte ptr [ebp - 9]
6EC822DA: leave    
6EC822DB: ret      
```

### `mems_reset_adjustments`

```asm
6EC822DC: push     ebp
6EC822DD: mov      ebp, esp
6EC822DF: sub      esp, 0x28
6EC822E2: mov      byte ptr [ebp - 9], 0
6EC822E6: mov      byte ptr [ebp - 0xa], 0xff
6EC822EA: mov      eax, dword ptr [ebp + 8]
6EC822ED: mov      dword ptr [esp], eax
6EC822F0: call     0x6ec81bfa ; -> mems_lock
6EC822F5: test     al, al
6EC822F7: je       0x6ec82352
6EC822F9: mov      dword ptr [esp + 4], 0xf
6EC82301: mov      eax, dword ptr [ebp + 8]
6EC82304: mov      dword ptr [esp], eax
6EC82307: call     0x6ec819c6 ; -> mems_send_command
6EC8230C: test     al, al
6EC8230E: je       0x6ec82337
6EC82310: mov      dword ptr [esp + 8], 1
6EC82318: lea      eax, [ebp - 0xa]
6EC8231B: mov      dword ptr [esp + 4], eax
6EC8231F: mov      eax, dword ptr [ebp + 8]
6EC82322: mov      dword ptr [esp], eax
6EC82325: call     0x6ec81874 ; -> mems_read_serial
6EC8232A: cmp      ax, 1
6EC8232E: jne      0x6ec82337
6EC82330: mov      eax, 1
6EC82335: jmp      0x6ec8233c
6EC82337: mov      eax, 0
6EC8233C: mov      byte ptr [ebp - 9], al
6EC8233F: and      byte ptr [ebp - 9], 1
6EC82343: mov      byte ptr [ebp - 9], 1
6EC82347: mov      eax, dword ptr [ebp + 8]
6EC8234A: mov      dword ptr [esp], eax
6EC8234D: call     0x6ec81c2d ; -> mems_unlock
6EC82352: movzx    eax, byte ptr [ebp - 9]
6EC82356: leave    
6EC82357: ret      
```

### `mems_clear_faults`

```asm
6EC821E4: push     ebp
6EC821E5: mov      ebp, esp
6EC821E7: sub      esp, 0x28
6EC821EA: mov      byte ptr [ebp - 9], 0
6EC821EE: mov      byte ptr [ebp - 0xa], 0xff
6EC821F2: mov      eax, dword ptr [ebp + 8]
6EC821F5: mov      dword ptr [esp], eax
6EC821F8: call     0x6ec81bfa ; -> mems_lock
6EC821FD: test     al, al
6EC821FF: je       0x6ec8225a
6EC82201: mov      dword ptr [esp + 4], 0xcc
6EC82209: mov      eax, dword ptr [ebp + 8]
6EC8220C: mov      dword ptr [esp], eax
6EC8220F: call     0x6ec819c6 ; -> mems_send_command
6EC82214: test     al, al
6EC82216: je       0x6ec8223f
6EC82218: mov      dword ptr [esp + 8], 1
6EC82220: lea      eax, [ebp - 0xa]
6EC82223: mov      dword ptr [esp + 4], eax
6EC82227: mov      eax, dword ptr [ebp + 8]
6EC8222A: mov      dword ptr [esp], eax
6EC8222D: call     0x6ec81874 ; -> mems_read_serial
6EC82232: cmp      ax, 1
6EC82236: jne      0x6ec8223f
6EC82238: mov      eax, 1
6EC8223D: jmp      0x6ec82244
6EC8223F: mov      eax, 0
6EC82244: mov      byte ptr [ebp - 9], al
6EC82247: and      byte ptr [ebp - 9], 1
6EC8224B: mov      byte ptr [ebp - 9], 1
6EC8224F: mov      eax, dword ptr [ebp + 8]
6EC82252: mov      dword ptr [esp], eax
6EC82255: call     0x6ec81c2d ; -> mems_unlock
6EC8225A: movzx    eax, byte ptr [ebp - 9]
6EC8225E: leave    
6EC8225F: ret      
```

### `mems_heartbeat`

```asm
6EC82358: push     ebp
6EC82359: mov      ebp, esp
6EC8235B: sub      esp, 0x28
6EC8235E: mov      byte ptr [ebp - 9], 0
6EC82362: mov      byte ptr [ebp - 0xa], 0xff
6EC82366: mov      eax, dword ptr [ebp + 8]
6EC82369: mov      dword ptr [esp], eax
6EC8236C: call     0x6ec81bfa ; -> mems_lock
6EC82371: test     al, al
6EC82373: je       0x6ec823bb
6EC82375: mov      dword ptr [esp + 4], 0xf4
6EC8237D: mov      eax, dword ptr [ebp + 8]
6EC82380: mov      dword ptr [esp], eax
6EC82383: call     0x6ec819c6 ; -> mems_send_command
6EC82388: test     al, al
6EC8238A: je       0x6ec823b0
6EC8238C: mov      dword ptr [esp + 8], 1
6EC82394: lea      eax, [ebp - 0xa]
6EC82397: mov      dword ptr [esp + 4], eax
6EC8239B: mov      eax, dword ptr [ebp + 8]
6EC8239E: mov      dword ptr [esp], eax
6EC823A1: call     0x6ec81874 ; -> mems_read_serial
6EC823A6: cmp      ax, 1
6EC823AA: jne      0x6ec823b0
6EC823AC: mov      byte ptr [ebp - 9], 1
6EC823B0: mov      eax, dword ptr [ebp + 8]
6EC823B3: mov      dword ptr [esp], eax
6EC823B6: call     0x6ec81c2d ; -> mems_unlock
6EC823BB: movzx    eax, byte ptr [ebp - 9]
6EC823BF: leave    
6EC823C0: ret      
```

### `mems_test_actuator`

```asm
6EC8216A: push     ebp
6EC8216B: mov      ebp, esp
6EC8216D: sub      esp, 0x28
6EC82170: mov      byte ptr [ebp - 9], 0
6EC82174: mov      byte ptr [ebp - 0xa], 0
6EC82178: mov      eax, dword ptr [ebp + 8]
6EC8217B: mov      dword ptr [esp], eax
6EC8217E: call     0x6ec81bfa ; -> mems_lock
6EC82183: test     al, al
6EC82185: je       0x6ec821de
6EC82187: mov      eax, dword ptr [ebp + 0xc]
6EC8218A: movzx    eax, al
6EC8218D: mov      dword ptr [esp + 4], eax
6EC82191: mov      eax, dword ptr [ebp + 8]
6EC82194: mov      dword ptr [esp], eax
6EC82197: call     0x6ec819c6 ; -> mems_send_command
6EC8219C: test     al, al
6EC8219E: je       0x6ec821d3
6EC821A0: mov      dword ptr [esp + 8], 1
6EC821A8: lea      eax, [ebp - 0xa]
6EC821AB: mov      dword ptr [esp + 4], eax
6EC821AF: mov      eax, dword ptr [ebp + 8]
6EC821B2: mov      dword ptr [esp], eax
6EC821B5: call     0x6ec81874 ; -> mems_read_serial
6EC821BA: cmp      ax, 1
6EC821BE: jne      0x6ec821d3
6EC821C0: cmp      dword ptr [ebp + 0x10], 0
6EC821C4: je       0x6ec821cf
6EC821C6: movzx    edx, byte ptr [ebp - 0xa]
6EC821CA: mov      eax, dword ptr [ebp + 0x10]
6EC821CD: mov      byte ptr [eax], dl
6EC821CF: mov      byte ptr [ebp - 9], 1
6EC821D3: mov      eax, dword ptr [ebp + 8]
6EC821D6: mov      dword ptr [esp], eax
6EC821D9: call     0x6ec81c2d ; -> mems_unlock
6EC821DE: movzx    eax, byte ptr [ebp - 9]
6EC821E2: leave    
6EC821E3: ret      
```

## Constantes immédiatement préparées avant `mems_send_command`

| Fonction | Valeurs immédiates 8-bit/32-bit observées dans les ~12 instructions avant un appel direct à `mems_send_command` |
|---|---|
| `mems_get_lib_version` | — |
| `mems_init_link` | 0x28, 0xCA, 0x75, 0xF4, 0xD0, 0x00 |
| `mems_reset_ECU` | 0x28, 0x00, 0xFF, 0xFA |
| `mems_reset_adjustments` | 0x28, 0x00, 0xFF, 0x0F |
| `mems_clear_faults` | 0x28, 0x00, 0xFF, 0xCC |
| `mems_heartbeat` | 0x28, 0x00, 0xFF, 0xF4 |
| `mems_test_actuator` | 0x00 |

**Attention :** cette colonne est une aide de lecture du désassemblage, pas une preuve sémantique à elle seule. La preuve est la séquence assembleur affichée au-dessus et doit être rapprochée de la source C.

## Désassemblage de tous les exports

### Ordinal 7 — `mems_init` — RVA `0x00001580`

```asm
6EC81580: push     ebp
6EC81581: mov      ebp, esp
6EC81583: sub      esp, 0x18
6EC81586: mov      eax, dword ptr [ebp + 8]
6EC81589: mov      dword ptr [eax], 0xffffffff
6EC8158F: mov      dword ptr [esp + 8], 0
6EC81597: mov      dword ptr [esp + 4], 1
6EC8159F: mov      dword ptr [esp], 0
6EC815A6: mov      eax, dword ptr [0x6ec89118]
6EC815AB: call     eax
6EC815AD: sub      esp, 0xc
6EC815B0: mov      edx, eax
6EC815B2: mov      eax, dword ptr [ebp + 8]
6EC815B5: mov      dword ptr [eax + 4], edx
6EC815B8: nop      
6EC815B9: leave    
6EC815BA: ret      
```

### Ordinal 1 — `mems_cleanup` — RVA `0x000015BB`

```asm
6EC815BB: push     ebp
6EC815BC: mov      ebp, esp
6EC815BE: sub      esp, 0x18
6EC815C1: mov      eax, dword ptr [ebp + 8]
6EC815C4: mov      dword ptr [esp], eax
6EC815C7: call     0x6ec81864 ; -> mems_is_connected
6EC815CC: test     al, al
6EC815CE: je       0x6ec815eb
6EC815D0: mov      eax, dword ptr [ebp + 8]
6EC815D3: mov      eax, dword ptr [eax]
6EC815D5: mov      dword ptr [esp], eax
6EC815D8: mov      eax, dword ptr [0x6ec89110]
6EC815DD: call     eax
6EC815DF: sub      esp, 4
6EC815E2: mov      eax, dword ptr [ebp + 8]
6EC815E5: mov      dword ptr [eax], 0xffffffff
6EC815EB: mov      eax, dword ptr [ebp + 8]
6EC815EE: mov      eax, dword ptr [eax + 4]
6EC815F1: mov      dword ptr [esp], eax
6EC815F4: mov      eax, dword ptr [0x6ec89110]
6EC815F9: call     eax
6EC815FB: sub      esp, 4
6EC815FE: nop      
6EC815FF: leave    
6EC81600: ret      
```

### Ordinal 5 — `mems_get_lib_version` — RVA `0x00001601`

```asm
6EC81601: push     ebp
6EC81602: mov      ebp, esp
6EC81604: sub      esp, 0x10
6EC81607: mov      byte ptr [ebp - 3], 0
6EC8160B: mov      byte ptr [ebp - 2], 1
6EC8160F: mov      byte ptr [ebp - 1], 0xc
6EC81613: mov      eax, dword ptr [ebp + 8]
6EC81616: movzx    edx, word ptr [ebp - 3]
6EC8161A: mov      word ptr [eax], dx
6EC8161D: movzx    edx, byte ptr [ebp - 1]
6EC81621: mov      byte ptr [eax + 2], dl
6EC81624: mov      eax, dword ptr [ebp + 8]
6EC81627: leave    
6EC81628: ret      
```

### Ordinal 4 — `mems_disconnect` — RVA `0x00001629`

```asm
6EC81629: push     ebp
6EC8162A: mov      ebp, esp
6EC8162C: sub      esp, 0x18
6EC8162F: mov      eax, dword ptr [ebp + 8]
6EC81632: mov      eax, dword ptr [eax + 4]
6EC81635: mov      dword ptr [esp + 4], 0xffffffff
6EC8163D: mov      dword ptr [esp], eax
6EC81640: mov      eax, dword ptr [0x6ec8918c]
6EC81645: call     eax
6EC81647: sub      esp, 8
6EC8164A: test     eax, eax
6EC8164C: jne      0x6ec8168b
6EC8164E: mov      eax, dword ptr [ebp + 8]
6EC81651: mov      dword ptr [esp], eax
6EC81654: call     0x6ec81864 ; -> mems_is_connected
6EC81659: test     al, al
6EC8165B: je       0x6ec81678
6EC8165D: mov      eax, dword ptr [ebp + 8]
6EC81660: mov      eax, dword ptr [eax]
6EC81662: mov      dword ptr [esp], eax
6EC81665: mov      eax, dword ptr [0x6ec89110]
6EC8166A: call     eax
6EC8166C: sub      esp, 4
6EC8166F: mov      eax, dword ptr [ebp + 8]
6EC81672: mov      dword ptr [eax], 0xffffffff
6EC81678: mov      eax, dword ptr [ebp + 8]
6EC8167B: mov      eax, dword ptr [eax + 4]
6EC8167E: mov      dword ptr [esp], eax
6EC81681: mov      eax, dword ptr [0x6ec89164]
6EC81686: call     eax
6EC81688: sub      esp, 4
6EC8168B: nop      
6EC8168C: leave    
6EC8168D: ret      
```

### Ordinal 3 — `mems_connect` — RVA `0x0000168E`

```asm
6EC8168E: push     ebp
6EC8168F: mov      ebp, esp
6EC81691: sub      esp, 0x28
6EC81694: mov      byte ptr [ebp - 9], 0
6EC81698: mov      eax, dword ptr [ebp + 8]
6EC8169B: mov      eax, dword ptr [eax + 4]
6EC8169E: mov      dword ptr [esp + 4], 0xffffffff
6EC816A6: mov      dword ptr [esp], eax
6EC816A9: mov      eax, dword ptr [0x6ec8918c]
6EC816AE: call     eax
6EC816B0: sub      esp, 8
6EC816B3: test     eax, eax
6EC816B5: jne      0x6ec81702
6EC816B7: mov      eax, dword ptr [ebp + 8]
6EC816BA: mov      dword ptr [esp], eax
6EC816BD: call     0x6ec81864 ; -> mems_is_connected
6EC816C2: test     al, al
6EC816C4: jne      0x6ec816dc
6EC816C6: mov      eax, dword ptr [ebp + 0xc]
6EC816C9: mov      dword ptr [esp + 4], eax
6EC816CD: mov      eax, dword ptr [ebp + 8]
6EC816D0: mov      dword ptr [esp], eax
6EC816D3: call     0x6ec81708 ; -> mems_openserial
6EC816D8: test     al, al
6EC816DA: je       0x6ec816e3
6EC816DC: mov      eax, 1
6EC816E1: jmp      0x6ec816e8
6EC816E3: mov      eax, 0
6EC816E8: mov      byte ptr [ebp - 9], al
6EC816EB: and      byte ptr [ebp - 9], 1
6EC816EF: mov      eax, dword ptr [ebp + 8]
6EC816F2: mov      eax, dword ptr [eax + 4]
6EC816F5: mov      dword ptr [esp], eax
6EC816F8: mov      eax, dword ptr [0x6ec89164]
6EC816FD: call     eax
6EC816FF: sub      esp, 4
6EC81702: movzx    eax, byte ptr [ebp - 9]
6EC81706: leave    
6EC81707: ret      
```

### Ordinal 12 — `mems_openserial` — RVA `0x00001708`

```asm
6EC81708: push     ebp
6EC81709: mov      ebp, esp
6EC8170B: sub      esp, 0x68
6EC8170E: mov      byte ptr [ebp - 9], 0
6EC81712: mov      dword ptr [esp + 0x18], 0
6EC8171A: mov      dword ptr [esp + 0x14], 0x80
6EC81722: mov      dword ptr [esp + 0x10], 3
6EC8172A: mov      dword ptr [esp + 0xc], 0
6EC81732: mov      dword ptr [esp + 8], 0
6EC8173A: mov      dword ptr [esp + 4], 0xc0000000
6EC81742: mov      eax, dword ptr [ebp + 0xc]
6EC81745: mov      dword ptr [esp], eax
6EC81748: mov      eax, dword ptr [0x6ec89114]
6EC8174D: call     eax
6EC8174F: sub      esp, 0x1c
6EC81752: mov      edx, eax
6EC81754: mov      eax, dword ptr [ebp + 8]
6EC81757: mov      dword ptr [eax], edx
6EC81759: mov      eax, dword ptr [ebp + 8]
6EC8175C: mov      eax, dword ptr [eax]
6EC8175E: cmp      eax, -1
6EC81761: je       0x6ec8185e
6EC81767: mov      eax, dword ptr [ebp + 8]
6EC8176A: mov      eax, dword ptr [eax]
6EC8176C: lea      edx, [ebp - 0x28]
6EC8176F: mov      dword ptr [esp + 4], edx
6EC81773: mov      dword ptr [esp], eax
6EC81776: mov      eax, dword ptr [0x6ec89128]
6EC8177B: call     eax
6EC8177D: sub      esp, 8
6EC81780: cmp      eax, 1
6EC81783: jne      0x6ec81841
6EC81789: mov      dword ptr [ebp - 0x24], 0x2580
6EC81790: movzx    eax, byte ptr [ebp - 0x20]
6EC81794: and      eax, 0xfffffffd
6EC81797: mov      byte ptr [ebp - 0x20], al
6EC8179A: movzx    eax, byte ptr [ebp - 0x20]
6EC8179E: and      eax, 0xfffffffb
6EC817A1: mov      byte ptr [ebp - 0x20], al
6EC817A4: movzx    eax, byte ptr [ebp - 0x20]
6EC817A8: and      eax, 0xfffffff7
6EC817AB: mov      byte ptr [ebp - 0x20], al
6EC817AE: movzx    eax, byte ptr [ebp - 0x20]
6EC817B2: and      eax, 0xffffffcf
6EC817B5: mov      byte ptr [ebp - 0x20], al
6EC817B8: movzx    eax, byte ptr [ebp - 0x1f]
6EC817BC: and      eax, 0xffffffcf
6EC817BF: mov      byte ptr [ebp - 0x1f], al
6EC817C2: mov      byte ptr [ebp - 0x16], 8
6EC817C6: mov      byte ptr [ebp - 0x15], 0
6EC817CA: mov      byte ptr [ebp - 0x14], 0
6EC817CE: mov      eax, dword ptr [ebp + 8]
6EC817D1: mov      eax, dword ptr [eax]
6EC817D3: lea      edx, [ebp - 0x28]
6EC817D6: mov      dword ptr [esp + 4], edx
6EC817DA: mov      dword ptr [esp], eax
6EC817DD: mov      eax, dword ptr [0x6ec89168]
6EC817E2: call     eax
6EC817E4: sub      esp, 8
6EC817E7: cmp      eax, 1
6EC817EA: jne      0x6ec81841
6EC817EC: mov      eax, dword ptr [ebp + 8]
6EC817EF: mov      eax, dword ptr [eax]
6EC817F1: lea      edx, [ebp - 0x3c]
6EC817F4: mov      dword ptr [esp + 4], edx
6EC817F8: mov      dword ptr [esp], eax
6EC817FB: mov      eax, dword ptr [0x6ec8912c]
6EC81800: call     eax
6EC81802: sub      esp, 8
6EC81805: cmp      eax, 1
```

### Ordinal 9 — `mems_is_connected` — RVA `0x00001864`

```asm
6EC81864: push     ebp
6EC81865: mov      ebp, esp
6EC81867: mov      eax, dword ptr [ebp + 8]
6EC8186A: mov      eax, dword ptr [eax]
6EC8186C: cmp      eax, -1
6EC8186F: setne    al
6EC81872: pop      ebp
6EC81873: ret      
```

### Ordinal 16 — `mems_read_serial` — RVA `0x00001874`

```asm
6EC81874: push     ebp
6EC81875: mov      ebp, esp
6EC81877: sub      esp, 0x48
6EC8187A: mov      eax, dword ptr [ebp + 0x10]
6EC8187D: mov      word ptr [ebp - 0x1c], ax
6EC81881: mov      word ptr [ebp - 0xa], 0
6EC81887: mov      word ptr [ebp - 0xc], 0xffff
6EC8188D: mov      eax, dword ptr [ebp + 0xc]
6EC81890: mov      dword ptr [ebp - 0x10], eax
6EC81893: mov      dword ptr [ebp - 0x14], 0
6EC8189A: mov      eax, dword ptr [ebp + 8]
6EC8189D: mov      dword ptr [esp], eax
6EC818A0: call     0x6ec81864 ; -> mems_is_connected
6EC818A5: test     al, al
6EC818A7: je       0x6ec8191b
6EC818A9: mov      dword ptr [ebp - 0x18], 0
6EC818B0: movzx    edx, word ptr [ebp - 0x1c]
6EC818B4: mov      eax, dword ptr [ebp + 8]
6EC818B7: mov      eax, dword ptr [eax]
6EC818B9: mov      dword ptr [esp + 0x10], 0
6EC818C1: lea      ecx, [ebp - 0x18]
6EC818C4: mov      dword ptr [esp + 0xc], ecx
6EC818C8: mov      dword ptr [esp + 8], edx
6EC818CC: mov      edx, dword ptr [ebp - 0x10]
6EC818CF: mov      dword ptr [esp + 4], edx
6EC818D3: mov      dword ptr [esp], eax
6EC818D6: mov      eax, dword ptr [0x6ec89160]
6EC818DB: call     eax
6EC818DD: sub      esp, 0x14
6EC818E0: cmp      eax, 1
6EC818E3: jne      0x6ec818f3
6EC818E5: mov      eax, dword ptr [ebp - 0x18]
6EC818E8: test     eax, eax
6EC818EA: je       0x6ec818f3
6EC818EC: mov      eax, dword ptr [ebp - 0x18]
6EC818EF: mov      word ptr [ebp - 0xc], ax
6EC818F3: movzx    edx, word ptr [ebp - 0xa]
6EC818F7: movzx    eax, word ptr [ebp - 0xc]
6EC818FB: add      eax, edx
6EC818FD: mov      word ptr [ebp - 0xa], ax
6EC81901: movsx    eax, word ptr [ebp - 0xc]
6EC81905: add      dword ptr [ebp - 0x10], eax
6EC81908: cmp      word ptr [ebp - 0xc], 0
6EC8190D: jle      0x6ec8191b
6EC8190F: movsx    edx, word ptr [ebp - 0xa]
6EC81913: movzx    eax, word ptr [ebp - 0x1c]
6EC81917: cmp      edx, eax
6EC81919: jl       0x6ec818a9
6EC8191B: movsx    edx, word ptr [ebp - 0xa]
6EC8191F: movzx    eax, word ptr [ebp - 0x1c]
6EC81923: cmp      edx, eax
6EC81925: jge      0x6ec81943
6EC81927: movsx    edx, word ptr [ebp - 0xa]
6EC8192B: movzx    eax, word ptr [ebp - 0x1c]
6EC8192F: mov      dword ptr [esp + 8], edx
6EC81933: mov      dword ptr [esp + 4], eax
6EC81937: mov      dword ptr [esp], 0x6ec85064
6EC8193E: call     0x6ec83220
6EC81943: movzx    eax, word ptr [ebp - 0xa]
6EC81947: leave    
6EC81948: ret      
```

### Ordinal 22 — `mems_write_serial` — RVA `0x00001949`

```asm
6EC81949: push     ebp
6EC8194A: mov      ebp, esp
6EC8194C: sub      esp, 0x48
6EC8194F: mov      eax, dword ptr [ebp + 0x10]
6EC81952: mov      word ptr [ebp - 0x1c], ax
6EC81956: mov      word ptr [ebp - 0xa], 0xffff
6EC8195C: mov      dword ptr [ebp - 0x10], 0
6EC81963: mov      eax, dword ptr [ebp + 8]
6EC81966: mov      dword ptr [esp], eax
6EC81969: call     0x6ec81864 ; -> mems_is_connected
6EC8196E: test     al, al
6EC81970: je       0x6ec819c0
6EC81972: mov      dword ptr [ebp - 0x14], 0
6EC81979: movzx    edx, word ptr [ebp - 0x1c]
6EC8197D: mov      eax, dword ptr [ebp + 8]
6EC81980: mov      eax, dword ptr [eax]
6EC81982: mov      dword ptr [esp + 0x10], 0
6EC8198A: lea      ecx, [ebp - 0x14]
6EC8198D: mov      dword ptr [esp + 0xc], ecx
6EC81991: mov      dword ptr [esp + 8], edx
6EC81995: mov      edx, dword ptr [ebp + 0xc]
6EC81998: mov      dword ptr [esp + 4], edx
6EC8199C: mov      dword ptr [esp], eax
6EC8199F: mov      eax, dword ptr [0x6ec89190]
6EC819A4: call     eax
6EC819A6: sub      esp, 0x14
6EC819A9: cmp      eax, 1
6EC819AC: jne      0x6ec819c0
6EC819AE: movzx    edx, word ptr [ebp - 0x1c]
6EC819B2: mov      eax, dword ptr [ebp - 0x14]
6EC819B5: cmp      edx, eax
6EC819B7: jne      0x6ec819c0
6EC819B9: mov      eax, dword ptr [ebp - 0x14]
6EC819BC: mov      word ptr [ebp - 0xa], ax
6EC819C0: movzx    eax, word ptr [ebp - 0xa]
6EC819C4: leave    
6EC819C5: ret      
```

### Ordinal 19 — `mems_send_command` — RVA `0x000019C6`

```asm
6EC819C6: push     ebp
6EC819C7: mov      ebp, esp
6EC819C9: sub      esp, 0x38
6EC819CC: mov      eax, dword ptr [ebp + 0xc]
6EC819CF: mov      byte ptr [ebp - 0x1c], al
6EC819D2: mov      byte ptr [ebp - 9], 0
6EC819D6: mov      byte ptr [ebp - 0xa], 0xff
6EC819DA: mov      dword ptr [esp + 8], 1
6EC819E2: lea      eax, [ebp - 0x1c]
6EC819E5: mov      dword ptr [esp + 4], eax
6EC819E9: mov      eax, dword ptr [ebp + 8]
6EC819EC: mov      dword ptr [esp], eax
6EC819EF: call     0x6ec81949 ; -> mems_write_serial
6EC819F4: cmp      ax, 1
6EC819F8: jne      0x6ec81a69
6EC819FA: mov      dword ptr [esp + 8], 1
6EC81A02: lea      eax, [ebp - 0xa]
6EC81A05: mov      dword ptr [esp + 4], eax
6EC81A09: mov      eax, dword ptr [ebp + 8]
6EC81A0C: mov      dword ptr [esp], eax
6EC81A0F: call     0x6ec81874 ; -> mems_read_serial
6EC81A14: cmp      ax, 1
6EC81A18: jne      0x6ec81a50
6EC81A1A: movzx    edx, byte ptr [ebp - 0xa]
6EC81A1E: movzx    eax, byte ptr [ebp - 0x1c]
6EC81A22: cmp      dl, al
6EC81A24: jne      0x6ec81a2c
6EC81A26: mov      byte ptr [ebp - 9], 1
6EC81A2A: jmp      0x6ec81a80
6EC81A2C: movzx    eax, byte ptr [ebp - 0x1c]
6EC81A30: movzx    edx, al
6EC81A33: movzx    eax, byte ptr [ebp - 0xa]
6EC81A37: movzx    eax, al
6EC81A3A: mov      dword ptr [esp + 8], edx
6EC81A3E: mov      dword ptr [esp + 4], eax
6EC81A42: mov      dword ptr [esp], 0x6ec85090
6EC81A49: call     0x6ec83220
6EC81A4E: jmp      0x6ec81a80
6EC81A50: movzx    eax, byte ptr [ebp - 0x1c]
6EC81A54: movzx    eax, al
6EC81A57: mov      dword ptr [esp + 4], eax
6EC81A5B: mov      dword ptr [esp], 0x6ec850e8
6EC81A62: call     0x6ec83220
6EC81A67: jmp      0x6ec81a80
6EC81A69: movzx    eax, byte ptr [ebp - 0x1c]
6EC81A6D: movzx    eax, al
6EC81A70: mov      dword ptr [esp + 4], eax
6EC81A74: mov      dword ptr [esp], 0x6ec85124
6EC81A7B: call     0x6ec83220
6EC81A80: movzx    eax, byte ptr [ebp - 9]
6EC81A84: leave    
6EC81A85: ret      
```

### Ordinal 8 — `mems_init_link` — RVA `0x00001A86`

```asm
6EC81A86: push     ebp
6EC81A87: mov      ebp, esp
6EC81A89: sub      esp, 0x28
6EC81A8C: mov      byte ptr [ebp - 9], 0xca
6EC81A90: mov      byte ptr [ebp - 0xa], 0x75
6EC81A94: mov      byte ptr [ebp - 0xb], 0xf4
6EC81A98: mov      byte ptr [ebp - 0xc], 0xd0
6EC81A9C: mov      byte ptr [ebp - 0xd], 0
6EC81AA0: movzx    eax, byte ptr [ebp - 9]
6EC81AA4: mov      dword ptr [esp + 4], eax
6EC81AA8: mov      eax, dword ptr [ebp + 8]
6EC81AAB: mov      dword ptr [esp], eax
6EC81AAE: call     0x6ec819c6 ; -> mems_send_command
6EC81AB3: xor      eax, 1
6EC81AB6: test     al, al
6EC81AB8: je       0x6ec81ad8
6EC81ABA: movzx    eax, byte ptr [ebp - 9]
6EC81ABE: mov      dword ptr [esp + 4], eax
6EC81AC2: mov      dword ptr [esp], 0x6ec85158
6EC81AC9: call     0x6ec83220
6EC81ACE: mov      eax, 0
6EC81AD3: jmp      0x6ec81bf8
6EC81AD8: movzx    eax, byte ptr [ebp - 0xa]
6EC81ADC: mov      dword ptr [esp + 4], eax
6EC81AE0: mov      eax, dword ptr [ebp + 8]
6EC81AE3: mov      dword ptr [esp], eax
6EC81AE6: call     0x6ec819c6 ; -> mems_send_command
6EC81AEB: xor      eax, 1
6EC81AEE: test     al, al
6EC81AF0: je       0x6ec81b10
6EC81AF2: movzx    eax, byte ptr [ebp - 0xa]
6EC81AF6: mov      dword ptr [esp + 4], eax
6EC81AFA: mov      dword ptr [esp], 0x6ec85158
6EC81B01: call     0x6ec83220
6EC81B06: mov      eax, 0
6EC81B0B: jmp      0x6ec81bf8
6EC81B10: movzx    eax, byte ptr [ebp - 0xb]
6EC81B14: mov      dword ptr [esp + 4], eax
6EC81B18: mov      eax, dword ptr [ebp + 8]
6EC81B1B: mov      dword ptr [esp], eax
6EC81B1E: call     0x6ec819c6 ; -> mems_send_command
6EC81B23: xor      eax, 1
6EC81B26: test     al, al
6EC81B28: je       0x6ec81b48
6EC81B2A: movzx    eax, byte ptr [ebp - 0xb]
6EC81B2E: mov      dword ptr [esp + 4], eax
6EC81B32: mov      dword ptr [esp], 0x6ec85158
6EC81B39: call     0x6ec83220
6EC81B3E: mov      eax, 0
6EC81B43: jmp      0x6ec81bf8
6EC81B48: mov      dword ptr [esp + 8], 1
6EC81B50: lea      eax, [ebp - 0xd]
6EC81B53: mov      dword ptr [esp + 4], eax
6EC81B57: mov      eax, dword ptr [ebp + 8]
6EC81B5A: mov      dword ptr [esp], eax
6EC81B5D: call     0x6ec81874 ; -> mems_read_serial
6EC81B62: cmp      ax, 1
6EC81B66: je       0x6ec81b83
6EC81B68: movzx    eax, byte ptr [ebp - 0xb]
6EC81B6C: mov      dword ptr [esp + 4], eax
6EC81B70: mov      dword ptr [esp], 0x6ec8518c
6EC81B77: call     0x6ec83220
6EC81B7C: mov      eax, 0
6EC81B81: jmp      0x6ec81bf8
```

### Ordinal 10 — `mems_lock` — RVA `0x00001BFA`

```asm
6EC81BFA: push     ebp
6EC81BFB: mov      ebp, esp
6EC81BFD: sub      esp, 0x18
6EC81C00: mov      eax, dword ptr [ebp + 8]
6EC81C03: mov      eax, dword ptr [eax + 4]
6EC81C06: mov      dword ptr [esp + 4], 0xffffffff
6EC81C0E: mov      dword ptr [esp], eax
6EC81C11: mov      eax, dword ptr [0x6ec8918c]
6EC81C16: call     eax
6EC81C18: sub      esp, 8
6EC81C1B: test     eax, eax
6EC81C1D: je       0x6ec81c26
6EC81C1F: mov      eax, 0
6EC81C24: jmp      0x6ec81c2b
6EC81C26: mov      eax, 1
6EC81C2B: leave    
6EC81C2C: ret      
```

### Ordinal 21 — `mems_unlock` — RVA `0x00001C2D`

```asm
6EC81C2D: push     ebp
6EC81C2E: mov      ebp, esp
6EC81C30: sub      esp, 0x18
6EC81C33: mov      eax, dword ptr [ebp + 8]
6EC81C36: mov      eax, dword ptr [eax + 4]
6EC81C39: mov      dword ptr [esp], eax
6EC81C3C: mov      eax, dword ptr [0x6ec89164]
6EC81C41: call     eax
6EC81C43: sub      esp, 4
6EC81C46: nop      
6EC81C47: leave    
6EC81C48: ret      
```

### Ordinal 15 — `mems_read_raw` — RVA `0x00001C49`

```asm
6EC81C49: push     ebp
6EC81C4A: mov      ebp, esp
6EC81C4C: sub      esp, 0x28
6EC81C4F: mov      byte ptr [ebp - 9], 0
6EC81C53: mov      eax, dword ptr [ebp + 8]
6EC81C56: mov      dword ptr [esp], eax
6EC81C59: call     0x6ec81bfa ; -> mems_lock
6EC81C5E: test     al, al
6EC81C60: je       0x6ec81d11
6EC81C66: mov      dword ptr [esp + 4], 0x80
6EC81C6E: mov      eax, dword ptr [ebp + 8]
6EC81C71: mov      dword ptr [esp], eax
6EC81C74: call     0x6ec819c6 ; -> mems_send_command
6EC81C79: test     al, al
6EC81C7B: je       0x6ec81cb1
6EC81C7D: mov      dword ptr [esp + 8], 0x1c
6EC81C85: mov      eax, dword ptr [ebp + 0xc]
6EC81C88: mov      dword ptr [esp + 4], eax
6EC81C8C: mov      eax, dword ptr [ebp + 8]
6EC81C8F: mov      dword ptr [esp], eax
6EC81C92: call     0x6ec81874 ; -> mems_read_serial
6EC81C97: cmp      ax, 0x1c
6EC81C9B: jne      0x6ec81ca3
6EC81C9D: mov      byte ptr [ebp - 9], 1
6EC81CA1: jmp      0x6ec81cbd
6EC81CA3: mov      dword ptr [esp], 0x6ec8521c
6EC81CAA: call     0x6ec83218
6EC81CAF: jmp      0x6ec81cbd
6EC81CB1: mov      dword ptr [esp], 0x6ec85260
6EC81CB8: call     0x6ec83218
6EC81CBD: cmp      byte ptr [ebp - 9], 0
6EC81CC1: je       0x6ec81d06
6EC81CC3: mov      dword ptr [esp + 4], 0x7d
6EC81CCB: mov      eax, dword ptr [ebp + 8]
6EC81CCE: mov      dword ptr [esp], eax
6EC81CD1: call     0x6ec819c6 ; -> mems_send_command
6EC81CD6: test     al, al
6EC81CD8: je       0x6ec81cf6
6EC81CDA: mov      dword ptr [esp + 8], 0x20
6EC81CE2: mov      eax, dword ptr [ebp + 0x10]
6EC81CE5: mov      dword ptr [esp + 4], eax
6EC81CE9: mov      eax, dword ptr [ebp + 8]
6EC81CEC: mov      dword ptr [esp], eax
6EC81CEF: call     0x6ec81874 ; -> mems_read_serial
6EC81CF4: jmp      0x6ec81d06
6EC81CF6: mov      dword ptr [esp], 0x6ec85294
6EC81CFD: call     0x6ec83218
6EC81D02: mov      byte ptr [ebp - 9], 0
6EC81D06: mov      eax, dword ptr [ebp + 8]
6EC81D09: mov      dword ptr [esp], eax
6EC81D0C: call     0x6ec81c2d ; -> mems_unlock
6EC81D11: movzx    eax, byte ptr [ebp - 9]
6EC81D15: leave    
6EC81D16: ret      
```

### Ordinal 13 — `mems_read` — RVA `0x00001D17`

```asm
6EC81D17: push     ebp
6EC81D18: mov      ebp, esp
6EC81D1A: sub      esp, 0x58
6EC81D1D: mov      byte ptr [ebp - 9], 0
6EC81D21: lea      eax, [ebp - 0x45]
6EC81D24: mov      dword ptr [esp + 8], eax
6EC81D28: lea      eax, [ebp - 0x25]
6EC81D2B: mov      dword ptr [esp + 4], eax
6EC81D2F: mov      eax, dword ptr [ebp + 8]
6EC81D32: mov      dword ptr [esp], eax
6EC81D35: call     0x6ec81c49 ; -> mems_read_raw
6EC81D3A: test     al, al
6EC81D3C: je       0x6ec82043
6EC81D42: mov      dword ptr [esp + 8], 0x3c
6EC81D4A: mov      dword ptr [esp + 4], 0
6EC81D52: mov      eax, dword ptr [ebp + 0xc]
6EC81D55: mov      dword ptr [esp], eax
6EC81D58: call     0x6ec83228
6EC81D5D: movzx    eax, byte ptr [ebp - 0x24]
6EC81D61: movzx    eax, al
6EC81D64: shl      eax, 8
6EC81D67: mov      edx, eax
6EC81D69: movzx    eax, byte ptr [ebp - 0x23]
6EC81D6D: movzx    eax, al
6EC81D70: or       eax, edx
6EC81D72: mov      edx, eax
6EC81D74: mov      eax, dword ptr [ebp + 0xc]
6EC81D77: mov      word ptr [eax], dx
6EC81D7A: movzx    edx, byte ptr [ebp - 0x22]
6EC81D7E: mov      eax, dword ptr [ebp + 0xc]
6EC81D81: mov      byte ptr [eax + 2], dl
6EC81D84: movzx    edx, byte ptr [ebp - 0x21]
6EC81D88: mov      eax, dword ptr [ebp + 0xc]
6EC81D8B: mov      byte ptr [eax + 3], dl
6EC81D8E: movzx    edx, byte ptr [ebp - 0x20]
6EC81D92: mov      eax, dword ptr [ebp + 0xc]
6EC81D95: mov      byte ptr [eax + 4], dl
6EC81D98: movzx    edx, byte ptr [ebp - 0x1f]
6EC81D9C: mov      eax, dword ptr [ebp + 0xc]
6EC81D9F: mov      byte ptr [eax + 5], dl
6EC81DA2: movzx    edx, byte ptr [ebp - 0x1e]
6EC81DA6: mov      eax, dword ptr [ebp + 0xc]
6EC81DA9: mov      byte ptr [eax + 6], dl
6EC81DAC: movzx    edx, byte ptr [ebp - 0x1d]
6EC81DB0: mov      eax, dword ptr [ebp + 0xc]
6EC81DB3: mov      byte ptr [eax + 7], dl
6EC81DB6: movzx    edx, byte ptr [ebp - 0x1c]
6EC81DBA: mov      eax, dword ptr [ebp + 0xc]
6EC81DBD: mov      byte ptr [eax + 8], dl
6EC81DC0: movzx    edx, byte ptr [ebp - 0x1b]
6EC81DC4: mov      eax, dword ptr [ebp + 0xc]
6EC81DC7: mov      byte ptr [eax + 9], dl
6EC81DCA: movzx    edx, byte ptr [ebp - 0x1a]
6EC81DCE: mov      eax, dword ptr [ebp + 0xc]
6EC81DD1: mov      byte ptr [eax + 0xa], dl
6EC81DD4: movzx    edx, byte ptr [ebp - 0x19]
6EC81DD8: mov      eax, dword ptr [ebp + 0xc]
6EC81DDB: mov      byte ptr [eax + 0xb], dl
6EC81DDE: mov      eax, dword ptr [ebp + 0xc]
6EC81DE1: mov      byte ptr [eax + 0xc], 0
6EC81DE5: movzx    edx, byte ptr [ebp - 0x16]
6EC81DE9: mov      eax, dword ptr [ebp + 0xc]
6EC81DEC: mov      byte ptr [eax + 0xd], dl
6EC81DEF: movzx    edx, byte ptr [ebp - 0x15]
6EC81DF3: mov      eax, dword ptr [ebp + 0xc]
6EC81DF6: mov      byte ptr [eax + 0xe], dl
6EC81DF9: movzx    edx, byte ptr [ebp - 0x14]
6EC81DFD: mov      eax, dword ptr [ebp + 0xc]
6EC81E00: mov      byte ptr [eax + 0xf], dl
6EC81E03: movzx    edx, byte ptr [ebp - 0x13]
6EC81E07: mov      eax, dword ptr [ebp + 0xc]
6EC81E0A: mov      byte ptr [eax + 0x10], dl
6EC81E0D: movzx    eax, byte ptr [ebp - 0x12]
6EC81E11: movzx    eax, al
6EC81E14: shl      eax, 8
```

### Ordinal 14 — `mems_read_iac_position` — RVA `0x00002049`

```asm
6EC82049: push     ebp
6EC8204A: mov      ebp, esp
6EC8204C: sub      esp, 0x28
6EC8204F: mov      byte ptr [ebp - 9], 0
6EC82053: mov      eax, dword ptr [ebp + 8]
6EC82056: mov      dword ptr [esp], eax
6EC82059: call     0x6ec81bfa ; -> mems_lock
6EC8205E: test     al, al
6EC82060: je       0x6ec820b7
6EC82062: mov      dword ptr [esp + 4], 0xfb
6EC8206A: mov      eax, dword ptr [ebp + 8]
6EC8206D: mov      dword ptr [esp], eax
6EC82070: call     0x6ec819c6 ; -> mems_send_command
6EC82075: test     al, al
6EC82077: je       0x6ec820a0
6EC82079: mov      dword ptr [esp + 8], 1
6EC82081: mov      eax, dword ptr [ebp + 0xc]
6EC82084: mov      dword ptr [esp + 4], eax
6EC82088: mov      eax, dword ptr [ebp + 8]
6EC8208B: mov      dword ptr [esp], eax
6EC8208E: call     0x6ec81874 ; -> mems_read_serial
6EC82093: cmp      ax, 1
6EC82097: jne      0x6ec820a0
6EC82099: mov      eax, 1
6EC8209E: jmp      0x6ec820a5
6EC820A0: mov      eax, 0
6EC820A5: mov      byte ptr [ebp - 9], al
6EC820A8: and      byte ptr [ebp - 9], 1
6EC820AC: mov      eax, dword ptr [ebp + 8]
6EC820AF: mov      dword ptr [esp], eax
6EC820B2: call     0x6ec81c2d ; -> mems_unlock
6EC820B7: movzx    eax, byte ptr [ebp - 9]
6EC820BB: leave    
6EC820BC: ret      
```

### Ordinal 11 — `mems_move_iac` — RVA `0x000020BD`

```asm
6EC820BD: push     ebp
6EC820BE: mov      ebp, esp
6EC820C0: sub      esp, 0x38
6EC820C3: mov      eax, dword ptr [ebp + 0xc]
6EC820C6: mov      byte ptr [ebp - 0x1c], al
6EC820C9: mov      byte ptr [ebp - 0xb], 0
6EC820CD: mov      word ptr [ebp - 0xa], 0
6EC820D3: mov      byte ptr [ebp - 0x11], 0
6EC820D7: lea      eax, [ebp - 0x11]
6EC820DA: mov      dword ptr [esp + 4], eax
6EC820DE: mov      eax, dword ptr [ebp + 8]
6EC820E1: mov      dword ptr [esp], eax
6EC820E4: call     0x6ec82049 ; -> mems_read_iac_position
6EC820E9: test     al, al
6EC820EB: je       0x6ec82157
6EC820ED: movzx    eax, byte ptr [ebp - 0x11]
6EC820F1: cmp      byte ptr [ebp - 0x1c], al
6EC820F4: jb       0x6ec82107
6EC820F6: movzx    eax, byte ptr [ebp - 0x11]
6EC820FA: cmp      byte ptr [ebp - 0x1c], al
6EC820FD: jbe      0x6ec82157
6EC820FF: movzx    eax, byte ptr [ebp - 0x11]
6EC82103: cmp      al, 0xb3
6EC82105: ja       0x6ec82157
6EC82107: movzx    eax, byte ptr [ebp - 0x11]
6EC8210B: cmp      byte ptr [ebp - 0x1c], al
6EC8210E: jbe      0x6ec82117
6EC82110: mov      eax, 0xfd
6EC82115: jmp      0x6ec8211c
6EC82117: mov      eax, 0xfe
6EC8211C: mov      dword ptr [ebp - 0x10], eax
6EC8211F: lea      eax, [ebp - 0x11]
6EC82122: mov      dword ptr [esp + 8], eax
6EC82126: mov      eax, dword ptr [ebp - 0x10]
6EC82129: mov      dword ptr [esp + 4], eax
6EC8212D: mov      eax, dword ptr [ebp + 8]
6EC82130: mov      dword ptr [esp], eax
6EC82133: call     0x6ec8216a ; -> mems_test_actuator
6EC82138: mov      byte ptr [ebp - 0xb], al
6EC8213B: add      word ptr [ebp - 0xa], 1
6EC82140: cmp      byte ptr [ebp - 0xb], 0
6EC82144: je       0x6ec82157
6EC82146: movzx    eax, byte ptr [ebp - 0x11]
6EC8214A: cmp      al, byte ptr [ebp - 0x1c]
6EC8214D: je       0x6ec82157
6EC8214F: cmp      word ptr [ebp - 0xa], 0x12b
6EC82155: jbe      0x6ec8211f
6EC82157: movzx    eax, byte ptr [ebp - 0x11]
6EC8215B: cmp      byte ptr [ebp - 0x1c], al
6EC8215E: sete     al
6EC82161: mov      byte ptr [ebp - 0xb], al
6EC82164: movzx    eax, byte ptr [ebp - 0xb]
6EC82168: leave    
6EC82169: ret      
```

### Ordinal 20 — `mems_test_actuator` — RVA `0x0000216A`

```asm
6EC8216A: push     ebp
6EC8216B: mov      ebp, esp
6EC8216D: sub      esp, 0x28
6EC82170: mov      byte ptr [ebp - 9], 0
6EC82174: mov      byte ptr [ebp - 0xa], 0
6EC82178: mov      eax, dword ptr [ebp + 8]
6EC8217B: mov      dword ptr [esp], eax
6EC8217E: call     0x6ec81bfa ; -> mems_lock
6EC82183: test     al, al
6EC82185: je       0x6ec821de
6EC82187: mov      eax, dword ptr [ebp + 0xc]
6EC8218A: movzx    eax, al
6EC8218D: mov      dword ptr [esp + 4], eax
6EC82191: mov      eax, dword ptr [ebp + 8]
6EC82194: mov      dword ptr [esp], eax
6EC82197: call     0x6ec819c6 ; -> mems_send_command
6EC8219C: test     al, al
6EC8219E: je       0x6ec821d3
6EC821A0: mov      dword ptr [esp + 8], 1
6EC821A8: lea      eax, [ebp - 0xa]
6EC821AB: mov      dword ptr [esp + 4], eax
6EC821AF: mov      eax, dword ptr [ebp + 8]
6EC821B2: mov      dword ptr [esp], eax
6EC821B5: call     0x6ec81874 ; -> mems_read_serial
6EC821BA: cmp      ax, 1
6EC821BE: jne      0x6ec821d3
6EC821C0: cmp      dword ptr [ebp + 0x10], 0
6EC821C4: je       0x6ec821cf
6EC821C6: movzx    edx, byte ptr [ebp - 0xa]
6EC821CA: mov      eax, dword ptr [ebp + 0x10]
6EC821CD: mov      byte ptr [eax], dl
6EC821CF: mov      byte ptr [ebp - 9], 1
6EC821D3: mov      eax, dword ptr [ebp + 8]
6EC821D6: mov      dword ptr [esp], eax
6EC821D9: call     0x6ec81c2d ; -> mems_unlock
6EC821DE: movzx    eax, byte ptr [ebp - 9]
6EC821E2: leave    
6EC821E3: ret      
```

### Ordinal 2 — `mems_clear_faults` — RVA `0x000021E4`

```asm
6EC821E4: push     ebp
6EC821E5: mov      ebp, esp
6EC821E7: sub      esp, 0x28
6EC821EA: mov      byte ptr [ebp - 9], 0
6EC821EE: mov      byte ptr [ebp - 0xa], 0xff
6EC821F2: mov      eax, dword ptr [ebp + 8]
6EC821F5: mov      dword ptr [esp], eax
6EC821F8: call     0x6ec81bfa ; -> mems_lock
6EC821FD: test     al, al
6EC821FF: je       0x6ec8225a
6EC82201: mov      dword ptr [esp + 4], 0xcc
6EC82209: mov      eax, dword ptr [ebp + 8]
6EC8220C: mov      dword ptr [esp], eax
6EC8220F: call     0x6ec819c6 ; -> mems_send_command
6EC82214: test     al, al
6EC82216: je       0x6ec8223f
6EC82218: mov      dword ptr [esp + 8], 1
6EC82220: lea      eax, [ebp - 0xa]
6EC82223: mov      dword ptr [esp + 4], eax
6EC82227: mov      eax, dword ptr [ebp + 8]
6EC8222A: mov      dword ptr [esp], eax
6EC8222D: call     0x6ec81874 ; -> mems_read_serial
6EC82232: cmp      ax, 1
6EC82236: jne      0x6ec8223f
6EC82238: mov      eax, 1
6EC8223D: jmp      0x6ec82244
6EC8223F: mov      eax, 0
6EC82244: mov      byte ptr [ebp - 9], al
6EC82247: and      byte ptr [ebp - 9], 1
6EC8224B: mov      byte ptr [ebp - 9], 1
6EC8224F: mov      eax, dword ptr [ebp + 8]
6EC82252: mov      dword ptr [esp], eax
6EC82255: call     0x6ec81c2d ; -> mems_unlock
6EC8225A: movzx    eax, byte ptr [ebp - 9]
6EC8225E: leave    
6EC8225F: ret      
```

### Ordinal 17 — `mems_reset_ECU` — RVA `0x00002260`

```asm
6EC82260: push     ebp
6EC82261: mov      ebp, esp
6EC82263: sub      esp, 0x28
6EC82266: mov      byte ptr [ebp - 9], 0
6EC8226A: mov      byte ptr [ebp - 0xa], 0xff
6EC8226E: mov      eax, dword ptr [ebp + 8]
6EC82271: mov      dword ptr [esp], eax
6EC82274: call     0x6ec81bfa ; -> mems_lock
6EC82279: test     al, al
6EC8227B: je       0x6ec822d6
6EC8227D: mov      dword ptr [esp + 4], 0xfa
6EC82285: mov      eax, dword ptr [ebp + 8]
6EC82288: mov      dword ptr [esp], eax
6EC8228B: call     0x6ec819c6 ; -> mems_send_command
6EC82290: test     al, al
6EC82292: je       0x6ec822bb
6EC82294: mov      dword ptr [esp + 8], 1
6EC8229C: lea      eax, [ebp - 0xa]
6EC8229F: mov      dword ptr [esp + 4], eax
6EC822A3: mov      eax, dword ptr [ebp + 8]
6EC822A6: mov      dword ptr [esp], eax
6EC822A9: call     0x6ec81874 ; -> mems_read_serial
6EC822AE: cmp      ax, 1
6EC822B2: jne      0x6ec822bb
6EC822B4: mov      eax, 1
6EC822B9: jmp      0x6ec822c0
6EC822BB: mov      eax, 0
6EC822C0: mov      byte ptr [ebp - 9], al
6EC822C3: and      byte ptr [ebp - 9], 1
6EC822C7: mov      byte ptr [ebp - 9], 1
6EC822CB: mov      eax, dword ptr [ebp + 8]
6EC822CE: mov      dword ptr [esp], eax
6EC822D1: call     0x6ec81c2d ; -> mems_unlock
6EC822D6: movzx    eax, byte ptr [ebp - 9]
6EC822DA: leave    
6EC822DB: ret      
```

### Ordinal 18 — `mems_reset_adjustments` — RVA `0x000022DC`

```asm
6EC822DC: push     ebp
6EC822DD: mov      ebp, esp
6EC822DF: sub      esp, 0x28
6EC822E2: mov      byte ptr [ebp - 9], 0
6EC822E6: mov      byte ptr [ebp - 0xa], 0xff
6EC822EA: mov      eax, dword ptr [ebp + 8]
6EC822ED: mov      dword ptr [esp], eax
6EC822F0: call     0x6ec81bfa ; -> mems_lock
6EC822F5: test     al, al
6EC822F7: je       0x6ec82352
6EC822F9: mov      dword ptr [esp + 4], 0xf
6EC82301: mov      eax, dword ptr [ebp + 8]
6EC82304: mov      dword ptr [esp], eax
6EC82307: call     0x6ec819c6 ; -> mems_send_command
6EC8230C: test     al, al
6EC8230E: je       0x6ec82337
6EC82310: mov      dword ptr [esp + 8], 1
6EC82318: lea      eax, [ebp - 0xa]
6EC8231B: mov      dword ptr [esp + 4], eax
6EC8231F: mov      eax, dword ptr [ebp + 8]
6EC82322: mov      dword ptr [esp], eax
6EC82325: call     0x6ec81874 ; -> mems_read_serial
6EC8232A: cmp      ax, 1
6EC8232E: jne      0x6ec82337
6EC82330: mov      eax, 1
6EC82335: jmp      0x6ec8233c
6EC82337: mov      eax, 0
6EC8233C: mov      byte ptr [ebp - 9], al
6EC8233F: and      byte ptr [ebp - 9], 1
6EC82343: mov      byte ptr [ebp - 9], 1
6EC82347: mov      eax, dword ptr [ebp + 8]
6EC8234A: mov      dword ptr [esp], eax
6EC8234D: call     0x6ec81c2d ; -> mems_unlock
6EC82352: movzx    eax, byte ptr [ebp - 9]
6EC82356: leave    
6EC82357: ret      
```

### Ordinal 6 — `mems_heartbeat` — RVA `0x00002358`

```asm
6EC82358: push     ebp
6EC82359: mov      ebp, esp
6EC8235B: sub      esp, 0x28
6EC8235E: mov      byte ptr [ebp - 9], 0
6EC82362: mov      byte ptr [ebp - 0xa], 0xff
6EC82366: mov      eax, dword ptr [ebp + 8]
6EC82369: mov      dword ptr [esp], eax
6EC8236C: call     0x6ec81bfa ; -> mems_lock
6EC82371: test     al, al
6EC82373: je       0x6ec823bb
6EC82375: mov      dword ptr [esp + 4], 0xf4
6EC8237D: mov      eax, dword ptr [ebp + 8]
6EC82380: mov      dword ptr [esp], eax
6EC82383: call     0x6ec819c6 ; -> mems_send_command
6EC82388: test     al, al
6EC8238A: je       0x6ec823b0
6EC8238C: mov      dword ptr [esp + 8], 1
6EC82394: lea      eax, [ebp - 0xa]
6EC82397: mov      dword ptr [esp + 4], eax
6EC8239B: mov      eax, dword ptr [ebp + 8]
6EC8239E: mov      dword ptr [esp], eax
6EC823A1: call     0x6ec81874 ; -> mems_read_serial
6EC823A6: cmp      ax, 1
6EC823AA: jne      0x6ec823b0
6EC823AC: mov      byte ptr [ebp - 9], 1
6EC823B0: mov      eax, dword ptr [ebp + 8]
6EC823B3: mov      dword ptr [esp], eax
6EC823B6: call     0x6ec81c2d ; -> mems_unlock
6EC823BB: movzx    eax, byte ptr [ebp - 9]
6EC823BF: leave    
6EC823C0: ret      
```

