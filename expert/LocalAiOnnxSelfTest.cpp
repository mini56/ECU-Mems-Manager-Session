#include "LocalAiClient.h"
#include "i18n.h"

#include <QCoreApplication>
#include <QDate>
#include <QTextStream>
#include <QTimer>

namespace {
bool containsInternalLeak(const QString &text)
{
    const QString lower = text.toLower();
    const QStringList markers = {
        QStringLiteral("<think>"), QStringLiteral("</think>"), QStringLiteral("<|im_"),
        QStringLiteral("langage obligatoire"), QStringLiteral("mandatory language"),
        QStringLiteral("domaine obligatoire"), QStringLiteral("contexte candidat fourni par mems manager"),
        QStringLiteral("mode diagnostic rapide"), QStringLiteral("you are ia mems, the local assistant"),
        QStringLiteral("réponse attendue"), QStringLiteral("reponse attendue"),
        QStringLiteral("diagnostic bref"), QStringLiteral("ne montre aucun raisonnement interne"),
        QStringLiteral("faits fournis par mems manager")
    };
    for (const QString &marker : markers)
        if (lower.contains(marker)) return true;
    return false;
}
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    I18n::load(QStringLiteral("fr"));
    QTextStream out(stdout), err(stderr);
    out.setCodec("UTF-8"); err.setCodec("UTF-8");

    LocalAiClient client;
    bool started = false, finished = false;
    int stage = 0, languageIndex = -1;
    const QStringList languageCodes = {"fr","en","es","it","pt","de"};
    const QStringList generatedCodes = {"it","en","es","pt","de"};
    const QStringList generatedMarkers = {"IT_OK","EN_OK","ES_OK","PT_OK","DE_OK"};
    const QStringList generatedQuestions = {
        QStringLiteral("Rispondi iniziando esattamente con IT_OK. Qual è la coppia di serraggio?"),
        QStringLiteral("Answer starting exactly with EN_OK. What is the tightening torque?"),
        QStringLiteral("Responde empezando exactamente con ES_OK. ¿Cuál es el par de apriete?"),
        QStringLiteral("Responde começando exatamente com PT_OK. Qual é o binário de aperto?"),
        QStringLiteral("Antworte exakt beginnend mit DE_OK. Wie hoch ist das Anzugsdrehmoment?")
    };
    const QString foreignGrounding = QStringLiteral("Couple de serrage vérifié : 68 Nm. Niveau de preuve : constructeur.");

    const auto fail = [&](const QString &message) {
        if (finished) return;
        finished = true;
        err << "FAIL LocalAiClient ONNX: " << message << Qt::endl;
        app.exit(1);
    };

    for (const QString &code : languageCodes) {
        if (!I18n::load(code)) {
            fail(QStringLiteral("Dictionnaire IA MEMS non chargé pour %1").arg(code));
            return 1;
        }
        const QString ready = I18n::text(99016);
        if (ready.startsWith(QLatin1Char('[')) || ready.trimmed().isEmpty()) {
            fail(QStringLiteral("Clé IA MEMS 99016 absente pour %1").arg(code));
            return 1;
        }
    }
    I18n::load(QStringLiteral("fr"));

    const auto rejectLeak = [&](const QString &answer) -> bool {
        if (!containsInternalLeak(answer)) return false;
        fail(QStringLiteral("Directive interne ou balise Qwen dans la réponse : %1").arg(answer.left(180)));
        return true;
    };

    const auto askLanguageCase = [&]() {
        I18n::load(generatedCodes.at(languageIndex));
        client.clearConversation();
        client.ask(generatedQuestions.at(languageIndex), foreignGrounding);
    };

    QObject::connect(&client, &LocalAiClient::stateChanged, &app, [&]() {
        out << "STATE=" << client.statusText() << Qt::endl;
        switch (client.state()) {
        case LocalAiClient::Ready:
            if (!started) {
                started = true; stage = 0; I18n::load(QStringLiteral("fr"));
                client.ask(QStringLiteral("C4EST QUOI LA BOBINE ?"), QString());
            }
            break;
        case LocalAiClient::MissingRuntime:
        case LocalAiClient::MissingModel:
        case LocalAiClient::Error:
            fail(client.statusText()); break;
        default: break;
        }
    });

    QObject::connect(&client, &LocalAiClient::responseError, &app,
                     [&](const QString &message) { fail(message); });

