#include "ExpertKnowledgeReader.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVariant>

ExpertKnowledgeReader::ExpertKnowledgeReader()
    : m_connectionName(QStringLiteral("MEMS_EXPERT_%1").arg(QUuid::createUuid().toString()))
{
}

ExpertKnowledgeReader::~ExpertKnowledgeReader()
{
    close();
}

bool ExpertKnowledgeReader::openReadOnly(const QString &databasePath)
{
    close();
    m_lastError.clear();

    m_database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
    m_database.setDatabaseName(databasePath);
    if (!m_database.open()) {
        m_lastError = m_database.lastError().text();
        return false;
    }

    QSqlQuery query(m_database);
    query.exec(QStringLiteral("PRAGMA query_only = ON"));
    return true;
}

void ExpertKnowledgeReader::close()
{
    if (!m_database.isValid())
        return;

    if (m_database.isOpen())
        m_database.close();

    const QString connection = m_connectionName;
    m_database = QSqlDatabase();
    QSqlDatabase::removeDatabase(connection);
}

bool ExpertKnowledgeReader::isOpen() const
{
    return m_database.isValid() && m_database.isOpen();
}

QString ExpertKnowledgeReader::lastError() const
{
    return m_lastError;
}

bool ExpertKnowledgeReader::tableExists(const QString &tableName) const
{
    if (!isOpen())
        return false;

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("SELECT 1 FROM sqlite_master WHERE type='table' AND name=:name LIMIT 1"));
    query.bindValue(QStringLiteral(":name"), tableName);
    return query.exec() && query.next();
}

bool ExpertKnowledgeReader::familyMatches(const QString &ruleFamily, const QString &contextFamily)
{
    const QString wanted = ruleFamily.trimmed();
    if (wanted.isEmpty() || wanted == QStringLiteral("*"))
        return true;
    return wanted.compare(contextFamily.trimmed(), Qt::CaseInsensitive) == 0;
}

bool ExpertKnowledgeReader::firmwareMatches(const QString &ruleFirmware, const QString &contextFirmware)
{
    const QString wanted = ruleFirmware.trimmed();
    if (wanted.isEmpty() || wanted == QStringLiteral("*"))
        return true;
    return wanted.compare(contextFirmware.trimmed(), Qt::CaseInsensitive) == 0;
}

QList<ExpertFact> ExpertKnowledgeReader::facts(const ExpertContext &context) const
{
    QList<ExpertFact> result;
    if (!tableExists(QStringLiteral("mems_expert_fact_external")))
        return result;

    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral(
            "SELECT source_key,fact_key,family,firmware_code,topic,statement,verification_level,notes "
            "FROM mems_expert_fact_external ORDER BY family,firmware_code,fact_key")))
        return result;

    while (query.next()) {
        ExpertFact fact;
        fact.sourceKey = query.value(0).toString();
        fact.factKey = query.value(1).toString();
        fact.family = query.value(2).toString();
        fact.firmware = query.value(3).toString();
        fact.topic = query.value(4).toString();
        fact.statement = query.value(5).toString();
        fact.verificationLevel = query.value(6).toString();
        fact.notes = query.value(7).toString();

        if (!familyMatches(fact.family, context.family))
            continue;
        if (!firmwareMatches(fact.firmware, context.firmware))
            continue;
        result.append(fact);
    }
    return result;
}

QList<ExpertRuleCondition> ExpertKnowledgeReader::conditionsForRule(const QString &ruleKey) const
{
    QList<ExpertRuleCondition> result;
    if (!tableExists(QStringLiteral("mems_expert_rule_condition")))
        return result;

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT parameter,metric,comparator,right_parameter,right_metric,threshold_low,threshold_high,"
        "right_multiplier,right_offset,required,weight,notes "
        "FROM mems_expert_rule_condition WHERE rule_key=:rule ORDER BY condition_order,id"));
    query.bindValue(QStringLiteral(":rule"), ruleKey);
    if (!query.exec())
        return result;

    while (query.next()) {
        ExpertRuleCondition condition;
        condition.parameter = query.value(0).toString();
        condition.metric = query.value(1).toString();
        condition.comparator = query.value(2).toString();
        condition.rightParameter = query.value(3).toString();
        condition.rightMetric = query.value(4).toString();
        condition.thresholdLow = query.value(5).toDouble();
        condition.thresholdHigh = query.value(6).toDouble();
        condition.rightMultiplier = query.value(7).isNull() ? 1.0 : query.value(7).toDouble();
        condition.rightOffset = query.value(8).toDouble();
        condition.required = query.value(9).isNull() ? true : query.value(9).toInt() != 0;
        condition.weight = query.value(10).isNull() ? 1.0 : query.value(10).toDouble();
        condition.notes = query.value(11).toString();
        result.append(condition);
    }
    return result;
}

QStringList ExpertKnowledgeReader::recommendationsForRule(const QString &ruleKey) const
{
    QStringList result;
    if (!tableExists(QStringLiteral("mems_expert_rule_recommendation")))
        return result;

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT recommendation FROM mems_expert_rule_recommendation "
        "WHERE rule_key=:rule ORDER BY step_order,id"));
    query.bindValue(QStringLiteral(":rule"), ruleKey);
    if (!query.exec())
        return result;

    while (query.next())
        result.append(query.value(0).toString());
    return result;
}

QList<ExpertRule> ExpertKnowledgeReader::rules(const ExpertContext &context) const
{
    QList<ExpertRule> result;
    if (!tableExists(QStringLiteral("mems_expert_rule")))
        return result;

    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral(
            "SELECT rule_key,family,firmware_code,title,category,severity,base_confidence,min_samples,"
            "verification_level,notes FROM mems_expert_rule WHERE enabled<>0 ORDER BY priority DESC,rule_key")))
        return result;

    while (query.next()) {
        ExpertRule rule;
        rule.ruleKey = query.value(0).toString();
        rule.family = query.value(1).toString();
        rule.firmware = query.value(2).toString();
        rule.title = query.value(3).toString();
        rule.category = query.value(4).toString();
        rule.severity = query.value(5).toInt();
        rule.baseConfidence = query.value(6).toDouble();
        rule.minimumSamples = qMax(1, query.value(7).toInt());
        rule.verificationLevel = query.value(8).toString();
        rule.notes = query.value(9).toString();

        if (!familyMatches(rule.family, context.family))
            continue;
        if (!firmwareMatches(rule.firmware, context.firmware))
            continue;

        rule.conditions = conditionsForRule(rule.ruleKey);
        rule.recommendations = recommendationsForRule(rule.ruleKey);
        result.append(rule);
    }
    return result;
}
