#include "ExpertKnowledgeReader.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVariant>

namespace {

QString canonicalFamily(QString value)
{
    value = value.trimmed().toLower();
    value.remove(QStringLiteral("mems"));
    return value.trimmed();
}

QString joinedNonEmpty(const QStringList &parts, const QString &separator)
{
    QStringList clean;
    for (const QString &part : parts) {
        const QString value = part.trimmed();
        if (!value.isEmpty())
            clean.append(value);
    }
    return clean.join(separator);
}

QString valueText(const QVariant &numeric,
                  const QVariant &minimum,
                  const QVariant &maximum,
                  const QVariant &angle,
                  const QString &text,
                  const QString &unit)
{
    QString value;
    if (!text.trimmed().isEmpty()) {
        value = text.trimmed();
    } else if (!angle.isNull()) {
        value = QStringLiteral("%1°").arg(angle.toDouble(), 0, 'g', 12);
    } else if (!minimum.isNull() || !maximum.isNull()) {
        const QString low = minimum.isNull() ? QStringLiteral("?")
                                              : QString::number(minimum.toDouble(), 'g', 12);
        const QString high = maximum.isNull() ? QStringLiteral("?")
                                               : QString::number(maximum.toDouble(), 'g', 12);
        value = QStringLiteral("%1–%2").arg(low, high);
    } else if (!numeric.isNull()) {
        value = QString::number(numeric.toDouble(), 'g', 12);
    }

    if (!value.isEmpty() && !unit.trimmed().isEmpty() && !value.endsWith(unit.trimmed()))
        value += QStringLiteral(" ") + unit.trimmed();
    return value;
}

QString inferredAssetFamily(const QString &relativePath)
{
    const QString text = relativePath.toLower();
    const struct Pattern { const char *a; const char *b; const char *c; const char *family; } patterns[] = {
        {"1_2", "1.2", "1-2", "1.2"},
        {"1_3", "1.3", "1-3", "1.3"},
        {"1_6", "1.6", "1-6", "1.6"},
        {"1_9", "1.9", "1-9", "1.9"}
    };
    for (const Pattern &pattern : patterns) {
        if (text.contains(QString::fromLatin1(pattern.a))
            || text.contains(QString::fromLatin1(pattern.b))
            || text.contains(QString::fromLatin1(pattern.c)))
            return QString::fromLatin1(pattern.family);
    }
    return QString();
}

QString humanAssetName(QString relativePath)
{
    QString text = QFileInfo(relativePath).completeBaseName();
    text.replace(QLatin1Char('_'), QLatin1Char(' '));
    text.replace(QLatin1Char('-'), QLatin1Char(' '));
    return text.simplified();
}

} // namespace

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
    return canonicalFamily(wanted) == canonicalFamily(contextFamily);
}

bool ExpertKnowledgeReader::firmwareMatches(const QString &ruleFirmware, const QString &contextFirmware)
{
    const QString wanted = ruleFirmware.trimmed();
    if (wanted.isEmpty() || wanted == QStringLiteral("*"))
        return true;
    return wanted.compare(contextFirmware.trimmed(), Qt::CaseInsensitive) == 0;
}

QList<ExpertFact> ExpertKnowledgeReader::historicalFacts(const ExpertContext &context) const
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

