# IA MEMS — moteur local x64

Cette couche fournit la conversation locale de l'onglet **IA MEMS** sur la voie Windows x64 de MEMS Manager.

## Architecture BUILD #30

- `ECU MEMS Manager` x64 charge **ONNX Runtime GenAI** directement dans son processus.
- Il n'y a plus de `llama-server.exe`, de `QProcess`, de serveur HTTP local ni de port TCP pour l'IA.
- Le modèle et la génération s'exécutent sur un thread dédié afin de ne pas bloquer l'interface Qt.
- MEMS Manager prépare le contexte technique depuis le moteur expert et la base MEMS avant de solliciter le modèle.
- Les réponses déterministes restent prioritaires ; le modèle ne doit pas inventer les faits ECU.
- Le package x64 n'est publiable que si le self-test du `LocalAiClient` de production charge le modèle ONNX et génère une réponse réelle depuis le package assemblé.
- La CI `MEMSX64` assemble le runtime officiel, le modèle épinglé et l'application complète avant publication de l'artefact de test.

## Arborescence d'exécution attendue

```text
ECU MEMS Manager/
  ecu_mems_manager.exe
  onnxruntime-genai.dll
  onnxruntime.dll
  ... dépendances officielles ONNX Runtime GenAI CPU ...
  ai/
    runtime_manifest.json
    models/
      qwen3-0.6b-int4/
        genai_config.json
        model.onnx
        model.onnx.data
        tokenizer.json
        tokenizer_config.json
        chat_template.jinja
```

Pour les tests, `MEMS_AI_RUNTIME` peut pointer vers `onnxruntime-genai.dll` et `MEMS_AI_MODEL` vers le dossier du modèle contenant `genai_config.json`.

## Runtime épinglé

Voir `runtime_manifest.json` pour les versions, révisions et SHA-256 exacts.

- Runtime : **ONNX Runtime GenAI 0.14.0**, package officiel Windows x64 CPU, licence MIT.
- Modèle : **Qwen3-0.6B ONNX INT4 CPU**, snapshot figé dans la CI.

## Principe de sûreté fonctionnelle

Le modèle conversationnel reçoit un contexte préparé par MEMS Manager à partir des mesures déjà acquises, de l'historique, du moteur expert, de la base de connaissance et de la description réelle des fonctions du logiciel.

Le modèle est chargé de comprendre la formulation de l'utilisateur et de produire une réponse naturelle. Les données ECU, les niveaux de preuve, les hypothèses diagnostiques et les fonctions du logiciel ne doivent pas être inventés par le modèle. Le moteur ONNX n'a aucun accès aux commandes ou mutations ECU.
