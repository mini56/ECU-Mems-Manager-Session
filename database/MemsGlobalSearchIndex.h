#ifndef MEMSGLOBALSEARCHINDEX_H
#define MEMSGLOBALSEARCHINDEX_H

#include <QDateTime>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QString>
#include <QVariantList>

class MemsGlobalSearchIndex
{
public:
    static bool ensureBuilt(QString *errorMessage=nullptr);
    static QString indexPath();
    static int documentCount();
    static QVariantList search(const QString &text,
                               const QString &category=QString(),
                               int limit=100);
};

#endif
