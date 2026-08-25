#include "ExpertRuntimeDatabase.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>

#include <algorithm>

namespace {

int numericSuffix(const QString &name)
{
    static const QRegularExpression rx(QStringLiteral("_(\\d+)\\.qz64$"),
                                       QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = rx.match(name);
    return match.hasMatch() ? match.captured(1).toInt() : 0;
}

bool looksLikeBase64Text(const QByteArray &data)
{
    const QByteArray trimmed = data.trimmed();
    if (trimmed.isEmpty())
        return false;
    for (char value : trimmed) {
        const unsigned char c = static_cast<unsigned char>(value);
        const bool base64 = (c >= 'A' && c <= 'Z')
            || (c >= 'a' && c <= 'z')
            || (c >= '0' && c <= '9')
            || c == '+' || c == '/' || c == '='
            || c == '\r' || c == '\n' || c == '\t' || c == ' ';
        if (!base64)
            return false;
    }
    return true;
}

QByteArray uncompressQz64Payload(const QByteArray &fileBytes)
{
    if (looksLikeBase64Text(fileBytes))
        return qUncompress(QByteArray::fromBase64(fileBytes.trimmed()));

    // Rover archive lot 1620 is stored as the raw qCompress byte stream rather
    // than as textual Base64. Keep the original data intact and decode that
    // historical packaging directly.
    return qUncompress(fileBytes);
}

QString cleanedSqlText(const QByteArray &sqlBytes)
{
    QString cleaned;
    const QList<QByteArray> lines = sqlBytes.split('\n');
    for (const QByteArray &rawLine : lines) {
        const QString line = QString::fromUtf8(rawLine);
        if (line.trimmed().startsWith(QStringLiteral("--")))
            continue;
        cleaned += line;
        cleaned += QLatin1Char('\n');
    }
    return cleaned;
}

bool hasSqlTerminator(const QString &sql)
{
    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    for (int i = 0; i < sql.size(); ++i) {
        const QChar ch = sql.at(i);
        if (ch == QLatin1Char('\'') && !inDoubleQuote) {
            if (inSingleQuote && i + 1 < sql.size() && sql.at(i + 1) == QLatin1Char('\'')) {
                ++i;
                continue;
            }
            inSingleQuote = !inSingleQuote;
            continue;
        }
        if (ch == QLatin1Char('"') && !inSingleQuote) {
            if (inDoubleQuote && i + 1 < sql.size() && sql.at(i + 1) == QLatin1Char('"')) {
                ++i;
                continue;
            }
            inDoubleQuote = !inDoubleQuote;
            continue;
        }
        if (ch == QLatin1Char(';') && !inSingleQuote && !inDoubleQuote)
            return true;
    }
    return false;
}

bool executeLegacySqlLines(QSqlDatabase &database, const QString &sql, QString *error)
{
    QSqlQuery query(database);
    const QStringList lines = sql.split(QLatin1Char('\n'));
    for (const QString &rawLine : lines) {
        const QString statement = rawLine.trimmed();
        if (statement.isEmpty())
            continue;
        if (!query.exec(statement)) {
            if (error)
                *error = query.lastError().text() + QStringLiteral(" | ") + statement.left(180);
            return false;
        }
    }
    return true;
}

bool executeTerminatedSql(QSqlDatabase &database, const QString &sql, QString *error)
{
    QSqlQuery query(database);
    QString statement;
    statement.reserve(sql.size());
    bool inSingleQuote = false;
    bool inDoubleQuote = false;

    auto executeStatement = [&](const QString &value) -> bool {
        const QString trimmed = value.trimmed();
        if (trimmed.isEmpty())
            return true;
        if (!query.exec(trimmed)) {
            if (error)
                *error = query.lastError().text() + QStringLiteral(" | ") + trimmed.left(180);
            return false;
        }
        return true;
    };

    for (int i = 0; i < sql.size(); ++i) {
        const QChar ch = sql.at(i);

        if (ch == QLatin1Char('\'') && !inDoubleQuote) {
            if (inSingleQuote && i + 1 < sql.size() && sql.at(i + 1) == QLatin1Char('\'')) {
                statement += ch;
                statement += sql.at(++i);
                continue;
            }
            inSingleQuote = !inSingleQuote;
            statement += ch;
            continue;
        }

        if (ch == QLatin1Char('"') && !inSingleQuote) {
            if (inDoubleQuote && i + 1 < sql.size() && sql.at(i + 1) == QLatin1Char('"')) {
                statement += ch;
                statement += sql.at(++i);
                continue;
            }
            inDoubleQuote = !inDoubleQuote;
            statement += ch;
            continue;
        }

        if (ch == QLatin1Char(';') && !inSingleQuote && !inDoubleQuote) {
            if (!executeStatement(statement))
                return false;
            statement.clear();
            continue;
        }

        statement += ch;
    }

    if (inSingleQuote || inDoubleQuote) {
        if (error)
            *error = QStringLiteral("Instruction SQL incomplète : guillemet non fermé");
        return false;
    }

    return executeStatement(statement);
}

bool executeSqlLines(QSqlDatabase &database, const QByteArray &sqlBytes, QString *error)
{
    const QString cleaned = cleanedSqlText(sqlBytes);

    // Historical MEMS seed/enrichment files use one complete SQL statement per
    // line and do not require a trailing semicolon. New expert lots can contain
    // multiline statements explicitly terminated by ';'. Support both formats
    // without changing the historical database representation.
    if (!hasSqlTerminator(cleaned))
        return executeLegacySqlLines(database, cleaned, error);

    return executeTerminatedSql(database, cleaned, error);
}

bool executeQz64(QSqlDatabase &database, const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QStringLiteral("Impossible d'ouvrir %1").arg(path);
        return false;
    }
    const QByteArray sqlBytes = uncompressQz64Payload(file.readAll());
    if (sqlBytes.isEmpty()) {
        if (error)
            *error = QStringLiteral("QZ64 vide ou invalide: %1").arg(path);
        return false;
    }
    return executeSqlLines(database, sqlBytes, error);
}

QStringList seedFiles(const QString &root)
{
    QDir dir(root);
    QStringList names = dir.entryList(QStringList() << QStringLiteral("mems_reference_seed_*.qz64"),
                                      QDir::Files, QDir::Name);
    std::sort(names.begin(), names.end(), [](const QString &a, const QString &b) {
        const int na = numericSuffix(a);
        const int nb = numericSuffix(b);
        if (na != nb)
            return na < nb;
        return a < b;
    });
    QStringList result;
    for (const QString &name : names)
        result << dir.filePath(name);
    return result;
}

bool readManifest(const QString &root, int *revision, QStringList *batches, QString *error)
{
    QFile file(QDir(root).filePath(QStringLiteral("manifest.json")));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error)
            *error = QStringLiteral("Manifest MEMS introuvable");
        return false;
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        if (error)
            *error = QStringLiteral("Manifest MEMS invalide");
        return false;
    }
    const QJsonObject object = document.object();
    if (revision)
        *revision = object.value(QStringLiteral("database_revision")).toInt(0);
    if (batches) {
        batches->clear();
        const QJsonArray array = object.value(QStringLiteral("research_enrichment_batches")).toArray();
        for (const QJsonValue &value : array) {
            const QString name = value.toString().trimmed();
            if (!name.isEmpty())
                batches->append(name);
        }
    }
    return true;
}

