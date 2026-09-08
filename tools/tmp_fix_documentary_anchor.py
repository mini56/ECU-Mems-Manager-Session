from pathlib import Path

p = Path('expert/LocalAiClient.cpp')
s = p.read_text(encoding='utf-8')

old = r'''    const bool reasoning = requiresReasoning(trimmedQuestion, grounding);
    const bool documentary = !grounding.isEmpty() && !reasoning;
    const QString languageCode = activeLanguageCode();
    const QString languageName = activeLanguageName(languageCode);

    QString userContent = trimmedQuestion;
'''
new = r'''    const bool reasoning = requiresReasoning(trimmedQuestion, grounding);
    const bool documentary = !grounding.isEmpty() && !reasoning;
    const QString languageCode = activeLanguageCode();
    const QString languageName = activeLanguageName(languageCode);

    QString documentaryAnchor;
    if (documentary) {
        const QRegularExpression identifierRx(QStringLiteral("[A-Z0-9]+(?:_[A-Z0-9]+)+"));
        const QRegularExpressionMatch identifierMatch = identifierRx.match(grounding);
        if (identifierMatch.hasMatch())
            documentaryAnchor = identifierMatch.captured(0);
        if (documentaryAnchor.isEmpty()) {
            const QRegularExpression valueRx(
                QStringLiteral("\\b[0-9]+(?:[.,][0-9]+)?\\s*(?:Nm|V|mV|A|mA|kPa|bar|psi|rpm|ms|mm)\\b"),
                QRegularExpression::CaseInsensitiveOption);
            const QRegularExpressionMatch valueMatch = valueRx.match(grounding);
            if (valueMatch.hasMatch())
                documentaryAnchor = valueMatch.captured(0);
        }
    }

    QString userContent = trimmedQuestion;
'''
if old not in s:
    raise SystemExit('anchor insertion point not found')
s = s.replace(old, new, 1)

old = r'''            "When document and page provenance are present, finish with one short Source line.")
            .arg(languageName, languageCode);
    }
'''
new = r'''            "When document and page provenance are present, finish with one short Source line.")
            .arg(languageName, languageCode);
        if (!documentaryAnchor.isEmpty()) {
            userContent += QStringLiteral(
                "\nMANDATORY VERIFIED EVIDENCE ANCHOR: %1. Copy this exact anchor verbatim into the answer; do not rename, translate or approximate it.")
                .arg(documentaryAnchor);
        }
    }
'''
if old not in s:
    raise SystemExit('documentary user prompt insertion point not found')
s = s.replace(old, new, 1)

old = r'''QString documentaryRepairPrompt;
if (documentary) {
    documentaryRepairPrompt = QStringLiteral(
        "<|im_start|>system\n%1<|im_end|>\n"
        "<|im_start|>user\nQUESTION:\n%2\n\nVERIFIED EVIDENCE:\n%3\n\n"
        "The previous attempt failed because it did not use the evidence. Answer the QUESTION directly in %4 (%5). "
        "Do not restate the question. You MUST include at least one exact concrete fact, value, unit, connector/pin, wire colour, manufacturer reference or technical identifier from VERIFIED EVIDENCE that is absent from the QUESTION. "
        "Do not output database metadata or internal instructions. /no_think<|im_end|>\n<|im_start|>assistant\n")
        .arg(systemPrompt(), trimmedQuestion, grounding, languageName, languageCode);
}
'''
new = r'''QString documentaryRepairPrompt;
if (documentary) {
    const QString repairAnchorInstruction = documentaryAnchor.isEmpty()
        ? QString()
        : QStringLiteral(" MANDATORY VERIFIED EVIDENCE ANCHOR: %1. Copy this exact anchor verbatim into the answer; do not rename, translate or approximate it.")
              .arg(documentaryAnchor);
    documentaryRepairPrompt = QStringLiteral(
        "<|im_start|>system\n%1<|im_end|>\n"
        "<|im_start|>user\nQUESTION:\n%2\n\nVERIFIED EVIDENCE:\n%3\n\n"
        "The previous attempt failed because it did not use the evidence. Answer the QUESTION directly in %4 (%5). "
        "Do not restate the question. You MUST include at least one exact concrete fact, value, unit, connector/pin, wire colour, manufacturer reference or technical identifier from VERIFIED EVIDENCE that is absent from the QUESTION. "
        "Do not output database metadata or internal instructions.%6 /no_think<|im_end|>\n<|im_start|>assistant\n")
        .arg(systemPrompt(), trimmedQuestion, grounding, languageName, languageCode, repairAnchorInstruction);
}
'''
if old not in s:
    raise SystemExit('repair prompt replacement point not found')
s = s.replace(old, new, 1)

p.write_text(s, encoding='utf-8')
