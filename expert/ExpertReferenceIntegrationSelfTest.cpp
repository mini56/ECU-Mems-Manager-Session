#include "ExpertKnowledgeReader.h"
#include "../database/MemsReferenceDatabase.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>
#include <QDebug>

namespace {

bool tableExists(const QString &databasePath, const QString &table)
{
    const QString connection = QStringLiteral("EXPERT_REFERENCE_SCHEMA_%1").arg(QUuid::createUuid().toString());
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
    db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
    db.setDatabaseName(databasePath);
    bool found = false;
    if (db.open()) {
        QSqlQuery query(db);
        query.prepare(QStringLiteral("SELECT 1 FROM sqlite_master WHERE type='table' AND name=:name LIMIT 1"));
        query.bindValue(QStringLiteral(":name"), table);
        found = query.exec() && query.next();
    }
    db.close();
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connection);
    return found;
}

bool manifestContains1640()
{
    QFile file(QCoreApplication::applicationDirPath() + QStringLiteral("/database/reference/manifest.json"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject())
        return false;
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("database_revision")).toInt() < 19)
        return false;
    const QJsonArray batches = root.value(QStringLiteral("research_enrichment_batches")).toArray();
    for (const QJsonValue &value : batches) {
        if (value.toString() == QStringLiteral("research_enrichment_1640.qz64"))
            return true;
    }
    return false;
}

}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("ECU-MEMS-Expert-Lab"));
    QCoreApplication::setApplicationName(QStringLiteral("reference-selftest-%1").arg(QUuid::createUuid().toString()));
    QStandardPaths::setTestModeEnabled(true);

    if (!manifestContains1640()) {
        qCritical() << "Expert reference manifest does not contain revision 19 / batch 1640";
        return 1;
    }

    MemsReferenceDatabase reference;
    if (!reference.open()) {
        qCritical() << "Could not build/open fused MEMS reference database";
        return 2;
    }

    const QString path = reference.databasePath();
    if (path.isEmpty() || !QFile::exists(path)) {
        qCritical() << "Reference database path is invalid" << path;
        return 3;
    }

    if (!tableExists(path, QStringLiteral("mems_expert_fact_external"))) {
        qCritical() << "Historical expert facts from batch 1630 are missing";
        return 4;
    }
    if (!tableExists(path, QStringLiteral("mems_expert_rule"))
        || !tableExists(path, QStringLiteral("mems_expert_rule_condition"))
        || !tableExists(path, QStringLiteral("mems_expert_rule_recommendation"))) {
        qCritical() << "Expert rule schema from batch 1640 is missing";
        return 5;
    }

    ExpertKnowledgeReader reader;
    if (!reader.openReadOnly(path)) {
        qCritical() << "Expert reader could not open fused reference DB read-only:" << reader.lastError();
        return 6;
    }

    ExpertContext context;
    context.family = QStringLiteral("1.3");
    context.firmware = QStringLiteral("ABEMR002");
    const QList<ExpertFact> facts = reader.facts(context);
    if (facts.isEmpty()) {
        qCritical() << "No 1.3 / ABEMR002 expert facts were recovered from the real fused DB";
        return 7;
    }

    bool foundC8 = false;
    for (const ExpertFact &fact : facts) {
        if (fact.factKey == QStringLiteral("mems13_abem_c8_service5")) {
            foundC8 = true;
            if (fact.verificationLevel != QStringLiteral("source_externe")) {
                qCritical() << "Historical C8 fact provenance was unexpectedly promoted" << fact.verificationLevel;
                return 8;
            }
        }
    }
    if (!foundC8) {
        qCritical() << "Expected ABEMR002 C8/service-5 fact is missing";
        return 9;
    }

    if (!reader.rules(context).isEmpty()) {
        qCritical() << "Batch 1640 must define schema only; no diagnostic rule should be silently invented yet";
        return 10;
    }

    qInfo() << "MEMS expert/reference integration self-test OK"
            << "database=" << path
            << "facts=" << facts.size();
    return 0;
}
