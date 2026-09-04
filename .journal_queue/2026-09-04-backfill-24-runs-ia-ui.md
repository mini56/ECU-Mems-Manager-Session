## 2026-09-04 — RATTRAPAGE DE TRAÇABILITÉ : 24 RUNS NON JOURNALISÉS

### MOTIF DU RATTRAPAGE
Cette entrée est un rattrapage rétrospectif. Des workflows applicatifs ont continué sur `tmp-ai-memslibrary-bridge` alors que le rapport maître n'était plus à jour puis est devenu illisible en UTF-8. Cette séquence a violé la règle de continuité RAPPORT -> action -> test -> RAPPORT. Le présent ajout restaure la traçabilité ; il ne valide pas rétroactivement cette méthode.

### INVARIANTS À NE PAS MODIFIER
- `MEMSX64` reste strictement sur BUILD #103 : `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`.
- Aucun BUILD #104.
- Ne pas perdre la base verte MEMSLibrary + `MEMSLibrary_Pack_001` + RAVEMEMS + Qwen.
- Ne pas modifier MEMSLibrary, RAVEMEMS, Qwen, le protocole ou un autre sous-système pour traiter un problème d'interface IA.

### INVENTAIRE GITHUB DES 24 RUNS À RATTRAPER
GitHub Actions retourne exactement 24 runs sur `tmp-ai-memslibrary-bridge` depuis le dernier point journalisé du 2026-09-04 15:05:22Z :
`33888177647`, `33889068041`, `33889068203`, `33891212452`, `33891213704`, `33891442871`, `33891444089`, `33891444095`, `33893571787`, `33893573603`, `33893599973`, `33893601650`, `33893620747`, `33893625250`, `33893889778`, `33893891412`, `33893891602`, `33894654745`, `33894656405`, `33894656502`, `33895312235`, `33895313391`, `33895313404`, `33895321969`.

Plusieurs pushes ont déclenché plusieurs workflows en parallèle sur le même commit. En particulier l'ancien `.github/workflows/tmp-restore-ia-scroll.yml` apparaît à répétition en échec en parallèle de workflows utiles ; ces exécutions ne doivent pas être confondues avec un nouveau BUILD de production.

### JALONS TECHNIQUES VÉRIFIÉS DANS CETTE SÉQUENCE
1. Run `33888177647` — `TMP IA MEMSLibrary Bridge` — SUCCESS — commit `49504a6834ad462c48127879e14b27e84b15d408` (`Stage Qwen self-test runtime beside executable`).
2. Commit `9da599e697aa96d099982a30f5f5c8a562a7c788` (`Build temporary x64 IA MEMSLibrary package`) : runs `33889068041` (`TMP IA MEMSLibrary Bridge`) et surtout `33889068203` (`TMP IA MEMSLibrary Windows Package`) en SUCCESS. Ce run constitue le paquet vert de référence IA + MEMSLibrary + RAVEMEMS + Qwen sur la base protégée #103. Artefact de référence du paquet : ID `9943343980`, nom `TMP-ECU-MEMS-Manager-x64-IA-MEMSLibrary-33889068203`, digest `sha256:4cf818b64670a8fb4dc5e6f77b7b30c35a3ab95251046ca24c529194a622f7f5`.
3. Commit `61ad62d293c68faa6cece61900ce79628d6392a4` (`Restore validated IA scroll in temporary test branch`) : run bridge `33891213704` SUCCESS ; le workflow ancien `33891212452` échoue en parallèle.
4. Commit `ad22232313469b0b3093bb676c1c7e16ceb3fdd5` (`Restore validated IA visual and scroll state`) : `33891444089` (`TMP Restore Validated IA Visual State`) SUCCESS et `33891444095` (`TMP IA MEMSLibrary Bridge`) SUCCESS ; `33891442871` ancien workflow scroll FAILURE en parallèle.
5. Commit `869cd231fb7a156e0fe9455fb7738718b85fdd4a` (`Add temporary evidence-led IA visual routing patch`) : `33893573603` bridge SUCCESS ; `33893571787` ancien workflow scroll FAILURE.
6. Commit `64783eeb202f69e36bd07a0e86e2845fd41e8035` (`Add temporary IA visual question self-test`) : `33893601650` bridge SUCCESS ; `33893599973` ancien workflow scroll FAILURE.
7. Commit `a65f060a0d313faa8f06d987eb062e96770bf051` (`Add temporary IA visual question test target`) : `33893625250` bridge SUCCESS ; `33893620747` ancien workflow scroll FAILURE.
8. Commit `b78c05598b2a903980b5ef1607aec00fc1a43775` (`Build temporary IA package with validated scroll and RAVEMEMS visuals`) : run `33893891602` (`TMP IA MEMSLibrary Visual Windows Package`) FAILURE. Les runs `33893889778` et `33893891412` appartiennent au même groupe de déclenchements et sont conservés dans l'inventaire ci-dessus.
9. Commit `d52a91f1a9344f01b9e2b21f35a51141fd5fa202` (`Capture temporary IA visual historical self-test diagnostics`) : run `33894656405` (`TMP IA Visual Historical Selftest Diagnostic`) FAILURE ; artefact diagnostic `9945284199`. Le run a néanmoins construit et exécuté les tests avant le garde final. Les runs `33894654745` et `33894656502` sont les autres déclenchements du même groupe.
10. Commit `35c539bfa7c64d416a202e66dc913904cbe67fd3` (`Trigger full restored IA MEMSLibrary visual package`) : run `33895321969` (`TMP IA MEMSLibrary Visual Windows Package`) FAILURE. Le workflow a validé l'ascendance du paquet vert #103, récupéré le paquet vert `33889068203`, récupéré le runtime visuel RAVEMEMS validé, puis compilé l'application complète avec MEMSLibrary + IA + scroll + visuels. Il s'est arrêté ensuite sur les contrôles historiques, donc les étapes finales de remplacement/validation/smoke/upload du nouveau paquet ont été skipped. Les runs `33895312235`, `33895313391` et `33895313404` sont les autres déclenchements du même groupe.

### ÉTAT DE CONTINUITÉ APRÈS RATTRAPAGE
- La dernière base de paquet complètement verte et réutilisable reste `33889068203` / commit `9da599e697aa96d099982a30f5f5c8a562a7c788`.
- La branche de travail IA est arrivée au commit `35c539bfa7c64d416a202e66dc913904cbe67fd3`, dérivé de cette base verte.
- Les travaux non journalisés ont porté sur la restauration/validation de l'interface IA, notamment le scroll et l'état visuel / bouton `Voir le schéma`, avec des workflows temporaires de contrôle. Ils ne doivent pas être transformés en modifications de MEMSX64 #103.
- Le problème UTF-8 du rapport a ensuite été réparé séparément ; cette entrée comble le trou de traçabilité créé avant cette réparation.

### PROCHAINE ACTION EXACTE
Reprendre uniquement le problème UI IA que l'utilisateur était en train de faire corriger : vérifier sur le HEAD `35c539bfa7c64d416a202e66dc913904cbe67fd3` le comportement réel du bouton `Voir le schéma` lorsqu'une image est disponible et le comportement du scroll de la fenêtre IA, comparer avec l'état validé antérieur, puis déterminer le delta UI minimal. Ne modifier ni MEMSLibrary, ni RAVEMEMS, ni Qwen, ni le protocole, ni `MEMSX64`. Ne lancer aucun nouveau run avant d'avoir identifié et documenté ce delta exact. Après toute future correction : un seul run justifié, analyse complète, puis mise à jour du RAPPORT avant l'action suivante.
