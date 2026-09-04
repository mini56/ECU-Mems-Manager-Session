from pathlib import Path

path = Path("expert/IaMemsDiagramCatalog.cpp")
text = path.read_text(encoding="utf-8")


def replace_once(old: str, new: str, label: str) -> None:
    global text
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one source match, found {count}")
    text = text.replace(old, new, 1)


replace_once(
    """        if (!result.contains(word))\n            result.append(word);\n    }\n    return result;\n}""",
    """        if (!result.contains(word))\n            result.append(word);\n    }\n\n    auto appendAlias = [&result](const QString &term) {\n        if (!result.contains(term))\n            result.append(term);\n    };\n    if (text.contains(QStringLiteral(\"batterie\")))\n        appendAlias(QStringLiteral(\"battery\"));\n    if (text.contains(QStringLiteral(\"restauration\")))\n        appendAlias(QStringLiteral(\"restoration\"));\n    if (text.contains(QStringLiteral(\"pignon\")) || text.contains(QStringLiteral(\"engrenage\")))\n        appendAlias(QStringLiteral(\"gear\"));\n    if (text.contains(QStringLiteral(\"primaire\")))\n        appendAlias(QStringLiteral(\"primary\"));\n    if (text.contains(QStringLiteral(\"jeu axial\"))) {\n        appendAlias(QStringLiteral(\"end\"));\n        appendAlias(QStringLiteral(\"float\"));\n    }\n    if (text.contains(QStringLiteral(\"vilebrequin\")))\n        appendAlias(QStringLiteral(\"crankshaft\"));\n    return result;\n}""",
    "French-to-English visual aliases",
)

replace_once(
    """int runtimeCandidateScore(const QString &question,\n                          const QString &generation,\n                          const QStringList &terms,\n                          const QJsonObject &entry)""",
    """int runtimeCandidateScore(const QString &question,\n                          const QString &generation,\n                          const QStringList &terms,\n                          const QJsonObject &entry,\n                          bool strongMatch)""",
    "runtime candidate signature",
)

replace_once(
    """    if (matched == 0)\n        return -1;\n\n    const QString publication = normalize(entry.value(QStringLiteral(\"publication_code\")).toString());""",
    """    if (matched == 0 || (strongMatch && matched < 2))\n        return -1;\n\n    if (strongMatch) {\n        const QString rawContext = entry.value(QStringLiteral(\"context_text\")).toString();\n        const QString normalizedContext = normalize(rawContext);\n        if (rawContext.count(QStringLiteral(\". .\")) > 20)\n            score -= 12;\n        if (containsAny(normalizedContext, {\n                QStringLiteral(\"measure\"), QStringLiteral(\"check\"),\n                QStringLiteral(\"procedure\"), QStringLiteral(\"adjust\"),\n                QStringLiteral(\"controle\"), QStringLiteral(\"reglage\")\n            }))\n            score += 4;\n    }\n\n    const QString publication = normalize(entry.value(QStringLiteral(\"publication_code\")).toString());""",
    "strong implicit visual score",
)

replace_once(
    """IaMemsDiagramSuggestion runtimeSuggestion(const QString &question,\n                                           const QString &generation,\n                                           const QStringList &terms,\n                                           const QString &root)""",
    """IaMemsDiagramSuggestion runtimeSuggestion(const QString &question,\n                                           const QString &generation,\n                                           const QStringList &terms,\n                                           const QString &root,\n                                           bool strongMatch)""",
    "runtime suggestion signature",
)

replace_once(
    "runtimeCandidateScore(question, generation, runtimeTerms, entry);",
    "runtimeCandidateScore(question, generation, runtimeTerms, entry, strongMatch);",
    "runtime candidate call",
)

replace_once(
    """    if (!diagramIntent)\n        return IaMemsDiagramSuggestion();\n\n    const bool asksRosco""",
    """    // Explicit diagram requests keep the historical routing. For an ordinary\n    // technical question, only a strong multi-term match in the validated\n    // RAVEMEMS runtime catalog may produce a visual suggestion.\n\n    const bool asksRosco""",
    "remove explicit-only runtime gate",
)

replace_once(
    """    const IaMemsDiagramSuggestion runtime = runtimeSuggestion(text, generation, terms, root);\n    if (runtime.isValid())\n        return runtime;\n\n    QFile manifest""",
    """    const IaMemsDiagramSuggestion runtime =\n        runtimeSuggestion(text, generation, terms, root, !diagramIntent);\n    if (runtime.isValid())\n        return runtime;\n\n    // Static legacy manifest diagrams remain opt-in: no implicit fallback.\n    if (!diagramIntent)\n        return IaMemsDiagramSuggestion();\n\n    QFile manifest""",
    "implicit runtime / explicit legacy split",
)

path.write_text(text, encoding="utf-8", newline="\n")
print("TMP_IA_IMPLICIT_VISUAL_PATCH_PASS")
