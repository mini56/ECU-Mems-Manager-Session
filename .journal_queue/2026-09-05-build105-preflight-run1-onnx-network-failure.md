## 2026-09-05 — BUILD #105 préflight temporaire : run 1 ROUGE sur transport ONNX externe

Branche temporaire : `tmp-memsx64-build105-filtered-bridge`
HEAD testé : `16e9a22fbac367a51b534ad3d867db15a13d09c5`
Run : `33956355098`
Job : `101280296674`
Conclusion : FAILURE.

Étapes passées avant l'échec :
- checkout exact source : SUCCESS ;
- garde source directe / aucune injection : SUCCESS (`DIRECT_SOURCE_NO_PATCH_PASS`) ;
- Python : SUCCESS ;
- Qt 5.15.2 MSVC x64 : SUCCESS.

Échec exact : étape historique `Download pinned ONNX Runtime development files`, avant le téléchargement de la DLL MEMSLibrary corrigée, avant compilation application/bridge, avant self-tests et avant packaging.
Log : `curl: (35) Send failure: Connection was reset`, puis `GenAI download failed`.

Diagnostic : défaillance de transport réseau externe sur le téléchargement GitHub Release ONNX GenAI. Ce run ne fournit aucune preuve d'un défaut du bridge filtré, de la DLL corrigée ou du Pack001 puisqu'aucun de ces tests n'a été exécuté.

PROCHAINE POUSSE AUTORISÉE APRÈS CE JOURNAL : correction workflow uniquement dans `.github/workflows/memsx64.yml`, sans changement applicatif : conserver les URLs, versions et SHA épinglés et ajouter aux deux commandes curl existantes la robustesse `--retry-all-errors --retry-delay 2` en plus de `--retry 3`. Aucun contournement des contrôles SHA, aucun changement bridge/DLL/Pack/protocole/UI.

Après pousse : relancer le même préflight temporaire #105 et journaliser immédiatement son résultat avant toute intégration dans `MEMSX64`.
