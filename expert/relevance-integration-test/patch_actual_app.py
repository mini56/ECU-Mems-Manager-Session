from pathlib import Path

# Temporary integration-only patch: validate a language-neutral relevance score
# inside the real BUILD108 application before changing the permanent bridge.
bridge = Path('expert/IaMemsLibraryBridge.cpp')
bridge_text = bridge.read_text(encoding='utf-8')
old_score = r'''int evidenceScore(const EvidenceCandidate &result,
                  const QString &query,
                  const QStringList &keywords)
{
    const QString searchable = searchableText(result);
    const QStringList queryTerms = normalizedTerms(query);
    if (queryTerms.isEmpty())
        return -1;

    int referenceBoost = 0;
    // MEMSLibrary uses substring LIKE matching. Accept ordinary terms only as
    // exact tokens (rejecting axial/coaxial). Manufacturer references use the
    // generic underscore form present in RAVEMEMS entity keys.
    for (const QString &term : queryTerms) {
        if (isManufacturerReferenceAlias(term)) {
            if (!containsManufacturerReference(result, term))
                return -1;
            referenceBoost += 500;
            continue;
        }
        if (!containsExactToken(searchable, term))
            return -1;
    }

    int score = queryTerms.size() * 100 + referenceBoost;
    QSet<QString> scoredKeywords;
    for (const QString &keyword : keywords) {
        const QString normalized = normalizeForMatching(keyword);
        if (normalized.isEmpty() || scoredKeywords.contains(normalized))
            continue;
        scoredKeywords.insert(normalized);
        if (isManufacturerReferenceAlias(normalized)) {
            if (containsManufacturerReference(result, normalized))
                score += 24;
        } else if (containsExactToken(searchable, normalized)) {
            score += 12;
        }
    }

    if (result.entityKind == QStringLiteral("step"))
        score += 8;
    else if (result.entityKind == QStringLiteral("requirement"))
        score += 7;
    else if (result.entityKind == QStringLiteral("notice"))
        score += 6;
    else if (result.entityKind == QStringLiteral("operation"))
        score += 5;
    else if (result.entityKind == QStringLiteral("section"))
        score += 4;

    // For an explicit operation-reference lookup, keep the operation heading
    // ahead of its steps so Qwen receives the procedure identity as context.
    bool referenceQuery = false;
    for (const QString &term : queryTerms) {
        if (isManufacturerReferenceAlias(term)) {
            referenceQuery = true;
            break;
        }
    }
    if (referenceQuery && result.entityKind == QStringLiteral("operation"))
        score += 30;

    if (!result.title.trimmed().isEmpty())
        score += 2;
    return score;
}
'''
new_score = r'''int evidenceScore(const EvidenceCandidate &result,
                  const QString &query,
                  const QStringList &keywords)
{
    const QString searchable = searchableText(result);
    const QString searchableTitle = normalizeForMatching(result.title);
    const QStringList queryTerms = normalizedTerms(query);
    if (queryTerms.isEmpty())
        return -1;

    int score = 0;
    int matchedTerms = 0;
    bool referenceQuery = false;

    // Rank by evidence actually present, not by the raw number of words in a
    // natural-language sub-query. This is deliberately language-neutral:
    // longer exact terms carry more information, title hits are stronger, and
    // missing conversational words do not eliminate a technically good row.
    // Explicit manufacturer references remain strict provenance constraints.
    for (const QString &term : queryTerms) {
        if (isManufacturerReferenceAlias(term)) {
            referenceQuery = true;
            if (!containsManufacturerReference(result, term))
                return -1;
            ++matchedTerms;
            score += 600;
            continue;
        }

        if (!containsExactToken(searchable, term))
            continue;

        ++matchedTerms;
        const int specificity = qBound(1, term.size() - 2, 10);
        score += 16 + (specificity * 7);
        if (containsExactToken(searchableTitle, term))
            score += 28 + (specificity * 5);
    }

    if (matchedTerms == 0)
        return -1;

    // Coverage helps a page matching several independent terms, while a
    // generic two-word phrase cannot win merely because both words occur.
    score += matchedTerms * matchedTerms * 8;
    if (matchedTerms == queryTerms.size() && queryTerms.size() > 1)
        score += 24 + (queryTerms.size() * 4);

    QSet<QString> scoredKeywords;
    for (const QString &keyword : keywords) {
        const QString normalized = normalizeForMatching(keyword);
        if (normalized.isEmpty() || scoredKeywords.contains(normalized))
            continue;
        scoredKeywords.insert(normalized);
        if (isManufacturerReferenceAlias(normalized)) {
            if (containsManufacturerReference(result, normalized))
                score += 24;
        } else if (containsExactToken(searchable, normalized)) {
            const int specificity = qBound(1, normalized.size() - 2, 10);
            score += 4 + (specificity * 2);
        }
    }

    if (result.entityKind == QStringLiteral("step"))
        score += 8;
    else if (result.entityKind == QStringLiteral("requirement"))
        score += 7;
    else if (result.entityKind == QStringLiteral("notice"))
        score += 6;
    else if (result.entityKind == QStringLiteral("operation"))
        score += 5;
    else if (result.entityKind == QStringLiteral("section"))
        score += 4;

    if (referenceQuery && result.entityKind == QStringLiteral("operation"))
        score += 30;

    if (!result.title.trimmed().isEmpty())
        score += 2;
    return score;
}
'''
if old_score not in bridge_text:
    raise SystemExit('bridge evidenceScore insertion point not found')
bridge.write_text(bridge_text.replace(old_score, new_score, 1), encoding='utf-8')
print('TEMP_BRIDGE_LANGUAGE_NEUTRAL_RELEVANCE_PATCHED')

