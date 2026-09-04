#include "IaMemsLibraryBridge.h"
#include "LocalAiClient.h"
#include "i18n.h"

#include <QCoreApplication>
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
        QStringLiteral("faits fournis par mems manager"),
        QStringLiteral("ne montre aucun raisonnement interne")
    };
    for (const QString &marker : markers) {
        if (lower.contains(marker))
            return true;
    }
    return false;
}

bool containsPrimaryGearValue(const QString &text)
{
    return text.contains(QStringLiteral("0.089"))
        || text.contains(QStringLiteral("0,089"))
        || text.contains(QStringLiteral("0.165"))
        || text.contains(QStringLiteral("0,165"));
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    I18n::load(QStringLiteral("fr"));

    QTextStream out(stdout);
    QTextStream err(stderr);
    out.setCodec("UTF-8");
    err.setCodec("UTF-8");

    const QString question = QStringLiteral(
        "À partir de la documentation RAVEMEMS fournie, explique le jeu axial de l'engrenage primaire et cite la valeur documentée.");
    const IaMemsLibraryGrounding grounding = IaMemsLibraryBridge::retrieve(
        question,
        QStringList() << QStringLiteral("primary") << QStringLiteral("gear")
                      << QStringLiteral("end") << QStringLiteral("float"));

    if (!grounding.libraryReady) {
        err << "FAIL QWEN_RAVEMEMS: MEMSLibrary not ready: " << grounding.error << Qt::endl;
        return 1;
    }
    if (grounding.resultCount <= 0
        || !grounding.text.contains(QStringLiteral("DOC_RCL0193ENG"))
        || !grounding.text.contains(QStringLiteral("page 53"), Qt::CaseInsensitive)
        || !containsPrimaryGearValue(grounding.text)) {
        err << "FAIL QWEN_RAVEMEMS: expected RCL0193ENG page 53 primary gear evidence missing" << Qt::endl;
        return 1;
    }

    out << "GROUNDING_OK source=DOC_RCL0193ENG page=53 results=" << grounding.resultCount << Qt::endl;

    LocalAiClient client;
    bool started = false;
    bool finished = false;

    const auto fail = [&](const QString &message) {
        if (finished)
            return;
        finished = true;
        err << "FAIL QWEN_RAVEMEMS: " << message << Qt::endl;
        app.exit(1);
    };

    QObject::connect(&client, &LocalAiClient::stateChanged, &app, [&]() {
        out << "STATE=" << client.statusText() << Qt::endl;
        if (client.state() == LocalAiClient::Ready && !started) {
            started = true;
            out << "QWEN_READY runtime=" << client.runtimePath()
                << " model=" << client.modelPath() << Qt::endl;
            client.ask(question, grounding.text);
            return;
        }
        if (client.state() == LocalAiClient::MissingRuntime
            || client.state() == LocalAiClient::MissingModel
            || client.state() == LocalAiClient::Error) {
            fail(client.statusText());
        }
    });

    QObject::connect(&client, &LocalAiClient::responseError, &app,
                     [&](const QString &message) { fail(message); });

    QObject::connect(&client, &LocalAiClient::responseReady, &app,
                     [&](const QString &text) {
        if (finished)
            return;
        const QString answer = text.trimmed();
        out << "QWEN_ANSWER=" << answer << Qt::endl;
        if (answer.isEmpty()) {
            fail(QStringLiteral("empty Qwen answer"));
            return;
        }
        if (containsInternalLeak(answer)) {
            fail(QStringLiteral("internal prompt/Qwen marker leaked into answer"));
            return;
        }
        if (!containsPrimaryGearValue(answer)) {
            fail(QStringLiteral("Qwen answer did not retain the documented 0.089/0.165 primary gear value"));
            return;
        }
        finished = true;
        out << "IA_MEMSLIBRARY_QWEN_PASS source=DOC_RCL0193ENG page=53 documented_value=1 provenance=1 model_grounded=1" << Qt::endl;
        app.exit(0);
    });

    QTimer::singleShot(180000, &app, [&]() {
        fail(QStringLiteral("timeout while loading or generating with native Qwen ONNX"));
    });

    client.initialize();
    const int result = app.exec();
    client.shutdown();
    return result;
}
