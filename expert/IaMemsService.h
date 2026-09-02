#ifndef IAMEMSSERVICE_H
#define IAMEMSSERVICE_H

#include <QObject>
#include <QList>
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
    void clearConversation();

    QString statusText() const;
    QString lastError() const;
    bool knowledgeReady() const { return m_knowledgeReady; }
    bool localAiReady() const;

signals:
    void statusChanged();
    void responseReady(const QString &text);
    void responseVisualReferenceReady(const QString &reference);
    void systemMessage(const QString &text);

private:
    explicit IaMemsService(QObject *parent = nullptr);

    void openPackagedKnowledge();
    void updateContextFromQuestion(const QString &question);
    QString groundingFor(const QString &question);
    QString softwareAnswer(const QString &question) const;
    QString currentValuesAnswer() const;
    QString historyAnswer() const;
    QString analysisAnswer();
    QString knowledgeAnswer(const QString &question);
    QString helpAnswer() const;
    QString verificationLabel(const QString &level) const;

private:
    LocalAiClient *m_localAi = nullptr;
    ExpertEngine m_engine;
    ExpertKnowledgeReader m_reader;
    ExpertContext m_context;
    QList<ExpertObservation> m_history;
    ExpertAnalysisResult m_lastAnalysis;

    QString m_databasePath;
    QString m_knowledgeError;
    QString m_pendingGrounding;
    QString m_pendingVisualReference;
    bool m_activated = false;
    bool m_knowledgeAttempted = false;
    bool m_knowledgeReady = false;
    bool m_connected = false;

    static const int kMaximumSamples = 600;
};

#endif // IAMEMSSERVICE_H
