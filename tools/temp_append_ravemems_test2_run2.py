from pathlib import Path
p=Path('RAPPORT_SUIVI_ECU_MEMS_MANAGER.md')
s=p.read_text(encoding='utf-8',errors='strict')
marker='## 2026-08-31 - RAVEMEMS TEST2 - RUN 2 ECHEC GARDE PIXEL'
if marker in s: raise SystemExit('already recorded')
entry=r'''
## 2026-08-31 - RAVEMEMS TEST2 - RUN 2 ECHEC GARDE PIXEL

Après consignation préalable du diagnostic OCR, le correctif minimal a été appliqué sur `tmp-rave-new-extraction-pilot`. HEAD final du correctif : `922531f6bbd2474e7c85597009768daf8f26319e` (`Preserve Tesseract OCR word order in TEST2`). La seule correction fonctionnelle est la conservation de l'ordre natif des mots Tesseract dans chaque groupe OCR ; le trigger TEST2 a aussi été renouvelé. Les fichiers temporaires de patch se sont supprimés dans le commit final.

Le même workflow TEST2 a été relancé sur le run `33442157473` (deuxième tentative, job `99656734760`) avec checkout confirmé sur le HEAD corrigé `922531f6bbd2474e7c85597009768daf8f26319e`. Résultat : l'étape de compilation passe, les ancres OCR passent désormais et le rendu progresse jusqu'au garde final de géométrie. Échec exact : `RuntimeError: pixels outside declared translation zones changed` dans le contrôle `outside_translation_masks_pixel_identical`.

Artefact de cette deuxième tentative : ID `9777109051`, nom `ravemems-test2-cdxn-page7-raster-text`, digest ZIP SHA-256 `1f20ab0584b772b92cd639db8cca934d906cb78c8c5370cbdbcabe3bc9ea88ca`. Le script s'arrête avant sauvegarde du PNG localisé et avant manifeste final ; aucune validation graphique n'est donc acquise.

### PROCHAINE ACTION EXACTE

Mesurer précisément les pixels différents détectés hors des masques de traduction pour déterminer s'il s'agit d'un débordement de glyphes/anticrénelage PIL ou d'une vraie altération de géométrie. Ne modifier aucun code tant que cette mesure n'est pas faite. Si le dépassement vient du rendu texte, corriger le rendu pour qu'il soit strictement limité à la zone de remplacement (clipping/paste dans un patch borné), sans élargir arbitrairement le garde et sans toucher aux éléments techniques. Puis rapport avant toute nouvelle pousse et relance du même TEST2.
'''
p.write_text(s.rstrip('\n')+'\n\n'+entry.strip()+'\n',encoding='utf-8',errors='strict',newline='\n')
check=p.read_text(encoding='utf-8',errors='strict')
for t in (marker,'922531f6bbd2474e7c85597009768daf8f26319e','99656734760','9777109051','1f20ab0584b772b92cd639db8cca934d906cb78c8c5370cbdbcabe3bc9ea88ca'):
    assert t in check,t
print('REPORT_TEST2_RUN2_FAILURE_OK')
