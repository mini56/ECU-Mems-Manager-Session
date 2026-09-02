#ifndef IAMEMSDIAGRAMCATALOG_H
#define IAMEMSDIAGRAMCATALOG_H

#include <QString>

struct IaMemsDiagramSuggestion
{
    QString key;
    QString relativePath;
    QString absolutePath;

    bool isValid() const
    {
        return !key.isEmpty() && !relativePath.isEmpty() && !absolutePath.isEmpty();
    }
};

class IaMemsDiagramCatalog
{
public:
    static IaMemsDiagramSuggestion suggestionForQuestion(
        const QString &question,
        const QString &referenceRoot = QString());

    static IaMemsDiagramSuggestion suggestionForResponse(
        const QString &response,
        const QString &referenceRoot = QString());
};

#endif // IAMEMSDIAGRAMCATALOG_H
