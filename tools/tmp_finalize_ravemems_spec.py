from pathlib import Path

cahier = Path('docs/CAHIER_DES_CHARGES_RAVEMEMS_BROUILLON_2026-09-02.md')
text = cahier.read_text(encoding='utf-8')

text = text.replace('**Statut : BROUILLON À RELIRE ET COMPLÉTER PAR L’UTILISATEUR**  ', '**Statut : VALIDÉ PAR L’UTILISATEUR — VERSION FIGÉE AVANT IMPLÉMENTATION**  ')

anchor = "Les **serrages en plusieurs étapes** doivent être conservés comme une séquence technique complète et ordonnée, et non réduits à une valeur unique. RAVEMEMS doit préserver chaque passe de serrage, chaque couple, chaque angle, l’ordre des vis ou fixations, les éventuelles conditions intermédiaires et tout ordre constructeur imposé. Exemple : `20 N·m`, puis `60°`, puis encore `60°` doit rester une séquence de trois étapes distinctes. Si le manuel fournit un schéma d’ordre de serrage, ce visuel doit être relié à la séquence et l’IA doit pouvoir le proposer avec la procédure."
if anchor not in text:
    raise SystemExit('section 26 anchor missing')
text = text.replace(anchor, anchor + "\n\nToute **condition d’applicabilité d’une valeur** fait partie intégrante de cette valeur et ne doit jamais être dissociée. Cela comprend notamment température moteur, régime, tension d’alimentation, pression, position, état de fonctionnement, équipement ou autre condition constructeur. Exemples : `0,8 mm moteur froid`, `900 tr/min moteur chaud`, `2,5 bar à 3000 tr/min`. L’IA ne doit jamais restituer seulement la valeur numérique en oubliant la condition qui la rend valable.", 1)

anchor = "Si deux valeurs, procédures ou informations différentes existent pour un même sujet selon l’année, le modèle, le moteur, le marché, la version ECU, l’équipement ou toute autre variante, RAVEMEMS doit **conserver les différentes informations avec leur contexte d’applicabilité**. L’IA ne doit jamais choisir arbitrairement une version. Elle doit identifier le discriminant réellement nécessaire et demander uniquement l’information manquante permettant de sélectionner la bonne réponse."
if anchor not in text:
    raise SystemExit('section 29 anchor missing')
text = text.replace(anchor, anchor + "\n\nLorsqu’il existe plusieurs **éditions ou révisions d’un même manuel**, chaque information reste rattachée à son édition, sa révision et son contexte d’applicabilité. Une édition plus récente ne remplace jamais automatiquement une information plus ancienne pour tous les véhicules.", 1)

anchor = "Tout élément dont l’extraction ou la structuration reste incertaine — par exemple numéro d’étape ambigu, valeur mal lue, OCR douteux, tableau mal structuré, relation image/procédure incertaine ou autre information non suffisamment fiable — doit être **conservé mais marqué `à vérifier`**. RAVEMEMS ne doit jamais le valider silencieusement comme certain. L’**audit post-extraction** doit pouvoir retrouver automatiquement l’ensemble de ces éléments et ils doivent être revus avant validation finale du corpus."
if anchor not in text:
    raise SystemExit('section 33 anchor missing')
text = text.replace(anchor, anchor + "\n\nLes **traductions provisoires ou non encore validées** doivent également être retrouvées par l’audit. Une information source ne doit jamais être perdue parce que sa traduction n’est pas encore validée. Une traduction provisoire peut être utilisée, mais elle reste `à vérifier` jusqu’à validation.\n\nUn **visuel techniquement dégradé ou incorrect n’est pas un résultat acceptable**. Tout doit être mis en œuvre pour l’extraire proprement et fidèlement depuis le document original. Si un cas résiste, il reste un défaut bloquant `à corriger` / `à vérifier` et ne doit pas être accepté comme visuel final.", 1)

anchor = "La **validation des traductions se fait en priorité en français**, puis dans les autres langues. Cet ordre de validation ne modifie pas l’architecture ni le statut de la langue source."
if anchor not in text:
    raise SystemExit('section 37 anchor missing')
