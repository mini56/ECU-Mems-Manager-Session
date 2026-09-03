## 2026-09-03 — RESULTAT DU TEST ALEATOIRE R8411BU AVEC LE PIPELINE ZERO-DEFAUT INCHANGE

PDF testé au hasard : `rave/Mini Tech Bulletins/R8411BU.PDF`.

Source :
- commit `643de091b474f4e27917a065bdf46d5a0c764276`
- blob `4948dca6152b13a0e19f8acc25362b33547276a3`
- SHA256 `cddf44f1531caf1fd67fbc313ec5bfe6932a7951a48d61485489e5f78b7dd28a`
- 18 097 octets
- 2 pages

Algorithme testé sans modification : SHA validé RCL0193ENG `b812e9660f0b325abc0764517b3a3e7bf79fc3ba`. Seule l'identité documentaire du profil a été adaptée de RCL0193ENG à R8411BU afin que le même extracteur accepte la source.

Workflow : run `33796851792` — SUCCESS.

Extraction :
- documents 1
- revisions 1
- pages 2
- opérations 1
- phases 0
- étapes 0
- notices 1
- requirements 0
- specifications 0
- visuels 1
- liens visuels 1
- review flags 0
- provenance 2
- défauts de séquence numérique 0
- candidats numériques rejetés 0
- audit avant validation visuelle 2 (1 fidélité + 1 lien en attente)

Replay visuel exact :
- 1/1 visuel vérifié
- 0 échec visuel
- 1/1 lien vérifié
- 0 échec de lien
- 0 manquant
- 0 inattendu
- audit final 0
- SQLite integrity_check=ok
- 0 FK cassée

Artefact : `RAVEMEMS-V2-R8411BU-RANDOM-ZERO-DEFECT-TEST`, ID `9909501323`.

INTERPRETATION IMPORTANTE : le run est vert selon les gardes existants, mais ce bulletin technique ne produit aucune phase ni aucune étape. Ce test ne prouve donc pas que le profil de manuel d'atelier généralise correctement aux Tech Bulletins. Il révèle au contraire que l'audit zéro actuel valide la cohérence de ce qui a été extrait, mais ne garantit pas encore une couverture sémantique suffisante pour un autre type documentaire. Ne pas présenter R8411BU comme extraction sémantique complète tant qu'un garde de couverture ou un profil bulletin adapté n'a pas été validé.

Production protégée : `MEMSX64` est restée exactement sur BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`.
