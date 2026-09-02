#include "IaMemsDiagramCatalog.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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

QString sha256Hex(const QByteArray &data)
{
    return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}

bool writeFile(const QString &path, const QByteArray &data)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    return file.write(data) == data.size();
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
        printLine(QStringLiteral("FAIL %1 unexpectedly resolved: %2 / %3")
                      .arg(label, suggestion.key, suggestion.relativePath));
        return false;
    }
    printLine(QStringLiteral("PASS no diagram: %1").arg(label));
    return true;
}

bool requireResponseSuggestion(const QString &root,
                               const QString &response,
                               const QString &expectedKey,
                               const QString &expectedRelativePath,
                               const QString &label)
{
    const IaMemsDiagramSuggestion suggestion =
        IaMemsDiagramCatalog::suggestionForResponse(response, root);
    if (!suggestion.isValid()) {
        printLine(QStringLiteral("FAIL no response-linked diagram: %1").arg(label));
        return false;
    }
    if (suggestion.key != expectedKey || suggestion.relativePath != expectedRelativePath) {
        printLine(QStringLiteral("FAIL wrong response-linked diagram: %1 -> %2 / %3")
                      .arg(label, suggestion.key, suggestion.relativePath));
        return false;
    }
    if (!QFileInfo::exists(suggestion.absolutePath)) {
        printLine(QStringLiteral("FAIL missing response-linked file: %1").arg(suggestion.absolutePath));
        return false;
    }
    printLine(QStringLiteral("PASS response-linked diagram: %1 -> %2")
                  .arg(label, suggestion.key));
    return true;
}

