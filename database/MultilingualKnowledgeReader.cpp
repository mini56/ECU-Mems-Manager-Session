#include "MultilingualKnowledgeReader.h"

#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace {

QVariant nullableValue(const QSqlQuery &query, int column)
{
    const QVariant value = query.value(column);
    return value.isNull() ? QVariant() : value;
}

} // namespace

MultilingualKnowledgeReader::MultilingualKnowledgeReader()
    : m_connectionName(QStringLiteral("MEMS_MULTILINGUAL_%1")
                           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
{
}

MultilingualKnowledgeReader::~MultilingualKnowledgeReader()
{
    close();
}

bool MultilingualKnowledgeReader::openReadOnly(const QString &databasePath)
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
    if (!query.exec(QStringLiteral("PRAGMA query_only = ON"))) {
        m_lastError = query.lastError().text();
        close();
        return false;
    }
    return true;
}

void MultilingualKnowledgeReader::close()
{
    if (!m_database.isValid())
        return;

    if (m_database.isOpen())
        m_database.close();

    const QString connection = m_connectionName;
    m_database = QSqlDatabase();
    QSqlDatabase::removeDatabase(connection);
}

bool MultilingualKnowledgeReader::isOpen() const
{
    return m_database.isValid() && m_database.isOpen();
}

QString MultilingualKnowledgeReader::lastError() const
{
    return m_lastError;
}

bool MultilingualKnowledgeReader::tableExists(const QString &tableName) const
{
    if (!isOpen())
        return false;

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT 1 FROM sqlite_master WHERE type='table' AND name=:name LIMIT 1"));
    query.bindValue(QStringLiteral(":name"), tableName);
    return query.exec() && query.next();
}

bool MultilingualKnowledgeReader::supportsFoundation() const
{
    static const char *const required[] = {
        "mems_doc_locale", "mems_doc_document", "mems_doc_unit", "mems_doc_entity",
        "mems_doc_text", "mems_doc_visual", "mems_doc_visual_region", "mems_doc_table",
        "mems_doc_table_cell", "mems_doc_value", "mems_doc_relation"
    };
    for (const char *table : required) {
        if (!tableExists(QString::fromLatin1(table)))
            return false;
    }
    return true;
}

QString MultilingualKnowledgeReader::fallbackLocale(const QString &locale) const
{
    if (!isOpen() || locale.trimmed().isEmpty())
        return QString();

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT fallback_locale FROM mems_doc_locale WHERE locale=:locale LIMIT 1"));
    query.bindValue(QStringLiteral(":locale"), locale.trimmed());
    if (!query.exec() || !query.next() || query.value(0).isNull())
        return QString();
    return query.value(0).toString().trimmed();
}

QString MultilingualKnowledgeReader::sourceLocale(const QString &entityKey) const
{
    if (!isOpen())
        return QString();

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT source_locale FROM mems_doc_entity WHERE entity_key=:entity LIMIT 1"));
    query.bindValue(QStringLiteral(":entity"), entityKey);
    if (!query.exec() || !query.next())
        return QString();
    return query.value(0).toString().trimmed();
}

MultilingualText MultilingualKnowledgeReader::text(const QString &entityKey,
                                                    const QString &fieldKey,
                                                    const QString &requestedLocale) const
{
    MultilingualText result;
    result.requestedLocale = requestedLocale.trimmed();
    if (!supportsFoundation() || entityKey.trimmed().isEmpty()
        || fieldKey.trimmed().isEmpty() || result.requestedLocale.isEmpty())
        return result;

    QSet<QString> visited;
    QString locale = result.requestedLocale;
    for (int depth = 0; depth < 32 && !locale.isEmpty(); ++depth) {
        const QString key = locale.toLower();
        if (visited.contains(key))
            break;
        visited.insert(key);

        QSqlQuery query(m_database);
        query.prepare(QStringLiteral(
            "SELECT text_value,translation_status FROM mems_doc_text "
            "WHERE entity_key=:entity AND field_key=:field AND locale=:locale LIMIT 1"));
        query.bindValue(QStringLiteral(":entity"), entityKey);
        query.bindValue(QStringLiteral(":field"), fieldKey);
        query.bindValue(QStringLiteral(":locale"), locale);
        if (query.exec() && query.next()) {
            result.resolvedLocale = locale;
            result.text = query.value(0).toString();
            result.translationStatus = query.value(1).toString();
            return result;
        }
        locale = fallbackLocale(locale);
    }
    return result;
}

MultilingualText MultilingualKnowledgeReader::sourceText(const QString &entityKey,
                                                          const QString &fieldKey) const
{
    const QString locale = sourceLocale(entityKey);
    if (locale.isEmpty())
        return MultilingualText();
    return text(entityKey, fieldKey, locale);
}

