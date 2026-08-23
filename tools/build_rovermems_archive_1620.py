#!/usr/bin/env python3
from pathlib import Path
import base64, json

ROOT = Path(__file__).resolve().parents[1]
REF = ROOT / "database" / "reference"
AUDITS = REF / "audits"

QZ64_B64 = """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"""

AUDIT_MD = r"""# Audit exact du ZIP RoverMEMS 386 fourni par l’utilisateur

## Archive auditée

- Fichier : `rover-mems-386(1).zip`
- SHA-256 : `8f1272530c2f850461d270ab41dfdfd5ddea9bc0469782eea0d6bc2d1b8aba7a`
- Entrées ZIP : **29**
- Fichiers : **27**
- Taille décompressée totale : **10 295 700 octets**
- Interface incluse : **App version 1.2.0**
- Exécutable : `rover-mems_386.exe`
- SHA-256 EXE : `18bf3d4b1eea047d785f50fae9c1b2ffdcec8a1b19f38e2300d94c9b340e2fd9`
- Taille EXE : **7 151 616 octets**
- Timestamp ZIP EXE : **14/03/2021 17:42:54**
- Chaîne imprimable trouvée dans l’EXE : `1.2.015`

Niveau de vérification de la source : `source_externe`.

## Identification de la génération du code

L’archive correspond très fortement à `james-portman/rover-mems-agent` au commit
`bf48bac0d43dd11894f22213b6cb69dbd4884e0c` du **14/03/2021 17:42:47 UTC**.

Preuve utilisée :
- **11 fichiers texte `web-static`** du ZIP donnent exactement le même Git blob SHA-1 que ce commit après normalisation CRLF → LF ;
- le timestamp de l’EXE du ZIP est seulement **7 secondes** après le commit ;
- le parseur de mesures associé est `ecu-1x-shared.go`, blob `792eb8f4eb5603ef8121a2972f46dd32ce3c0b97`.

C’est une **preuve forte de génération/source**, mais pas une preuve cryptographique que chaque octet de l’EXE a été compilé depuis chaque fichier de ce commit.

## Mesures live intégrées depuis la génération exacte du ZIP

Lot : `research_enrichment_1620.qz64`

| Données structurées | Lignes |
|---|---:|
| Snapshot archive | 1 |
| Fichiers de l’archive | 27 |
| Fichiers web recoupés exactement avec le commit | 11 |
| Mesures scalaires `0x80` / `0x7D` | **31** |
| Bits d’état / défauts | **24** |
| Octets explicitement inconnus | **19** |
| Différences de révision tracées | 4 |

### Trame `0x80`

La génération exacte du ZIP expose notamment :
RPM, température liquide, température ambiante candidate, température admission,
température carburant candidate, MAP, tension batterie, tension TPS, état ralenti,
park/neutral, consigne ralenti, HotDB candidate, position IAC, déviation ralenti,
offset avance candidat, **avance brute**, et dwell bobine.

Les mots 16 bits `RPM`, déviation ralenti et dwell sont décodés **high-byte puis low-byte**.

### Trame `0x7D`

La génération exacte du ZIP expose notamment :
contact, angle papillon, A/F candidate, états lambda, lambda, boucle fermée,
`long_term_trim`, `short_term_trim_percent`, purge canister candidate,
synchro primaire, position de base ralenti candidate, erreur ralenti candidate,
drivers injecteurs et compteur vilebrequin candidat.

`0x7D:0x0B` reste `long_term_trim` dans cette génération et **ne doit pas être utilisé comme temps d’injection total**.

## Différence importante par rapport au code RoverMEMS plus récent utilisé lors du premier audit 1610

Dans **ton ZIP de 2021**, `0x80:0x16` est seulement publié sous :
`ignition_advance_raw = raw`.

Le **25/05/2022**, le commit `befb6e6ef5332deb96885ef32b7fb80f1b24baef`
a ajouté une nouvelle sortie calculée :
`ignition_advance = raw/2`.

Cette conversion postérieure n’est donc **pas attribuée à ton archive**.
Elle reste séparée dans la base comme évolution ultérieure.

## Conflits et incertitudes conservés

- `0x80:0x09` TPS : l’implémentation fait `raw/200`, alors que le commentaire adjacent annonce `0.02 V/LSB`.
- `0x80:0x0A` idle switch : le code masque `0x1000` sur un octet 8 bits ; le commentaire indique bit 4.
- `0x7D:0x06` lambda : la génération exacte du ZIP utilise `raw*5`; la documentation Rover externe utilisée dans `1610` fournit une échelle contradictoire.
- `0x80:0x16` avance : la génération du ZIP ne fournit qu’une valeur brute ; son commentaire propose une conversion candidate différente.
- Les **19 octets inconnus** restent inconnus : aucune signification n’est inventée.

## Relation avec le lot 1610

`1610` = synthèse RoverMEMS/documentation avec provenance externe et conflits.

`1620` = **snapshot exact de l’archive utilisateur de mars 2021**, avec empreintes et comportement du parseur correspondant.

`1620` **n’écrase pas** `1610`. Le futur moteur expert pourra choisir la provenance, la date et le niveau de confiance.

Aucun code protocolaire ou UI n’est modifié par ce lot.
`ui-rebuild` ne doit pas être touchée.
"""

