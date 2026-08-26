# ÉTAPE BUILD #30 — REMPLACEMENT DU MOTEUR IA LOCAL

Date : 2026-08-26

Autorisation utilisateur : remplacer la voie llama.cpp qui tourne en boucle par une solution IA locale Windows x64 propre et testée sous Windows.

## Constat avant étape

- `MEMSX64` réel avant modification : `bdd7de2da64cb1308852e0900c3e20287cbf128b`.
- GitHub Actions #73 : ROUGE.
- `llama-server.exe --version` fonctionne, mais le chargement de Qwen replante avec `-1073740791 = 0xC0000409`.
- Le retour à la reconstruction/staging llama.cpp de #73 est contraire à la voie propre précédemment retenue et est abandonné.
- Les essais #68/#72 basés sur la distribution officielle llama.cpp étaient VERTS en CI mais échouaient sur le PC réel avec `QProcess 0 / FailedToStart`.

## Décision autorisée

Remplacer uniquement le backend génératif local par **ONNX Runtime GenAI**, sans modifier l'ExpertEngine, la base MEMS/RAVE, le protocole ECU, l'UI générale, le 32 bits ni le numéro BUILD.

Architecture cible :

`IaMemsService -> ExpertEngine + ExpertKnowledgeReader(read-only) -> LocalAiClient -> ONNX Runtime GenAI C/C++ in-process -> Qwen3 ONNX INT4 CPU`

Conséquences recherchées :

- suppression de `llama-server.exe` ;
- suppression de `QProcess` pour l'IA ;
- suppression du serveur HTTP local et du port associé ;
- runtime CPU Windows x64 officiel et autonome ;
- modèle Qwen3 ONNX CPU quantifié ;
- génération dans un thread dédié afin de ne pas bloquer l'interface ;
- conservation des réponses déterministes, du contexte MEMS et de la base experte RAVE.

## Composants épinglés pour le premier test Windows

- ONNX Runtime GenAI **0.14.0**, asset officiel `onnxruntime-genai-0.14.0-win-x64.zip`.
- SHA-256 officiel : `8a303e52dc7be8fb2a5331929af451a25ac59774102d7fd09ef673adc85c5ebf`.
- Modèle : Qwen3-0.6B ONNX GenAI INT4 CPU, révision immuable `e6bf97818c142808967a48cbab4f0aef18b64621`, dossier `cpu_and_mobile/cpu-int4-rtn-block-32-acc-level-4`.
- Hashes critiques vérifiés avant packaging : `model.onnx`, `model.onnx.data`, `tokenizer.json`.

## Critères GO

Le workflow Windows doit obligatoirement vérifier : téléchargement/hash du runtime officiel, compilation MSVC x64, chargement réel du modèle ONNX, génération réelle d'au moins une réponse, validation du package autonome avec PATH Windows nettoyé, absence totale de `llama-server.exe`/GGML/GGUF, intégrité de la base experte r20 et smoke launch de MEMS Manager.

Toujours **BUILD #30 / v1.0.30**. Aucun BUILD #31. Aucun changement protocole ECU. Aucun changement 32 bits.
