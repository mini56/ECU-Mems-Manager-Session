#include "ExpertEngine.h"
#include "ExpertKnowledgeReader.h"

#include <QCoreApplication>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUuid>

namespace {

bool execSql(QSqlDatabase &db, const QString &sql)
{
    QSqlQuery query(db);
    if (query.exec(sql))
        return true;
    qCritical() << "SQL self-test failure:" << query.lastError().text() << sql;
    return false;
}

bool createTestDatabase(const QString &path)
{
    const QString connection = QStringLiteral("EXPERT_SELFTEST_BUILD_%1").arg(QUuid::createUuid().toString());
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
    db.setDatabaseName(path);
    if (!db.open())
        return false;

    bool ok = true;
    ok = ok && execSql(db, QStringLiteral(
        "CREATE TABLE mems_expert_fact_external("
        "source_key TEXT,fact_key TEXT,family TEXT,firmware_code TEXT,topic TEXT,statement TEXT,verification_level TEXT,notes TEXT)"));
    ok = ok && execSql(db, QStringLiteral(
        "CREATE TABLE mems_expert_rule("
        "rule_key TEXT PRIMARY KEY,family TEXT,firmware_code TEXT,title TEXT,category TEXT,severity INTEGER,"
        "base_confidence REAL,min_samples INTEGER,verification_level TEXT,notes TEXT,enabled INTEGER,priority INTEGER)"));
    ok = ok && execSql(db, QStringLiteral(
        "CREATE TABLE mems_expert_rule_condition("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,rule_key TEXT,condition_order INTEGER,parameter TEXT,metric TEXT,comparator TEXT,"
        "right_parameter TEXT,right_metric TEXT,threshold_low REAL,threshold_high REAL,right_multiplier REAL,right_offset REAL,"
        "required INTEGER,weight REAL,notes TEXT)"));
    ok = ok && execSql(db, QStringLiteral(
        "CREATE TABLE mems_expert_rule_recommendation("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,rule_key TEXT,step_order INTEGER,recommendation TEXT)"));

    ok = ok && execSql(db, QStringLiteral(
        "INSERT INTO mems_expert_fact_external VALUES("
        "'project_test','fact_1','1.6','AANMP002','selftest','Validated test fact','decoded_by_project','self-test only')"));

    ok = ok && execSql(db, QStringLiteral(
        "INSERT INTO mems_expert_rule VALUES("
        "'project_rule','1.6','AANMP002','Running voltage inconsistency','electrical',2,0.90,3,'decoded_by_project',"
        "'Synthetic self-test rule: two independent parameters are required.',1,100)"));
    ok = ok && execSql(db, QStringLiteral(
        "INSERT INTO mems_expert_rule_condition(rule_key,condition_order,parameter,metric,comparator,threshold_low,required,weight,notes) VALUES"
        "('project_rule',1,'engine_rpm','mean','gt',700,1,1.0,'engine running'),"
        "('project_rule',2,'battery_voltage','mean','lt',12.0,1,1.0,'synthetic low-voltage threshold')"));
    ok = ok && execSql(db, QStringLiteral(
        "INSERT INTO mems_expert_rule_recommendation(rule_key,step_order,recommendation) VALUES"
        "('project_rule',1,'Verify charging voltage and wiring before replacing components.')"));

    ok = ok && execSql(db, QStringLiteral(
        "INSERT INTO mems_expert_rule VALUES("
        "'external_rule','1.6','AANMP002','External-source hypothesis','external',1,0.95,3,'source_externe',"
        "'Synthetic self-test external rule.',1,50)"));
    ok = ok && execSql(db, QStringLiteral(
        "INSERT INTO mems_expert_rule_condition(rule_key,condition_order,parameter,metric,comparator,threshold_low,required,weight) VALUES"
        "('external_rule',1,'engine_rpm','mean','gt',700,1,1.0)"));

    ok = ok && execSql(db, QStringLiteral(
        "INSERT INTO mems_expert_rule VALUES("
        "'conflict_rule','1.6','AANMP002','Conflicted rule must be blocked','safety',3,1.0,1,'conflit_a_verifier',"
        "'This rule must never generate a hypothesis.',1,1000)"));
    ok = ok && execSql(db, QStringLiteral(
        "INSERT INTO mems_expert_rule_condition(rule_key,condition_order,parameter,metric,comparator,threshold_low,required,weight) VALUES"
        "('conflict_rule',1,'engine_rpm','mean','gt',1,1,1.0)"));

    db.close();
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connection);
    return ok;
}

ExpertObservation sample(qint64 timestamp, double rpm, double voltage, double map)
{
    ExpertObservation value;
    value.timestampMs = timestamp;
    value.values.insert(QStringLiteral("engine_rpm"), rpm);
    value.values.insert(QStringLiteral("battery_voltage"), voltage);
    value.values.insert(QStringLiteral("map_kpa"), map);
    return value;
}

}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir temp;
    if (!temp.isValid()) {
        qCritical() << "Could not create expert self-test directory";
        return 1;
    }

    const QString dbPath = temp.filePath(QStringLiteral("expert.sqlite"));
    if (!createTestDatabase(dbPath)) {
        qCritical() << "Could not create expert self-test database";
        return 2;
    }

    ExpertKnowledgeReader reader;
    if (!reader.openReadOnly(dbPath)) {
        qCritical() << "Could not open expert self-test database read-only:" << reader.lastError();
        return 3;
    }

    ExpertEngine engine;
    ExpertContext context;
    context.family = QStringLiteral("1.6");
    context.firmware = QStringLiteral("AANMP002");
    engine.setContext(context);
    engine.setKnowledgeReader(&reader);

    engine.addSample(sample(0,    900.0, 11.5, 35.0));
    engine.addSample(sample(1000, 1000.0, 11.6, 40.0));
    engine.addSample(sample(2000, 1100.0, 11.7, 45.0));

    const ExpertAnalysisResult result = engine.analyze();
    if (result.sampleCount != 3) {
        qCritical() << "Unexpected expert sample count" << result.sampleCount;
        return 4;
    }
    if (!result.statistics.contains(QStringLiteral("engine_rpm"))) {
        qCritical() << "Missing RPM statistics";
        return 5;
    }
    const ExpertSeriesStats rpm = result.statistics.value(QStringLiteral("engine_rpm"));
    if (rpm.count != 3 || qAbs(rpm.mean - 1000.0) > 0.001 || qAbs(rpm.slopePerSecond - 100.0) > 0.001) {
        qCritical() << "Unexpected RPM statistics" << rpm.count << rpm.mean << rpm.slopePerSecond;
        return 6;
    }
    if (result.contextFacts.size() != 1 || result.contextFacts.first().factKey != QStringLiteral("fact_1")) {
        qCritical() << "Expert fact filtering failed" << result.contextFacts.size();
        return 7;
    }
    if (result.hypotheses.size() != 2) {
        qCritical() << "Expected exactly two non-conflicted hypotheses, got" << result.hypotheses.size();
        return 8;
    }
    if (result.hypotheses.first().ruleKey != QStringLiteral("project_rule")) {
        qCritical() << "Hypothesis ranking failed" << result.hypotheses.first().ruleKey;
        return 9;
    }
    if (!result.hypotheses.first().strongConclusionAllowed) {
        qCritical() << "Project-validated rule should permit strong conclusion at this confidence";
        return 10;
    }

    bool externalFound = false;
    for (const ExpertHypothesis &hypothesis : result.hypotheses) {
        if (hypothesis.ruleKey == QStringLiteral("conflict_rule")) {
            qCritical() << "Conflicted rule was not blocked";
            return 11;
        }
        if (hypothesis.ruleKey == QStringLiteral("external_rule")) {
            externalFound = true;
            if (hypothesis.strongConclusionAllowed) {
                qCritical() << "External-only rule must not permit a strong conclusion";
                return 12;
            }
        }
    }
    if (!externalFound) {
        qCritical() << "External rule did not produce its expected hypothesis";
        return 13;
    }

    qInfo() << "MEMS expert engine self-test OK"
            << "hypotheses=" << result.hypotheses.size()
            << "rpm_mean=" << rpm.mean
            << "rpm_slope=" << rpm.slopePerSecond;
    return 0;
}
