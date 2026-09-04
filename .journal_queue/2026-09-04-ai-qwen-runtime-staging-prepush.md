## 2026-09-04 — IA MEMSLibrary / Qwen — diagnostic runtime avant correction

- Branche technique active : `tmp-ai-memslibrary-bridge`.
- Production protégée : `MEMSX64` reste BUILD #103, commit `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`. Aucun BUILD #104 autorisé à ce stade.
- Dernier run temporaire analysé : `33881624501` puis reproduction sur le chemin Qwen du workflow temporaire.
- Le self-test final `ia_memslibrary_qwen_selftest.exe` échoue au démarrage avec `-1073741502`, soit `0xC0000142` (`STATUS_DLL_INIT_FAILED`).
- Le bridge MEMSLibrary, Pack001, la provenance RAVEMEMS, la compilation complète de l'application, la compilation du self-test Qwen et le téléchargement du modèle sont déjà verts avant cet échec.

### Comparaison avec BUILD #103

Le workflow production `.github/workflows/memsx64.yml` de `MEMSX64` utilise déjà exactement :
- ONNX Runtime GenAI `0.14.0` ;
- ONNX Runtime `1.24.4` ;
- le même Qwen3-0.6B ONNX INT4 et les mêmes hashes.

Donc la coexistence `GenAI 0.14.0 + ORT 1.24.4` n'est pas retenue comme cause à elle seule : BUILD #103 l'a déjà validée.

La divergence réelle trouvée est dans le staging Windows : BUILD #103 analyse les imports PE des DLL ONNX avec `pefile`, copie dans le dossier runtime les DLL Microsoft VC++ redistribuables requises (`msvcp*`, `vcruntime*`, `concrt*`) depuis `System32`, vérifie que toutes les DLL sont AMD64, puis copie le self-test `local_ai_onnx_selftest.exe` dans le dossier runtime avant exécution. Le workflow temporaire Qwen avait omis cette étape et exécutait son self-test depuis le répertoire de build avec seulement le runtime ajouté au `PATH`.

### Correction autorisée suivante

Aligner uniquement le staging du workflow temporaire sur celui déjà validé par BUILD #103 :
1. installer `pefile` dans les outils de validation ;
2. reprendre le scan/copie des dépendances VC++ et le contrôle AMD64 du runtime #103 ;
3. exécuter `ia_memslibrary_qwen_selftest.exe` depuis le dossier `ORT_GENAI_RUNTIME_STAGE`, comme le self-test production ;
4. ne modifier ni Qwen, ni les versions ONNX, ni MEMSLibrary, ni Pack001, ni l'application, ni `MEMSX64`.

Objectif : supprimer le `0xC0000142` par parité de runtime avec #103, puis relancer le workflow temporaire complet et exiger un verdict Qwen + grounding RAVEMEMS réel.
