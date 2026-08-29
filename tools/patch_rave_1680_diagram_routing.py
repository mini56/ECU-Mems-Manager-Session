#!/usr/bin/env python3
from pathlib import Path

path = Path("iamemstab.cpp")
text = path.read_text(encoding="utf-8")
old = '''    if (IaMemsConversationRouting::needsInductionClarification(question))
        return QStringLiteral("Pour éviter de mélanger les brochages Mini, est-ce une SPi ou une MPi ? Si tu ne sais pas, réponds « cherche » : j'utiliserai l'ECU connecté et la base avant de te redemander une information.");
    if (IaMemsConversationRouting::needsGenerationClarification(question, m_detectedFamily))
        return QStringLiteral("Quelle génération MEMS faut-il utiliser : 1.2, 1.3, 1.6 ou 1.9 ? Si tu ne sais pas, réponds « cherche » et j'utiliserai d'abord le contexte ECU disponible.");
'''
new = '''    if (IaMemsConversationRouting::needsInductionClarification(question))
        return QStringLiteral("Pour éviter de mélanger les brochages Mini, est-ce une SPi ou une MPi ? Si tu ne sais pas, réponds « cherche » : j'utiliserai l'ECU connecté et la base avant de te redemander une information.");

    // A locally packaged RAVE diagram with enough deterministic evidence is
    // already a documentary answer. Do not ask for a MEMS generation when the
    // vehicle/variant terms resolve one exact declared illustration.
    const IaMemsDiagramSuggestion diagram =
        IaMemsDiagramCatalog::suggestionForQuestion(question);
    if (diagram.isValid())
        return QString();

    if (IaMemsConversationRouting::needsGenerationClarification(question, m_detectedFamily))
        return QStringLiteral("Quelle génération MEMS faut-il utiliser : 1.2, 1.3, 1.6 ou 1.9 ? Si tu ne sais pas, réponds « cherche » et j'utiliserai d'abord le contexte ECU disponible.");
'''
count = text.count(old)
if count != 1:
    raise SystemExit(f"Expected one clarification routing block, found {count}")
path.write_text(text.replace(old, new), encoding="utf-8")
print("PASS patched RAVE diagram routing: exact local diagram bypasses generation clarification only")
