from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    p = Path(path)
    text = p.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{path}: expected one exact patch location, found {count}")
    p.write_text(text.replace(old, new, 1), encoding="utf-8")


# 1) Treat plural French "fils" as a real wire-colour documentary request.
routing_marker = '''inline bool isVariantSensitiveQuestion(const QString &question)\n{'''
routing_insert = '''inline bool isWireColourQuestion(const QString &question)\n{\n    const QString text = normalize(question);\n    const bool frenchColour = text.contains(QStringLiteral("couleur"))\n        && (containsWord(text, QStringLiteral("fil"))\n            || containsWord(text, QStringLiteral("fils"))\n            || text.contains(QStringLiteral("cable"))\n            || text.contains(QStringLiteral("cablage")));\n    return frenchColour\n        || text.contains(QStringLiteral("wire color"))\n        || text.contains(QStringLiteral("wire colour"));\n}\n\ninline bool isVariantSensitiveQuestion(const QString &question)\n{'''
replace_once("expert/IaMemsConversationRouting.h", routing_marker, routing_insert)

# 2) Recognise a generation XML/documentation request separately from diagram assets.
reference_marker = '''inline QString explicitInduction(const QString &question)\n{'''
reference_insert = '''inline bool isReferenceSheetRequest(const QString &question)\n{\n    if (requestedGeneration(question).isEmpty())\n        return false;\n    const QString text = normalize(question);\n    return containsAny(text, {\n        QStringLiteral("documentation"), QStringLiteral("fiche xml"),\n        QStringLiteral(".xml"), QStringLiteral(" xml")\n    });\n}\n\ninline QString explicitInduction(const QString &question)\n{'''
replace_once("expert/IaMemsConversationRouting.h", reference_marker, reference_insert)

# 3) Use the shared/testable wire-colour classifier in the service.
old_kind = '''    const bool asksColor = questionText.contains(QStringLiteral("couleur"))\n        && (containsWord(questionText, QStringLiteral("fil"))\n            || questionText.contains(QStringLiteral("cable"))\n            || questionText.contains(QStringLiteral("cablage")));\n    if (asksColor\n        || questionText.contains(QStringLiteral("wire color"))\n        || questionText.contains(QStringLiteral("wire colour")))\n        return KnowledgeQueryKind::WireColor;'''
new_kind = '''    if (IaMemsConversationRouting::isWireColourQuestion(questionText))\n        return KnowledgeQueryKind::WireColor;'''
replace_once("expert/IaMemsService.cpp", old_kind, new_kind)

# 4) A fiche/documentation request gets one deterministic fiche answer; the UI button
#    already owns the actual XML opening. Do not enumerate ECU+OBD assets as two answers.
old_doc = '''    if (IaMemsConversationRouting::isDocumentationQuestion(question)) {\n        const QString documentaryKnowledge = knowledgeAnswer(question);'''
new_doc = '''    if (IaMemsConversationRouting::isDocumentationQuestion(question)) {\n        if (IaMemsConversationRouting::isReferenceSheetRequest(question)) {\n            const QString generation = IaMemsConversationRouting::requestedGeneration(question);\n            return QStringLiteral("La fiche XML MEMS %1 est disponible dans le package local. Utilisez le bouton proposé pour l'ouvrir.")\n                .arg(generation);\n        }\n        const QString documentaryKnowledge = knowledgeAnswer(question);'''
replace_once("expert/IaMemsService.cpp", old_doc, new_doc)

# 5) Consume the pending clarification immediately. Any early local answer must leave
#    the next user question independent from the previous pinout clarification.
old_pending = '''    if (!m_pendingClarificationQuestion.isEmpty()) {\n        const QString pending = m_pendingClarificationQuestion;'''
new_pending = '''    if (!m_pendingClarificationQuestion.isEmpty()) {\n        const QString pending = m_pendingClarificationQuestion;\n        m_pendingClarificationQuestion.clear();'''
replace_once("iamemstab.cpp", old_pending, new_pending)

# 6) Deterministic regressions for the two routing bugs found on the real #100 package.
test_marker = '''    ok &= require(IaMemsConversationRouting::requestedGeneration(QStringLiteral("Je cherche la documentation MEMS 1.9")) == QStringLiteral("1.9"),\n                  "documentation generation extraction");'''
test_insert = '''    ok &= require(IaMemsConversationRouting::requestedGeneration(QStringLiteral("Je cherche la documentation MEMS 1.9")) == QStringLiteral("1.9"),\n                  "documentation generation extraction");\n    ok &= require(IaMemsConversationRouting::isReferenceSheetRequest(QStringLiteral("Je cherche la documentation MEMS 1.9")),\n                  "MEMS 1.9 documentation is a single reference-sheet request");\n    ok &= require(IaMemsConversationRouting::isWireColourQuestion(QStringLiteral("Couleur des fils sonde lambda")),\n                  "plural French fils is recognised as a wire-colour request");\n    ok &= require(!IaMemsConversationRouting::needsInductionClarification(QStringLiteral("Valeur MAP ?")),\n                  "standalone MAP value request never asks SPi/MPi pinout clarification");'''
replace_once("expert/IaResponseLogicTest.cpp", test_marker, test_insert)

print("PASS applied #100 real-test fixes")