text = text.replace(anchor, anchor + "\n\nSi une traduction dans la langue active n’est pas encore validée, l’information source reste disponible. Une **traduction provisoire** peut être utilisée pour ne pas perdre l’information, mais elle reste identifiée comme non validée et enregistrée `à vérifier`. L’audit doit la contrôler avant validation finale du corpus.", 1)

old41 = '''## 41. POINTS À RELIRE / COMPLÉTER PAR L’UTILISATEUR

Avant développement, vérifier notamment :
- quantité maximale de choix proposés pour une demande générale ;
- comportement lorsque deux variantes sont possibles ;
- catégories de visuels à ajouter ou retirer ;
- niveau de nettoyage acceptable des images ;
- types de tableaux prioritaires ;
- comportement des suggestions `Je vous conseille aussi de voir…` ;
- toute autre règle utilisateur à ajouter avant développement.'''
new41 = '''## 41. RELECTURE UTILISATEUR TERMINÉE

La relecture du cahier des charges a été menée avec l’utilisateur et les décisions ont été intégrées au présent document. Il ne reste plus de point fonctionnel ouvert avant conception du prototype.'''
if old41 not in text:
    raise SystemExit('section 41 missing')
text = text.replace(old41, new41, 1)

old42 = '''## 42. PROCHAINE ACTION

**Ne lancer aucune nouvelle extraction avant validation explicite de ce cahier des charges.**

Après relecture utilisateur :
1. intégrer les corrections et ajouts ;
2. figer une version validée du cahier des charges ;
3. inscrire cette validation dans le rapport maître ;
4. choisir le manuel prototype ;
5. concevoir le nouvel extracteur RAVEMEMS ;
6. tester le prototype ;
7. seulement ensuite envisager le corpus complet.'''
new42 = '''## 42. PROCHAINE ACTION

**Le cahier des charges est validé et figé. Aucun développement, aucune nouvelle extraction et aucun BUILD de production ne doivent démarrer sans un `GO` explicite de l’utilisateur.**

Après ce `GO` seulement :
1. relire le rapport maître et cette version validée du cahier ;
2. travailler uniquement sur une branche de test dédiée ;
3. conserver `MEMSX64` sur BUILD #103 et ne pas toucher au 32 bits ni au protocole ;
4. concevoir le nouvel extracteur RAVEMEMS ;
5. réaliser le prototype sur `RCL0193ENG` ;
6. auditer intégralement le prototype ;
7. demander validation utilisateur avant toute extraction du corpus complet.'''
if old42 not in text:
    raise SystemExit('section 42 missing')
text = text.replace(old42, new42, 1)

text = text.replace('# FIN DU BROUILLON\n\n**Ce document est volontairement un cahier des charges à relire. Il peut être corrigé, complété ou simplifié avant toute implémentation.**', '# FIN DU CAHIER DES CHARGES VALIDÉ\n\n**Version figée après relecture et validation explicite de l’utilisateur le 2 septembre 2026. Toute modification fonctionnelle ultérieure devra être explicitement validée avant implémentation.**', 1)
cahier.write_text(text, encoding='utf-8')

report = Path('RAPPORT_SUIVI_ECU_MEMS_MANAGER.md')
with report.open('a', encoding='utf-8') as f:
    f.write('''\n\n## 2026-09-02 — VALIDATION FINALE DU CAHIER DES CHARGES RAVEMEMS\n\nDécisions finales utilisateur : conditions inséparables des valeurs ; éditions/révisions conservées avec leur contexte ; traductions provisoires autorisées mais obligatoirement retrouvées et vérifiées par l’audit ; aucun visuel dégradé accepté, tout doit être fait pour obtenir une extraction propre et fidèle.\n\nLe cahier des charges RAVEMEMS est désormais VALIDÉ et FIGÉ avant implémentation. Aucun code applicatif, aucune nouvelle extraction et aucun BUILD de production ne sont lancés. `MEMSX64` reste protégée sur BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`.\n\nPROCHAINE ACTION EXACTE : attendre un `GO` explicite de l’utilisateur. Après ce GO seulement, relire le rapport et le cahier validé, travailler sur branche de test dédiée, concevoir le nouvel extracteur RAVEMEMS et réaliser le prototype `RCL0193ENG`. Le corpus complet reste interdit avant audit et validation utilisateur du prototype.\n''')
