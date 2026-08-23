#include "ExpertKnowledgeReader.h"
#include "ExpertRuntimeDatabase.h"

#include <QCoreApplication>
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
    QCoreApplication::setApplicationName(QStringLiteral("runtime-selftest-%1").arg(QUuid::createUuid().toString()));
    QStandardPaths::setTestModeEnabled(true);

    if (!manifestContains1640()) {
        qCritical() << "Expert runtime manifest does not contain revision 19 / batch 1640";
        return 1;
    }

    ExpertRuntimeDatabase reference;
    if (!reference.buildOrOpen()) {
        qCritical() << "Could not build/open compact IA MEMS reference database:" << reference.lastError();
        return 2;
    }

    const QString path = reference.databasePath();
    if (path.isEmpty() || !QFile::exists(path)) {
        qCritical() << "IA MEMS runtime database path is invalid" << path;
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

    // The 3.5M-cell raw preservation layer remains shipped as qz64 but is not
    // materialized into the 32-bit IA runtime. Its decoded semantic tables from
    // 1500-1540 remain present.
    if (tableExists(path, QStringLiteral("mems_correlation_cell_external"))) {
        qCritical() << "Archive-only batch 1600 was unexpectedly materialized in IA runtime";
        return 6;
    }
    if (!tableExists(path, QStringLiteral("mems_scalar_definition"))
        || !tableExists(path, QStringLiteral("mems_variable_correlation"))) {
        qCritical() << "Semantic Andrew MEMSTools knowledge is missing from IA runtime";
        return 7;
    }

    ExpertKnowledgeReader reader;
    if (!reader.openReadOnly(path)) {
        qCritical() << "Expert reader could not open IA runtime DB read-only:" << reader.lastError();
        return 8;
    }

    ExpertContext context;
    context.family = QStringLiteral("1.3");
    context.firmware = QStringLiteral("ABEMR002");
    const QList<ExpertFact> facts = reader.facts(context);
    if (facts.isEmpty()) {
        qCritical() << "No 1.3 / ABEMR002 expert facts were recovered from IA runtime";
        return 9;
    }

    bool foundC8 = false;
    for (const ExpertFact &fact : facts) {
        if (fact.factKey == QStringLiteral("mems13_abem_c8_service5")) {
            foundC8 = true;
            if (fact.verificationLevel != QStringLiteral("source_externe")) {
                qCritical() << "Historical C8 fact provenance was unexpectedly promoted" << fact.verificationLevel;
                return 10;
            }
        }
    }
    if (!foundC8) {
        qCritical() << "Expected ABEMR002 C8/service-5 fact is missing";
        return 11;
    }

    if (!reader.rules(context).isEmpty()) {
        qCritical() << "Batch 1640 must define schema only; no diagnostic rule should be silently invented yet";
        return 12;
    }

    qInfo() << "IA MEMS compact expert/reference self-test OK"
            << "revision=" << reference.manifestRevision()
            << "database=" << path
            << "facts=" << facts.size();
    return 0;
}
