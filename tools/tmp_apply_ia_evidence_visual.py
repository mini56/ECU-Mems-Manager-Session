from pathlib import Path


def replace_once(path, old, new):
    p = Path(path)
    text = p.read_text(encoding='utf-8')
    if old not in text:
        raise SystemExit(f'Expected patch anchor not found in {path}')
    if text.count(old) != 1:
        raise SystemExit(f'Patch anchor not unique in {path}: {text.count(old)}')
    p.write_text(text.replace(old, new, 1), encoding='utf-8')


replace_once(
    'expert/IaMemsLibraryIntegration.cpp',
    '''    const IaMemsLibraryGrounding libraryGrounding =\n        IaMemsLibraryBridge::retrieve(trimmed, libraryKeywords(trimmed));\n    m_pendingGrounding = mergeGrounding(legacyGrounding, libraryGrounding);\n''',
    '''    const IaMemsLibraryGrounding libraryGrounding =\n        IaMemsLibraryBridge::retrieve(trimmed, libraryKeywords(trimmed));\n    setProperty("iaMemsLastLibraryQuestion", trimmed);\n    setProperty("iaMemsLastLibraryEvidence", libraryGrounding.text);\n    m_pendingGrounding = mergeGrounding(legacyGrounding, libraryGrounding);\n''')

replace_once(
    'expert/IaMemsDiagramCatalog.h',
    '''    static IaMemsDiagramSuggestion suggestionForQuestion(\n        const QString &question,\n        const QString &referenceRoot = QString());\n''',
    '''    static IaMemsDiagramSuggestion suggestionForQuestion(\n        const QString &question,\n        const QString &referenceRoot = QString());\n\n    static IaMemsDiagramSuggestion suggestionForEvidence(\n        const QString &question,\n        const QString &evidence,\n        const QString &referenceRoot = QString());\n''')

p = Path('expert/IaMemsDiagramCatalog.cpp')
text = p.read_text(encoding='utf-8')
if '#include <QRegularExpression>\n' not in text:
    text = text.replace('#include <QJsonObject>\n', '#include <QJsonObject>\n#include <QRegularExpression>\n', 1)
if 'IaMemsDiagramCatalog::suggestionForEvidence(' in text:
    raise SystemExit('Evidence visual routing already exists unexpectedly')
text += r'''

IaMemsDiagramSuggestion IaMemsDiagramCatalog::suggestionForEvidence(
    const QString &question,
    const QString &evidence,
    const QString &referenceRoot)
{
    const QString evidenceText = evidence.trimmed();
    if (evidenceText.isEmpty())
        return IaMemsDiagramSuggestion();

    const QString questionText = normalize(question);
    const bool asksRosco = questionText.contains(QStringLiteral("rosco"));
    const bool asksObd = questionText.contains(QStringLiteral("obd"))
        || questionText.contains(QStringLiteral("j1962"));
    const bool genericDiagnostic = questionText.contains(QStringLiteral("diagnostic"))
        && !asksRosco && !asksObd;
    if (genericDiagnostic)
        return IaMemsDiagramSuggestion();

    const QString root = effectiveReferenceRoot(referenceRoot);
    QFile catalog(QDir(root).filePath(QStringLiteral("runtime_visual_catalog.json")));
    if (!catalog.open(QIODevice::ReadOnly))
        return IaMemsDiagramSuggestion();

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(catalog.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return IaMemsDiagramSuggestion();
    const QJsonArray entries = document.object().value(QStringLiteral("entries")).toArray();

    const QRegularExpression sourceRx(
        QStringLiteral("Source\\s+DOC_([A-Za-z0-9_]+),\\s*page\\s+(\\d+)"),
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator sources = sourceRx.globalMatch(evidenceText);

    IaMemsDiagramSuggestion best;
    int bestScore = -1;
    QString bestStableKey;
    while (sources.hasNext()) {
        const QRegularExpressionMatch source = sources.next();
        const QString publication = normalize(source.captured(1));
        const int page = source.captured(2).toInt();
        if (publication.isEmpty() || page <= 0)
            continue;

        int blockEnd = evidenceText.indexOf(QStringLiteral("\n\n"), source.capturedEnd());
        if (blockEnd < 0)
            blockEnd = evidenceText.size();
        const QString sourceBody = evidenceText.mid(source.capturedEnd(), blockEnd - source.capturedEnd());
        const QStringList terms = significantTerms(normalize(question + QLatin1Char(' ') + sourceBody));
        if (terms.isEmpty())
            continue;

        for (const QJsonValue &value : entries) {
            if (!value.isObject())
                continue;
            const QJsonObject entry = value.toObject();
            if (normalize(entry.value(QStringLiteral("source_type")).toString()) != QStringLiteral("ravemems"))
                continue;
            if (!entry.value(QStringLiteral("ui_visible")).toBool(false))
                continue;
            if (normalize(entry.value(QStringLiteral("ui_label")).toString()) != QStringLiteral("voir le schema"))
                continue;
            if (normalize(entry.value(QStringLiteral("publication_code")).toString()) != publication)
                continue;
            if (entry.value(QStringLiteral("physical_page")).toInt() != page)
                continue;

            const QString searchable = normalize(
                entry.value(QStringLiteral("runtime_key")).toString() + QLatin1Char(' ')
                + entry.value(QStringLiteral("source_occurrence_key")).toString() + QLatin1Char(' ')
                + entry.value(QStringLiteral("asset_entity_key")).toString() + QLatin1Char(' ')
                + entry.value(QStringLiteral("context_text")).toString() + QLatin1Char(' ')
                + entry.value(QStringLiteral("runtime_path")).toString());

            int matched = 0;
            int score = 0;
            for (const QString &term : terms) {
                bool numeric = true;
                for (const QChar ch : term) {
                    if (!ch.isDigit()) {
                        numeric = false;
                        break;
                    }
                }
                if (term.size() < 3 || numeric)
                    continue;
                if (searchable.contains(term)) {
                    ++matched;
                    score += term.size() >= 5 ? 10 : 8;
                }
            }
            if (matched < 2 || score < 16)
                continue;

            const QString relativePath = QDir::cleanPath(
                entry.value(QStringLiteral("runtime_path")).toString().trimmed());
            const QString displayKey = runtimeDisplayKey(entry);
            const QString stableKey = entry.value(QStringLiteral("runtime_key")).toString();
            if (score < bestScore || (score == bestScore && !bestStableKey.isEmpty()
                                      && stableKey >= bestStableKey))
                continue;

            const IaMemsDiagramSuggestion candidate = resolvePath(root, displayKey, relativePath, false);
            if (!candidate.isValid())
                continue;
            if (!fileMatchesSha256(candidate.absolutePath,
                                   entry.value(QStringLiteral("sha256")).toString()))
                continue;

            bestScore = score;
            bestStableKey = stableKey;
            best = candidate;
        }
    }
    return best;
}
'''
p.write_text(text, encoding='utf-8')

