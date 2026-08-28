from pathlib import Path

path = Path('RAPPORT_SUIVI_ECU_MEMS_MANAGER.md')
text = path.read_text(encoding='utf-8')
marker = '# TEST RÉEL BUILD #100 ET CORRECTION BUILD #101 — 28 AOÛT 2026'
if marker in text:
    print('Report block already present')
    raise SystemExit(0)

block = r'''

# TEST RÉEL BUILD #100 ET CORRECTION BUILD #101 — 28 AOÛT 2026

Cette section **supplante comme état courant** la section `PROCHAINE ACTION EXACTE — APRÈS BUILD #100 VERT`. Les sections précédentes restent conservées pour l’historique.

## TEST RÉEL UTILISATEUR BUILD #100

Artefact testé : **`ECU-MEMS-Manager-x64-BUILD-100-v1.0.100`**, HEAD `MEMSX64` **`cd6e52c714ee35a3fffb405797f14a58c2da4fa5`**.

Résultats confirmés sur le PC réel :
- **`Broche MAP Mini MPi 1997` : VALIDÉ**. Réponse ciblée MPi : C186, fils RG/YP/KB, ECU C159-36/C159-8 et retour C159-13 ; aucun fait SPi Japon/CKP/bobine parasite.
- **`Broche MAP Mini SPi Japan 1997` : VALIDÉ**. IA explique correctement que, sur la Mini SPi documentée, le MAP est intégré au calculateur MEMS et relié au collecteur par une durite de dépression ; **aucune broche de signal MAP externe ne doit être inventée**.
- **`Couple de serrage sonde ECT` : VALIDÉ**. Réponse constructeur ciblée **15 Nm**, procédure cohérente, sans IAT, doublon ni fuite interne.
- **Fiches XML MEMS 1.6 / 1.9 : rendu VALIDÉ**. Les couleurs de fils sont visibles et les rectangles/fonds gris rendent notamment le noir lisible ; la correction graphique #99 est préservée dans #100.
- **`Broche MAP Mini` → `CHERCHE` : comportement de preuve VALIDÉ**. ECU déconnecté et sans référence forte, IA refuse de choisir arbitrairement SPi/MPi et demande une référence ECU ou année + marché.
- **`Valeur MAP ?` posée seule après relance : VALIDÉ**. ECU déconnecté => `Je n'ai encore aucune mesure ECU disponible.` Le routage de mesure MAP live lui-même est donc correct.

### Défauts réels restant dans #100

1. **Couleurs lambda — PARTIEL** : `Couleur des fils sonde lambda` remonte notamment BG noir/vert (commande relais C159-36) et B noir (blindage C159-29), mais pas les deux voies signal disponibles dans la base : C159-7 `S` gris (+VE) et C159-18 `LGS` vert clair/gris (-VE).
2. **Documentation MEMS 1.9 — réponse doublée** : la fiche est bien proposée et son rendu est correct, mais le texte IA énumère deux assets locaux (connecteur ECU + OBD 16 broches) alors que la fiche XML est déjà proposée comme ressource documentaire.
3. **Contexte de clarification non fermé** : après `Broche MAP Mini` → `CHERCHE` sans preuve, une question suivante `Valeur MAP ?` peut être polluée par l’ancienne clarification et redemander SPi/MPi. Après redémarrage, `Valeur MAP ?` seule fonctionne normalement.

## CAUSES EXACTES AUDITÉES DANS #100

- `expert/IaMemsService.cpp` : `knowledgeQueryKind()` reconnaissait `fil` au singulier mais pas **`fils` au pluriel**. `Couleur des fils sonde lambda` restait donc en classement général au lieu du chemin WireColor spécialisé.
- Le chemin documentaire MEMS 1.9 laissait `knowledgeAnswer()` énumérer les deux assets techniques alors que l’UI propose déjà la fiche XML ; la demande de fiche/documentation devait être distinguée d’une demande d’illustrations individuelles.
- `iamemstab.cpp` : dans `sendQuestion()`, `m_pendingClarificationQuestion.clear()` était exécuté après plusieurs branches avec `return`. Le cas `CHERCHE` non résolu quittait donc la fonction avant nettoyage et laissait la question de brochage en attente.

## CORRECTION CANDIDAT TEMPORAIRE

Branche de validation utilisée : **`tmp-fix100-realtest-final`**, créée depuis exactement #100 `cd6e52c714ee35a3fffb405797f14a58c2da4fa5`.

Workflow temporaire : **`TEMP FIX100 REALTEST VALIDATION`**, run **`33177675749`**, job **`98870422470`** — **SUCCESS**.

Validations vertes :
- portée exacte des fichiers source ;
- protections protocole existantes ;
- Qt 5.15.2 MSVC 2019 x64 ;
- configuration x64 ;
- compilation de `ecu_mems_manager` et `ia_response_selftest` ;
- nouveaux tests déterministes IA ;
- suppression finale des fichiers temporaires de validation.

Arbre final temporaire validé : **`3c023d5c548a4d98ece5762789a83c65b0a31213`**.

## BUILD #101 — CORRECTION DES DÉFAUTS RÉELS #100

Commit production propre : **`22dbe75ed14e0a61e694159d505ef72245116b48`** — `Build #101: fix #100 real-test IA regressions`.

Parent direct : **#100 `cd6e52c714ee35a3fffb405797f14a58c2da4fa5`**. `MEMSX64` avancée **sans force**.

Comparaison #100 → #101 : **1 commit, exactement 4 fichiers** :
1. `expert/IaMemsConversationRouting.h` — +24 / -0 ;
2. `expert/IaMemsService.cpp` — +6 / -7 ;
3. `expert/IaResponseLogicTest.cpp` — +6 / -0 ;
4. `iamemstab.cpp` — +1 / -0.

Aucun changement protocole ECU, acquisition/RAM, UI générale/responsive, base RAVE, Qwen/ONNX, workflow de production ou 32 bits.

### Correction fonctionnelle #101

- ajout d’un classifieur commun de demande de couleur de fil reconnaissant **`fil` et `fils`**, utilisé par `IaMemsService` ;
- ajout d’une détection distincte des demandes de **fiche/documentation MEMS X.X** : réponse documentaire unique et bouton de fiche, sans énumération doublée des assets ECU/OBD ;
- consommation/effacement immédiat de la clarification en attente avant toute branche susceptible de sortir par `return` ; la question suivante ne doit plus hériter du brochage précédent ;
- tests de régression pour `Couleur des fils sonde lambda`, la fiche MEMS 1.9 unique et `Valeur MAP ?` sans clarification SPi/MPi.

## BUILD OFFICIEL #101 — VALIDATION GITHUB ACTIONS

Workflow officiel **BUILD**, run **`33178021830`**, job **`98871634811`**, run number **101**, HEAD **`22dbe75ed14e0a61e694159d505ef72245116b48`** — **SUCCESS**.

Toutes les étapes principales sont vertes : protections protocole, compilation x64 ONNX native, self-tests déterministes, base expert r20, modèle Qwen3-0.6B ONNX INT4 et hashes, LocalAiClient natif, assemblage/validation package, LocalAiClient packagé, smoke launch, manifeste/hashes et upload.

Artefact officiel :
- nom : **`ECU-MEMS-Manager-x64-BUILD-101-v1.0.101`** ;
- ID : **`9688684682`** ;
- taille : **386 852 607 octets** ;
- digest : **`sha256:9374152a904feb7d46a99089bd854d2c385e7d77279607296f6f73aa80a05df3`**.

## ÉTAT COURANT

- Production x64 : **BUILD #101 / v1.0.101**.
- HEAD `MEMSX64` : **`22dbe75ed14e0a61e694159d505ef72245116b48`**.
- #101 est validé côté CI/package/smoke, **mais les trois corrections ci-dessus ne sont pas encore déclarées validées sur le PC réel**.
- Aucun test ECU connecté supplémentaire n’a été effectué dans cette correction.

# PROCHAINE ACTION EXACTE — APRÈS BUILD #101 VERT

Tester sur le PC réel **uniquement les régressions corrigées**, puis les contrôles de non-régression essentiels :
1. `Couleur des fils sonde lambda` → vérifier que les faits de signal +VE/-VE sont correctement privilégiés/étiquetés avec relais et blindage, sans description HO2S générique en tête ;
2. `Je cherche la documentation MEMS 1.9` → une réponse documentaire unique + fiche proposée, sans double liste ECU/OBD ;
3. `Broche MAP Mini` → `CHERCHE` sans preuve → puis `Valeur MAP ?` **sans relancer l’application** : la dernière question doit être indépendante et répondre absence de mesure si ECU déconnecté ;
4. contrôle rapide : MAP MPi 1997 reste ciblé ; MAP SPi reste MAP intégré sans broche externe ; ECT reste 15 Nm ; rectangles gris/couleurs XML restent corrects.

**Référence de reprise obligatoire : `MEMSX64` BUILD #101 `22dbe75ed14e0a61e694159d505ef72245116b48`, run `33178021830`, artefact `9688684682`.**
'''

if not text.endswith('\n'):
    text += '\n'
path.write_text(text + block.lstrip('\n'), encoding='utf-8')
print('PASS appended #100 real-test / #101 report block')
