#include "IaMemsLibraryBridge.h"
#include "IaMemsDiagramCatalog.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

#include <iostream>

namespace {

int fail(int code, const QString &message)
{
    std::cerr << "FAIL " << message.toStdString() << "\n";
    return code;
}

bool containsInsensitive(const QString &text, const QString &needle)
{
    return text.contains(needle, Qt::CaseInsensitive);
}

bool visualMatchesPrimaryProof(const QString &referenceRoot,
                               const IaMemsDiagramSuggestion &suggestion,
                               QString *assetKey)
{
    QFile catalog(referenceRoot + QStringLiteral("/runtime_visual_catalog.json"));
    if (!catalog.open(QIODevice::ReadOnly))
        return false;

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(catalog.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        return false;

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
            != QDir::cleanPath(suggestion.relativePath))
            continue;

        const QString key = entry.value(QStringLiteral("asset_entity_key")).toString();
        if (key != QStringLiteral("VIS_P0053_001") && key != QStringLiteral("VIS_P0053_002"))
            return false;
        if (assetKey)
            *assetKey = key;
        return true;
    }
    return false;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (argc != 4) {
        std::cerr << "usage: ia_memslibrary_bridge_selftest <MEMSLibrary.dll> <Pack001-dir> <reference-root>\n";
        return 1;
    }

    const QString dllPath = QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath();
    const QString packDirectory = QFileInfo(QString::fromLocal8Bit(argv[2])).absoluteFilePath();
    const QString referenceRoot = QFileInfo(QString::fromLocal8Bit(argv[3])).absoluteFilePath();
    if (!QFileInfo::exists(dllPath))
        return fail(2, QStringLiteral("corrected MEMSLibrary.dll missing"));
    if (!QFileInfo(packDirectory).isDir())
        return fail(3, QStringLiteral("Pack001 directory missing"));
    if (!QFileInfo(referenceRoot + QStringLiteral("/runtime_visual_catalog.json")).isFile())
        return fail(4, QStringLiteral("clean V2 visual catalog missing"));

    qputenv("MEMS_LIBRARY_DLL", dllPath.toLocal8Bit());
    qputenv("MEMS_LIBRARY_PACK", packDirectory.toLocal8Bit());

    const QString primaryQuestion = QStringLiteral(
        "Quel est le jeu axial du pignon primaire et comment le contrôler ?");
    const QStringList primaryKeywords = {
        QStringLiteral("jeu"), QStringLiteral("axial"), QStringLiteral("pignon"),
        QStringLiteral("primaire"), QStringLiteral("comment"), QStringLiteral("controler"),
        QStringLiteral("primary"), QStringLiteral("end"), QStringLiteral("float")
    };

    const IaMemsLibraryGrounding primary =
        IaMemsLibraryBridge::retrieve(primaryQuestion, primaryKeywords);
    if (!primary.libraryReady)
        return fail(5, QStringLiteral("MEMSLibrary bridge not ready: %1").arg(primary.error));
    if (!primary.provenanceFiltered)
        return fail(6, QStringLiteral("primary proof was not verified through filtered provenance"));
    if (primary.selectedDocument != QStringLiteral("DOC_RCL0193ENG"))
        return fail(7, QStringLiteral("primary wrong document: %1").arg(primary.selectedDocument));
    if (primary.selectedRevision != QStringLiteral("REV_RCL0193ENG_SOURCE"))
        return fail(8, QStringLiteral("primary wrong revision: %1").arg(primary.selectedRevision));
    if (primary.selectedSourceLanguage != QStringLiteral("en"))
        return fail(9, QStringLiteral("primary wrong source language: %1").arg(primary.selectedSourceLanguage));
    if (primary.selectedPage != 53)
        return fail(10, QStringLiteral("primary wrong page: %1").arg(primary.selectedPage));
    if (!containsInsensitive(primary.text, QStringLiteral("0.089"))
        || !containsInsensitive(primary.text, QStringLiteral("0.165"))
        || !containsInsensitive(primary.text, QStringLiteral("feeler gauges"))
        || !containsInsensitive(primary.text, QStringLiteral("12.21.28"))) {
        return fail(11, QStringLiteral("primary proof missing value/control/operation"));
    }
    if (containsInsensitive(primary.text, QStringLiteral("page 342"))
        || containsInsensitive(primary.text, QStringLiteral("page 343"))
        || containsInsensitive(primary.text, QStringLiteral("coaxial"))
        || containsInsensitive(primary.text, QStringLiteral("aerial"))) {
        return fail(12, QStringLiteral("primary proof contaminated by antenna/coax evidence"));
    }

