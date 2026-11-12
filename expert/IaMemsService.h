#ifndef IAMEMSSERVICE_H
#define IAMEMSSERVICE_H

#include <QObject>
#include <QList>
#include <QQueue>
#include <QStringList>
#include <QString>

#include "ExpertEngine.h"
#include "ExpertKnowledgeReader.h"
#include "ExpertTypes.h"

class LocalAiClient;

class IaMemsService final : public QObject
{
    Q_OBJECT

public:
    static IaMemsService *instance();

    void activate();
    void setConnected(bool connected);
    void setContext(const ExpertContext &context);
    void addObservation(const ExpertObservation &observation);
    void ask(const QString &question);
    void askWithLibrary(const QString &question);
    // Shared read-only Rover translation service for the database and IA MEMS.
    void requestDocumentTranslation(const QString &requestId, const QString &source,
                                    const QString &language);
    static QString documentTranslationNotice(const QString &language, bool pending);
    void clearConversation();

    QString statusText() const;
    QString lastError() const;
    bool knowledgeReady() const { return m_knowledgeReady; }
    bool localAiReady() const;

signals:
    void statusChanged();
    void responseReady(const QString &text);
    void systemMessage(const QString &text);
    void documentTranslationReady(const QString &requestId,
                                  const QString &translated, bool complete);

private:
    explicit IaMemsService(QObject *parent = nullptr);

    void openPackagedKnowledge();
    void advanceDocumentTranslation();
    void acceptTranslatedLine(const QString &token, const QString &result,
                              const QString &error);
    void updateContextFromQuestion(const QString &question);
    QString groundingFor(const QString &question);
    QString softwareAnswer(const QString &question) const;
    QString currentValuesAnswer() const;
    QString historyAnswer() const;
    QString analysisAnswer();
    QString knowledgeAnswer(const QString &question) const;
    QString helpAnswer() const;
    QString verificationLabel(const QString &level) const;

private:
    struct DocumentTranslationJob {
        QString requestId;
        QString source;
        QString language;
        QStringList lines;
        QStringList translated;
        int line = 0;
        bool complete = true;
    };
    QQueue<DocumentTranslationJob> m_translationQueue;
    DocumentTranslationJob m_translationJob;
    QString m_translationActiveLineToken;
    bool m_translationActive = false;
    LocalAiClient *m_localAi = nullptr;
    ExpertEngine m_engine;
    ExpertKnowledgeReader m_reader;
    ExpertContext m_context;
    QList<ExpertObservation> m_history;
    ExpertAnalysisResult m_lastAnalysis;

    QString m_databasePath;
    QString m_knowledgeError;
    QString m_pendingGrounding;
    bool m_activated = false;
    bool m_knowledgeAttempted = false;
    bool m_knowledgeReady = false;
    bool m_connected = false;

    static const int kMaximumSamples = 600;
};

#endif // IAMEMSSERVICE_H
