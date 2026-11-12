#include "MemsDocumentaryPackCatalog.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QFileInfo>
#include <cstdio>

static bool expect(bool ok, const char *message)
{
    if (!ok)
        std::fprintf(stderr,"DOCUMENTARY_UPDATE_SELFTEST_FAILED %s\n",message);
    return ok;
}

static QJsonObject initialManifest()
{
    QJsonObject entry;
    entry.insert(QStringLiteral("language"), QStringLiteral("fr"));
    entry.insert(QStringLiteral("database_file"),
                 QStringLiteral("mems_documentary_fr.sqlite"));
    entry.insert(QStringLiteral("available"), false);
    entry.insert(QStringLiteral("document_count"), 0);
    entry.insert(QStringLiteral("installation_password_required"), true);
    entry.insert(QStringLiteral("encrypted_package_required"), true);
    QJsonObject bases;
    bases.insert(QStringLiteral("fr"), entry);
    QJsonObject root;
    root.insert(QStringLiteral("schema_version"), 2);
    root.insert(QStringLiteral("application"),
                QStringLiteral("ECU MEMS Manager SESSION"));
    root.insert(QStringLiteral("documentary_schema"),
                QStringLiteral("mems-documentary-multisource-v1"));
    root.insert(QStringLiteral("documentary_bases"), bases);
    return root;
}

static QByteArray json(const QJsonObject &object)
{
    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

int main(int argc,char **argv)
{
    QCoreApplication application(argc,argv);
    QTemporaryDir temporary;
    if (!expect(temporary.isValid(),"TEMPORARY_TEST_DIR"))
        return 1;
    qputenv("XDG_DATA_HOME",temporary.path().toUtf8());
    QJsonObject manifest=initialManifest();
    MemsDocumentaryOffer result =
        MemsDocumentaryPackCatalog::parseManifest(
            json(manifest),QStringLiteral("fr"));
    if (!expect(!result.available && result.error.isEmpty(),
                "UNPUBLISHED_PACK_MUST_NOT_DOWNLOAD"))
        return 1;
    if (!expect(MemsDocumentaryPackCatalog::manifestUrl().startsWith(
                    QStringLiteral("https://raw.githubusercontent.com/")),
                "OFFICIAL_MANIFEST_URL"))
        return 1;
    if (!expect(MemsDocumentaryPackCatalog::installedDatabasePath(
                    QStringLiteral("fr")).contains(
                    QStringLiteral("mems_documentary_fr.sqlite")),
                "FR_DOCUMENTARY_ONLY_PATH"))
        return 1;
    if (!expect(MemsDocumentaryPackCatalog::installedDatabasePath(
                    QStringLiteral("en")) !=
                MemsDocumentaryPackCatalog::installedDatabasePath(
                    QStringLiteral("fr")),
                "LANGUAGE_DATABASES_ARE_SEPARATE"))
        return 1;

    QJsonObject bases=manifest.value(QStringLiteral("documentary_bases")).toObject();
    QJsonObject entry=bases.value(QStringLiteral("fr")).toObject();
    entry.insert(QStringLiteral("available"),true);
    entry.insert(QStringLiteral("version"),QStringLiteral("fr-2026.11.12"));
    entry.insert(QStringLiteral("release_url"),QStringLiteral(
        "https://github.com/mini56/ECU-Mems-Manager-Session-MAJ/"
        "releases/download/fr-2026.11.12/mems_documentary_fr.encrypted"));
    entry.insert(QStringLiteral("sha256"),QString(64,QLatin1Char('a')));
    entry.insert(QStringLiteral("document_count"),3);
    QJsonArray sources;
    QJsonObject source;
    source.insert(QStringLiteral("document_id"),QStringLiteral("rover-akm7169"));
    sources.append(source);
    entry.insert(QStringLiteral("published_documents"),sources);
    bases.insert(QStringLiteral("fr"),entry);
    manifest.insert(QStringLiteral("documentary_bases"),bases);
    result=MemsDocumentaryPackCatalog::parseManifest(
        json(manifest),QStringLiteral("fr"));
    if (!expect(result.available && result.error.isEmpty() &&
                result.language==QStringLiteral("fr") &&
                result.version==QStringLiteral("fr-2026.11.12"),
                "VALID_ENCRYPTED_RELEASE_ACCEPTED"))
        return 1;

    entry.insert(QStringLiteral("encrypted_package_required"),false);
    bases.insert(QStringLiteral("fr"),entry);
    manifest.insert(QStringLiteral("documentary_bases"),bases);
    result=MemsDocumentaryPackCatalog::parseManifest(
        json(manifest),QStringLiteral("fr"));
    if (!expect(!result.available && !result.error.isEmpty(),
                "UNENCRYPTED_RELEASE_REJECTED"))
        return 1;

    entry.insert(QStringLiteral("encrypted_package_required"),true);
    entry.insert(QStringLiteral("release_url"),QStringLiteral(
        "https://github.com/mini56/OTHER/release/filename"));
    bases.insert(QStringLiteral("fr"),entry);
    manifest.insert(QStringLiteral("documentary_bases"),bases);
    result=MemsDocumentaryPackCatalog::parseManifest(
        json(manifest),QStringLiteral("fr"));
    if (!expect(!result.available && !result.error.isEmpty(),
                "FOREIGN_REPO_URL_REJECTED"))
        return 1;

    result=MemsDocumentaryPackCatalog::parseManifest(
        json(manifest),QStringLiteral("pt"));
    if (!expect(!result.available && !result.error.isEmpty(),
                "WRONG_LANGUAGE_PACKAGE_REJECTED"))
        return 1;
    std::puts("DOCUMENTARY_UPDATE_CATALOG_TEST_OK");
    return 0;
}
