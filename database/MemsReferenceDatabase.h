#ifndef MEMSREFERENCEDATABASE_H
#define MEMSREFERENCEDATABASE_H

#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QSqlDatabase>

class MemsReferenceDatabase
{
public:
    MemsReferenceDatabase();
    ~MemsReferenceDatabase();

    bool open();
    void close();
    bool isOpen() const;
    QString databasePath() const;

    QVariantList searchEcus(const QString &text,
                            const QString &generationToken = QString(),
                            int limit = 250) const;
    QVariantMap ecu(const QString &partNumber) const;
    QVariantList fitments(const QString &partNumber) const;
    QVariantList files(const QString &partNumber) const;
    QVariantList protocolProfiles(const QString &systemFamily) const;
    QVariantList protocolCommands(const QString &systemFamily, int limit = 80) const;
    QVariantList protocolCapabilities(const QString &systemFamily, int limit = 80) const;
    QVariantList protocolDataFields(const QString &systemFamily, int limit = 80) const;
    QVariantList protocolSettings(const QString &systemFamily, int limit = 80) const;
    QVariantList dtcs(const QString &systemFamily) const;
    QVariantList actuatorSpecs() const;

    int targetEcuCount() const;
    int targetFitmentCount() const;
    int targetFileCount() const;
    int commandCount() const;
    int dataFieldCount() const;
    int capabilityCount() const;

    QString generationXmlPath(const QString &systemFamily) const;
    QString generationPdfPath(const QString &systemFamily) const;

private:
    QVariantList queryList(const QString &sql,
                           const QVariantMap &bindings = QVariantMap()) const;
    QVariantMap queryOne(const QString &sql,
                         const QVariantMap &bindings = QVariantMap()) const;
    int scalarInt(const QString &sql) const;
    QString referenceRoot() const;
    QString generationToken(const QString &systemFamily) const;

private:
    QSqlDatabase m_database;
    QString m_connectionName;
    QString m_databasePath;
};

#endif
