## 2026-09-04 - REPRISE IA EN CODE DIRECT, SANS PATCH EMPILABLE

Regle confirmee : les patchs temporaires ne doivent pas devenir une base de travail. Ils peuvent masquer l'etat reel du code, etre oublies dans les workflows, puis etre empiles jusqu'a produire des erreurs difficiles a diagnostiquer.

### BASE PROPRE A RETENIR

La reprise applicative propre part du commit :
- `06dfb885478cb4ccd1e6a2ceffa49a1da8ec5054`

Une nouvelle branche de code a ete creee directement depuis ce socle :
- branche : `tmp-ai-memslibrary-reprise-code`

L'ancienne branche contenant les essais de patch/workflow reste seulement historique. Elle ne doit plus etre utilisee comme base de reprise.

### CORRECTIONS DE CODE DIRECTES

Deux commits applicatifs directs sont presents sur la nouvelle branche :
- `0fccbc89442bcaf7eb6288ecae73d4f64bbbde68` - routage du bouton IA depuis la preuve MEMSLibrary ;
- `5b899f62662270d7eb65a793d45394548db2b2a2` - integration directe du bridge MEMSLibrary dans le code/CMake normal.

Le HEAD applicatif a retenir est donc :
- `5b899f62662270d7eb65a793d45394548db2b2a2`

Le code normal contient maintenant directement :
- `IaMemsLibraryBridge.cpp/.h` et `IaMemsLibraryIntegration.cpp` dans `CMakeLists.txt` ;
- l'appel `askWithLibrary()` depuis l'onglet IA ;
- la conservation de la preuve documentaire MEMSLibrary ;
- le routage de `Voir le schema` a partir du document/page RAVEMEMS retrouve ;
- le garde negatif contre les questions diagnostiques generiques ;
- le scroll vertical valide conserve.

Aucun injecteur CMake n'est necessaire pour ce branchement normal.

### TEST DE COMPILATION SANS TRANSFORMATION DE SOURCE

Une branche de test separee a ete creee uniquement pour verifier la compilation :
- `tmp-ai-memslibrary-reprise-code-test`

Run GitHub Actions :
- `33910275653`
- conclusion : SUCCESS

Le test a valide :
- branche de production `MEMSX64` toujours sur BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730` ;
- scroll present ;
- bouton `Voir le schema` present ;
- routage par preuve MEMSLibrary present ;
- bridge MEMSLibrary integre au CMake normal ;
- configuration CMake x64 sans injecteur ;
- compilation x64 de `ecu_mems_manager` reussie ;
- hashes des fichiers source identiques avant et apres configure/build : aucune transformation de source pendant la CI.

### REGLE POUR LA SUITE

Ne plus corriger MEMS Manager par empilement de patchs temporaires.

Toute correction destinee a etre conservee doit etre inscrite directement dans les fichiers source puis compilee telle quelle. Les workflows de test peuvent verifier le code, mais ne doivent pas reecrire les sources avant compilation.

Aucun changement de `MEMSX64`, aucun BUILD #104.
