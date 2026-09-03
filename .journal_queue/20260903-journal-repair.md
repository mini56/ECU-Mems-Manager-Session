## 2026-09-03 — REPARATION PERMANENTE DU CANAL DE JOURNALISATION

Incident confirme sur le run de journalisation `33786653330` : **FAILURE** dans l'etape d'ajout au rapport. Deux causes racines ont ete identifiees dans les logs :
- les accents graves Markdown contenus dans une chaine `python3 -c "..."` etaient interpretes par le shell comme des substitutions de commandes (`command not found`) ;
- `RAPPORT_SUIVI_ECU_MEMS_MANAGER.md` contient au moins un octet historique non UTF-8, ce qui faisait echouer `Path.read_text(encoding='utf-8')` avec `UnicodeDecodeError`.

Decision permanente : **abandon des workflows temporaires differents pour chaque entree de rapport**.

Nouveau canal unique :
- `tools/append_master_report.py` : appender binaire, idempotent par SHA-256, qui preserve octet pour octet tout le contenu historique du rapport et n'interprete jamais le Markdown dans le shell ;
- `.github/workflows/report-master-journal.yml` : workflow permanent unique, declenche uniquement par une entree dans `.journal_queue/`, avec concurrence serialisee, commit/push du rapport puis verification du blob distant et verification que la file est vide ;
- les fichiers de `.journal_queue/` sont uniquement des transports temporaires et sont supprimes par le meme commit qui ajoute leur contenu au rapport maitre ; ils ne constituent pas de nouveaux rapports.

Regle operationnelle a partir de maintenant : pour toute journalisation, creer uniquement une entree texte UTF-8 dans `.journal_queue/`; **ne plus fabriquer de YAML de journalisation ad hoc et ne plus injecter le contenu Markdown dans une commande shell**.

Cette entree sert egalement de test reel du nouveau canal. Aucun changement applicatif, RAVEMEMS, ECU, protocole ou `MEMSX64` n'est effectue pendant cette reparation.