bool buildRuntimeDatabase(const QString &referenceRoot,
                          const QString &destination,
                          int revision,
                          const QStringList &batches,
                          QString *error)
{
    const QString temporary = destination + QStringLiteral(".tmp-")
        + QUuid::createUuid().toString(QUuid::WithoutBraces);
    QFile::remove(temporary);

    const QString connection = QStringLiteral("IA_MEMS_BUILD_%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
    database.setDatabaseName(temporary);

    bool ok = database.open();
    if (!ok && error)
        *error = database.lastError().text();

    if (ok) {
        QSqlQuery pragma(database);
        pragma.exec(QStringLiteral("PRAGMA journal_mode=MEMORY"));
        pragma.exec(QStringLiteral("PRAGMA synchronous=OFF"));
        pragma.exec(QStringLiteral("PRAGMA temp_store=MEMORY"));
    }

    // The seed is one compressed/base64 stream split over several files.
    if (ok) {
        QByteArray encodedSeed;
        const QStringList parts = seedFiles(referenceRoot);
        if (parts.isEmpty()) {
            ok = false;
            if (error)
                *error = QStringLiteral("Seed MEMS absent");
        }
        for (const QString &path : parts) {
            if (!ok)
                break;
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                ok = false;
                if (error)
                    *error = QStringLiteral("Seed MEMS illisible: %1").arg(path);
                break;
            }
            encodedSeed += file.readAll().trimmed();
        }
        if (ok) {
            const QByteArray seedSql = qUncompress(QByteArray::fromBase64(encodedSeed));
            if (seedSql.isEmpty()) {
                ok = false;
                if (error)
                    *error = QStringLiteral("Seed MEMS invalide");
            } else {
                ok = executeSqlLines(database, seedSql, error);
            }
        }
    }

    // 1600 is a loss-preserving archive of 3.5M raw correlation cells. The
    // understood semantic tables are already in 1500-1540. Keeping 1600 out of
    // the runtime avoids a very large transient SQL allocation while the
    // original qz64 remains shipped, audited and available for future tooling.
    const QString archiveOnly = QStringLiteral("research_enrichment_1600.qz64");
    if (ok) {
        QDir root(referenceRoot);
        for (const QString &batch : batches) {
            if (batch == archiveOnly)
                continue;
            const QString path = root.filePath(batch);
            if (!QFileInfo::exists(path)) {
                ok = false;
                if (error)
                    *error = QStringLiteral("Lot MEMS absent: %1").arg(batch);
                break;
            }
            QString batchError;
            if (!executeQz64(database, path, &batchError)) {
                ok = false;
                if (error)
                    *error = QStringLiteral("Échec lot %1: %2").arg(batch, batchError);
                break;
            }
        }
    }

    if (ok && revision > 0) {
        QSqlQuery query(database);
        ok = query.exec(QStringLiteral("PRAGMA user_version=%1").arg(revision));
        if (!ok && error)
            *error = query.lastError().text();
    }

    database.close();
    database = QSqlDatabase();
    QSqlDatabase::removeDatabase(connection);

    if (!ok) {
        QFile::remove(temporary);
        return false;
    }

    QFile::remove(destination);
    if (!QFile::rename(temporary, destination)) {
        QFile::remove(temporary);
        if (error)
            *error = QStringLiteral("Impossible de finaliser le cache IA MEMS");
        return false;
    }
    return true;
}

} // namespace

