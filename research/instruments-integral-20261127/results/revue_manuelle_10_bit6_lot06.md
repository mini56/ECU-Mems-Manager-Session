# MEMS 1.6 NA — revue manuelle des dix pistes bit 6 du lot 05

Les 10 ASM ont été lus intégralement via leurs blobs GitHub. L'examen ci-dessous sépare fusion de port, producteurs candidats (y compris octet voisin A3), et balayage de protection. Une adresse de sortie seule ne prouve pas un test de jauge. Aucun accès ECU.

## KBP8F064
Blob ASM : `4289a24b9536dd8950de86bc1d8b9cd465347629`

Producteurs candidats :
```asm
ROM:272E                 ldb     INTMEM_A3, 0A5h[INTMEM_56]
ROM:27B1                 ldb     INTMEM_A3, 0A5h[INTMEM_56]
ROM:27C9                 clrb    INTMEM_A3
```
Fusion et balayage :
```asm
ROM:28D0                 ldb     INTMEM_1A, #-60h
ROM:28D3                 orb     INTMEM_1A, INTMEM_A6
ROM:28DB                 stb     INTMEM_1A, 401h[]
ROM:28E9                 shlb    INTMEM_A6, #1
ROM:28EC                 cmpb    INTMEM_A6, #40h ; '@'
ROM:28F6                 orb     INTMEM_54, INTMEM_A6
ROM:28F9                 orb     INTMEM_50, INTMEM_A6
ROM:28FC                 ldb     INTMEM_1C, INTMEM_54
ROM:28FF                 ldb     INTMEM_1C+1, INTMEM_15D[]
ROM:2904                 ldb     INTMEM_1C+2, INTMEM_164[]
ROM:2909                 notb    INTMEM_1C+1
ROM:290B                 andb    INTMEM_1C+1, INTMEM_A4
ROM:290E                 orb     INTMEM_1C+1, INTMEM_15E[]
ROM:2913                 andb    INTMEM_1C+2, #-60h
ROM:2916                 notb    INTMEM_1C
ROM:2918                 andb    INTMEM_1C+1, INTMEM_1C
ROM:291B                 orb     INTMEM_1C+2, INTMEM_1C+1
ROM:291E                 stb     INTMEM_1C+2, 401h[]
ROM:2923                 stb     INTMEM_1C+2, INTMEM_164[]
```

## KBP8F065
Blob ASM : `28f2d38bbcfa9ea19826241de965bffd4d1eeb87`

Producteurs candidats :
```asm
ROM:24F9                 ldb     INTMEM_A3, INTMEM_1C
```
Fusion et balayage :
```asm
ROM:26CE                 ldb     INTMEM_1A, #-60h
ROM:26D1                 orb     INTMEM_1A, INTMEM_A6
ROM:26D9                 stb     INTMEM_1A, 401h[]
ROM:26E7                 shlb    INTMEM_A6, #1
ROM:26EA                 cmpb    INTMEM_A6, #40h ; '@'
ROM:26F4                 orb     INTMEM_54, INTMEM_A6
ROM:26F7                 orb     INTMEM_50, INTMEM_A6
ROM:26FA                 ldb     INTMEM_1C, INTMEM_54
ROM:26FD                 ldb     INTMEM_1C+1, INTMEM_152[]
ROM:2702                 ldb     INTMEM_1C+2, INTMEM_157[]
ROM:2707                 notb    INTMEM_1C+1
ROM:2709                 andb    INTMEM_1C+1, INTMEM_A4
ROM:270C                 orb     INTMEM_1C+1, INTMEM_153[]
ROM:2711                 andb    INTMEM_1C+2, #-60h
ROM:2714                 notb    INTMEM_1C
ROM:2716                 andb    INTMEM_1C+1, INTMEM_1C
ROM:2719                 orb     INTMEM_1C+2, INTMEM_1C+1
ROM:271C                 stb     INTMEM_1C+2, 401h[]
ROM:2721                 stb     INTMEM_1C+2, INTMEM_157[]
```

## KBP8F066
Blob ASM : `aefe7ede57dc2876ac8e0b223cd4c33101e4223c`

