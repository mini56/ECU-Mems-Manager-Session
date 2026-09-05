#include "IaMemsDiagramCatalog.h"
#include "IaMemsLibrarySynthesis.h"
#include "LocalAiClient.h"
#include "i18n.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QTimer>

namespace {

QString visualAssetKey(const QString &referenceRoot,
                       const IaMemsDiagramSuggestion &suggestion)
{
    QFile file(QDir(referenceRoot).filePath(QStringLiteral("runtime_visual_catalog.json")));
    if (!file.open(QIODevice::ReadOnly))
        return QString();

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        return QString();

    const QJsonArray entries = document.object().value(QStringLiteral("entries")).toArray();
    for (const QJsonValue &value : entries) {
        if (!value.isObject())
            continue;
        const QJsonObject entry = value.toObject();
        if (entry.value(QStringLiteral("publication_code")).toString() != QStringLiteral("RCL0193ENG"))
            continue;
        if (entry.value(QStringLiteral("physical_page")).toInt() != 53)
            continue;
        if (QDir::cleanPath(entry.value(QStringLiteral("runtime_path")).toString())
            == QDir::cleanPath(suggestion.relativePath))
            return entry.value(QStringLiteral("asset_entity_key")).toString();
    }
    return QString();
}

bool containsNumber(const QString &text, const QString &dot, const QString &comma)
{
    return text.contains(dot, Qt::CaseInsensitive)
        || text.contains(comma, Qt::CaseInsensitive);
}

bool containsControlMethod(const QString &text)
{
    return text.contains(QStringLiteral("cale"), Qt::CaseInsensitive)
        || text.contains(QStringLiteral("jauge"), Qt::CaseInsensitive)
        || text.contains(QStringLiteral("feeler"), Qt::CaseInsensitive);
}

bool containsRawDocumentDump(const QString &text)
{
    const QString lower = text.toLower();
    return lower.contains(QStringLiteral("documentation ravemems retrouvée"))
        || lower.contains(QStringLiteral("source doc_rcl0193eng"))
        || lower.contains(QStringLiteral("revision rev_rcl0193eng"))
        || lower.contains(QStringLiteral("révision rev_rcl0193eng"))
        || lower.contains(QStringLiteral("langue en"))
        || lower.contains(QStringLiteral("type step"))
        || lower.contains(QStringLiteral("extraits documentaires vérifiés fournis par mems manager"));
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    QTextStream err(stderr);
    out.setCodec("UTF-8");
    err.setCodec("UTF-8");

    if (argc != 2) {
        err << "usage: build105_regression_selftest <reference-root>" << Qt::endl;
        return 1;
    }

    const QString referenceRoot = QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath();
    if (!QFileInfo(QDir(referenceRoot).filePath(QStringLiteral("runtime_visual_catalog.json"))).isFile()) {
        err << "FAIL clean V2 visual catalog missing" << Qt::endl;
        return 2;
    }

    const QString question = QStringLiteral(
        "Quel est le jeu axial du pignon primaire et comment le contrôler ?");
    const QString evidence = QStringLiteral(
        "Documentation RAVEMEMS retrouvée dans MEMSLibrary_Pack_001. "
        "Utiliser uniquement les extraits pertinents ci-dessous et conserver leur provenance ; "
        "si aucun extrait ne répond exactement à la question, ne pas extrapoler.\n"
        "Source DOC_RCL0193ENG, page 53 [révision REV_RCL0193ENG_SOURCE, langue en, type step] "
        "— ENGINE CRANKSHAFT PRIMARY GEAR END-FLOAT 12.21.28\n"
        "Check the end-float of the primary gear. The correct end-float is 0.089 to 0.165 mm. "
        "Measure using feeler gauges between the backing ring and primary gear.");

    const IaMemsDiagramSuggestion visual =
        IaMemsDiagramCatalog::suggestionForEvidence(question, evidence, referenceRoot);
    if (!visual.isValid()) {
        err << "FAIL no visual selected from p53 evidence" << Qt::endl;
        return 3;
    }
    const QString assetKey = visualAssetKey(referenceRoot, visual);
    if (assetKey != QStringLiteral("VIS_P0053_002")) {
        err << "FAIL useful p53 visual not selected, got " << assetKey << Qt::endl;
        return 4;
    }
    out << "VISUAL_PASS asset=VIS_P0053_002" << Qt::endl;

    I18n::load(QStringLiteral("fr"));
    LocalAiClient client;
    bool asked = false;
    bool finished = false;

    const auto fail = [&](const QString &message) {
        if (finished)
            return;
        finished = true;
        err << "FAIL documentary synthesis: " << message << Qt::endl;
        app.exit(5);
    };

    QObject::connect(&client, &LocalAiClient::stateChanged, &app, [&]() {
        out << "STATE=" << client.statusText() << Qt::endl;
        if (client.state() == LocalAiClient::Ready && !asked) {
            asked = true;
            const QString synthesisQuestion =
                IaMemsLibrarySynthesis::promptQuestion(question, evidence);
            client.ask(synthesisQuestion, QString());
        } else if (client.state() == LocalAiClient::MissingRuntime
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
        out << "ANSWER=" << answer << Qt::endl;
        if (answer.isEmpty()) {
            fail(QStringLiteral("réponse vide"));
            return;
        }
        if (!containsNumber(answer, QStringLiteral("0.089"), QStringLiteral("0,089"))
            || !containsNumber(answer, QStringLiteral("0.165"), QStringLiteral("0,165"))) {
            fail(QStringLiteral("la plage 0.089–0.165 mm n'est pas conservée"));
            return;
        }
        if (!containsControlMethod(answer)) {
            fail(QStringLiteral("la méthode de contrôle à la cale/jauge n'est pas rendue"));
            return;
        }
        if (containsRawDocumentDump(answer)) {
            fail(QStringLiteral("le bloc documentaire brut ou sa provenance a été recopié"));
            return;
        }
        if (answer.size() > 1200) {
            fail(QStringLiteral("la réponse reste anormalement longue (%1 caractères)").arg(answer.size()));
            return;
        }

        finished = true;
        out << "SYNTHESIS_PASS value=0.089-0.165 method=feeler concise=1 raw_dump=0" << Qt::endl;
        app.exit(0);
    });

    QTimer::singleShot(180000, &app, [&]() {
        fail(QStringLiteral("délai dépassé pendant le test ONNX"));
    });

    client.initialize();
    const int result = app.exec();
    client.shutdown();
    return result;
}
