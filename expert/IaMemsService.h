#ifndef IAMEMSSERVICE_H
#define IAMEMSSERVICE_H

#include <QObject>
#include <QList>
#include <QString>

#include "ExpertEngine.h"
#include "ExpertKnowledgeReader.h"
#include "ExpertTypes.h"
#include "../mems16na_session_evidence.h"
#include "../rosco.h"

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
    void setNaDiagnostic(const Mems16Na::SessionEvidence &na,const mems_data &data,const QString &report);
    void clearNaDiagnostic();
    void ask(const QString &question);
    void askWithLibrary(const QString &question);
    void clearConversation();

    QString statusText() const;
    QString lastError() const;
    bool knowledgeReady() const { return m_knowledgeReady; }
    bool localAiReady() const;

signals:
    void statusChanged();
    void responseReady(const QString &text);
    void systemMessage(const QString &text);

private:
    explicit IaMemsService(QObject *parent = nullptr);

    void openPackagedKnowledge();
    void updateContextFromQuestion(const QString &question);
    QString groundingFor(const QString &question);
    QString naAnswer(const QString &question) const;
    QString softwareAnswer(const QString &question) const;
    QString currentValuesAnswer() const;
    QString historyAnswer() const;
    QString analysisAnswer();
    QString knowledgeAnswer(const QString &question) const;
    QString helpAnswer() const;
    QString verificationLabel(const QString &level) const;

private:
    LocalAiClient *m_localAi = nullptr;
    ExpertEngine m_engine;
    ExpertKnowledgeReader m_reader;
    ExpertContext m_context;
    QList<ExpertObservation> m_history;
    ExpertAnalysisResult m_lastAnalysis;
    Mems16Na::SessionEvidence m_naEvidence;
    QString m_naReport;
    QList<ExpertObservation> m_naHistory;

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