Producteurs candidats :
```asm
ROM:24F9                 ldb     INTMEM_A3, INTMEM_1C
```
Fusion et balayage :
```asm
ROM:26CE                 ldb     INTMEM_1A, #-60h
ROM:26D1                 orb     INTMEM_1A, INTMEM_A6
ROM:26D9                 stb     INTMEM_1A, 401h[]
ROM:26E7                 shlb    INTMEM_A6, #1
ROM:26EA                 cmpb    INTMEM_A6, #40h ; '@'
ROM:26F4                 orb     INTMEM_54, INTMEM_A6
ROM:26F7                 orb     INTMEM_50, INTMEM_A6
ROM:26FA                 ldb     INTMEM_1C, INTMEM_54
ROM:26FD                 ldb     INTMEM_1C+1, INTMEM_152[]
ROM:2702                 ldb     INTMEM_1C+2, INTMEM_157[]
ROM:2707                 notb    INTMEM_1C+1
ROM:2709                 andb    INTMEM_1C+1, INTMEM_A4
ROM:270C                 orb     INTMEM_1C+1, INTMEM_153[]
ROM:2711                 andb    INTMEM_1C+2, #-60h
ROM:2714                 notb    INTMEM_1C
ROM:2716                 andb    INTMEM_1C+1, INTMEM_1C
ROM:2719                 orb     INTMEM_1C+2, INTMEM_1C+1
ROM:271C                 stb     INTMEM_1C+2, 401h[]
ROM:2721                 stb     INTMEM_1C+2, INTMEM_157[]
```

## KCE6F018
Blob ASM : `4543313310b9761b2b811bd6a3f4e11670ec5ab0`

Producteurs candidats :
```asm
Aucun selon les opérandes directs contrôlés
```
Fusion et balayage :
```asm
ROM:2873                 ldb     INTMEM_1A, #-60h
ROM:2876                 orb     INTMEM_1A, INTMEM_A6
ROM:287E                 stb     INTMEM_1A, 401h[]
ROM:288C                 shlb    INTMEM_A6, #1
ROM:288F                 cmpb    INTMEM_A6, #40h ; '@'
ROM:2899                 orb     INTMEM_54, INTMEM_A6
ROM:289C                 orb     INTMEM_50, INTMEM_A6
ROM:289F                 ldb     INTMEM_1C, INTMEM_54
ROM:28A2                 ldb     INTMEM_1C+1, INTMEM_15D[]
ROM:28A7                 ldb     INTMEM_1C+2, INTMEM_165[]
ROM:28AC                 notb    INTMEM_1C+1
ROM:28AE                 andb    INTMEM_1C+1, INTMEM_A4
ROM:28B1                 orb     INTMEM_1C+1, INTMEM_15E[]
ROM:28B6                 andb    INTMEM_1C+2, #-60h
ROM:28B9                 notb    INTMEM_1C
ROM:28BB                 andb    INTMEM_1C+1, INTMEM_1C
ROM:28BE                 orb     INTMEM_1C+2, INTMEM_1C+1
ROM:28C1                 stb     INTMEM_1C+2, 401h[]
ROM:28C6                 stb     INTMEM_1C+2, INTMEM_165[]
```

## KCE8F007
Blob ASM : `9ae6305b0d9f20e7b9bd843cc19cb771a4e36323`

Producteurs candidats :
```asm
ROM:25E4                 ldb     INTMEM_A3, INTMEM_1C
```
Fusion et balayage :
```asm
ROM:2873                 ldb     INTMEM_1A, #-60h
ROM:2876                 orb     INTMEM_1A, INTMEM_A6
ROM:287E                 stb     INTMEM_1A, 401h[]
ROM:288C                 shlb    INTMEM_A6, #1
ROM:288F                 cmpb    INTMEM_A6, #40h ; '@'
ROM:2899                 orb     INTMEM_54, INTMEM_A6
ROM:289C                 orb     INTMEM_4F+1, INTMEM_A6
ROM:289F                 ldb     INTMEM_1C, INTMEM_54
ROM:28A2                 ldb     INTMEM_1C+1, INTMEM_15D[]
ROM:28A7                 ldb     INTMEM_1C+2, INTMEM_165[]
ROM:28AC                 notb    INTMEM_1C+1
ROM:28AE                 andb    INTMEM_1C+1, INTMEM_A4
ROM:28B1                 orb     INTMEM_1C+1, INTMEM_15E[]
ROM:28B6                 andb    INTMEM_1C+2, #-60h
ROM:28B9                 notb    INTMEM_1C
ROM:28BB                 andb    INTMEM_1C+1, INTMEM_1C
ROM:28BE                 orb     INTMEM_1C+2, INTMEM_1C+1
ROM:28C1                 stb     INTMEM_1C+2, 401h[]
ROM:28C6                 stb     INTMEM_1C+2, INTMEM_165[]
```

