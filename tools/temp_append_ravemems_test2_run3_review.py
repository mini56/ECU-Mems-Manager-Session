from pathlib import Path

p = Path('RAPPORT_SUIVI_ECU_MEMS_MANAGER.md')
s = p.read_text(encoding='utf-8', errors='strict')
marker = '## 2026-08-31 - RAVEMEMS TEST2 - RUN 3 VERT AUTOMATIQUE MAIS REFUSE VISUELLEMENT'
if marker in s:
    raise SystemExit('run3 result already recorded')
entry = r'''
## 2026-08-31 - RAVEMEMS TEST2 - RUN 3 VERT AUTOMATIQUE MAIS REFUSE VISUELLEMENT

Après consignation de la mesure des cinq pixels hors masque, le rendu a été borné par construction dans chaque bbox de traduction. HEAD pilote réellement checkouté par le job : `001443511ddcde0116d6d263746df25f7673f8c9` (`Clip TEST2 localized text rendering to declared bboxes`). Le log du job `99659033934` confirme explicitement ce SHA.

Le troisième passage du même run GitHub `33442157473` est **SUCCESS côté automatisation** : 490 mots OCR, 45 régions, 33 opérations, 11 tokens techniques contrôlés, `outside_masks_identical=true`, tous les tokens techniques pixel-identiques et aucun identifiant interne visible. Artefact : ID `9777363401`, nom `ravemems-test2-cdxn-page7-raster-text`, digest ZIP SHA-256 `998fb973c72f3acfefefe98bc541348302f540988dfa7a8fc0f0fe715ff9713c`. PNG localisé SHA-256 `86dce95ff6ec1ec7446cd2c8f6300e885808829c8ce826c4aceb936d9a3383a4`.

**Ce vert automatique ne vaut pas validation.** Inspection manuelle du vrai PNG `CDXN990E_P007_MEMS_SIM_FR.png` :
1. La région OCR `CDXN_P007_PIX_R026`, bbox `[1214,1213,1863,1284]`, reste en anglais : `A. Plug on lead (Flylead) wired directly to the component. B. Connector plugs directly into component.`
2. La région `CDXN_P007_PIX_R006`, bbox `[2820,314,3009,345]`, affiche encore `C24 EARTH 1`. `C24` et `1` sont des références à préserver ; le mot humain `EARTH` doit être localisé (`MASSE`) sans toucher aux références.
3. La région connecteur `CDXN_P007_PIX_R042` est utilisée deux fois comme opération de paragraphe avec exactement le même bbox `[179,1925,1149,2049]`. L'opération 15 écrit l'explication du numéro de connecteur, puis l'opération 16 remplace à nouveau tout ce bbox par la seule phrase `À utiliser avec la section Détails des connecteurs...`. Le premier contenu traduit est donc effacé : **information source perdue**.
4. L'opération 19 traduit la région `R025` mais y injecte aussi les textes A/B qui appartiennent en réalité à `R026`, ce qui force la police à 14 px (`font_ratio=0.636`) tandis que les vraies lignes A/B restent en anglais plus bas. La traduction doit respecter les régions réelles au lieu de déplacer leur contenu dans une autre zone.

Verdict manuel : **TEST2 NON VALIDÉ** malgré le vert automatisé. Les gardes actuels protègent correctement la géométrie et les références techniques mais ne garantissent pas encore la complétude linguistique ni l'absence de double remplacement destructif d'un même paragraphe.

### PROCHAINE ACTION EXACTE

Corriger uniquement la fixture/résolution de traduction TEST2 et renforcer le garde correspondant :
- fusionner les deux opérations de `R042` en une seule traduction complète qui conserve les deux informations du paragraphe source ;
- limiter `R025` à la traduction de sa seule phrase `Connector - Direction...` ;
- traduire `R026` dans son propre bbox en conservant les repères `A.` et `B.` ;
- dans `R006`, conserver `C24` et `1` et remplacer uniquement `EARTH` par `MASSE` ;
- ajouter un garde empêchant deux opérations `paragraph` sur la même région et un contrôle de complétude pour ces régions humaines afin que le même défaut ne puisse plus produire un vert automatique.

Ne modifier ni les bboxes techniques, ni les références constructeur, ni la géométrie, ni `MEMSX64`. Rapport avant la prochaine pousse, puis relancer exactement le même TEST2 et inspecter de nouveau le PNG réel avant toute validation utilisateur.
'''
p.write_text(s.rstrip('\n') + '\n\n' + entry.strip() + '\n', encoding='utf-8', errors='strict', newline='\n')
check = p.read_text(encoding='utf-8', errors='strict')
for token in (marker, '001443511ddcde0116d6d263746df25f7673f8c9', '9777363401', 'CDXN_P007_PIX_R026', 'TEST2 NON VALIDÉ'):
    assert token in check, token
print('REPORT_TEST2_RUN3_MANUAL_REJECT_OK')
