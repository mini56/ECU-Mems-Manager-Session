from pathlib import Path

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