## KCE8F009
Blob ASM : `98db0763d713049f3f1306c50ce9f028b090a505`

Producteurs candidats :
```asm
ROM:25E4                 ldb     INTMEM_A3, INTMEM_1C
```
Fusion et balayage :
```asm
ROM:2873                 ldb     INTMEM_1A, #-60h
ROM:2876                 orb     INTMEM_1A, INTMEM_A6
ROM:287E                 stb     INTMEM_1A, 401h[]
ROM:288C                 shlb    INTMEM_A6, #1
ROM:288F                 cmpb    INTMEM_A6, #40h ; '@'
ROM:2899                 orb     INTMEM_54, INTMEM_A6
ROM:289C                 orb     INTMEM_4F+1, INTMEM_A6
ROM:289F                 ldb     INTMEM_1C, INTMEM_54
ROM:28A2                 ldb     INTMEM_1C+1, INTMEM_15D[]
ROM:28A7                 ldb     INTMEM_1C+2, INTMEM_165[]
ROM:28AC                 notb    INTMEM_1C+1
ROM:28AE                 andb    INTMEM_1C+1, INTMEM_A4
ROM:28B1                 orb     INTMEM_1C+1, INTMEM_15E[]
ROM:28B6                 andb    INTMEM_1C+2, #-60h
ROM:28B9                 notb    INTMEM_1C
ROM:28BB                 andb    INTMEM_1C+1, INTMEM_1C
ROM:28BE                 orb     INTMEM_1C+2, INTMEM_1C+1
ROM:28C1                 stb     INTMEM_1C+2, 401h[]
ROM:28C6                 stb     INTMEM_1C+2, INTMEM_165[]
```

## KDE6F022
Blob ASM : `3781ef97222a0705d11f819d1f3f4e248015ebbf`

Producteurs candidats :
```asm
ROM:24DF                 ldb     INTMEM_A3, INTMEM_87
```
Fusion et balayage :
```asm
ROM:29E4                 ldb     INTMEM_1A, #-60h
ROM:29E7                 orb     INTMEM_1A, INTMEM_A6
ROM:29EF                 stb     INTMEM_1A, 401h[]
ROM:29FD                 shlb    INTMEM_A6, #1
ROM:2A00                 cmpb    INTMEM_A6, #40h ; '@'
ROM:2A0A                 orb     INTMEM_54, INTMEM_A6
ROM:2A0D                 orb     INTMEM_50, INTMEM_A6
ROM:2A10                 ldb     INTMEM_1C, INTMEM_54
ROM:2A13                 ldb     INTMEM_1C+1, INTMEM_163[]
ROM:2A18                 ldb     INTMEM_1C+2, INTMEM_16E[]
ROM:2A1D                 notb    INTMEM_1C+1
ROM:2A1F                 andb    INTMEM_1C+1, INTMEM_A4
ROM:2A22                 orb     INTMEM_1C+1, INTMEM_164[]
ROM:2A27                 andb    INTMEM_1C+2, #-60h
ROM:2A2A                 notb    INTMEM_1C
ROM:2A2C                 andb    INTMEM_1C+1, INTMEM_1C
ROM:2A2F                 orb     INTMEM_1C+2, INTMEM_1C+1
ROM:2A32                 stb     INTMEM_1C+2, 401h[]
ROM:2A37                 stb     INTMEM_1C+2, INTMEM_16E[]
```

## KGM6F004
Blob ASM : `5263d40e57f97de4fe9ae4150b4a91a30fb634e3`

