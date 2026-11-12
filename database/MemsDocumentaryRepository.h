#ifndef MEMSDOCUMENTARYREPOSITORY_H
#define MEMSDOCUMENTARYREPOSITORY_H

#include <QSqlDatabase>
#include <QString>
#include <QVariantList>

// The sole read-only interface to the versioned Rover documentary SQLite.
// This interface never issues ECU commands or treats OCR discovery as approved
// technical instructions. Both the database browser and IA MEMS use search().
class MemsDocumentaryRepository
{
public:
    MemsDocumentaryRepository();
    ~MemsDocumentaryRepository();
    MemsDocumentaryRepository(const MemsDocumentaryRepository &) = delete;
    MemsDocumentaryRepository &operator=(const MemsDocumentaryRepository &) = delete;

    static QString databasePath();
    static QString originalPdfPath();
    static bool isPackInstalled();

    bool open();
    bool isOpen() const;
    QString lastError() const;
    QVariantList search(const QString &question, int maximum = 12,
                        bool allowOcrDiscovery = true,
                        const QString &language = QString()) const;
    static QString answerFromHits(const QVariantList &hits);
    static QString numberedMenu(const QString &question, const QVariantList &choices,
                                bool french);

private:
    QSqlDatabase m_database;
    QString m_connectionName;
    QString m_error;
};

#endif
