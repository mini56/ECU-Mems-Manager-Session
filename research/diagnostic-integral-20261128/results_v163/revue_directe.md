# V1.63 — vérification directe du service 23
Source immuable: 182cc66bb1acbd347289c13babd0452b3ad1dd3b.
ADJMP002, CS73P001 et DS43I003: ASM vérifié, répartiteur, lecture adresse 24 bits, longueur, bornes, copie et réponse +40. Les 9 BIN MEMS 2J ont été contrôlés individuellement (comparaison du service, cible BSR calculée, construction d'adresse, copie 1298). Aucun essai physique.
|Firmware|Dispatch|Lecteur|Copie|
|---|---|---|---|
|ADJMP002|10BB2C|10BEBA|10C03C|
|ADJMP003|10BC34|10BFC2|10C144|
|KMJ7P003|10CEA4|10D232|10D3B4|
|KMJ7V001|10D048|10D6B4|10D7AA|
|KSJ3P008|10C0A4|10C432|10C528|
|KSJ3P010|10C0E2|10C470|10C5F2|
|KSJBI009|10BCE6|10C074|10C1F6|
|KSJBP005|10BCC0|10C04E|10C1D0|
|KSJBP007|10BCE6|10C074|10C1F6|
|CS73P001|1227A2|123102|1232DE|
|DS43I003|123A5E|1243BE|12459A|
ADJMP002: service 21 table 11177E, routine 10BDF4; autre chemin selon état de session. Ne pas extrapoler. 
Le contrôle exhaustif doit traiter les 483 images, conserver les preuves antérieures et marquer explicitement les autres architectures et signatures non résolues. Les 93 MEMS 3 restants et 34 autres restent ouverts à cette étape. Les limites d'accès et la stratégie de remplacement de température ne sont pas qualifiées globalement. Aucun changement de Manager ou DLL.
