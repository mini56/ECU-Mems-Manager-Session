# SESSION 219 — unités et température
Base bf94a3be7c8dae82219ece8bb2e8474de7d7fb33, SESSION 218 : première recette échouée sur ancien contrôle visuel ; second run 45991085223 annulé par utilisateur. Reprendre contrôle visuel adapté, ne pas relancer 218.
- Réglages : correction d'injection visible hors connexion, -- et unité non attribuée si profil non confirmé ; correction instantanée lambda distincte. Cadrans température °F/°C et bandes suivant l'option, effacement historique si unité change.
- µs pour correction additive des profils qualifiés dans données/résumé/diagnostic ; % seulement pour lambda qualifiée ; pas de seuil commun %/µs. Position IAC en pas dans données/résumé. Correction avance facteur 0,5 comme Réglages ; décimales angle historique. Noms voies Analyse conformes aux encodages existants, sans changer CSV. Températures sentinelles et mV lambda ; cohérence du régime signé NA.
- Sonde température : minimum/maximum, axes °C et secondes sur courbe, résultat dans rapport.
- Jauge : verrouillage 218 conservé ; séquence minimum 00, maximum 10, retour F4 modélisée par simulation autonome sans transport ; refus d'un deuxième ordre, retour non confirmé si liaison perdue. Aucun essai physique ni raccordement réel : qualification banc nécessaire avant activation.
- Audit partiel U01–U12 ; vérification exhaustive par génération, aides/IA, mode 4, données inconnues et validation terrain restent à terminer. Pas de modification DLL. Six catalogues et tests. Un seul run 219.

## Validation CI et Setup
- Run 45992016185 : success ; commit a4f6cc5aa09927a3857245c479375e149fc3d80c. Jobs Linux 140481927693 et Windows 140482086226 : success. Recette installée et Setup réussis.
- Release privée brouillon 460542346, asset ECU-Mems-Manager-Session-219-Setup.exe, 602260802 octets, SHA256 384ef476c630c835c63469cd6d6138c03c65ed27db396af1fdd91bd2ad9bccd7.
- Aucun essai sur véhicule ou banc réalisé. Jauge active verrouillée. Audit unités encore partiel. Prochain lot proposé : nouvelles mesures ordinaires 1.6 NA/1.9 comparées à l'existant, puis DTC, sans attendre la qualification physique des commandes actives. Pas de nouveau run pour ce suivi documentaire.
