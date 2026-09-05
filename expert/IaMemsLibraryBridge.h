#pragma once

#include <QString>
#include <QStringList>

struct IaMemsLibraryGrounding
{
    bool libraryReady = false;
    bool provenanceFiltered = false;
    int resultCount = 0;
    int selectedPage = -1;
    QString selectedDocument;
    QString selectedRevision;
    QString selectedSourceLanguage;
    QString text;
    QString error;
};

class IaMemsLibraryBridge final
{
public:
    static IaMemsLibraryGrounding retrieve(const QString &question,
                                            const QStringList &keywords = QStringList());
};
