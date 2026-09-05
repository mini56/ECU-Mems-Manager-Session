from pathlib import Path

main = Path('main.cpp')
text = main.read_text(encoding='utf-8')
text = text.replace('#include <QStringList>\n', '#include <QStringList>\n#include <QDebug>\n#include <QTimer>\n')
text = text.replace('#include "navigationorderpatch.h"\n', '#include "navigationorderpatch.h"\n#include "expert/LocalAiClient.h"\n')
needle = '''    QApplication::setOrganizationName("ECU Mems Manager");\n'''
trace = r'''

    if (qEnvironmentVariableIsSet("MEMS_QWEN_TRACE_APP"))
    {
        I18n::load(QStringLiteral("fr"));
        I18n::install(&app);
        LocalAiClient traceClient;
        bool asked = false;
        int traceResult = 3;

        QObject::connect(&traceClient, &LocalAiClient::stateChanged, &app, [&]() {
            qInfo().noquote() << "TRACE_STATE=" << traceClient.statusText();
            if (traceClient.state() == LocalAiClient::Ready && !asked)
            {
                asked = true;
                const QString grounding = QStringLiteral(
                    "Documentation RAVEMEMS retrouvée dans MEMSLibrary_Pack_001. Utiliser uniquement les extraits pertinents ci-dessous et conserver leur provenance ; si aucun extrait ne répond exactement à la question, ne pas extrapoler.\n"
                    "Source DOC_RCL0193ENG, page 53 [révision REV_RCL0193ENG_SOURCE, langue en, type step]\n"
                    "2 Using feeler gauges, check primary gear end-float between backing ring and primary gear. DATA: End-float = 0.089 to 0.165 mm.\n\n"
                    "Source DOC_RCL0193ENG, page 53 [révision REV_RCL0193ENG_SOURCE, langue en, type operation] — ENGINE CRANKSHAFT PRIMARY GEAR END-FLOAT\n"
                    "12.21.28 manufacturer_operation ENGINE CRANKSHAFT PRIMARY GEAR END-FLOAT\n\n"
                    "Source DOC_RCL0193ENG, page 53 [révision REV_RCL0193ENG_SOURCE, langue en, type step]\n"
                    "3 Adjust end-float if outside of tolerance.\n\n"
                    "Source DOC_RCL0193ENG, page 53 [révision REV_RCL0193ENG_SOURCE, langue en, type step]\n"
                    "7 Select a thrust washer which will give the correct end-float figure.\n\n"
                    "Source DOC_RCL0193ENG, page 53 [révision REV_RCL0193ENG_SOURCE, langue en, type step]\n"
                    "10 Check that end-float is now within tolerance.\n\n"
                    "Source DOC_RCL0193ENG, page 53 [révision REV_RCL0193ENG_SOURCE, langue en, type step]\n"
                    "5 Remove primary gear front thrust washer.");
                traceClient.ask(QStringLiteral("Quel est le jeu axial du pignon primaire ?"), grounding);
            }
        });
        QObject::connect(&traceClient, &LocalAiClient::responseReady, &app, [&](const QString &answer) {
            qInfo().noquote() << "TRACE_RESPONSE_READY_BEGIN\n" << answer << "\nTRACE_RESPONSE_READY_END";
            traceResult = 0;
            app.quit();
        });
        QObject::connect(&traceClient, &LocalAiClient::responseError, &app, [&](const QString &error) {
            qCritical().noquote() << "TRACE_RESPONSE_ERROR=" << error;
            traceResult = 2;
            app.quit();
        });
        QTimer::singleShot(180000, &app, [&]() {
            qCritical().noquote() << "TRACE_TIMEOUT";
            traceResult = 4;
            app.quit();
        });
        traceClient.initialize();
        app.exec();
        traceClient.shutdown();
        return traceResult;
    }
'''
if needle not in text:
    raise SystemExit('main trace insertion point not found')
text = text.replace(needle, needle + trace, 1)
main.write_text(text, encoding='utf-8')

client = Path('expert/LocalAiClient.cpp')
text = client.read_text(encoding='utf-8')
needle = '''            if (invalidAnswer) {\n                // A documentary question must never fall back to the raw RAVE\n'''
trace = '''            if (invalidAnswer) {\n                if (QProcessEnvironment::systemEnvironment().contains(QStringLiteral("MEMS_QWEN_TRACE_APP"))) {\n                    QStringList reasons;\n                    if (answer.isEmpty()) reasons.append(QStringLiteral("empty"));\n                    if (isQuestionEcho(trimmedQuestion, answer)) reasons.append(QStringLiteral("question_echo"));\n                    if (likelyWrongLanguage(answer)) reasons.append(QStringLiteral("wrong_language"));\n                    if (containsInternalInstructionLeak(answer)) reasons.append(QStringLiteral("instruction_leak"));\n                    if (looksLikeRawDocumentaryGrounding(answer)) reasons.append(QStringLiteral("raw_grounding"));\n                    emit responseError(QStringLiteral("TRACE_REJECT reasons=%1 RAW=[%2] CLEAN=[%3]")\n                        .arg(reasons.join(QLatin1Char(',')), rawAnswer.left(1600), answer.left(1600)));\n                    return;\n                }\n                // A documentary question must never fall back to the raw RAVE\n'''
if needle not in text:
    raise SystemExit('LocalAiClient trace insertion point not found')
text = text.replace(needle, trace, 1)
client.write_text(text, encoding='utf-8')

print('ACTUAL_APP_QWEN_TRACE_PATCHED')
