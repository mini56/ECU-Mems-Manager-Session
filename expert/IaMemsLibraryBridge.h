#pragma once

#include <QString>
#include <QStringList>

struct IaMemsLibraryGrounding
{
    bool libraryReady = false;
    int resultCount = 0;
    QString text;
    QString error;
};

class IaMemsLibraryBridge final
{
public:
    static IaMemsLibraryGrounding retrieve(const QString &question,
                                            const QStringList &keywords = QStringList());
};