QString ExpertKnowledgeReader::scopeText(const QString &knowledgeKey,
                                         const ExpertContext &context,
                                         bool *compatible,
                                         QString *resolvedFamily) const
{
    if (compatible)
        *compatible = true;
    if (resolvedFamily)
        resolvedFamily->clear();
    if (!tableExists(QStringLiteral("mems_knowledge_scope"))
        || !tableExists(QStringLiteral("mems_applicability_scope")))
        return QStringLiteral("Portée: non précisée (UNKNOWN)");

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT s.scope_key,s.scope_kind,s.make,s.model,s.engine_family,s.engine_code,"
        "s.engine_variant,s.induction,s.mems_family,s.transmission,s.year_from,s.year_to,"
        "s.vin_from,s.vin_to,s.market,s.compression_variant,s.catalyst_state,"
        "s.air_conditioning_state,s.source_scope_text,ks.applicability "
        "FROM mems_knowledge_scope ks "
        "JOIN mems_applicability_scope s ON s.scope_key=ks.scope_key "
        "WHERE ks.knowledge_key=:key ORDER BY s.scope_key"));
    query.bindValue(QStringLiteral(":key"), knowledgeKey);
    if (!query.exec())
        return QStringLiteral("Portée: non précisée (UNKNOWN)");

    QStringList scopes;
    bool haveRows = false;
    bool exactFamily = false;
    bool unknownFamily = false;
    const QString wantedFamily = canonicalFamily(context.family);

    while (query.next()) {
        haveRows = true;
        const QString memsFamily = query.value(8).toString().trimmed();
        if (memsFamily.isEmpty()) {
            unknownFamily = true;
        } else if (!wantedFamily.isEmpty() && canonicalFamily(memsFamily) == wantedFamily) {
            exactFamily = true;
            if (resolvedFamily && resolvedFamily->isEmpty())
                *resolvedFamily = memsFamily;
        } else if (wantedFamily.isEmpty() && resolvedFamily && resolvedFamily->isEmpty()) {
            *resolvedFamily = memsFamily;
        }

        QStringList parts;
        parts << query.value(1).toString()
              << query.value(2).toString()
              << query.value(3).toString()
              << query.value(4).toString()
              << query.value(5).toString()
              << query.value(6).toString()
              << query.value(7).toString()
              << memsFamily
              << query.value(9).toString();
        if (!query.value(10).isNull() || !query.value(11).isNull()) {
            const QString from = query.value(10).isNull() ? QStringLiteral("?") : query.value(10).toString();
            const QString to = query.value(11).isNull() ? QStringLiteral("+") : query.value(11).toString();
            parts << QStringLiteral("%1-%2").arg(from, to);
        }
        if (!query.value(12).toString().trimmed().isEmpty() || !query.value(13).toString().trimmed().isEmpty())
            parts << QStringLiteral("VIN %1..%2").arg(query.value(12).toString(), query.value(13).toString());
        parts << query.value(14).toString()
              << query.value(15).toString()
              << query.value(16).toString()
              << query.value(17).toString()
              << query.value(18).toString();
        const QString applicability = query.value(19).toString().trimmed();
        if (!applicability.isEmpty() && applicability != QStringLiteral("applies"))
            parts << applicability;
        scopes << joinedNonEmpty(parts, QStringLiteral(" | "));
    }

    if (!haveRows)
        return QStringLiteral("Portée: non précisée (UNKNOWN)");

    // NULL / absent MEMS family remains UNKNOWN and therefore stays a candidate.
    // A knowledge item is rejected only when every proved scope explicitly names
    // another MEMS family. This prevents a wrong-family fact reaching Qwen while
    // never turning missing source information into an invented ANY.
    if (!wantedFamily.isEmpty() && !exactFamily && !unknownFamily) {
        if (compatible)
            *compatible = false;
        return QStringLiteral("Portée incompatible avec MEMS %1").arg(context.family.trimmed());
    }

    const QString rank = exactFamily ? QStringLiteral("EXACT") : QStringLiteral("UNKNOWN");
    return QStringLiteral("Portée %1: %2").arg(rank, scopes.join(QStringLiteral(" ; ")));
}

QString ExpertKnowledgeReader::specificationText(const QString &knowledgeKey) const
{
    if (!tableExists(QStringLiteral("mems_specification"))
        || !tableExists(QStringLiteral("mems_specification_value")))
        return QString();

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT s.spec_key,s.operation,s.parameter,s.default_unit,s.condition_text,"
        "v.sequence_no,v.value_numeric,v.value_min,v.value_max,v.angle_deg,v.value_text,"
        "COALESCE(v.unit,s.default_unit),v.instruction_text "
        "FROM mems_specification s "
        "LEFT JOIN mems_specification_value v ON v.spec_key=s.spec_key "
        "WHERE s.knowledge_key=:key ORDER BY s.spec_key,v.sequence_no"));
    query.bindValue(QStringLiteral(":key"), knowledgeKey);
    if (!query.exec())
        return QString();

    QStringList lines;
    while (query.next()) {
        const QString value = valueText(query.value(6), query.value(7), query.value(8),
                                        query.value(9), query.value(10).toString(),
                                        query.value(11).toString());
        QStringList parts;
        parts << QStringLiteral("Spécification %1").arg(query.value(2).toString());
        if (!query.value(1).toString().trimmed().isEmpty())
            parts << QStringLiteral("opération=%1").arg(query.value(1).toString());
        if (!value.isEmpty())
            parts << QStringLiteral("valeur=%1").arg(value);
        if (!query.value(4).toString().trimmed().isEmpty())
            parts << QStringLiteral("condition=%1").arg(query.value(4).toString());
        if (!query.value(12).toString().trimmed().isEmpty())
            parts << query.value(12).toString();
        lines << joinedNonEmpty(parts, QStringLiteral(" ; "));
    }
    return lines.join(QLatin1Char('\n'));
}

