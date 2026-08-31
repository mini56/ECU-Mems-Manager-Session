#ifndef MULTILINGUALKNOWLEDGEREADER_H
#define MULTILINGUALKNOWLEDGEREADER_H

#include <QList>
#include <QRectF>
#include <QSize>
#include <QSqlDatabase>
#include <QString>
#include <QVariant>

struct MultilingualText
{
    QString requestedLocale;
    QString resolvedLocale;
    QString text;
    QString translationStatus;

    bool isValid() const { return !resolvedLocale.isEmpty(); }
    bool usedFallback() const
    {
        return isValid() && requestedLocale.compare(resolvedLocale, Qt::CaseInsensitive) != 0;
    }
};

struct MultilingualVisualRegion
{
    QString entityKey;
    QString kind;
    QRectF normalizedRect;
    bool hasRect = false;
    QString anchorRef;
    int zOrder = 0;
    MultilingualText label;
};

struct MultilingualVisual
{
    bool found = false;
    QString entityKey;
    QString relativePath;
    QString sha256;
    QString kind;
    QSize sourceSize;
    bool originalIntact = false;
    QList<MultilingualVisualRegion> regions;
};

struct MultilingualProcedureStep
{
    QString entityKey;
    int sortOrder = 0;
    MultilingualText display;
    MultilingualText sourceExact;
};

struct MultilingualValue
{
    bool found = false;
    QString entityKey;
    QString kind;
    int sequenceNo = 0;
    QVariant numeric;
    QVariant minimum;
    QVariant maximum;
    QVariant toleranceMinus;
    QVariant tolerancePlus;
    QVariant angleDegrees;
    QString unitCode;
    MultilingualText label;
    MultilingualText sourceExact;
};

struct MultilingualTableCell
{
    QString entityKey;
    int row = 0;
    int column = 0;
    int rowSpan = 1;
    int columnSpan = 1;
    QVariant numeric;
    QVariant minimum;
    QVariant maximum;
    QString unitCode;
    MultilingualText display;
};

struct MultilingualTable
{
    bool found = false;
    QString entityKey;
    int rowCount = 0;
    int columnCount = 0;
    int headerRows = 0;
    int headerColumns = 0;
    QList<MultilingualTableCell> cells;
};

class MultilingualKnowledgeReader
{
public:
    MultilingualKnowledgeReader();
    ~MultilingualKnowledgeReader();

    bool openReadOnly(const QString &databasePath);
    void close();
    bool isOpen() const;
    QString lastError() const;
    bool supportsFoundation() const;

    MultilingualText text(const QString &entityKey,
                          const QString &fieldKey,
                          const QString &requestedLocale) const;
    MultilingualText sourceText(const QString &entityKey,
                                const QString &fieldKey = QStringLiteral("source_exact")) const;
    MultilingualVisual visual(const QString &entityKey,
                              const QString &requestedLocale) const;
    QList<MultilingualProcedureStep> procedureSteps(const QString &procedureEntityKey,
                                                     const QString &requestedLocale) const;
    MultilingualValue value(const QString &entityKey,
                            const QString &requestedLocale) const;
    MultilingualTable table(const QString &entityKey,
                            const QString &requestedLocale) const;

private:
    bool tableExists(const QString &tableName) const;
    QString fallbackLocale(const QString &locale) const;
    QString sourceLocale(const QString &entityKey) const;

private:
    QSqlDatabase m_database;
    QString m_connectionName;
    QString m_lastError;
};

#endif // MULTILINGUALKNOWLEDGEREADER_H
