from pathlib import Path

main = Path('main.cpp')
text = main.read_text(encoding='utf-8')
text = text.replace('#include <QStringList>\n', '#include <QStringList>\n#include <QDebug>\n')
text = text.replace('#include "navigationorderpatch.h"\n', '#include "navigationorderpatch.h"\n#include "expert/IaMemsService.h"\n')
needle = '''    QApplication::setOrganizationName("ECU Mems Manager");\n'''
probe = r'''

    if (qEnvironmentVariableIsSet("MEMS_RELEVANCE_INTEGRATION_TEST"))
    {
        I18n::load(QStringLiteral("en"));
        I18n::install(&app);
        IaMemsService *service = IaMemsService::instance();

        auto checkEvidence = [&](const QString &label,
                                 const QString &question,
                                 const QString &document,
                                 int page,
                                 const QStringList &required,
                                 const QStringList &forbidden) -> bool {
            service->askWithLibrary(question);
            const QString evidence = service->property("iaMemsLastLibraryEvidence").toString();
            qInfo().noquote() << "RELEVANCE_CASE=" << label;
            qInfo().noquote() << "QUESTION=" << question;
            qInfo().noquote() << "EVIDENCE_BEGIN\n" << evidence << "\nEVIDENCE_END";

            const QString sourceNeedle = QStringLiteral("Source %1, page %2")
                                             .arg(document)
                                             .arg(page);
            if (!evidence.contains(sourceNeedle, Qt::CaseInsensitive)) {
                qCritical().noquote() << "FAIL missing target provenance" << sourceNeedle;
                return false;
            }
            for (const QString &needleText : required) {
                if (!evidence.contains(needleText, Qt::CaseInsensitive)) {
                    qCritical().noquote() << "FAIL missing required evidence" << needleText;
                    return false;
                }
            }
            for (const QString &needleText : forbidden) {
                if (evidence.contains(needleText, Qt::CaseInsensitive)) {
                    qCritical().noquote() << "FAIL contaminated evidence" << needleText;
                    return false;
                }
            }
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

        if (!primaryOk || !batteryOk)
            return 5;

        qInfo().noquote() << "ACTUAL_MEMS_MANAGER_MEMSLIBRARY_RELEVANCE_PASS primary=DOC_RCL0193ENG:p53 battery=DOC_RCL0221ENG:p20";
        return 0;
    }
'''
if needle not in text:
    raise SystemExit('main integration insertion point not found')
text = text.replace(needle, needle + probe, 1)
main.write_text(text, encoding='utf-8')
print('ACTUAL_APP_RELEVANCE_INTEGRATION_PATCHED')
