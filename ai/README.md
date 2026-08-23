# IA MEMS — runtime local

Cette couche fournit la conversation locale de l'onglet **IA MEMS**. Elle est volontairement séparée du processus principal 32 bits de MEMS Manager.

## Architecture

- `ECU MEMS Manager` reste le programme Qt/MinGW 32 bits.
- `llama-server.exe` fonctionne comme processus Windows x64 séparé.
- Le serveur écoute uniquement `127.0.0.1:18089`.
- MEMS Manager lui envoie les questions et un contexte technique déjà préparé par le moteur expert / la base MEMS.
- Le modèle ne décide pas seul des faits ECU : si le contexte ne suffit pas, il doit le dire.

## Arborescence d'exécution attendue

```text
ECU MEMS Manager/
  ecu_mems_manager.exe
  ai/
    llama-server.exe
    ggml*.dll / dépendances de la release llama.cpp
    models/
      ia-mems.gguf
```

Le chemin peut aussi être fourni avec les variables d'environnement `MEMS_AI_SERVER` et `MEMS_AI_MODEL`, principalement pour les tests.

## Runtime épinglé

Voir `runtime_manifest.json` pour les versions et SHA-256 exacts.

- Runtime : llama.cpp Windows x64 CPU, release `b10516`, licence MIT.
- Modèle initial : `Qwen3-0.6B-Q8_0`, licence Apache-2.0.

Le modèle initial est choisi pour permettre une première distribution locale raisonnablement légère. L'architecture accepte ensuite un modèle GGUF plus puissant sans modifier le moteur expert.

## Principe de sûreté fonctionnelle

Le modèle conversationnel reçoit un **CONTEXTE FIABLE FOURNI PAR MEMS MANAGER**. Ce contexte provient des mesures déjà acquises, de l'historique, du moteur expert, de la base de connaissance et de la description réelle des fonctions du logiciel.

Le modèle est chargé de comprendre la formulation de l'utilisateur et de produire une réponse naturelle. Les données ECU, les niveaux de preuve, les hypothèses diagnostiques et les fonctions du logiciel ne doivent pas être inventés par le modèle.
