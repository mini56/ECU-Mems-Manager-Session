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
        QStringLiteral("<think>"),
        QStringLiteral("</think>"),
        QStringLiteral("<|im_"),
        QStringLiteral("langage obligatoire"),
        QStringLiteral("mandatory language"),
        QStringLiteral("domaine obligatoire"),
        QStringLiteral("contexte candidat fourni par mems manager"),
        QStringLiteral("mode diagnostic rapide"),
        QStringLiteral("you are ia mems, the local assistant")
    };
    for (const QString &marker : markers) {
        if (lower.contains(marker))
            return true;
    }
    return false;
}
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    I18n::load(QStringLiteral("fr"));

    QTextStream out(stdout);
    QTextStream err(stderr);
    out.setCodec("UTF-8");
    err.setCodec("UTF-8");

    LocalAiClient client;
    bool started = false;
    bool finished = false;
    int stage = 0;

    const auto fail = [&](const QString &message) {
        if (finished)
            return;
        finished = true;
        err << "FAIL LocalAiClient ONNX: " << message << Qt::endl;
        app.exit(1);
    };

    const auto rejectLeak = [&](const QString &answer) -> bool {
        if (!containsInternalLeak(answer))
            return false;
        fail(QStringLiteral("Une directive interne ou une balise Qwen a fui dans la réponse : %1")
                 .arg(answer.left(180)));
        return true;
    };

    QObject::connect(&client, &LocalAiClient::stateChanged, &app, [&]() {
        out << "STATE=" << client.statusText() << Qt::endl;
        switch (client.state()) {
        case LocalAiClient::Ready:
            if (!started) {
                started = true;
                stage = 0;
                client.ask(QStringLiteral("C4EST QUOI LA BOBINE ?"), QString());
            }
            break;
        case LocalAiClient::MissingRuntime:
        case LocalAiClient::MissingModel:
        case LocalAiClient::Error:
            fail(client.statusText());
            break;
        default:
            break;
        }
    });

    QObject::connect(&client, &LocalAiClient::responseError, &app,
                     [&](const QString &message) { fail(message); });

    QObject::connect(&client, &LocalAiClient::responseReady, &app,
                     [&](const QString &text) {
        if (finished)
            return;
        const QString answer = text.trimmed();
        out << "ANSWER_STAGE_" << stage << "=" << answer << Qt::endl;
        if (answer.isEmpty()) {
            fail(QStringLiteral("Réponse vide à l'étape %1.").arg(stage));
            return;
        }
        if (rejectLeak(answer))
            return;

        if (stage == 0) {
            if (!answer.contains(QStringLiteral("bobine"), Qt::CaseInsensitive)
                || !answer.contains(QStringLiteral("haute tension"), Qt::CaseInsensitive)
                || !answer.contains(QStringLiteral("bougie"), Qt::CaseInsensitive)) {
                fail(QStringLiteral("La définition déterministe de la bobine avec C4EST n'est pas exploitable."));
                return;
            }
            stage = 1;
            client.ask(QStringLiteral("QUELLE JOURS SOMME NOUS ?"), QString());
            return;
        }

        if (stage == 1) {
            if (!answer.contains(QString::number(QDate::currentDate().year()))
                || !answer.contains(QStringLiteral("Nous sommes"), Qt::CaseInsensitive)) {
                fail(QStringLiteral("La question de date avec faute de frappe n'est pas reconnue."));
                return;
            }
            stage = 2;
            client.ask(QStringLiteral("BROCHE ECU 1.3"),
                       QStringLiteral("BROCHAGE_TEST_ECU_13 : information de brochage vérifiée fournie par la base."));
            return;
        }

        if (stage == 2) {
            if (!answer.contains(QStringLiteral("BROCHAGE_TEST_ECU_13"))
                || answer.contains(QStringLiteral("Micro-Electro-Mechanical"), Qt::CaseInsensitive)) {
                fail(QStringLiteral("BROCHE ECU 1.3 est encore détourné par la définition générique MEMS."));
                return;
            }
            stage = 3;
            client.ask(QStringLiteral("BROCHE OBD 1.9 ?"),
                       QStringLiteral("BROCHAGE_TEST_OBD_19 : information OBD vérifiée fournie par la base."));
            return;
        }

        if (stage == 3) {
            if (!answer.contains(QStringLiteral("BROCHAGE_TEST_OBD_19"))
                || answer.contains(QStringLiteral("Micro-Electro-Mechanical"), Qt::CaseInsensitive)) {
                fail(QStringLiteral("BROCHE OBD 1.9 est encore détourné par la définition générique MEMS."));
                return;
            }
            stage = 4;
            client.ask(QStringLiteral("Réponds uniquement par OK."), QString());
            return;
        }

        if (stage == 4) {
            if (!answer.contains(QStringLiteral("OK"), Qt::CaseInsensitive)) {
                fail(QStringLiteral("La génération native ne contient pas le marqueur OK attendu."));
                return;
            }
            finished = true;
            out << "PASS LocalAiClient native ONNX response quality and real targeting cases" << Qt::endl;
            app.exit(0);
        }
    });

    QTimer::singleShot(180000, &app, [&]() {
        fail(QStringLiteral("Délai dépassé pendant le chargement/génération ONNX."));
    });

    client.initialize();
    const int result = app.exec();
    client.shutdown();
    return result;
}
