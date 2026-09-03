## RAVEMEMS V2 — second test ciblé fallback identité SHA rouge avant extraction

- Run ciblé : `33809783625`.
- Job : `100828659140`.
- Résultat : RED dans `Apply exact generic fallback patch` avec `expected exactly one legacy publication error line, found 0`.
- Les étapes installation, extraction, replay et commit ont été skipped ; ce run n'a donc poussé aucune modification du coeur.
- Découverte importante après le run : le `ravemems/v2/core_extract.py` réellement présent au HEAD `289ab08dbb66ebdfc88dbd0d3827f99878a7ad5c` contient déjà la logique voulue : `publication_code = publication_match.group(0).upper() if publication_match else None` et ne contient plus le rejet `Expected document identity pattern not found in source PDF`.
- Blob courant du coeur : `77ec2bd68229a9166ff9c69f1081a9ca4a6a0455`.
- Le SHA ancien `7fb163cc6cc8c74b2e48c1d61bc09102db7f8817`, utilisé par le premier run corpus, contenait encore le garde fautif (blob `643391beb03f403a117e25863d813419e38b11a2`).
- Prochaine action exacte : identifier dans l'historique le commit qui a introduit le coeur corrigé, puis valider directement ce SHA sur le PDF sans publication avant de relancer les 47 PDF.
- Aucun changement `MEMSX64`; production protégée #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`.
