#include "MemsDocumentaryPackCatalog.h"

#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

namespace {

QString normalizedLanguage(QString language)
{
    language = language.left(2).toLower();
    static const QStringList supported = {
        QStringLiteral("fr"), QStringLiteral("en"), QStringLiteral("pt"),
        QStringLiteral("es"), QStringLiteral("it"), QStringLiteral("de")
    };
    return supported.contains(language) ? language : QStringLiteral("fr");
}

bool validHttpsRelease(const QString &url)
{
    const QUrl parsed(url, QUrl::StrictMode);
    if (!parsed.isValid() || parsed.scheme() != QStringLiteral("https") ||
        parsed.host().compare(QStringLiteral("github.com"), Qt::CaseInsensitive) != 0 ||
        !parsed.userInfo().isEmpty() || !parsed.query().isEmpty() ||
        !parsed.fragment().isEmpty() || parsed.port(-1) != -1)
        return false;
    // Only release assets in the updater repository may be fetched. Reject
    // redirector URLs or links to any other repo, including private data.
    static const QRegularExpression releaseAsset(
        QStringLiteral("^/mini56/ECU-Mems-Manager-Session-MAJ/releases/"
                       "download/[A-Za-z0-9._-]+/[A-Za-z0-9._-]+$"));
    return releaseAsset.match(parsed.path()).hasMatch();
}

} // namespace

QString MemsDocumentaryPackCatalog::manifestUrl()
{
    return QStringLiteral(
        "https://raw.githubusercontent.com/mini56/"
        "ECU-Mems-Manager-Session-MAJ/main/manifest.json");
}

QString MemsDocumentaryPackCatalog::selectedLanguage()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       QStringLiteral("ECU_Mems_Manager"));
    settings.beginGroup(QStringLiteral("Settings"));
    return normalizedLanguage(
        settings.value(QStringLiteral("Language"), QStringLiteral("fr")).toString());
}

QString MemsDocumentaryPackCatalog::installedDatabasePath(const QString &language)
{
    const QString root = QStandardPaths::writableLocation(
        QStandardPaths::AppLocalDataLocation);
    if (root.isEmpty())
        return QString();
    const QString code = normalizedLanguage(language);
    return QDir(root).filePath(
        QStringLiteral("documentary/bases/%1/mems_documentary_%1.sqlite").arg(code));
}

MemsDocumentaryOffer MemsDocumentaryPackCatalog::parseManifest(
    const QByteArray &json, const QString &language)
{
    MemsDocumentaryOffer offer;
    offer.language = normalizedLanguage(language);
    if (json.size() > 256 * 1024) {
        offer.error = QStringLiteral("MANIFEST_TOO_LARGE");
        return offer;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        offer.error = QStringLiteral("MANIFEST_INVALID_JSON");
        return offer;
    }
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("schema_version")).toInt() != 2 ||
        root.value(QStringLiteral("application")).toString() !=
            QStringLiteral("ECU MEMS Manager SESSION") ||
        root.value(QStringLiteral("documentary_schema")).toString() !=
            QStringLiteral("mems-documentary-multisource-v1")) {
        offer.error = QStringLiteral("MANIFEST_UNSUPPORTED_SCHEMA");
        return offer;
    }
    const QJsonObject bases = root.value(QStringLiteral("documentary_bases")).toObject();
    const QJsonObject entry = bases.value(offer.language).toObject();
    if (entry.value(QStringLiteral("language")).toString() != offer.language ||
        entry.value(QStringLiteral("database_file")).toString() !=
            QStringLiteral("mems_documentary_%1.sqlite").arg(offer.language)) {
        offer.error = QStringLiteral("MANIFEST_WRONG_LANGUAGE");
        return offer;
    }
    // The published manifest currently advertises NO bases. "Unavailable"
    // must never turn into a request to install the old embedded RAVE SQLite.
    if (!entry.value(QStringLiteral("available")).toBool(false))
        return offer;

    const QString version = entry.value(QStringLiteral("version")).toString();
    const QString url = entry.value(QStringLiteral("release_url")).toString();
    const QString hash = entry.value(QStringLiteral("sha256")).toString();
    const QRegularExpression safeVersion(QStringLiteral("^[0-9A-Za-z._-]{1,50}$"));
    const QRegularExpression hashShape(QStringLiteral("^[a-fA-F0-9]{64}$"));
    if (!safeVersion.match(version).hasMatch() ||
        !validHttpsRelease(url) || !hashShape.match(hash).hasMatch() ||
        !entry.value(QStringLiteral("installation_password_required")).toBool() ||
        !entry.value(QStringLiteral("encrypted_package_required")).toBool() ||
        entry.value(QStringLiteral("document_count")).toInt() <= 0 ||
        entry.value(QStringLiteral("published_documents")).toArray().isEmpty()) {
        offer.error = QStringLiteral("MANIFEST_PUBLISHED_PACK_NOT_VERIFIED");
        return offer;
    }

    offer.available = true;
    offer.version = version;
    offer.releaseUrl = url;
    offer.sha256 = hash.toLower();
    return offer;
}
