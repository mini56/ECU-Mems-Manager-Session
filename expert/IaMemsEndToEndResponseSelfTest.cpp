#include "IaMemsLibraryBridge.h"
#include "LocalAiClient.h"
#include "i18n.h"

#include <QCoreApplication>
#include <QTextStream>
#include <QTimer>

namespace {

int fail(QTextStream &err, QCoreApplication &app, const QString &message)
{
    err << "FAIL E2E RESPONSE: " << message << Qt::endl;
    app.exit(1);
    return 1;
}

bool containsRawEvidence(const QString &answer)
{
    return answer.contains(QStringLiteral("Documentation RAVEMEMS retrouvée"), Qt::CaseInsensitive)
        || answer.contains(QStringLiteral("type step"), Qt::CaseInsensitive)
        || answer.contains(QStringLiteral("révision REV_"), Qt::CaseInsensitive)
        || answer.contains(QStringLiteral("langue en"), Qt::CaseInsensitive);
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    QTextStream err(stderr);
    out.setCodec("UTF-8");
    err.setCodec("UTF-8");

    if (argc != 3) {
        err << "usage: ia_mems_e2e_response_selftest <MEMSLibrary.dll> <Pack001-dir>" << Qt::endl;
        return 2;
    }

    qputenv("MEMS_LIBRARY_DLL", QByteArray(argv[1]));
    qputenv("MEMS_LIBRARY_PACK", QByteArray(argv[2]));
    I18n::load(QStringLiteral("fr"));

    const QString primaryQuestion = QStringLiteral(
        "Quel est le jeu axial du pignon primaire et comment le contrôler ?");
    const QStringList primaryKeywords = {
        QStringLiteral("jeu"), QStringLiteral("axial"), QStringLiteral("pignon"),
        QStringLiteral("primaire"), QStringLiteral("comment"), QStringLiteral("controler"),
        QStringLiteral("primary"), QStringLiteral("end"), QStringLiteral("float")
    };
    const IaMemsLibraryGrounding primary =
        IaMemsLibraryBridge::retrieve(primaryQuestion, primaryKeywords);
    if (!primary.libraryReady || !primary.provenanceFiltered
        || primary.selectedDocument != QStringLiteral("DOC_RCL0193ENG")
        || primary.selectedPage != 53
        || !primary.text.contains(QStringLiteral("0.089"))
        || !primary.text.contains(QStringLiteral("0.165"))
        || !primary.text.contains(QStringLiteral("12.21.28"))) {
        err << "FAIL E2E RESPONSE: primary MEMSLibrary proof incorrect doc="
            << primary.selectedDocument << " page=" << primary.selectedPage << Qt::endl;
        return 3;
    }

    const QString batteryQuestion = QStringLiteral(
        "Quelle est la procédure de restauration de la batterie ?");
    const IaMemsLibraryGrounding battery = IaMemsLibraryBridge::retrieve(
        batteryQuestion,
        {QStringLiteral("restauration"), QStringLiteral("batterie"),
         QStringLiteral("battery"), QStringLiteral("restoration")});
    if (!battery.libraryReady || !battery.provenanceFiltered
        || battery.selectedDocument != QStringLiteral("DOC_RCL0221ENG")
        || battery.selectedPage != 20
        || !battery.text.contains(QStringLiteral("BATTERY RESTORATION PROCEDURE"), Qt::CaseInsensitive)) {
        err << "FAIL E2E RESPONSE: battery MEMSLibrary proof incorrect doc="
            << battery.selectedDocument << " page=" << battery.selectedPage << Qt::endl;
        return 4;
    }

    out << "E2E_PROOF_PRIMARY=DOC_RCL0193ENG:p53" << Qt::endl;
    out << "E2E_PROOF_BATTERY=DOC_RCL0221ENG:p20" << Qt::endl;

    LocalAiClient client;
    bool started = false;
    bool finished = false;
    int stage = 0;

    QObject::connect(&client, &LocalAiClient::stateChanged, &app, [&]() {
        out << "STATE=" << client.statusText() << Qt::endl;
        if (client.state() == LocalAiClient::Ready && !started) {
            started = true;
            client.ask(primaryQuestion, primary.text);
        } else if (client.state() == LocalAiClient::MissingRuntime
                   || client.state() == LocalAiClient::MissingModel
                   || client.state() == LocalAiClient::Error) {
            fail(err, app, client.statusText());
        }
    });

    QObject::connect(&client, &LocalAiClient::responseError, &app,
                     [&](const QString &message) { fail(err, app, message); });

    QObject::connect(&client, &LocalAiClient::responseReady, &app,
                     [&](const QString &text) {
        if (finished)
            return;
        const QString answer = text.trimmed();
        out << "ANSWER_STAGE_" << stage << "=" << answer << Qt::endl;

        if (stage == 0) {
            if (!answer.contains(QStringLiteral("0.089"))
                || !answer.contains(QStringLiteral("0.165"))
                || !answer.contains(QStringLiteral("12.21.28"))) {
                fail(err, app, QStringLiteral("primary answer lost exact RAVEMEMS values/reference"));
                return;
            }
            if (containsRawEvidence(answer)
                || answer.contains(QStringLiteral("coaxial"), Qt::CaseInsensitive)
                || answer.contains(QStringLiteral("page 343"), Qt::CaseInsensitive)) {
                fail(err, app, QStringLiteral("primary answer is raw or contaminated"));
                return;
            }
            if (!answer.contains(QStringLiteral("jeu"), Qt::CaseInsensitive)
                && !answer.contains(QStringLiteral("contrô"), Qt::CaseInsensitive)
                && !answer.contains(QStringLiteral("control"), Qt::CaseInsensitive)) {
                fail(err, app, QStringLiteral("primary answer is not a direct usable answer in the active language"));
                return;
            }
            out << "E2E_PRIMARY_RESPONSE_PASS" << Qt::endl;
            stage = 1;
            client.ask(batteryQuestion, battery.text);
            return;
        }

        if (containsRawEvidence(answer)) {
            fail(err, app, QStringLiteral("battery answer still dumps raw evidence"));
            return;
        }
        if (!answer.contains(QStringLiteral("batter"), Qt::CaseInsensitive)
            || !answer.contains(QStringLiteral("restaur"), Qt::CaseInsensitive)) {
            fail(err, app, QStringLiteral("battery answer does not answer the selected documentary subject"));
            return;
        }

        out << "E2E_BATTERY_RESPONSE_PASS" << Qt::endl;
        out << "IA_MEMSLIBRARY_QWEN_RESPONSE_CONSTRUCTION_PASS" << Qt::endl;
        finished = true;
        app.exit(0);
    });

    QTimer::singleShot(300000, &app, [&]() {
        if (!finished)
            fail(err, app, QStringLiteral("timeout"));
    });

    client.initialize();
    const int result = app.exec();
    client.shutdown();
    return result;
}
