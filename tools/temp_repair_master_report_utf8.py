from __future__ import annotations

import hashlib
import re
import subprocess
from collections import Counter
from pathlib import Path

from ftfy import fix_text

REPORT = Path("RAPPORT_SUIVI_ECU_MEMS_MANAGER.md")
EXPECTED_BLOB = "c5397573c1b9a3ac371686338a455820c29e0144"


def git_blob(path: Path) -> str:
    return subprocess.check_output(["git", "hash-object", str(path)], text=True).strip()


def invalid_utf8_spans(data: bytes):
    spans = []
    offset = 0
    while offset < len(data):
        try:
            data[offset:].decode("utf-8")
            break
        except UnicodeDecodeError as exc:
            start = offset + exc.start
            end = offset + exc.end
            spans.append((start, end, data[start:end].hex()))
            offset = end
    return spans


def decode_preserving_valid_utf8(data: bytes) -> str:
    text = data.decode("utf-8", errors="surrogateescape")
    out = []
    for ch in text:
        code = ord(ch)
        if 0xDC80 <= code <= 0xDCFF:
            raw = bytes([code - 0xDC00])
            try:
                out.append(raw.decode("cp1252"))
            except UnicodeDecodeError:
                out.append(raw.decode("latin-1"))
        else:
            out.append(ch)
    return "".join(out)


def long_hex_tokens(text: str, n: int) -> Counter:
    return Counter(x.lower() for x in re.findall(rf"(?i)(?<![0-9a-f])[0-9a-f]{{{n}}}(?![0-9a-f])", text))


def urls(text: str) -> Counter:
    return Counter(re.findall(r"https?://[^\s`)>]+", text))


raw = REPORT.read_bytes()
actual_blob = git_blob(REPORT)
assert actual_blob == EXPECTED_BLOB, (actual_blob, EXPECTED_BLOB)
raw_sha256 = hashlib.sha256(raw).hexdigest()
spans = invalid_utf8_spans(raw)
assert spans, "The expected invalid UTF-8 condition is no longer present; aborting rather than guessing."

mixed = decode_preserving_valid_utf8(raw)
fixed_core = fix_text(mixed)

# Structural guards: encoding repair may change Unicode characters, never technical ASCII identity.
assert "\ufffd" not in fixed_core
assert not any(0xD800 <= ord(ch) <= 0xDFFF for ch in fixed_core)
assert fixed_core.count("\n") == mixed.count("\n")
assert fixed_core.count("`") == mixed.count("`")
assert long_hex_tokens(fixed_core, 40) == long_hex_tokens(mixed, 40)
assert long_hex_tokens(fixed_core, 64) == long_hex_tokens(mixed, 64)
assert urls(fixed_core) == urls(mixed)
for token in (
    "RAPPORT_SUIVI_ECU_MEMS_MANAGER.md",
    "MEMSX64",
    "RCL0193ENG",
    "c050a3eebe50c5a85bf8a69b7722bd2052079944e09d58578a498984ecf06715",
    "1d6316bd1746d6f2b4cfb751cab88d18e27ef730",
):
    assert fixed_core.count(token) == mixed.count(token), token
fixed_core.encode("utf-8", errors="strict")

bad_markers_before = sum(mixed.count(x) for x in ("Ã", "Â", "â€", "ðŸ", "ï¸"))
bad_markers_after = sum(fixed_core.count(x) for x in ("Ã", "Â", "â€", "ðŸ", "ï¸"))
assert bad_markers_after < bad_markers_before, (bad_markers_before, bad_markers_after)

marker = "## 2026-08-31 - REPARATION UTF-8 DU RAPPORT MAITRE ET CHECKPOINT RAVEMEMS"
assert marker not in fixed_core
span_text = ", ".join(f"{a}-{b - 1}:0x{hx}" for a, b, hx in spans)

