# RCL0193 visual backfill — batch 1760 candidate audit

Date: 2026-08-29

Production remains `MEMSX64` BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`. Documentary-only candidate: no ECU communication, protocol, acquisition, RAM, write/reset, Qwen/ONNX, UI or 32-bit code is modified. Base/IA/RAVE remains consultative only.

Primary source: Rover `RCL0193ENG`, 372-page factory PDF, SHA-256 `c050a3eebe50c5a85bf8a69b7722bd2052079944e09d58578a498984ecf06715`.

Historical scope: 31 RCL0193 RAVE facts from batch 1660. Historical facts remain 31/31 unchanged and keep empty historical `image_ref`. V3 mapping retains every page reference, including non-contiguous 114+118 and 135+170.

This candidate retains 26 unique factory pages once and creates 43 fact→page source links. A fact may therefore expose more than one original page without duplicating image files.

Validation on reconstructed BUILD #101 reference DB with batch 1750 already applied:
- integrity: `ok`
- user_version: `20`
- historical RAVE facts: `93`
- historical expert facts: `105`
- RCL0193 assets: `26`
- RCL0193 fact coverage: `31/31`
- RCL0193 source links: `43`
- RCL0194 assets preserved: `8`

Generated `research_enrichment_1760.qz64`: 3213 bytes, SHA-256 `d3c3580d9f32d92e42db6eeb81e92a3e0be591b007ec1071d1a95baf6057ebc8`; decompressed SQL 22817 bytes, SHA-256 `ce6e1ba2b441f1102f78e49ea87f81dadb3db66ccbf64d63b76fd0e1974d8173`.

## Retained factory pages
- PDF p.38: `table_page`, `RCL0193ENG_PDF_038.png`, SHA-256 `a390767a247972909140568093539c1c3370edad63cdd010b1f8ce2a5cfb8d98`
- PDF p.39: `table_page`, `RCL0193ENG_PDF_039.png`, SHA-256 `d881fbd4d315eb9c9e87e0e2958b6ec6fe4754cfb64e4eab8794330a04b15dd1`
- PDF p.40: `table_page`, `RCL0193ENG_PDF_040.png`, SHA-256 `65d280e6676529d10da5b48cb401e2ac4e53cdd5b6a6f8a3743b512bf119bb04`
- PDF p.98: `technical_description_page`, `RCL0193ENG_PDF_098.png`, SHA-256 `d8d4812a42700f9f3f2ce39d8e2aa3ec44a1704e86f76e2f0ba0c35bf324c9f3`
- PDF p.101: `procedure_page`, `RCL0193ENG_PDF_101.png`, SHA-256 `aa9db58404ba918f17db025348cd355965029efa0bd99895adf974860ff92a0e`
- PDF p.107: `technical_description_page`, `RCL0193ENG_PDF_107.png`, SHA-256 `7295cced222821c1cf43b4ddc001b6e9d892d5293c6bd504741302cc3dbe919a`
- PDF p.108: `technical_description_page`, `RCL0193ENG_PDF_108.png`, SHA-256 `7ac5fd15a19ebd554e4bc68230eb3fb78f5d1d2293b50d4a86a4c0e760a15dfe`
- PDF p.109: `technical_description_page`, `RCL0193ENG_PDF_109.png`, SHA-256 `6eab041c321b21d3a709654690b1080fd03f42ef18b494ddc9efe6ec102c5456`
- PDF p.112: `technical_description_page`, `RCL0193ENG_PDF_112.png`, SHA-256 `faff199daf138675bd4b119fbc97d4e80390267b95788de36c4fc5f6634f8293`
- PDF p.113: `technical_description_page`, `RCL0193ENG_PDF_113.png`, SHA-256 `d4f83143b698784c9012461c3b32f282562dd20b6e25930000fdb6cdea1f0830`
- PDF p.114: `technical_description_page`, `RCL0193ENG_PDF_114.png`, SHA-256 `9337a1b44946f74f800da84b9128adb7bc2fa06c2a04fbe21869da237058166f`
- PDF p.117: `technical_description_page`, `RCL0193ENG_PDF_117.png`, SHA-256 `d2458744736adaf240e2afee10ede3f7a6bed7ced3ad3c7ed14982a41166693e`
- PDF p.118: `procedure_page`, `RCL0193ENG_PDF_118.png`, SHA-256 `a10faab6394433096be2069a6301bbfa1ebea9c7db4ce59eb150728edbc9888a`
- PDF p.120: `procedure_page`, `RCL0193ENG_PDF_120.png`, SHA-256 `39f05ded8e37cb7447bf5e7504c7673d6106deefb0c7fbaaf2da5c6fabedd165`
- PDF p.121: `procedure_page`, `RCL0193ENG_PDF_121.png`, SHA-256 `879b10b867da77bd59c9f1ff8a13d81377692ce7e9b5a6174f5d7bf982f9b40a`
- PDF p.122: `procedure_page`, `RCL0193ENG_PDF_122.png`, SHA-256 `43f288b733cf74169f1bcc3c4284d6a949dfb1a46a7db434185b2bc78a1f89de`
- PDF p.123: `procedure_page`, `RCL0193ENG_PDF_123.png`, SHA-256 `8977f0918a5ae78c7eef27d96265297b30e6f1021c10241afc9696b97a350412`
- PDF p.125: `procedure_page`, `RCL0193ENG_PDF_125.png`, SHA-256 `232f50c99af2802b56a8d4cb086329d7eafe946083222940f3a0ae15e3ce3728`
- PDF p.126: `procedure_page`, `RCL0193ENG_PDF_126.png`, SHA-256 `6745b1b239e58d4af63ce2835f44f5dc6ca53db06d8be493f6a888c28e2ea83f`
- PDF p.127: `procedure_page`, `RCL0193ENG_PDF_127.png`, SHA-256 `3095ec02dc93966b5f0904ca286b17fdf2e1cb91fd6f1c80b6f1590a611bba77`
- PDF p.128: `procedure_page`, `RCL0193ENG_PDF_128.png`, SHA-256 `a98c69e7270ecbc075111775fe4859c319d22286e464d0737cb8d841e782f720`
- PDF p.129: `procedure_page`, `RCL0193ENG_PDF_129.png`, SHA-256 `6d0048fc01b70c42ef5c9dcf33e9c213866b84a48cffd295b740181d5705c250`
- PDF p.130: `procedure_page`, `RCL0193ENG_PDF_130.png`, SHA-256 `0c5e859a1331dbb95cf26e6864a965043ca5c08c7860961608facf7a9a22edaf`
- PDF p.131: `procedure_page`, `RCL0193ENG_PDF_131.png`, SHA-256 `80f518f28b7d98809ce2cb1a4e5e87239c8722089c64dde9b04698324a91960f`
- PDF p.135: `procedure_page`, `RCL0193ENG_PDF_135.png`, SHA-256 `b552668f5dd879b1fdc7a141a48cc2990ff46b8b363f3b6f262c413030c506f8`
- PDF p.170: `procedure_page`, `RCL0193ENG_PDF_170.png`, SHA-256 `732bf9fc49254db52908659c575fc508a1afce555a4c98706f738f5a66f03ef8`