bool requireNoResponseSuggestion(const QString &root,
                                 const QString &response,
                                 const QString &label)
{
    const IaMemsDiagramSuggestion suggestion =
        IaMemsDiagramCatalog::suggestionForResponse(response, root);
    if (suggestion.isValid()) {
        printLine(QStringLiteral("FAIL response unexpectedly resolved: %1 -> %2 / %3")
                      .arg(label, suggestion.key, suggestion.relativePath));
        return false;
    }
    printLine(QStringLiteral("PASS no response-linked diagram: %1").arg(label));
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

QJsonObject runtimeEntry(const QString &sourceType,
                         const QString &runtimeKey,
                         const QString &runtimePath,
                         const QByteArray &bytes,
                         const QString &context,
                         bool visible,
                         const QString &publication = QString(),
                         int page = 0,
                         const QString &treatment = QStringLiteral("ravemems_source"))
{
    QJsonObject entry;
    entry.insert(QStringLiteral("source_type"), sourceType);
    entry.insert(QStringLiteral("runtime_key"), runtimeKey);
    entry.insert(QStringLiteral("runtime_path"), runtimePath);
    entry.insert(QStringLiteral("sha256"), sha256Hex(bytes));
    entry.insert(QStringLiteral("context_text"), context);
    entry.insert(QStringLiteral("ui_label"), QStringLiteral("Voir le schéma"));
    entry.insert(QStringLiteral("ui_visible"), visible);
    entry.insert(QStringLiteral("publication_code"), publication);
    if (page > 0)
        entry.insert(QStringLiteral("physical_page"), page);
    entry.insert(QStringLiteral("treatment"), treatment);
    return entry;
}

bool testRuntimeCatalog()
{
    QTemporaryDir temporary;
    if (!temporary.isValid()) {
        printLine(QStringLiteral("FAIL cannot create runtime catalog temporary directory"));
        return false;
    }

    const QByteArray raveBytes("PNG-RAVEMEMS-PURGE\n");
    const QByteArray legacyBytes("PNG-LEGACY-AKM6348-HUB\n");
    const QByteArray hiddenBytes("PNG-HIDDEN-TEXT-CAPTURE\n");
    const QByteArray replacedBytes("PNG-REPLACED-LEGACY\n");
    const QByteArray traversalBytes("PNG-OUTSIDE-ROOT\n");

    if (!writeFile(QDir(temporary.path()).filePath(QStringLiteral("ravemems/purge.png")), raveBytes)
        || !writeFile(QDir(temporary.path()).filePath(QStringLiteral("legacy/akm6348_hub.png")), legacyBytes)
        || !writeFile(QDir(temporary.path()).filePath(QStringLiteral("hidden/text_page.png")), hiddenBytes)
        || !writeFile(QDir(temporary.path()).filePath(QStringLiteral("legacy/replaced.png")), replacedBytes)
        || !writeFile(QDir(temporary.path()).filePath(QStringLiteral("../outside.png")), traversalBytes)) {
        printLine(QStringLiteral("FAIL cannot create runtime catalog fixture files"));
        return false;
    }

    QJsonArray entries;
    entries.append(runtimeEntry(
        QStringLiteral("ravemems"),
        QStringLiteral("RAVEMEMS::OCC::PURGE"),
        QStringLiteral("ravemems/purge.png"),
        raveBytes,
        QStringLiteral("EVAP canister purge valve ECM wiring and hose routing"),
        true,
        QStringLiteral("RCL0193ENG"),
        99));
    entries.append(runtimeEntry(
        QStringLiteral("legacy"),
        QStringLiteral("LEGACY::AKM6348_FRONT_HUB"),
        QStringLiteral("legacy/akm6348_hub.png"),
        legacyBytes,
        QStringLiteral("AKM6348 front hub exploded view wheel bearing"),
        true,
        QString(),
        0,
        QStringLiteral("conserver_migrer_legacy")));
    entries.append(runtimeEntry(
        QStringLiteral("legacy"),
        QStringLiteral("LEGACY::TEXT_CAPTURE"),
        QStringLiteral("hidden/text_page.png"),
        hiddenBytes,
        QStringLiteral("pure text page hidden capture uniquephrase"),
        false,
        QString(),
        0,
        QStringLiteral("retirer_de_ui_conserver_provenance")));
    entries.append(runtimeEntry(
        QStringLiteral("legacy"),
        QStringLiteral("LEGACY::REPLACED"),
        QStringLiteral("legacy/replaced.png"),
        replacedBytes,
        QStringLiteral("old purge legacy replaced uniqueold"),
        true,
        QString(),
        0,
        QStringLiteral("remplacer_par_ravemems")));
    entries.append(runtimeEntry(
        QStringLiteral("ravemems"),
        QStringLiteral("RAVEMEMS::MISSING"),
        QStringLiteral("ravemems/missing.png"),
        QByteArray("missing"),
        QStringLiteral("absent unique diagram missingfixture"),
        true,
        QStringLiteral("RCL0194ENG"),
        17));
    entries.append(runtimeEntry(
        QStringLiteral("ravemems"),
        QStringLiteral("RAVEMEMS::TRAVERSAL"),
        QStringLiteral("../outside.png"),
        traversalBytes,
        QStringLiteral("traversal unique diagram escapeprobe"),
        true,
        QStringLiteral("RCL0194ENG"),
        19));

    QJsonObject root;
    root.insert(QStringLiteral("ui_label"), QStringLiteral("Voir le schéma"));
    root.insert(QStringLiteral("entries"), entries);
    if (!writeFile(QDir(temporary.path()).filePath(QStringLiteral("runtime_visual_catalog.json")),
                   QJsonDocument(root).toJson(QJsonDocument::Indented))) {
        printLine(QStringLiteral("FAIL cannot write runtime visual catalog"));
        return false;
    }

    bool ok = true;
    ok = requireSuggestion(
             temporary.path(),
             QStringLiteral("Voir le schéma de la purge canister RCL0193ENG"),
             QStringLiteral("RCL0193ENG p.99"),
             QStringLiteral("ravemems/purge.png")) && ok;
    ok = requireSuggestion(
             temporary.path(),
             QStringLiteral("Voir le schéma front hub AKM6348 wheel bearing"),
             QStringLiteral("LEGACY::AKM6348_FRONT_HUB"),
             QStringLiteral("legacy/akm6348_hub.png")) && ok;
    ok = requireNoSuggestion(
             temporary.path(),
             QStringLiteral("Voir le schéma pure text page uniquephrase"),
             QStringLiteral("runtime text-only capture hidden from UI")) && ok;
    ok = requireSuggestion(
             temporary.path(),
             QStringLiteral("Voir le schéma old purge legacy uniqueold"),
             QStringLiteral("RCL0193ENG p.99"),
             QStringLiteral("ravemems/purge.png")) && ok;
    ok = requireNoSuggestion(
             temporary.path(),
             QStringLiteral("Voir le schéma absent unique missingfixture"),
             QStringLiteral("runtime path absent")) && ok;
    ok = requireNoSuggestion(
             temporary.path(),
             QStringLiteral("Voir le schéma traversal unique escapeprobe"),
             QStringLiteral("runtime path traversal")) && ok;

    ok = requireResponseSuggestion(
             temporary.path(),
             QStringLiteral("Illustration locale: rave:RCL0193ENG:PDF:99"),
             QStringLiteral("RCL0193ENG p.99"),
             QStringLiteral("ravemems/purge.png"),
             QStringLiteral("structured response reference FR")) && ok;
    ok = requireResponseSuggestion(
             temporary.path(),
             QStringLiteral("Local illustration: rave:RCL0193ENG:PDF:99"),
             QStringLiteral("RCL0193ENG p.99"),
             QStringLiteral("ravemems/purge.png"),
             QStringLiteral("structured response reference EN")) && ok;
    ok = requireResponseSuggestion(
             temporary.path(),
             QStringLiteral("Ilustración local: rave:RCL0193ENG:PDF:99"),
             QStringLiteral("RCL0193ENG p.99"),
             QStringLiteral("ravemems/purge.png"),
             QStringLiteral("structured response reference ES")) && ok;
    ok = requireResponseSuggestion(
             temporary.path(),
             QStringLiteral("Technischer Hinweis: ravemems/purge.png"),
             QStringLiteral("RCL0193ENG p.99"),
             QStringLiteral("ravemems/purge.png"),
             QStringLiteral("runtime path reference DE")) && ok;
    ok = requireNoResponseSuggestion(
             temporary.path(),
             QStringLiteral("A response without any packaged image reference."),
             QStringLiteral("response without visual reference")) && ok;
    ok = requireNoResponseSuggestion(
             temporary.path(),
             QStringLiteral("rave:RCL0194ENG:PDF:17"),
             QStringLiteral("response reference whose runtime file is absent")) && ok;

    printLine(ok ? QStringLiteral("PASS runtime visual catalog fixture")
                 : QStringLiteral("FAIL runtime visual catalog fixture"));
    return ok;
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
    ok = testRuntimeCatalog() && ok;

    printLine(ok ? QStringLiteral("PASS IA MEMS deterministic diagram self-test")
                 : QStringLiteral("FAIL IA MEMS deterministic diagram self-test"));
    return ok ? 0 : 1;
}
