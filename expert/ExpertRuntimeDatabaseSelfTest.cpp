#include "ExpertRuntimeDatabase.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTextStream>
#include <QUuid>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("ECU Mems Manager"));
    QCoreApplication::setApplicationName(QStringLiteral("ECU Mems Manager"));

    ExpertRuntimeDatabase runtime;
    if (!runtime.buildOrOpen()) {
        QTextStream(stderr) << "EXPERT RUNTIME DB FAIL: " << runtime.lastError() << Qt::endl;
        return 1;
    }

    const QString path = runtime.databasePath();
    const QFileInfo file(path);
    if (!file.exists() || !file.isFile() || file.size() <= 0) {
        QTextStream(stderr) << "EXPERT RUNTIME DB FAIL: generated database missing: " << path << Qt::endl;
        return 2;
    }

    const QString connection = QStringLiteral("EXPERT_RUNTIME_SELFTEST_%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
    database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
    database.setDatabaseName(path);
    if (!database.open()) {
        QTextStream(stderr) << "EXPERT RUNTIME DB FAIL: " << database.lastError().text() << Qt::endl;
        database = QSqlDatabase();
        QSqlDatabase::removeDatabase(connection);
        return 3;
    }

    int userVersion = 0;
    int tableCount = 0;
    {
        QSqlQuery query(database);
        if (!query.exec(QStringLiteral("PRAGMA user_version")) || !query.next()) {
            QTextStream(stderr) << "EXPERT RUNTIME DB FAIL: cannot read user_version" << Qt::endl;
            database.close();
            database = QSqlDatabase();
            QSqlDatabase::removeDatabase(connection);
            return 4;
        }
        userVersion = query.value(0).toInt();
    }
    {
        QSqlQuery query(database);
        if (!query.exec(QStringLiteral("SELECT COUNT(*) FROM sqlite_master WHERE type='table'")) || !query.next()) {
            QTextStream(stderr) << "EXPERT RUNTIME DB FAIL: cannot count tables" << Qt::endl;
            database.close();
            database = QSqlDatabase();
            QSqlDatabase::removeDatabase(connection);
            return 5;
        }
        tableCount = query.value(0).toInt();
    }

    database.close();
    database = QSqlDatabase();
    QSqlDatabase::removeDatabase(connection);

    if (userVersion != runtime.manifestRevision() || userVersion <= 0 || tableCount <= 0) {
        QTextStream(stderr) << "EXPERT RUNTIME DB FAIL: revision=" << userVersion
                            << " manifest=" << runtime.manifestRevision()
                            << " tables=" << tableCount << Qt::endl;
        return 6;
    }

    QTextStream(stdout) << "EXPERT_RUNTIME_DATABASE=" << file.absoluteFilePath() << Qt::endl;
    QTextStream(stdout) << "EXPERT_RUNTIME_REVISION=" << userVersion << Qt::endl;
    QTextStream(stdout) << "EXPERT_RUNTIME_TABLES=" << tableCount << Qt::endl;
    return 0;
}
