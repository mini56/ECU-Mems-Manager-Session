#include "IaMemsDiagramCatalog.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include <cstdio>

#ifndef MEMS_SOURCE_REFERENCE_DIR
#define MEMS_SOURCE_REFERENCE_DIR ""
#endif

namespace {

void printLine(const QString &line)
{
    const QByteArray bytes = line.toUtf8();
    std::fprintf(stderr, "%s\n", bytes.constData());
    std::fflush(stderr);
}

bool requireSuggestion(const QString &root,
                       const QString &question,
                       const QString &expectedKey,
                       const QString &expectedRelativePath)
{
    const IaMemsDiagramSuggestion suggestion =
        IaMemsDiagramCatalog::suggestionForQuestion(question, root);
    if (!suggestion.isValid()) {
        printLine(QStringLiteral("FAIL no diagram: %1").arg(question));
        return false;
    }
    if (suggestion.key != expectedKey || suggestion.relativePath != expectedRelativePath) {
        printLine(QStringLiteral("FAIL wrong diagram: %1 -> %2 / %3")
                      .arg(question, suggestion.key, suggestion.relativePath));
        return false;
    }
    if (!QFileInfo::exists(suggestion.absolutePath)) {
        printLine(QStringLiteral("FAIL missing resolved file: %1").arg(suggestion.absolutePath));
        return false;
    }
    printLine(QStringLiteral("PASS %1 -> %2").arg(question, suggestion.key));
    return true;
}

bool requireNoSuggestion(const QString &root, const QString &question, const QString &label)
{
    const IaMemsDiagramSuggestion suggestion =
        IaMemsDiagramCatalog::suggestionForQuestion(question, root);
    if (suggestion.isValid()) {
        printLine(QStringLiteral("FAIL %1 unexpectedly resolved: %2")
                      .arg(label, suggestion.key));
        return false;
    }
    printLine(QStringLiteral("PASS no diagram: %1").arg(label));
    return true;
}

bool testMissingFilesAreRejected(const QString &sourceRoot)
{
    QTemporaryDir temporary;
    if (!temporary.isValid()) {
        printLine(QStringLiteral("FAIL cannot create temporary reference directory"));
        return false;
    }

    const QString sourceManifest = QDir(sourceRoot).filePath(QStringLiteral("manifest.json"));
    const QString targetManifest = QDir(temporary.path()).filePath(QStringLiteral("manifest.json"));
    if (!QFile::copy(sourceManifest, targetManifest)) {
        printLine(QStringLiteral("FAIL cannot copy manifest for missing-file test"));
        return false;
    }

    QDir().mkpath(QDir(temporary.path()).filePath(QStringLiteral("images")));
    return requireNoSuggestion(
        temporary.path(),
        QStringLiteral("Ouvre le schéma de brochage MEMS 1.3 ECU"),
        QStringLiteral("manifest entry whose local SVG is absent"));
}

bool testUndeclaredDiagramIsRejected()
{
    QTemporaryDir temporary;
    if (!temporary.isValid()) {
        printLine(QStringLiteral("FAIL cannot create temporary reference directory"));
        return false;
    }

    QDir().mkpath(QDir(temporary.path()).filePath(QStringLiteral("images")));
    QFile manifest(QDir(temporary.path()).filePath(QStringLiteral("manifest.json")));
    if (!manifest.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        printLine(QStringLiteral("FAIL cannot create reduced manifest"));
        return false;
    }
    manifest.write("{\"diagrams\":{}}\n");
    manifest.close();

    QFile fake(QDir(temporary.path()).filePath(QStringLiteral("images/mems_1_3_ecu_connectors.svg")));
    if (!fake.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        printLine(QStringLiteral("FAIL cannot create undeclared SVG"));
        return false;
    }
    fake.write("<svg xmlns=\"http://www.w3.org/2000/svg\"></svg>\n");
    fake.close();

    return requireNoSuggestion(
        temporary.path(),
        QStringLiteral("Ouvre le schéma de brochage MEMS 1.3 ECU"),
        QStringLiteral("local SVG absent from manifest"));
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const QString referenceRoot = QDir::cleanPath(QStringLiteral(MEMS_SOURCE_REFERENCE_DIR));
    if (referenceRoot.isEmpty() || !QFileInfo(referenceRoot).isDir()) {
        printLine(QStringLiteral("FAIL reference root unavailable: %1").arg(referenceRoot));
        return 2;
    }

    printLine(QStringLiteral("START IA MEMS deterministic diagram self-test"));
    bool ok = true;
    ok = requireSuggestion(referenceRoot,
                           QStringLiteral("Montre-moi le schéma de brochage MEMS 1.2 ECU"),
                           QStringLiteral("MEMS 1.2 ECU"),
                           QStringLiteral("images/mems_1_2_ecu_connector.svg")) && ok;
    ok = requireSuggestion(referenceRoot,
                           QStringLiteral("Quel est le pinout du connecteur MEMS 1.3 ECU ?"),
                           QStringLiteral("MEMS 1.3 ECU"),
                           QStringLiteral("images/mems_1_3_ecu_connectors.svg")) && ok;
    ok = requireSuggestion(referenceRoot,
                           QStringLiteral("Je veux le schéma du connecteur MEMS 1.6 ECU"),
                           QStringLiteral("MEMS 1.6 ECU"),
                           QStringLiteral("images/mems_1_6_ecu_connector.svg")) && ok;
    ok = requireSuggestion(referenceRoot,
                           QStringLiteral("Montre le brochage du connecteur MEMS 1.9 ECU"),
                           QStringLiteral("MEMS 1.9 ECU"),
                           QStringLiteral("images/mems_1_9_ecu_connector.svg")) && ok;
    ok = requireSuggestion(referenceRoot,
                           QStringLiteral("Ouvre le schéma ROSCO 3 broches"),
                           QStringLiteral("ROSCO 3 broches"),
                           QStringLiteral("images/rover_rosco_3pin_black.svg")) && ok;
    ok = requireSuggestion(referenceRoot,
                           QStringLiteral("Schéma de la prise OBD 16 broches MEMS 1.9 J1962"),
                           QStringLiteral("MEMS 1.9 OBD 16 broches"),
                           QStringLiteral("images/mems_1_9_obd_16pin.svg")) && ok;

    ok = requireNoSuggestion(referenceRoot,
                             QStringLiteral("Mon moteur chauffe-t-il trop au ralenti ?"),
                             QStringLiteral("ordinary diagnostic question")) && ok;
    ok = requireNoSuggestion(referenceRoot,
                             QStringLiteral("Montre le schéma de la prise OBD"),
                             QStringLiteral("OBD request without explicit MEMS 1.9")) && ok;
    ok = requireNoSuggestion(referenceRoot,
                             QStringLiteral("Quel est le connecteur de diagnostic MEMS 1.3 ?"),
                             QStringLiteral("ambiguous diagnostic socket request")) && ok;
    ok = testMissingFilesAreRejected(referenceRoot) && ok;
    ok = testUndeclaredDiagramIsRejected() && ok;

    printLine(ok ? QStringLiteral("PASS IA MEMS deterministic diagram self-test")
                 : QStringLiteral("FAIL IA MEMS deterministic diagram self-test"));
    return ok ? 0 : 1;
}
