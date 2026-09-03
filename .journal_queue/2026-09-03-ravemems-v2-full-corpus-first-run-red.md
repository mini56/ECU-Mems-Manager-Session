## RAVEMEMS V2 — Premier run corpus complet générique : ROUGE, 1 échec d’extraction sur 47

Date : 2026-09-03
Run : 33806975993
Job : 100819672056
Moteur exact : 745f41243adb202f638f4039016dedfff8f272d6
Source figée : 643de091b474f4e27917a065bdf46d5a0c764276
MEMSX64 protégé : 1d6316bd1746d6f2b4cfb751cab88d18e27ef730

Résultat factuel :
- corpus figé matérialisé : 47 PDF ;
- 46 PDF ont terminé l’étape d’extraction ;
- 1 PDF a échoué : `rave/library/libxn.pdf` ;
- pages du PDF fautif : 1 ;
- publication détectée : aucune / unresolved ;
- langue détectée : `und` ;
- identité de secours calculée par le probe : `PDFSHA256AC352F6D35C2F8A5` ;
- message d’échec : `Expected document identity pattern not found in source PDF` ;
- fin du lanceur : `SOURCE_PDF_COUNT 47`, `SOURCE_FAILURE_COUNT 1` ;
- le replay/audit corpus global a été sauté parce que l’étape d’extraction a retourné 1 ; il ne faut donc pas présenter les 46 autres comme strictement validés global-zero à ce stade.

Artefact de preuve conservé :
- nom : `RAVEMEMS-V2-FULL-CORPUS-GENERIC-33806975993` ;
- ID : 9913351248 ;
- taille : 100727773 octets ;
- SHA256 ZIP : `f65e8fd84d042d775e28de6a5eba66b0d41316a2e993cb518799fcd04a34da45`.

Interprétation :
Il reste un défaut générique dans la gestion de l’identité de secours. Un PDF dont le contenu ne fournit aucune publication exploitable doit pouvoir être conservé et analysé sous une identité stable dérivée du SHA, sans imposer un motif d’identité absent du texte. La correction doit être générique, sans cas spécial pour `libxn.pdf`, et doit préserver les régressions RCL0179ENG, RCL0213ENG et RCL0193ENG.
