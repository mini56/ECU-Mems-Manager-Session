# SESSION 232 — lot 1 remise en conformité multi-calculateurs
Base : SESSION 231 figée (4b465406cca3a3a619b0396bc8ca85c3af4c9dc0).
- Suppression du traitement d'affichage réservé à AANMP000/AANMP002 lors de la réponse 3 au test injecteur. Pour tout ECU et pour les deux actionneurs concernés (injecteur, bobine), l'octet reçu reste affiché, accompagné de l'avertissement d'activation physique non confirmée. Absence de réponse conservée. Aucun changement de commande, transport, garde de sécurité, DLL ou catalogue de compatibilité.
- Nouveau contrôle source + six langues : tools/test_actuator_reply_multiecu.py. Il ne constitue ni un test physique, ni un test Qt dynamique, ni une preuve de couverture de toutes les autres fonctions.
- Workflow SESSION 232 : reprend les contrôles 231, ajoute le contrôle ci-dessus, Setup 1.2.32 en brouillon de test seulement. Pas de publication MAJ.
- Statut : correction source proposée dans ce commit ; compilation, Setup et validation terrain non acquis à la rédaction. Les autres limitations du rapport principal restent ouvertes.
