# SESSION 241 — historique des défauts de température hors NA
Base 48caa89aba94bf8628f3df6b9b63b507d021f1be. Demande utilisateur du 28/11 : défaut de sonde détecté pendant la session, sur MEMS 1.6 NA et 1.9, sans privilège AANMP002 ; conservation de toutes les fonctions.
## État de départ
234 et 236–239 terminées succès ; 235 échec ancien corrigé en 236 ; 240 encore en fabrication Setup lors de la préparation. Aucun nouveau run en parallèle : commit [skip ci] pour préparer le lot et son workflow.
## Vérification
mems16na_session_evidence.h réservait l'historique à profile_ NA. Le diagnostic générique n'avait aucune ligne historique équivalente. Le fichier de recherche matrice_capteurs.csv contient 215 entrées clnt MEMS 1.9, dont 125 avec flag courant 003F/08, 89 avec 003F/10 et une avec 0035/10 ; 214 mémorisés 003C/02 et un 0032/02. Ces résultats statiques de producteurs ne démontrent pas la transmission du bit ni la substitution active. La qualification de transmission existante dans mems217_passive.h (KBHBG009, KBHBV002) est réutilisée avec identité exacte ; les autres variantes ne sont pas exclues de la collecte, mais leur attribution à un circuit reste non confirmée.
## Modifications
- Les 123 profils NA conservent l'historique existant.
- Toutes les lectures ordinaires non-NA, y compris non identifiées, peuvent mémoriser la première apparition observée du bit historique, si la trame contient le champ, sa longueur est valide et la valeur correspond à la lecture DLL. Distinction entre défaut déjà mémorisé à la première lecture et apparition observée en session. Pas de nouvelle commande, aucune écriture ECU.
- Ligne et export de diagnostic générique : appellation de défaut de sonde uniquement pour deux profils 1.9 qualifiés ; indicateur historique explicitement non confirmé sur les autres. Six langues.
- Tests autonomes : 123 NA existants plus identités 1.9 confirmées, variantes non qualifiées, identité contradictoire, trames manquantes, désaccord DLL, réinitialisation, première lecture anonyme. Tests Qt : ligne et rapport qualifié/non qualifié.
## Limites
Ce lot ne démontre pas la cartographie de transmission de tous les 215 MEMS 1.9. La détection de substitution actuellement active reste ouverte et distincte du défaut mémorisé. Aucun changement 2J/3/Lucas 5AS, ni DLL. La qualification des autres fonctions et la recette utilisateur complète restent ouvertes. Tests et Setup 241 non exécutés au moment de l'enregistrement. La préparation sans déclenchement évite un nouveau run concurrent tant que 240 n'est pas terminée.

## Complément avant lancement du même run 241
Inspection de la 240 : le délai de 3 s effaçait aussi la référence capturée alors qu'il ne prouve pas un changement de calculateur ; une notification répétée du même identifiant reconstruisait le diagnostic à partir de l'ancien échantillon, y compris après expiration. Correction source : distinction expiration / perte de connexion ; conservation de la référence dans la même connexion après expiration, effacement à la perte explicite ou au changement d'identité. La notification d'identité ne redéclenche plus de diagnostic. Tests Qt ajoutés pour le maintien de référence après expiration, le même ID sans rafraîchissement fictif, MEMS 2J/3, puis effacement à la perte réelle. Ces tests restent à exécuter. Pas de nouvelle version ni de run concurrent.

## Lancement séquentiel
Run 240 (46182183046) terminé avec succès : tests autonomes et Windows, Setup, installation et démarrage. Runs 234 et 236–239 également terminés. Lancement de la validation cumulative 241 par modification du workflow, sans autre run concurrent sur cette branche. La 241 ne constitue pas une recette finale : qualification des autres firmwares, contrôles historiques et essais matériels restent ouverts.