MultilingualVisual MultilingualKnowledgeReader::visual(const QString &entityKey,
                                                        const QString &requestedLocale) const
{
    MultilingualVisual result;
    result.entityKey = entityKey;
    if (!supportsFoundation())
        return result;

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT relative_path,sha256,visual_kind,source_width,source_height,original_intact "
        "FROM mems_doc_visual WHERE entity_key=:entity LIMIT 1"));
    query.bindValue(QStringLiteral(":entity"), entityKey);
    if (!query.exec() || !query.next())
        return result;

    result.found = true;
    result.relativePath = query.value(0).toString();
    result.sha256 = query.value(1).toString();
    result.kind = query.value(2).toString();
    const int width = query.value(3).isNull() ? -1 : query.value(3).toInt();
    const int height = query.value(4).isNull() ? -1 : query.value(4).toInt();
    if (width > 0 && height > 0)
        result.sourceSize = QSize(width, height);
    result.originalIntact = query.value(5).toInt() != 0;

    QSqlQuery regions(m_database);
    regions.prepare(QStringLiteral(
        "SELECT region_entity_key,region_kind,x_norm,y_norm,width_norm,height_norm,anchor_ref,z_order "
        "FROM mems_doc_visual_region WHERE visual_entity_key=:visual "
        "ORDER BY z_order,region_entity_key"));
    regions.bindValue(QStringLiteral(":visual"), entityKey);
    if (!regions.exec())
        return result;

    while (regions.next()) {
        MultilingualVisualRegion region;
        region.entityKey = regions.value(0).toString();
        region.kind = regions.value(1).toString();
        const bool hasX = !regions.value(2).isNull();
        const bool hasY = !regions.value(3).isNull();
        const bool hasW = !regions.value(4).isNull();
        const bool hasH = !regions.value(5).isNull();
        region.hasRect = hasX && hasY && hasW && hasH;
        if (region.hasRect) {
            region.normalizedRect = QRectF(regions.value(2).toDouble(),
                                           regions.value(3).toDouble(),
                                           regions.value(4).toDouble(),
                                           regions.value(5).toDouble());
        }
        region.anchorRef = regions.value(6).toString();
        region.zOrder = regions.value(7).toInt();
        region.label = text(region.entityKey, QStringLiteral("label"), requestedLocale);
        result.regions.append(region);
    }
    return result;
}

QList<MultilingualProcedureStep> MultilingualKnowledgeReader::procedureSteps(
    const QString &procedureEntityKey, const QString &requestedLocale) const
{
    QList<MultilingualProcedureStep> result;
    if (!supportsFoundation())
        return result;

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT entity_key,sort_order FROM mems_doc_entity "
        "WHERE parent_entity_key=:parent AND entity_kind='procedure_step' "
        "ORDER BY sort_order,entity_key"));
    query.bindValue(QStringLiteral(":parent"), procedureEntityKey);
    if (!query.exec())
        return result;

    while (query.next()) {
        MultilingualProcedureStep step;
        step.entityKey = query.value(0).toString();
        step.sortOrder = query.value(1).toInt();
        step.display = text(step.entityKey, QStringLiteral("display"), requestedLocale);
        step.sourceExact = sourceText(step.entityKey, QStringLiteral("source_exact"));
        result.append(step);
    }
    return result;
}

MultilingualValue MultilingualKnowledgeReader::value(const QString &entityKey,
                                                      const QString &requestedLocale) const
{
    MultilingualValue result;
    result.entityKey = entityKey;
    if (!supportsFoundation())
        return result;

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT value_kind,sequence_no,value_numeric,value_min,value_max,tolerance_minus,"
        "tolerance_plus,angle_deg,unit_code FROM mems_doc_value WHERE entity_key=:entity LIMIT 1"));
    query.bindValue(QStringLiteral(":entity"), entityKey);
    if (!query.exec() || !query.next())
        return result;

    result.found = true;
    result.kind = query.value(0).toString();
    result.sequenceNo = query.value(1).toInt();
    result.numeric = nullableValue(query, 2);
    result.minimum = nullableValue(query, 3);
    result.maximum = nullableValue(query, 4);
    result.toleranceMinus = nullableValue(query, 5);
    result.tolerancePlus = nullableValue(query, 6);
    result.angleDegrees = nullableValue(query, 7);
    result.unitCode = query.value(8).toString();
    result.label = text(entityKey, QStringLiteral("label"), requestedLocale);
    result.sourceExact = sourceText(entityKey, QStringLiteral("source_exact"));
    return result;
}

MultilingualTable MultilingualKnowledgeReader::table(const QString &entityKey,
                                                      const QString &requestedLocale) const
{
    MultilingualTable result;
    result.entityKey = entityKey;
    if (!supportsFoundation())
        return result;

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT row_count,column_count,header_rows,header_columns "
        "FROM mems_doc_table WHERE entity_key=:entity LIMIT 1"));
    query.bindValue(QStringLiteral(":entity"), entityKey);
    if (!query.exec() || !query.next())
        return result;

    result.found = true;
    result.rowCount = query.value(0).toInt();
    result.columnCount = query.value(1).toInt();
    result.headerRows = query.value(2).toInt();
    result.headerColumns = query.value(3).toInt();

    QSqlQuery cells(m_database);
    cells.prepare(QStringLiteral(
        "SELECT cell_entity_key,row_no,column_no,row_span,column_span,value_numeric,value_min,value_max,unit_code "
        "FROM mems_doc_table_cell WHERE table_entity_key=:table ORDER BY row_no,column_no"));
    cells.bindValue(QStringLiteral(":table"), entityKey);
    if (!cells.exec())
        return result;

    while (cells.next()) {
        MultilingualTableCell cell;
        cell.entityKey = cells.value(0).toString();
        cell.row = cells.value(1).toInt();
        cell.column = cells.value(2).toInt();
        cell.rowSpan = cells.value(3).toInt();
        cell.columnSpan = cells.value(4).toInt();
        cell.numeric = nullableValue(cells, 5);
        cell.minimum = nullableValue(cells, 6);
        cell.maximum = nullableValue(cells, 7);
        cell.unitCode = cells.value(8).toString();
        cell.display = text(cell.entityKey, QStringLiteral("display"), requestedLocale);
        result.cells.append(cell);
    }
    return result;
}
