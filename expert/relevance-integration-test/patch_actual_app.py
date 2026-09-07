from pathlib import Path

# Broad validation only. The language-neutral bridge relevance fix is already
# persisted on this temporary branch; do not alter it during this benchmark.
bridge = Path('expert/IaMemsLibraryBridge.cpp')
bridge_text = bridge.read_text(encoding='utf-8')
if 'Rank by evidence actually present' not in bridge_text:
    raise SystemExit('validated language-neutral bridge relevance fix is not present')
print('PERSISTED_BRIDGE_RELEVANCE_FIX_PRESENT')

# Temporary generic query-preparation test: do not discard technical words
# merely because they occur after the first seven words of a natural question.
integration = Path('expert/IaMemsLibraryIntegration.cpp')
integration_text = integration.read_text(encoding='utf-8')
old_terms = '''    QStringList terms;\n    for (const QString &word : text.split(QLatin1Char(' '), Qt::SkipEmptyParts)) {\n        if (word.size() >= 3 && !stop.contains(word))\n            appendUnique(terms, word);\n        if (terms.size() >= 7)\n            break;\n    }\n'''
new_terms = '''    QStringList terms;\n    for (const QString &word : text.split(QLatin1Char(' '), Qt::SkipEmptyParts)) {\n        if (word.size() >= 3 && !stop.contains(word))\n            appendUnique(terms, word);\n    }\n'''
if old_terms not in integration_text:
    raise SystemExit('libraryKeywords seven-term cutoff insertion point not found')
integration.write_text(integration_text.replace(old_terms, new_terms, 1), encoding='utf-8')
print('TEMP_LIBRARY_KEYWORDS_FULL_QUESTION_PATCHED')

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

        trace(QStringLiteral("BROAD_PROBE_START"));
        I18n::load(QStringLiteral("en"));
        I18n::install(&app);
        IaMemsService *service = IaMemsService::instance();

        auto checkEvidence = [&](const QString &label,
                                 const QString &question,
                                 const QString &document,
                                 int page,
                                 const QStringList &required,
                                 const QStringList &forbidden) -> bool {
            trace(QStringLiteral("CASE_BEGIN=%1").arg(label));
            service->askWithLibrary(question);
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
                trace(QStringLiteral("FAIL %1 missing target provenance %2").arg(label, sourceNeedle));
                return false;
            }
            for (const QString &needleText : required) {
                if (!evidence.contains(needleText, Qt::CaseInsensitive)) {
                    trace(QStringLiteral("FAIL %1 missing required evidence %2").arg(label, needleText));
                    return false;
                }
            }
            for (const QString &needleText : forbidden) {
                if (evidence.contains(needleText, Qt::CaseInsensitive)) {
                    trace(QStringLiteral("FAIL %1 contaminated evidence %2").arg(label, needleText));
                    return false;
                }
            }
            trace(QStringLiteral("CASE_PASS=%1").arg(label));
            return true;
        };

        int passed = 0;
        int total = 0;
        const auto runCase = [&](bool ok) {
            ++total;
            if (ok)
                ++passed;
        };

        runCase(checkEvidence(
            QStringLiteral("control_primary_en"),
            QStringLiteral("How do I check the primary gear end float?"),
            QStringLiteral("DOC_RCL0193ENG"), 53,
            {QStringLiteral("0.089"), QStringLiteral("0.165"), QStringLiteral("feeler gauges")},
            {QStringLiteral("page 342"), QStringLiteral("coaxial"), QStringLiteral("aerial")}));

        runCase(checkEvidence(
            QStringLiteral("control_battery_en"),
            QStringLiteral("How do I carry out the battery restoration procedure?"),
            QStringLiteral("DOC_RCL0221ENG"), 20,
            {QStringLiteral("BATTERY RESTORATION PROCEDURE")}, {}));

        runCase(checkEvidence(
            QStringLiteral("rocker_clearance_en"),
            QStringLiteral("What clearance should I use when adjusting the engine valve rockers?"),
            QStringLiteral("DOC_RCL0193ENG"), 54,
            {QStringLiteral("ENGINE VALVE ROCKER ADJUSTMENT"), QStringLiteral("0.30 mm"), QStringLiteral("feeler gauge")}, {}));

        runCase(checkEvidence(
            QStringLiteral("electrical_safety_en"),
            QStringLiteral("What precautions should I take before undertaking electrical work on a vehicle?"),
            QStringLiteral("DOC_RCL0213ENG"), 9,
            {QStringLiteral("Before undertaking any electrical work on a vehicle")}, {}));

        runCase(checkEvidence(
            QStringLiteral("harness_engine_compartment_en"),
            QStringLiteral("What does the manual say about connectors and harnesses in the engine compartment?"),
            QStringLiteral("DOC_RCL0213ENG"), 11,
            {QStringLiteral("Connectors and Harness"), QStringLiteral("engine compartment")}, {}));

        runCase(checkEvidence(
            QStringLiteral("external_connectors_en"),
            QStringLiteral("How do I identify the external connectors used by TestBook?"),
            QStringLiteral("DOC_RCL0238ENG"), 14,
            {QStringLiteral("Identifying the External Connectors")}, {}));

        runCase(checkEvidence(
            QStringLiteral("vcsi_en"),
            QStringLiteral("Where is the Vehicle Communication Serial Interface VCSI documented?"),
            QStringLiteral("DOC_RCL0238ENG"), 16,
            {QStringLiteral("Vehicle Communication Serial Interface"), QStringLiteral("VCSI")}, {}));

        runCase(checkEvidence(
            QStringLiteral("sunroof_manual_close_en"),
            QStringLiteral("How can I close the sunroof if the electric motor will not close it?"),
            QStringLiteral("DOC_RCL0179ENX"), 24,
            {QStringLiteral("electric motor will not close the sunroof")}, {}));

        trace(QStringLiteral("BROAD_RESULT passed=%1 total=%2").arg(passed).arg(total));
        qInfo().noquote() << "BROAD_RELEVANCE_RESULT passed=" << passed << "total=" << total;
        if (passed != total) {
            trace(QStringLiteral("BROAD_PROBE_FAIL"));
            return 5;
        }

        trace(QStringLiteral("BROAD_PROBE_PASS"));
        return 0;
    }
'''
if needle not in text:
    raise SystemExit('main integration insertion point not found')
text = text.replace(needle, needle + probe, 1)
main.write_text(text, encoding='utf-8')
print('ACTUAL_APP_BROAD_RELEVANCE_PROBE_PATCHED')
