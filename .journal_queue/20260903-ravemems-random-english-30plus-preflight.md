## 2026-09-03 — RAVEMEMS V2 test d’un autre PDF anglais >=30 pages

Demande utilisateur : tester encore un autre PDF anglais de RAVE, avec un minimum de 30 pages, en conservant le pipeline générique actuellement validé.

Candidat choisi pour le préflight : `rave/xn/elxn970e.pdf`, source figée au commit `643de091b474f4e27917a065bdf46d5a0c764276`, blob Git `337f957045cb9ea79f38a1a5e3ddb01f194c7156` (3 955 242 octets). Ce PDF est distinct de `rave/xn/wmxn990e.pdf` / RCL0193ENG et de R8411BU.

Prochaine action exacte : exécuter un préflight GitHub Actions qui vérifie le nombre réel de pages et l’identité/publication du PDF. Le test complet ne doit être lancé que si `page_count >= 30`. Si ce garde passe, réutiliser le même code générique RAVEMEMS V2 validé au SHA `a9e47832a2fbd3bf365a32e00d374ed7c70fba5c`, sans changement d’algorithme, puis rapporter les défauts réels tels quels.

Protection production maintenue : `MEMSX64` doit rester sur BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`; aucun BUILD #104.