QString ExpertKnowledgeReader::procedureText(const QString &knowledgeKey) const
{
    if (!tableExists(QStringLiteral("mems_procedure")))
        return QString();

    QSqlQuery procedures(m_database);
    procedures.prepare(QStringLiteral(
        "SELECT procedure_key,operation,title_source FROM mems_procedure "
        "WHERE knowledge_key=:key ORDER BY procedure_key"));
    procedures.bindValue(QStringLiteral(":key"), knowledgeKey);
    if (!procedures.exec())
        return QString();

    QStringList output;
    while (procedures.next()) {
        const QString procedureKey = procedures.value(0).toString();
        QString header = QStringLiteral("Procédure %1").arg(procedures.value(1).toString());
        if (!procedures.value(2).toString().trimmed().isEmpty())
            header += QStringLiteral(" — ") + procedures.value(2).toString().trimmed();
        output << header;

        if (tableExists(QStringLiteral("mems_procedure_step"))) {
            QSqlQuery steps(m_database);
            steps.prepare(QStringLiteral(
                "SELECT step_no,instruction_source,condition_text,figure_ref,related_spec_key "
                "FROM mems_procedure_step WHERE procedure_key=:procedure ORDER BY step_no"));
            steps.bindValue(QStringLiteral(":procedure"), procedureKey);
            if (steps.exec()) {
                while (steps.next()) {
                    QString line = QStringLiteral("Étape %1: %2")
                                       .arg(steps.value(0).toInt())
                                       .arg(steps.value(1).toString());
                    if (!steps.value(2).toString().trimmed().isEmpty())
                        line += QStringLiteral(" [condition: %1]").arg(steps.value(2).toString());
                    if (!steps.value(3).toString().trimmed().isEmpty())
                        line += QStringLiteral(" [figure: %1]").arg(steps.value(3).toString());
                    if (!steps.value(4).toString().trimmed().isEmpty())
                        line += QStringLiteral(" [spec: %1]").arg(steps.value(4).toString());
                    output << line;
                }
            }
        }

        if (tableExists(QStringLiteral("mems_procedure_requirement"))) {
            QSqlQuery requirements(m_database);
            requirements.prepare(QStringLiteral(
                "SELECT step_no,requirement_type,requirement_source,part_number,quantity,unit,figure_ref "
                "FROM mems_procedure_requirement WHERE procedure_key=:procedure "
                "ORDER BY COALESCE(step_no,0),requirement_no"));
            requirements.bindValue(QStringLiteral(":procedure"), procedureKey);
            if (requirements.exec()) {
                while (requirements.next()) {
                    QStringList parts;
                    parts << QStringLiteral("Exigence %1").arg(requirements.value(1).toString())
                          << requirements.value(2).toString();
                    if (!requirements.value(0).isNull())
                        parts << QStringLiteral("étape %1").arg(requirements.value(0).toInt());
                    if (!requirements.value(3).toString().trimmed().isEmpty())
                        parts << QStringLiteral("réf. %1").arg(requirements.value(3).toString());
                    if (!requirements.value(4).isNull())
                        parts << QStringLiteral("%1 %2").arg(requirements.value(4).toString(), requirements.value(5).toString());
                    if (!requirements.value(6).toString().trimmed().isEmpty())
                        parts << QStringLiteral("figure %1").arg(requirements.value(6).toString());
                    output << joinedNonEmpty(parts, QStringLiteral(" ; "));
                }
            }
        }
    }
    return output.join(QLatin1Char('\n'));
}

QList<ExpertFact> ExpertKnowledgeReader::foundationFacts(const ExpertContext &context) const
{
    QList<ExpertFact> result;
    if (!tableExists(QStringLiteral("mems_knowledge_item")))
        return result;

    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral(
            "SELECT knowledge_key,domain,knowledge_type,topic,component_key,source_key,document,"
            "source_section,verification_level,legacy_rave_fact_key,source_text,image_ref,notes "
            "FROM mems_knowledge_item ORDER BY knowledge_key")))
        return result;

    while (query.next()) {
        const QString knowledgeKey = query.value(0).toString();
        bool compatible = true;
        QString resolvedFamily;
        const QString scope = scopeText(knowledgeKey, context, &compatible, &resolvedFamily);
        if (!compatible)
            continue;

        ExpertFact fact;
        fact.sourceKey = query.value(5).toString();
        fact.factKey = knowledgeKey;
        fact.family = resolvedFamily;
        fact.topic = joinedNonEmpty({query.value(3).toString(), query.value(4).toString(),
                                     query.value(1).toString(), query.value(2).toString()},
                                    QStringLiteral(" "));
        fact.verificationLevel = query.value(8).toString();

        QStringList statement;
        statement << query.value(10).toString();
        const QString specifications = specificationText(knowledgeKey);
        if (!specifications.isEmpty())
            statement << specifications;
        const QString procedures = procedureText(knowledgeKey);
        if (!procedures.isEmpty())
            statement << procedures;
        if (!query.value(11).toString().trimmed().isEmpty())
            statement << QStringLiteral("Illustration locale: %1").arg(query.value(11).toString().trimmed());
        fact.statement = joinedNonEmpty(statement, QLatin1Char('\n'));

        QStringList notes;
        notes << scope;
        if (!query.value(6).toString().trimmed().isEmpty())
            notes << QStringLiteral("Document: %1").arg(query.value(6).toString());
        if (!query.value(7).toString().trimmed().isEmpty())
            notes << QStringLiteral("Section: %1").arg(query.value(7).toString());
        if (!query.value(9).toString().trimmed().isEmpty())
            notes << QStringLiteral("RAVE historique: %1").arg(query.value(9).toString());
        notes << query.value(12).toString();
        fact.notes = joinedNonEmpty(notes, QStringLiteral(" | "));

        if (!fact.statement.trimmed().isEmpty())
            result.append(fact);
    }
    return result;
}