    const IaMemsDiagramSuggestion primaryVisual =
        IaMemsDiagramCatalog::suggestionForEvidence(primaryQuestion, primary.text, referenceRoot);
    if (!primaryVisual.isValid())
        return fail(13, QStringLiteral("no visual followed the selected p53 proof"));
    if (primaryVisual.key != QStringLiteral("RCL0193ENG p.53"))
        return fail(14, QStringLiteral("wrong visual proof key: %1").arg(primaryVisual.key));
    QString primaryAsset;
    if (!visualMatchesPrimaryProof(referenceRoot, primaryVisual, &primaryAsset))
        return fail(15, QStringLiteral("visual is not VIS_P0053_001/002 from the selected proof"));

    const QString batteryQuestion = QStringLiteral(
        "Quelle est la procédure de restauration de la batterie ?");
    const IaMemsLibraryGrounding battery = IaMemsLibraryBridge::retrieve(
        batteryQuestion,
        {QStringLiteral("restauration"), QStringLiteral("batterie"),
         QStringLiteral("battery"), QStringLiteral("restoration")});
    if (!battery.provenanceFiltered
        || battery.selectedDocument != QStringLiteral("DOC_RCL0221ENG")
        || battery.selectedPage != 20
        || !containsInsensitive(battery.text, QStringLiteral("BATTERY RESTORATION PROCEDURE"))) {
        return fail(16, QStringLiteral("battery regression failed doc=%1 page=%2")
            .arg(battery.selectedDocument).arg(battery.selectedPage));
    }

    const IaMemsLibraryGrounding throttle = IaMemsLibraryBridge::retrieve(
        QStringLiteral("throttle potentiometer"),
        {QStringLiteral("throttle"), QStringLiteral("potentiometer")});
    if (!throttle.provenanceFiltered
        || throttle.selectedDocument != QStringLiteral("DOC_RCL0195ENG")
        || throttle.selectedPage != 35
        || !containsInsensitive(throttle.text, QStringLiteral("18.30.24"))) {
        return fail(17, QStringLiteral("throttle regression failed doc=%1 page=%2")
            .arg(throttle.selectedDocument).arg(throttle.selectedPage));
    }

    const IaMemsLibraryGrounding substring = IaMemsLibraryBridge::retrieve(
        QStringLiteral("axial"), {QStringLiteral("axial")});
    if (substring.selectedPage == 342
        || containsInsensitive(substring.text, QStringLiteral("coaxial"))
        || containsInsensitive(substring.text, QStringLiteral("aerial"))) {
        return fail(18, QStringLiteral("substring guard failed: axial still matched coaxial"));
    }

    std::cout
        << "IA_MEMSLIBRARY_BRIDGE_FILTERED_PASS"
        << " primary=DOC_RCL0193ENG:p53"
        << " revision=REV_RCL0193ENG_SOURCE"
        << " language=en"
        << " visual=" << primaryAsset.toStdString()
        << " battery=DOC_RCL0221ENG:p20"
        << " throttle=DOC_RCL0195ENG:p35"
        << " axial_coaxial_rejected=1"
        << " p342=0"
        << "\n";
    return 0;
}
