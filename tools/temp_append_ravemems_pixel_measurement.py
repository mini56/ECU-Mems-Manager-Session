from pathlib import Path

p = Path('RAPPORT_SUIVI_ECU_MEMS_MANAGER.md')
s = p.read_text(encoding='utf-8', errors='strict')
marker = '## 2026-08-31 - RAVEMEMS TEST2 - MESURE EXACTE DU GARDE PIXEL AVANT CORRECTION'
if marker in s:
    raise SystemExit('measurement already recorded')
entry = r'''
## 2026-08-31 - RAVEMEMS TEST2 - MESURE EXACTE DU GARDE PIXEL AVANT CORRECTION

Conformément à la PROCHAINE ACTION EXACTE du run 2, le dépassement du garde pixel a été mesuré localement sans aucune modification GitHub, avec la source canonique `cdxn990e.pdf` page physique 7, le code corrigé au HEAD pilote `922531f6bbd2474e7c85597009768daf8f26319e` et les mêmes 33 opérations de rendu.

Résultat exact : **341 860 pixels modifiés au total**, dont seulement **5 pixels hors des masques déclarés**. Les 5 pixels sont tous à **exactement 1 pixel** du masque de traduction le plus proche. Répartition : **2 pixels** au voisinage de l'opération 2 (`All of the information in this folder...`) et **3 pixels** au voisinage de l'opération 12 (`Sealed joints` -> `Jonctions étanches`). La boîte englobante de ces cinq pixels hors masque est `x=164..176`, `y=227..1497`.

Conclusion technique : ce résultat exclut une altération réelle de la géométrie du schéma. Le dépassement correspond à un débordement de glyphe/anticrénelage ou de left-bearing PIL produit par `draw.text()` lorsqu'il dessine directement sur la page complète. Le garde a donc correctement détecté cinq pixels écrits hors de la zone autorisée, et **il ne doit pas être affaibli ni élargi arbitrairement**.

### CORRECTION AUTORISEE AVANT LA PROCHAINE POUSSE

Corriger uniquement le rendu des textes localisés afin qu'il soit borné par construction : créer pour chaque opération un patch image de la taille exacte du bbox de remplacement, rendre le texte dans ce patch avec des coordonnées locales, puis coller le patch dans la page au bbox prévu. Ainsi aucune antialiasing/left-bearing de police ne pourra modifier un pixel hors masque. Conserver les mêmes bboxes, les mêmes traductions, les mêmes tokens techniques immuables et le même garde global de pixels hors zones. Ne pas modifier `MEMSX64`.

Après cette correction : relancer exactement le même TEST2 sur `cdxn990e.pdf` page 7. Si le garde passe, télécharger et inspecter manuellement le PNG réel avant toute déclaration de validation. Rapport immédiat après le résultat, qu'il soit vert ou rouge.
'''
p.write_text(s.rstrip('\n') + '\n\n' + entry.strip() + '\n', encoding='utf-8', errors='strict', newline='\n')
check = p.read_text(encoding='utf-8', errors='strict')
for token in (marker, '341 860 pixels', '5 pixels hors des masques', 'exactement 1 pixel', '922531f6bbd2474e7c85597009768daf8f26319e'):
    assert token in check, token
print('REPORT_TEST2_PIXEL_MEASUREMENT_OK')
