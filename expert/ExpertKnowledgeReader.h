#ifndef EXPERTKNOWLEDGEREADER_H
#define EXPERTKNOWLEDGEREADER_H

#include "ExpertTypes.h"

#include <QSqlDatabase>
#include <QSqlError>

class ExpertKnowledgeReader
{
public:
    ExpertKnowledgeReader();
    ~ExpertKnowledgeReader();

    bool openReadOnly(const QString &databasePath);
    void close();
    bool isOpen() const;
    QString lastError() const;

    QList<ExpertFact> facts(const ExpertContext &context) const;
    QList<ExpertRule> rules(const ExpertContext &context) const;

private:
    bool tableExists(const QString &tableName) const;
    QList<ExpertRuleCondition> conditionsForRule(const QString &ruleKey) const;
    QStringList recommendationsForRule(const QString &ruleKey) const;
    static bool familyMatches(const QString &ruleFamily, const QString &contextFamily);
    static bool firmwareMatches(const QString &ruleFirmware, const QString &contextFirmware);

private:
    QSqlDatabase m_database;
    QString m_connectionName;
    QString m_lastError;
};

#endif // EXPERTKNOWLEDGEREADER_H