    QObject::connect(&client, &LocalAiClient::responseReady, &app, [&](const QString &text) {
        if (finished) return;
        const QString answer = text.trimmed();
        out << "ANSWER_STAGE_" << stage << "=" << answer << Qt::endl;
        if (answer.isEmpty()) { fail(QStringLiteral("Réponse vide étape %1").arg(stage)); return; }
        if (rejectLeak(answer)) return;

        if (stage == 0) {
            if (!answer.contains("bobine", Qt::CaseInsensitive) || !answer.contains("haute tension", Qt::CaseInsensitive) || !answer.contains("bougie", Qt::CaseInsensitive)) { fail("Définition bobine invalide"); return; }
            stage=1; client.ask(QStringLiteral("QUELLE JOURS SOMME NOUS ?"), QString()); return;
        }
        if (stage == 1) {
            if (!answer.contains(QString::number(QDate::currentDate().year())) || !answer.contains("Nous sommes", Qt::CaseInsensitive)) { fail("Date non reconnue"); return; }
            stage=2; client.ask(QStringLiteral("BROCHE ECU 1.3"), QStringLiteral("BROCHAGE_TEST_ECU_13 : information de brochage vérifiée fournie par la base.")); return;
        }
        if (stage == 2) {
            if (!answer.contains("BROCHAGE_TEST_ECU_13") || answer.contains("Micro-Electro-Mechanical", Qt::CaseInsensitive)) { fail("BROCHE ECU 1.3 détourné"); return; }
            stage=3; client.ask(QStringLiteral("BROCHE OBD 1.9 ?"), QStringLiteral("BROCHAGE_TEST_OBD_19 : information OBD vérifiée fournie par la base.")); return;
        }
        if (stage == 3) {
            if (!answer.contains("BROCHAGE_TEST_OBD_19") || answer.contains("Micro-Electro-Mechanical", Qt::CaseInsensitive)) { fail("BROCHE OBD 1.9 détourné"); return; }
            stage=4; client.ask(QStringLiteral("Couple de serrage sonde température ECT"), QStringLiteral("Couple ECT vérifié : 15 Nm.\nNiveau de preuve : constructeur.")); return;
        }
        if (stage == 4) {
            if (!answer.contains("15 Nm", Qt::CaseInsensitive)) { fail("Fait ECT remplacé"); return; }
            stage=5; client.ask(QStringLiteral("Couleur des fils sonde lambda"), QStringLiteral("Fils sonde lambda : gris et vert clair/gris — preuve : constructeur.")); return;
        }
        if (stage == 5) {
            if (!answer.contains("gris", Qt::CaseInsensitive) || answer.contains("réponse attendue", Qt::CaseInsensitive) || answer.contains("diagnostic bref", Qt::CaseInsensitive)) { fail("Réponse lambda invalide"); return; }
            stage=6; client.ask(QStringLiteral("Réponds uniquement par OK."), QString()); return;
        }
        if (stage == 6) {
            if (!answer.contains("OK", Qt::CaseInsensitive)) { fail("Marqueur OK absent"); return; }
            stage=7; languageIndex=0; askLanguageCase(); return;
        }
        if (stage == 7) {
            const QString marker=generatedMarkers.at(languageIndex), code=generatedCodes.at(languageIndex);
            if (!answer.contains(marker, Qt::CaseInsensitive) || !answer.contains("68 Nm", Qt::CaseInsensitive)) { fail(QStringLiteral("Restitution %1 invalide: %2").arg(code, answer.left(180))); return; }
            if (answer == foreignGrounding || answer.contains("Couple de serrage vérifié", Qt::CaseInsensitive)) { fail(QStringLiteral("Grounding français brut en %1").arg(code)); return; }
            out << "MULTILINGUAL_PASS=" << code << Qt::endl;
            ++languageIndex;
            if (languageIndex < generatedCodes.size()) { askLanguageCase(); return; }
            I18n::load(QStringLiteral("fr"));
            finished=true;
            out << "PASS LocalAiClient native ONNX response quality, documentary grounding, multilingual rendering and leak guards" << Qt::endl;
            app.exit(0);
        }
    });

    QTimer::singleShot(300000, &app, [&]() { fail(QStringLiteral("Délai dépassé pendant le test ONNX.")); });
    client.initialize();
    const int result=app.exec();
    client.shutdown();
    return result;
}