main = Path('main.cpp')
text = main.read_text(encoding='utf-8')
text = text.replace('#include <QStringList>\n', '#include <QStringList>\n#include <QDebug>\n#include <QFile>\n#include <QTextStream>\n')
text = text.replace('#include "navigationorderpatch.h"\n', '#include "navigationorderpatch.h"\n#include "expert/IaMemsService.h"\n')
main_needle = '''int main(int argc, char *argv[])\n{\n    QApplication app(argc, argv);\n'''
main_replacement = '''int main(int argc, char *argv[])\n{\n    if (qEnvironmentVariableIsSet("MEMS_RELEVANCE_INTEGRATION_TEST")) {\n        qputenv("QT_PLUGIN_PATH", QByteArray("C:/Qt/5.15.2/msvc2019_64/plugins"));\n        qputenv("QT_QPA_PLATFORM_PLUGIN_PATH", QByteArray("C:/Qt/5.15.2/msvc2019_64/plugins/platforms"));\n    }\n    QApplication app(argc, argv);\n'''
if main_needle not in text:
    raise SystemExit('main QApplication insertion point not found')
text = text.replace(main_needle, main_replacement, 1)
needle = '''    QApplication::setOrganizationName("ECU Mems Manager");\n'''
probe = r'''

    if (qEnvironmentVariableIsSet("MEMS_RELEVANCE_INTEGRATION_TEST"))
    {
        const QString tracePath = qEnvironmentVariable("MEMS_RELEVANCE_TRACE_FILE");
        const auto trace = [&tracePath](const QString &line) {
            if (tracePath.isEmpty())
                return;
            QFile file(tracePath);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
                return;
            QTextStream stream(&file);
            stream.setCodec("UTF-8");
            stream << line << '\n';
            stream.flush();
            file.flush();
        };

        trace(QStringLiteral("PROBE_START"));
        I18n::load(QStringLiteral("en"));
        I18n::install(&app);
        trace(QStringLiteral("SERVICE_CREATE_BEGIN"));
        IaMemsService *service = IaMemsService::instance();
        trace(QStringLiteral("SERVICE_CREATE_END"));

        auto checkEvidence = [&](const QString &label,
                                 const QString &question,
                                 const QString &document,
                                 int page,
                                 const QStringList &required,
                                 const QStringList &forbidden) -> bool {
            trace(QStringLiteral("CASE_BEGIN=%1").arg(label));
            service->askWithLibrary(question);
            trace(QStringLiteral("CASE_RETURNED=%1").arg(label));
            const QString evidence = service->property("iaMemsLastLibraryEvidence").toString();
            trace(QStringLiteral("EVIDENCE_BEGIN=%1").arg(label));
            trace(evidence);
            trace(QStringLiteral("EVIDENCE_END=%1").arg(label));
            qInfo().noquote() << "RELEVANCE_CASE=" << label;
            qInfo().noquote() << "QUESTION=" << question;
            qInfo().noquote() << "EVIDENCE_BEGIN\n" << evidence << "\nEVIDENCE_END";

            const QString sourceNeedle = QStringLiteral("Source %1, page %2")
                                             .arg(document)
                                             .arg(page);
            if (!evidence.contains(sourceNeedle, Qt::CaseInsensitive)) {
                trace(QStringLiteral("FAIL missing target provenance %1").arg(sourceNeedle));
                qCritical().noquote() << "FAIL missing target provenance" << sourceNeedle;
                return false;
            }
            for (const QString &needleText : required) {
                if (!evidence.contains(needleText, Qt::CaseInsensitive)) {
                    trace(QStringLiteral("FAIL missing required evidence %1").arg(needleText));
                    qCritical().noquote() << "FAIL missing required evidence" << needleText;
                    return false;
                }
            }
            for (const QString &needleText : forbidden) {
                if (evidence.contains(needleText, Qt::CaseInsensitive)) {
                    trace(QStringLiteral("FAIL contaminated evidence %1").arg(needleText));
                    qCritical().noquote() << "FAIL contaminated evidence" << needleText;
                    return false;
                }
            }
            trace(QStringLiteral("CASE_PASS=%1").arg(label));
            return true;
        };

        const bool primaryOk = checkEvidence(
            QStringLiteral("natural_primary_en"),
            QStringLiteral("How do I check the primary gear end float?"),
            QStringLiteral("DOC_RCL0193ENG"),
            53,
            {QStringLiteral("0.089"), QStringLiteral("0.165"), QStringLiteral("feeler gauges")},
            {QStringLiteral("page 342"), QStringLiteral("coaxial"), QStringLiteral("aerial")});

        const bool batteryOk = checkEvidence(
            QStringLiteral("natural_battery_en"),
            QStringLiteral("How do I carry out the battery restoration procedure?"),
            QStringLiteral("DOC_RCL0221ENG"),
            20,
            {QStringLiteral("BATTERY RESTORATION PROCEDURE")},
            {});

        if (!primaryOk || !batteryOk) {
            trace(QStringLiteral("PROBE_FAIL"));
            return 5;
        }

        trace(QStringLiteral("PROBE_PASS"));
        qInfo().noquote() << "ACTUAL_MEMS_MANAGER_MEMSLIBRARY_RELEVANCE_PASS primary=DOC_RCL0193ENG:p53 battery=DOC_RCL0221ENG:p20";
        return 0;
    }
'''
if needle not in text:
    raise SystemExit('main integration insertion point not found')
text = text.replace(needle, needle + probe, 1)
main.write_text(text, encoding='utf-8')
print('ACTUAL_APP_RELEVANCE_INTEGRATION_PATCHED')
