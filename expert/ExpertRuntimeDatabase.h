#ifndef EXPERTRUNTIMEDATABASE_H
#define EXPERTRUNTIMEDATABASE_H

#include <QString>

class ExpertRuntimeDatabase
{
public:
    ExpertRuntimeDatabase() = default;

    bool buildOrOpen();
    QString databasePath() const;
    QString lastError() const;
    int manifestRevision() const;

private:
    QString referenceRoot() const;
    QString cacheRoot() const;

private:
    QString m_databasePath;
    QString m_lastError;
    int m_manifestRevision = 0;
};

#endif // EXPERTRUNTIMEDATABASE_H
