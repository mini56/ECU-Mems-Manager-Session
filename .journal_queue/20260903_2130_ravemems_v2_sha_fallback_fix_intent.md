## RAVEMEMS V2 — intention de correction générique identité SHA

- Contexte : le run corpus complet `33806975993` a trouvé 47 PDF, 46 ont continué, 1 seul (`rave/library/libxn.pdf`) a échoué à l'entrée.
- Cause exacte : `probe_pdf()` calcule déjà une identité de secours stable `PDFSHA256...` quand aucune publication n'est détectée, mais `ravemems/v2/core_extract.py` arrête encore l'extraction si `publication_code_regex` ne matche pas le texte du PDF.
- Correction autorisée : rendre `publication_code` nullable dans le coeur et laisser `document_identity_token` / SHA prendre le relais lorsque la publication est inconnue.
- Interdiction : aucune règle spéciale pour `libxn.pdf`, aucun usage sémantique du chemin/nom de fichier, aucune modification de `MEMSX64`.
- Validation exigée après pousse : conserver les régressions exactes déjà validées et relancer le corpus complet des 47 PDF sur le SHA corrigé avec audit/replay strict.
- Branche active : `tmp-ravemems-v2-foundation`.
- HEAD avant correction : `7fb163cc6cc8c74b2e48c1d61bc09102db7f8817`.
- Production protégée : `MEMSX64` #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`.