replace_once(
    'iamemstab.cpp',
    '''    if (!m_diagramButton)\n        return;\n\n    const IaMemsDiagramSuggestion suggestion =\n        IaMemsDiagramCatalog::suggestionForQuestion(question);\n''',
    '''    if (!m_diagramButton)\n        return;\n\n    m_diagramButton->setProperty("iaMemsDiagramEvidence", QString());\n\n    const IaMemsDiagramSuggestion suggestion =\n        IaMemsDiagramCatalog::suggestionForQuestion(question);\n''')

replace_once(
    'iamemstab.cpp',
    '''    const IaMemsDiagramSuggestion suggestion =\n        IaMemsDiagramCatalog::suggestionForQuestion(m_diagramQuestion);\n''',
    '''    const QString evidence = m_diagramButton\n        ? m_diagramButton->property("iaMemsDiagramEvidence").toString()\n        : QString();\n    const IaMemsDiagramSuggestion suggestion = evidence.trimmed().isEmpty()\n        ? IaMemsDiagramCatalog::suggestionForQuestion(m_diagramQuestion)\n        : IaMemsDiagramCatalog::suggestionForEvidence(m_diagramQuestion, evidence);\n''')

replace_once(
    'iamemstab.cpp',
    '''void IaMemsTab::onServiceResponse(const QString &text)\n{\n    appendMessage(QStringLiteral("IA MEMS"), text);\n\n    if (m_sendButton)\n''',
    '''void IaMemsTab::onServiceResponse(const QString &text)\n{\n    appendMessage(QStringLiteral("IA MEMS"), text);\n\n    if (m_service && m_diagramButton) {\n        const QString evidenceQuestion =\n            m_service->property("iaMemsLastLibraryQuestion").toString().trimmed();\n        const QString evidence =\n            m_service->property("iaMemsLastLibraryEvidence").toString().trimmed();\n        if (!evidenceQuestion.isEmpty() && !evidence.isEmpty()) {\n            const IaMemsDiagramSuggestion suggestion =\n                IaMemsDiagramCatalog::suggestionForEvidence(evidenceQuestion, evidence);\n            if (suggestion.isValid()) {\n                m_diagramTitle = suggestion.key;\n                m_diagramQuestion = evidenceQuestion;\n                m_diagramButton->setProperty("iaMemsDiagramEvidence", evidence);\n                m_diagramButton->setText(QStringLiteral("Voir le schéma"));\n                m_diagramButton->setVisible(true);\n            }\n        }\n    }\n\n    if (m_sendButton)\n''')


tab = Path('iamemstab.cpp').read_text(encoding='utf-8')
catalog = Path('expert/IaMemsDiagramCatalog.cpp').read_text(encoding='utf-8')
integration = Path('expert/IaMemsLibraryIntegration.cpp').read_text(encoding='utf-8')
checks = {
    'scroll_object': 'iaMemsTranscriptScroll' in tab,
    'vertical_scroll': 'new QScrollBar(Qt::Vertical, transcriptPane)' in tab,
    'scroll_sync': 'transcriptScroll, &QScrollBar::valueChanged' in tab,
    'diagram_button_text': 'QStringLiteral("Voir le schéma")' in tab,
    'response_evidence_hook': 'suggestionForEvidence(evidenceQuestion, evidence)' in tab,
    'evidence_exact_source_page': 'physical_page' in catalog and 'publication_code' in catalog and 'matched < 2' in catalog,
    'library_evidence_property': 'iaMemsLastLibraryEvidence' in integration,
}
failed = [name for name, ok in checks.items() if not ok]
for name, ok in checks.items():
    print(f'{name}={"PASS" if ok else "FAIL"}')
if failed:
    raise SystemExit('UI guard failed: ' + ', '.join(failed))
print('PATCH_AND_UI_GUARD_OK')
