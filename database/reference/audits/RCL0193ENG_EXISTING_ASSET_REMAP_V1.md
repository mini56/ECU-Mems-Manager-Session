# RCL0193ENG EXISTING ASSET REMAP V1

- Exact source file: `rave/xn/wmxn990e.pdf`
- Physical pages: **372**
- Existing BUILD #103 English RCL0193 visual assets compared against all source pages by normalized low-resolution page-image similarity.
- Result: all 26 existing assets map unambiguously to the uploaded source and their filename token is consistently **physical page - 1**.

| Existing asset | Filename token | Exact physical source page | Best MSE | Next-best MSE |
|---|---:|---:|---:|---:|
| `RCL0193ENG_PDF_038.png` | 38 | 39 | 0.000551 | 0.000835 |
| `RCL0193ENG_PDF_039.png` | 39 | 40 | 0.001015 | 0.014092 |
| `RCL0193ENG_PDF_040.png` | 40 | 41 | 0.000543 | 0.000819 |
| `RCL0193ENG_PDF_098.png` | 98 | 99 | 0.000498 | 0.006913 |
| `RCL0193ENG_PDF_101.png` | 101 | 102 | 0.001125 | 0.021686 |
| `RCL0193ENG_PDF_107.png` | 107 | 108 | 0.000771 | 0.019046 |
| `RCL0193ENG_PDF_108.png` | 108 | 109 | 0.000472 | 0.005342 |
| `RCL0193ENG_PDF_109.png` | 109 | 110 | 0.000867 | 0.018546 |
| `RCL0193ENG_PDF_112.png` | 112 | 113 | 0.000397 | 0.005461 |
| `RCL0193ENG_PDF_113.png` | 113 | 114 | 0.000580 | 0.017829 |
| `RCL0193ENG_PDF_114.png` | 114 | 115 | 0.000503 | 0.006450 |
| `RCL0193ENG_PDF_117.png` | 117 | 118 | 0.000976 | 0.022904 |
| `RCL0193ENG_PDF_118.png` | 118 | 119 | 0.000381 | 0.005766 |
| `RCL0193ENG_PDF_120.png` | 120 | 121 | 0.000550 | 0.006524 |
| `RCL0193ENG_PDF_121.png` | 121 | 122 | 0.000758 | 0.013241 |
| `RCL0193ENG_PDF_122.png` | 122 | 123 | 0.000428 | 0.005780 |
| `RCL0193ENG_PDF_123.png` | 123 | 124 | 0.001919 | 0.026416 |
| `RCL0193ENG_PDF_125.png` | 125 | 126 | 0.000619 | 0.016115 |
| `RCL0193ENG_PDF_126.png` | 126 | 127 | 0.000523 | 0.006464 |
| `RCL0193ENG_PDF_127.png` | 127 | 128 | 0.000980 | 0.013902 |
| `RCL0193ENG_PDF_128.png` | 128 | 129 | 0.000471 | 0.006148 |
| `RCL0193ENG_PDF_129.png` | 129 | 130 | 0.000851 | 0.013684 |
| `RCL0193ENG_PDF_130.png` | 130 | 131 | 0.000369 | 0.005154 |
| `RCL0193ENG_PDF_131.png` | 131 | 132 | 0.001435 | 0.025783 |
| `RCL0193ENG_PDF_135.png` | 135 | 136 | 0.000846 | 0.018373 |
| `RCL0193ENG_PDF_170.png` | 170 | 171 | 0.000529 | 0.005446 |

## Consequence

The existing names `RCL0193ENG_PDF_NNN.png` must **not** be interpreted as physical PDF page NNN for this source. For this exact archive, they correspond to physical page `NNN + 1`. Example: `RCL0193ENG_PDF_098.png` is physical PDF page 99, which is the EVAP/emission-control page; physical page 98 is a blank separator.

Future source links must store the exact physical page independently from the legacy asset filename token. Existing visual files can be preserved, but their source-page metadata must be corrected/backfilled rather than inferred from the filename.