QString ExpertRuntimeDatabase::referenceRoot() const
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/database/reference");
}

QString ExpertRuntimeDatabase::cacheRoot() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        + QStringLiteral("/ia-mems");
}

bool ExpertRuntimeDatabase::buildOrOpen()
{
    m_lastError.clear();
    m_databasePath.clear();
    m_manifestRevision = 0;

    QStringList batches;
    if (!readManifest(referenceRoot(), &m_manifestRevision, &batches, &m_lastError))
        return false;
    if (m_manifestRevision <= 0) {
        m_lastError = QStringLiteral("Révision de base MEMS invalide");
        return false;
    }

    // Normal packaged BUILD path: GitHub Actions prepares the expert SQLite
    // database once and ships it read-only beside the application. This avoids
    // rebuilding/decompressing the knowledge database when the IA tab opens,
    // while Qwen is also resident in memory. The historical runtime builder is
    // retained below only as a fallback for development/legacy packages.
    const QString packagedDatabase = QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("database/expert/ia_mems_reference_r%1.sqlite")
                      .arg(m_manifestRevision));
    const QFileInfo packagedInfo(packagedDatabase);
    if (packagedInfo.exists() && packagedInfo.isFile() && packagedInfo.size() > 0) {
        m_databasePath = packagedInfo.absoluteFilePath();
        return true;
    }

    const QString root = cacheRoot();
    if (!QDir().mkpath(root)) {
        m_lastError = QStringLiteral("Impossible de créer le dossier cache IA MEMS");
        return false;
    }

    m_databasePath = QDir(root).filePath(
        QStringLiteral("ia_mems_reference_r%1.sqlite").arg(m_manifestRevision));
    if (QFileInfo::exists(m_databasePath))
        return true;

    QLockFile lock(m_databasePath + QStringLiteral(".lock"));
    lock.setStaleLockTime(10 * 60 * 1000);
    if (!lock.tryLock(120000)) {
        m_lastError = QStringLiteral("Le cache IA MEMS est déjà en cours de construction");
        return false;
    }

    if (QFileInfo::exists(m_databasePath))
        return true;

    if (!buildRuntimeDatabase(referenceRoot(), m_databasePath,
                              m_manifestRevision, batches, &m_lastError)) {
        m_databasePath.clear();
        return false;
    }
    return true;
}

QString ExpertRuntimeDatabase::databasePath() const
{
    return m_databasePath;
}

QString ExpertRuntimeDatabase::lastError() const
{
    return m_lastError;
}

int ExpertRuntimeDatabase::manifestRevision() const
{
    return m_manifestRevision;
}