Producteurs candidats :
```asm
ROM:25E4                 ldb     INTMEM_A3, INTMEM_1C
```
Fusion et balayage :
```asm
ROM:2873                 ldb     INTMEM_1A, #-60h
ROM:2876                 orb     INTMEM_1A, INTMEM_A6
ROM:287E                 stb     INTMEM_1A, 401h[]
ROM:288C                 shlb    INTMEM_A6, #1
ROM:288F                 cmpb    INTMEM_A6, #40h ; '@'
ROM:2899                 orb     INTMEM_54, INTMEM_A6
ROM:289C                 orb     INTMEM_50, INTMEM_A6
ROM:289F                 ldb     INTMEM_1C, INTMEM_54
ROM:28A2                 ldb     INTMEM_1C+1, INTMEM_15D[]
ROM:28A7                 ldb     INTMEM_1C+2, INTMEM_165[]
ROM:28AC                 notb    INTMEM_1C+1
ROM:28AE                 andb    INTMEM_1C+1, INTMEM_A4
ROM:28B1                 orb     INTMEM_1C+1, INTMEM_15E[]
ROM:28B6                 andb    INTMEM_1C+2, #-60h
ROM:28B9                 notb    INTMEM_1C
ROM:28BB                 andb    INTMEM_1C+1, INTMEM_1C
ROM:28BE                 orb     INTMEM_1C+2, INTMEM_1C+1
ROM:28C1                 stb     INTMEM_1C+2, 401h[]
ROM:28C6                 stb     INTMEM_1C+2, INTMEM_165[]
```

## KGM6F007
Blob ASM : `0a3f7fdd93f23e6ee56fe8bb4c2ac158e666d387`

Producteurs candidats :
```asm
ROM:25E4                 ldb     INTMEM_A3, INTMEM_1C
```
Fusion et balayage :
```asm
ROM:2881                 ldb     INTMEM_1A, #-60h
ROM:2884                 orb     INTMEM_1A, INTMEM_A6
ROM:288C                 stb     INTMEM_1A, 401h[]
ROM:289A                 shlb    INTMEM_A6, #1
ROM:289D                 cmpb    INTMEM_A6, #40h ; '@'
ROM:28A7                 orb     INTMEM_54, INTMEM_A6
ROM:28AA                 orb     INTMEM_50, INTMEM_A6
ROM:28AD                 ldb     INTMEM_1C, INTMEM_54
ROM:28B0                 ldb     INTMEM_1C+1, INTMEM_15D[]
ROM:28B5                 ldb     INTMEM_1C+2, INTMEM_165[]
ROM:28BA                 notb    INTMEM_1C+1
ROM:28BC                 andb    INTMEM_1C+1, INTMEM_A4
ROM:28BF                 orb     INTMEM_1C+1, INTMEM_15E[]
ROM:28C4                 andb    INTMEM_1C+2, #-60h
ROM:28C7                 notb    INTMEM_1C
ROM:28C9                 andb    INTMEM_1C+1, INTMEM_1C
ROM:28CC                 orb     INTMEM_1C+2, INTMEM_1C+1
ROM:28CF                 stb     INTMEM_1C+2, 401h[]
ROM:28D4                 stb     INTMEM_1C+2, INTMEM_165[]
```

## KGM6F008
Blob ASM : `aa746cfb0aed144c2c2b297aa93bf13d2d0b328c`

Producteurs candidats :
```asm
ROM:25E4                 ldb     INTMEM_A3, INTMEM_1C
```
Fusion et balayage :
```asm
ROM:2881                 ldb     INTMEM_1A, #-60h
ROM:2884                 orb     INTMEM_1A, INTMEM_A6
ROM:288C                 stb     INTMEM_1A, 401h[]
ROM:289A                 shlb    INTMEM_A6, #1
ROM:289D                 cmpb    INTMEM_A6, #40h ; '@'
ROM:28A7                 orb     INTMEM_54, INTMEM_A6
ROM:28AA                 orb     INTMEM_50, INTMEM_A6
ROM:28AD                 ldb     INTMEM_1C, INTMEM_54
ROM:28B0                 ldb     INTMEM_1C+1, INTMEM_15D[]
ROM:28B5                 ldb     INTMEM_1C+2, INTMEM_165[]
ROM:28BA                 notb    INTMEM_1C+1
ROM:28BC                 andb    INTMEM_1C+1, INTMEM_A4
ROM:28BF                 orb     INTMEM_1C+1, INTMEM_15E[]
ROM:28C4                 andb    INTMEM_1C+2, #-60h
ROM:28C7                 notb    INTMEM_1C
ROM:28C9                 andb    INTMEM_1C+1, INTMEM_1C
ROM:28CC                 orb     INTMEM_1C+2, INTMEM_1C+1
ROM:28CF                 stb     INTMEM_1C+2, 401h[]
ROM:28D4                 stb     INTMEM_1C+2, INTMEM_165[]
```
