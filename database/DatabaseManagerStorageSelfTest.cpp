#include "database/DatabaseManager.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QDebug>

// Exercised from a read-only application directory, to emulate an ordinary
// non-admin user executing from Program Files. Test mode never touches normal
// ECU MEMS Manager user data.
static bool check(bool ok, const QString &message)
{
    if (!ok) qCritical().noquote() << "SQLITE_STORAGE_SELFTEST_FAIL:" << message;
    return ok;
}

static QByteArray digest(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd()) hash.addData(file.read(65536));
    return hash.result();
}

static bool legacyFixture(const QString &path)
{
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) return false;
    bool good = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("fixture"));
        db.setDatabaseName(path);
        if (db.open()) {
            QSqlQuery query(db);
            good = query.exec(QStringLiteral(
                "CREATE TABLE migration_marker (id INTEGER PRIMARY KEY, payload TEXT)"))
                && query.exec(QStringLiteral(
                "INSERT INTO migration_marker VALUES (1,'preexisting-user-record')"));
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(QStringLiteral("fixture"));
    return good;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (argc < 2) return 2;
    const QString mode = QString::fromLatin1(argv[1]);
    const bool upgrade = mode == QStringLiteral("prepare-upgrade")
        || mode == QStringLiteral("upgrade")
        || mode == QStringLiteral("reopen-upgrade");
    if (!check(upgrade || mode == QStringLiteral("fresh")
        || mode == QStringLiteral("reopen-fresh"), QStringLiteral("invalid test mode")))
        return 2;

    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("ECU MEMS Manager SQLite CI"));
    QCoreApplication::setApplicationName(upgrade
        ? QStringLiteral("Upgrade117")
        : QStringLiteral("Fresh117"));

    const QString oldFile = QCoreApplication::applicationDirPath()
        + QStringLiteral("/database/ecu_mems_manager.sqlite");
    const QString newFile = QStandardPaths::writableLocation(
        QStandardPaths::AppLocalDataLocation)
        + QStringLiteral("/database/ecu_mems_manager.sqlite");

    if (mode == QStringLiteral("prepare-upgrade")) {
        if (!check(!QFileInfo::exists(oldFile), QStringLiteral("legacy fixture already exists"))) return 1;
        if (!check(legacyFixture(oldFile), QStringLiteral("failed to create legacy SQLite"))) return 1;
        qInfo() << "SQLITE_LEGACY_FIXTURE_PASS";
        return 0;
    }

    const bool firstRun = mode == QStringLiteral("fresh")
        || mode == QStringLiteral("upgrade");

    if (firstRun && !check(!QFileInfo::exists(newFile),
            QStringLiteral("test profile already contains SQLite")))
        return 1;
    if (mode == QStringLiteral("fresh") && !check(
            !QFileInfo::exists(oldFile), QStringLiteral("fresh install has legacy database")))
        return 1;
    if (upgrade && !check(
            QFileInfo::exists(oldFile), QStringLiteral("upgrade fixture missing")))
        return 1;

    const QByteArray oldHash = upgrade ? digest(oldFile) : QByteArray();
    if (upgrade && !check(!oldHash.isEmpty(), QStringLiteral("legacy hash missing"))) return 1;

    {
        DatabaseManager database;
        if (!check(database.open(), QStringLiteral("DatabaseManager::open failed"))) return 1;
        if (!check(QDir::cleanPath(database.databasePath()) == QDir::cleanPath(newFile),
                QStringLiteral("database is not under writable per-user AppLocalData"))) return 1;
        if (!check(QFileInfo::exists(newFile), QStringLiteral("personal database not created"))) return 1;

        {
            QSqlQuery sql(database.database());
            if (upgrade) {
                if (!check(sql.exec(QStringLiteral(
                        "SELECT payload FROM migration_marker WHERE id=1"))
                    && sql.next() && sql.value(0).toString()
                        == QStringLiteral("preexisting-user-record"),
                        QStringLiteral("existing personal data were not preserved")))
                    return 1;
            } else if (mode == QStringLiteral("fresh")) {
                if (!check(sql.exec(QStringLiteral(
                        "CREATE TABLE ci_persistent (value TEXT)"))
                    && sql.exec(QStringLiteral(
                        "INSERT INTO ci_persistent VALUES ('saved-user-record')")),
                        QStringLiteral("fresh install cannot persist data")))
                    return 1;
            } else {
                if (!check(sql.exec(QStringLiteral(
                        "SELECT value FROM ci_persistent"))
                    && sql.next() && sql.value(0).toString()
                        == QStringLiteral("saved-user-record"),
                        QStringLiteral("previous fresh-install record disappeared")))
                    return 1;
            }
        }
        database.close();
    }

    if (upgrade && !check(digest(oldFile) == oldHash,
            QStringLiteral("installer modified legacy database"))) return 1;
    if (mode == QStringLiteral("fresh") && !check(!QFileInfo::exists(oldFile),
            QStringLiteral("new install wrote SQLite into Program Files"))) return 1;

    qInfo() << "SQLITE_STORAGE_SELFTEST_PASS" << mode
            << "per_user_db=" << newFile;
    return 0;
}
