## IA MEMS — correction ciblée après tests PC FREIN / CULASSE

Date : 2026-09-06

Base de reprise validée pour cette étape :
- branche de test : `tmp-ia-clarification-test-1b106ee`
- état précédent testé : `acca4fc1a1b158f5595bba588675be25050cecd9`
- run précédent : `33984083333` — SUCCESS

Constats utilisateur sur PC réel :
- `FREIN` : la clarification fonctionne et plusieurs libellés sont devenus lisibles, mais `RCL0193FRE` restait exposé comme choix interne et `Freins boulons d etrier de frein sur porte fusee` restait mal présenté.
- `CULASSE` : la clarification fonctionne, mais certains choix mélangeaient des fragments de procédure (`alternateur`, `ventilation carburant`) ou exposaient le numéro technique `12.29.02`.
- Après sélection de `culasse écrous séquence de serrage`, le bon fait était bien présent (`34 N.m` puis serrage final `68 N.m`) mais noyé dans le préambule interne RAVEMEMS, les métadonnées `DOC/REV/type`, les champs `Spécification`, les illustrations/provenances et de longues sections sans rapport direct.

Autorisation utilisateur explicite : corriger ces deux défauts, mettre à jour le rapport et pousser un build correctif.

Correction poussée :
- commit : `8aa57b4b3afac8771ed9146357eeb88f57bcc677`
- fichier programme modifié par rapport à `acca4fc1...` : `database/MemsSearchCompletenessPatch.cpp` uniquement.

Périmètre fonctionnel de la correction :
- rejeter les identifiants documentaires internes dans les choix (`RCL...`, `SRC-RCL...`, `KNOW-RCL...`, `DOC-RCL...`, `REV-RCL...`) ;
- nettoyer les intitulés métiers confirmés par les tests FREIN / CULASSE ;
- fusionner les doublons de libellés après nettoyage ;
- conserver la requête technique séparée du libellé utilisateur ;
- nettoyer la réponse documentaire avant affichage : masquer par défaut le préambule interne, `DOC/REV`, `Source`, `Spécification`, `Illustration locale`, niveau de preuve et autres métadonnées ;
- sélectionner le bloc documentaire le plus pertinent par rapport à la question raffinée ;
- pour les couples/serrages et jeux, privilégier une réponse numérique courte ;
- cas écrous de culasse : rendre explicitement `34 N.m` puis serrage final `68 N.m` dans l'ordre indiqué ;
- conserver les sources/provenances seulement lorsqu'elles sont explicitement demandées ;
- remplacer de façon contrôlée les trois connexions `IaMemsService -> IaMemsTab` afin d'appliquer le nettoyage une seule fois, sans doubler la réponse, tout en conservant `responseReady`, `systemMessage` et `statusChanged`.

Hors périmètre, non modifié : Qwen, ONNX Runtime GenAI, MEMSLibrary/Pack001, contenu des bases documentaires, logique visuelle/images, scroll, protocole ECU, acquisition, commandes ECU.

Build correctif déclenché automatiquement par le push :
- run : `34018952004`
- head : `8aa57b4b3afac8771ed9146357eeb88f57bcc677`
- workflow : `TMP IA Clarification Test from BUILD105`
- état au moment de cette journalisation : IN PROGRESS

Aucun verdict de validation PC n'est inscrit avant compilation verte puis test utilisateur réel.