QList<ExpertFact> ExpertKnowledgeReader::packagedAssetFacts(const ExpertContext &context) const
{
    QList<ExpertFact> result;
    if (!isOpen())
        return result;

    const QFileInfo databaseInfo(m_database.databaseName());
    QDir expertDir(databaseInfo.absolutePath());
    const QString referenceRoot = QDir::cleanPath(expertDir.filePath(QStringLiteral("../reference")));
    const QFileInfo rootInfo(referenceRoot);
    if (!rootInfo.exists() || !rootInfo.isDir())
        return result;

    const QSet<QString> imageExtensions = {
        QStringLiteral("svg"), QStringLiteral("png"), QStringLiteral("jpg"),
        QStringLiteral("jpeg"), QStringLiteral("webp"), QStringLiteral("gif")
    };
    const QSet<QString> documentExtensions = {
        QStringLiteral("pdf"), QStringLiteral("html"), QStringLiteral("htm"),
        QStringLiteral("md"), QStringLiteral("txt"), QStringLiteral("csv"),
        QStringLiteral("xml")
    };

    QDir base(referenceRoot);
    QDirIterator iterator(referenceRoot, QDir::Files | QDir::NoDotAndDotDot,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        const QString absolute = iterator.next();
        const QFileInfo file(absolute);
        const QString suffix = file.suffix().toLower();
        const bool image = imageExtensions.contains(suffix);
        const bool document = documentExtensions.contains(suffix);
        if (!image && !document)
            continue;

        const QString relative = base.relativeFilePath(absolute).replace(QLatin1Char('\\'), QLatin1Char('/'));
        if (relative == QStringLiteral("manifest.json"))
            continue;

        const QString family = inferredAssetFamily(relative);
        if (!context.family.trimmed().isEmpty() && !family.isEmpty()
            && !familyMatches(family, context.family))
            continue;

        ExpertFact fact;
        fact.sourceKey = QStringLiteral("packaged_reference_asset");
        fact.factKey = QStringLiteral("asset:%1").arg(relative);
        fact.family = family;
        fact.topic = joinedNonEmpty({humanAssetName(relative), file.fileName(), relative,
                                     image ? QStringLiteral("schema image illustration wiring")
                                           : QStringLiteral("document documentation source")},
                                    QStringLiteral(" "));
        fact.statement = image
            ? QStringLiteral("Illustration technique locale disponible dans le package: %1").arg(relative)
            : QStringLiteral("Document technique local disponible dans le package: %1").arg(relative);
        fact.verificationLevel = QStringLiteral("non_verifie");
        fact.notes = QStringLiteral(
            "Asset local indexé automatiquement. Sa présence est certaine, mais son contenu ne doit pas être inventé s'il n'est pas aussi décrit par une connaissance structurée.");
        result.append(fact);
    }
    return result;
}

QList<ExpertFact> ExpertKnowledgeReader::facts(const ExpertContext &context) const
{
    QList<ExpertFact> result = historicalFacts(context);

    // The additive foundation is the preferred route for all new RAVE/service
    // knowledge. Historical tables stay readable during the transition.
    const QList<ExpertFact> foundation = foundationFacts(context);
    result.append(foundation);

    // Reference files are discovered from the packaged directory itself. New
    // documents and images therefore become searchable as assets without a new
    // C++ switch/case or a hard-coded filename list.
    const QList<ExpertFact> assets = packagedAssetFacts(context);
    result.append(assets);
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