AUDIT_JSON = {'source_key': 'user_rover_mems_386_20210314', 'verification_level': 'source_externe', 'archive': {'name': 'rover-mems-386(1).zip', 'sha256': '8f1272530c2f850461d270ab41dfdfd5ddea9bc0469782eea0d6bc2d1b8aba7a', 'entry_count': 29, 'file_count': 27, 'uncompressed_bytes': 10295700, 'app_version_from_web_ui': '1.2.0', 'executable': {'path': 'rover-mems_386.exe', 'sha256': '18bf3d4b1eea047d785f50fae9c1b2ffdcec8a1b19f38e2300d94c9b340e2fd9', 'size': 7151616, 'zip_timestamp': '2021-03-14 17:42:54', 'printable_version_string': '1.2.015'}}, 'source_generation_match': {'repository': 'james-portman/rover-mems-agent', 'commit': 'bf48bac0d43dd11894f22213b6cb69dbd4884e0c', 'commit_timestamp': '2021-03-14T17:42:47Z', 'parser_path': 'ecu-1x-shared.go', 'parser_blob': '792eb8f4eb5603ef8121a2972f46dd32ce3c0b97', 'normalized_text_assets_exactly_matching': 11, 'confidence': 'strong', 'method': '11 web-static text assets have exact Git blob SHA-1 after CRLF-to-LF normalization; ZIP EXE timestamp is 7 seconds after commit', 'caveat': 'strong source-generation evidence, not cryptographic proof that the executable was compiled from every source file in the commit'}, 'structured_counts': {'rovermems_archive_snapshot': 1, 'rovermems_archive_file': 27, 'rovermems_archive_repo_match': 11, 'rovermems_archive_live_measurement': 31, 'rovermems_archive_live_bitfield': 24, 'rovermems_archive_unknown_byte': 19, 'rovermems_archive_revision_delta': 4}, 'live_measurement_summary': {'measurements': 31, 'bitfields': 24, 'unknown_bytes': 19, 'packets': ['0x80', '0x7D'], 'critical_differences_vs_later_2023_source': ['Uploaded 2021 archive parser emits ignition_advance_raw only at 0x80:0x16.', 'Calculated ignition_advance = raw/2 was added later on 2022-05-25, so it is not attributed to this ZIP.']}, 'critical_conflicts_preserved': ['0x80:0x09 TPS: implementation raw/200 conflicts with adjacent 0.02 V/LSB comment.', '0x80:0x0A idle switch: implementation masks 0x1000 on an 8-bit byte; adjacent comment says bit 4.', '0x7D:0x06 lambda: exact ZIP-generation parser uses raw*5; external Rover documentation used in lot 1610 has a conflicting scale.', '0x80:0x16 ignition advance: ZIP-generation parser exposes raw only; comment proposes a candidate raw/2 - 24 conversion.', '0x7D:0x0B is named long_term_trim by the archive-generation parser and must not be treated as total injection pulse width.'], 'relationship_to_1610': '1620 is an exact user-archive snapshot/provenance layer. It does not overwrite 1610; later-source interpretations stay separately traceable.', 'ui_rebuild_modified': False}

def main():
    REF.mkdir(parents=True, exist_ok=True)
    AUDITS.mkdir(parents=True, exist_ok=True)

    qz = base64.b64decode(QZ64_B64)
    (REF / "research_enrichment_1620.qz64").write_bytes(qz)
    (AUDITS / "rovermems_archive_386_audit.md").write_text(AUDIT_MD, encoding="utf-8")
    (AUDITS / "rovermems_archive_386_audit.json").write_text(
        json.dumps(AUDIT_JSON, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )

    mp = REF / "manifest.json"
    manifest = json.loads(mp.read_text(encoding="utf-8"))
    manifest["database_revision"] = max(int(manifest.get("database_revision", 0)), 17)
    batches = manifest.setdefault("research_enrichment_batches", [])
    name = "research_enrichment_1620.qz64"
    if name not in batches:
        batches.append(name)
    mp.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

if __name__ == "__main__":
    main()
