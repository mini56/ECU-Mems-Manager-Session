#include "LocalAiClient.h"
#include "i18n.h"

#include <QCoreApplication>
#include <QTextStream>
#include <QTimer>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    I18n::load(QStringLiteral("fr"));

    QTextStream out(stdout);
    QTextStream err(stderr);
    out.setCodec("UTF-8");
    err.setCodec("UTF-8");

    LocalAiClient client;
    bool questionSent = false;
    bool finished = false;

    const auto fail = [&](const QString &message) {
        if (finished)
            return;
        finished = true;
        err << "FAIL LocalAiClient ONNX: " << message << Qt::endl;
        app.exit(1);
    };

    QObject::connect(&client, &LocalAiClient::stateChanged, &app, [&]() {
        out << "STATE=" << client.statusText() << Qt::endl;
        switch (client.state()) {
        case LocalAiClient::Ready:
            if (!questionSent) {
                questionSent = true;
                client.ask(QStringLiteral("Réponds uniquement par OK."), QString());
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
        out << "ANSWER=" << answer << Qt::endl;
        if (answer.isEmpty() || !answer.contains(QStringLiteral("OK"), Qt::CaseInsensitive)) {
            fail(QStringLiteral("La génération native ne contient pas le marqueur OK attendu."));
            return;
        }
        finished = true;
        out << "PASS LocalAiClient native ONNX Runtime GenAI" << Qt::endl;
        app.exit(0);
    });

    QTimer::singleShot(180000, &app, [&]() {
        fail(QStringLiteral("Délai dépassé pendant le chargement/génération ONNX."));
    });

    client.initialize();
    const int result = app.exec();
    client.shutdown();
    return result;
}