appendix = f"""

{marker}

### Réparation du fichier maître

Le fichier maître était devenu impossible à lire/mettre à jour par les outils UTF-8 stricts. État exact avant réparation : Git blob `{EXPECTED_BLOB}`, {len(raw)} octets, SHA-256 brut `{raw_sha256}`. Les séquences UTF-8 valides ont été conservées ; seuls les octets réellement invalides ont été récupérés avec leur caractère Windows-1252 correspondant, puis les séquences de mojibake historiques ont été remises en Unicode lisible. Zone(s) d'octets invalides détectée(s) avant réparation : `{span_text}`.

Gardes appliqués avant écriture : nombre de lignes inchangé avant ajout de cette section, nombre de backticks inchangé, mêmes ensembles de SHA-1/identifiants Git 40 hex, mêmes SHA-256 64 hex, mêmes URL et mêmes références techniques critiques. Le résultat est réencodé en UTF-8 strict sans caractère de remplacement. Les sections historiques ne sont pas supprimées : leurs anciennes méthodes restent lisibles comme historique mais peuvent être explicitement supplantées ci-dessous.

### RAVEMEMS - méthode canonique actuelle pour RAVE

Le nom canonique de la méthode décidée avec l'utilisateur est **`ravemems`**. Cette section supplante les anciennes règles graphiques qui imposaient un masque numéroté visible et une légende traduite dans l'image. **Les numéros artificiels visibles sont désormais interdits.**

Séparation des rôles :
- **GitHub** extrait et conserve la source constructeur, le texte source, les coordonnées, les vues/schémas/tableaux, les zones de texte humain traduisible, les repères constructeur existants, les associations repère -> texte, les couleurs/styles, les pictogrammes, les dimensions et la provenance. GitHub ne fabrique pas une image raster différente pour chaque langue et n'a pas à traduire lui-même toutes les langues finales.
- **MEMS Manager** applique la langue choisie par l'utilisateur au moment du rendu. Il remplace uniquement le texte humain traduisible, à partir des données extraites, tout en conservant les éléments techniques immuables.

Règles graphiques `ravemems` :
1. L'original constructeur reste intact et disponible comme preuve/source.
2. Si un numéro/repère constructeur existe déjà (`1`, `2`, `3`, etc.), **on garde ce numéro tel quel et on change uniquement le texte qui lui est associé**. Aucun second numéro n'est créé.
3. S'il n'existe pas de numéro exploitable, une clé stable interne peut identifier la zone, mais cette clé reste invisible pour l'utilisateur.
4. Dans une image qui contient directement du texte humain, le texte source est remplacé directement par le texte dans la langue utilisateur. Pas de mélange anglais/français et pas de légende artificielle numérotée.
5. Les valeurs, unités, références Rover, numéros de pièce/procédure, connecteurs, broches, codes fils, flèches, géométrie et autres identifiants techniques ne sont pas traduits ni modifiés.
6. **La couleur et le rôle graphique du texte sont conservés** : un texte vert reste vert, un texte noir reste noir, la graisse/style utile est conservée et les pictogrammes associés restent présents.
7. Si la traduction prend plus de place, **agrandir la zone libre/le canevas autour de la vue** et repositionner proprement le bloc. Ne pas tronquer le texte, ne pas déformer le schéma et ne pas réduire abusivement la police.
8. Un garde doit refuser un rendu où un texte humain source traduisible reste visible, où une traduction manque/déborde/se chevauche, où un identifiant interne apparaît à l'utilisateur ou où la géométrie technique est altérée.

### État des deux premiers tests `ravemems`

- **RAVE page physique 112** : cas page mixte texte + schéma avec remplacement direct des libellés. La méthode est presque correcte visuellement, mais le dernier contrôle utilisateur a signalé un détail non acceptable : le texte sous l'icône verte était tronqué. Cette page ne doit donc pas être déclarée validation graphique finale. La correction générale retenue est : texte jamais tronqué, couleur verte et pictogramme conservés, espace agrandi si nécessaire.
- **RAVE page physique 107 - ENGINE COMPARTMENT COMPONENT LOCATIONS** : cas différent avec illustration portant déjà les repères constructeur `1` à `14` et liste de textes associée. Référence correcte : l'illustration/les numéros constructeur restent inchangés ; MEMS Manager traduit seulement les textes associés aux repères. Aucun numéro artificiel n'est ajouté.

### TEST2 - prochaine action exacte après réparation du rapport

Tester maintenant un troisième cas réel, sur une autre page RAVE : **vue/illustration dont le texte humain est réellement intégré dans la partie graphique**, et non simplement une liste externe ou un texte PDF natif facilement séparé. Le test doit vérifier extraction des zones par GitHub puis simulation du rendu par MEMS Manager avec remplacement direct, conservation des couleurs/pictogrammes/repères, agrandissement de l'espace si nécessaire et zéro texte tronqué.

Ne pas industrialiser le retraitement complet de RAVE avant contrôle de ce TEST2. `main/rave/` reste la source canonique. Les anciennes pages/lots traités avec la méthode précédente ne valent pas validation sous `ravemems`. `MEMSX64` reste protégé et inchangé pendant ce test ; référence production à préserver : BUILD #103 `1d6316bd1746d6f2b4cfb751cab88d18e27ef730`.
"""

new_text = fixed_core.rstrip("\n") + appendix
new_bytes = new_text.encode("utf-8", errors="strict")
REPORT.write_bytes(new_bytes)

# Final checks after the canonical appendix is added.
REPORT.read_text(encoding="utf-8", errors="strict")
assert marker in new_text
assert "**`ravemems`**" in new_text
assert "on garde ce numéro tel quel et on change uniquement le texte qui lui est associé" in new_text
assert "agrandir la zone libre/le canevas autour de la vue" in new_text
assert "TEST2 - prochaine action exacte" in new_text
assert long_hex_tokens(new_text, 40) >= long_hex_tokens(fixed_core, 40)
assert long_hex_tokens(new_text, 64) >= long_hex_tokens(fixed_core, 64)

print(f"OLD_GIT_BLOB={EXPECTED_BLOB}")
print(f"OLD_BYTES={len(raw)}")
print(f"OLD_SHA256={raw_sha256}")
print(f"INVALID_UTF8_SPANS={spans}")
print(f"MOJIBAKE_MARKERS_BEFORE={bad_markers_before}")
print(f"MOJIBAKE_MARKERS_AFTER={bad_markers_after}")
print(f"NEW_BYTES={len(new_bytes)}")
print(f"NEW_SHA256={hashlib.sha256(new_bytes).hexdigest()}")
print(f"NEW_GIT_BLOB={git_blob(REPORT)}")
print("MASTER_REPORT_UTF8_REPAIR_PASS")
