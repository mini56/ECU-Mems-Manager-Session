#ifndef IAMEMSTAB_H
#define IAMEMSTAB_H

#include <QFont>
#include <QHash>
#include <QList>
#include <QPointer>
#include <QScrollArea>
#include <QString>
#include <QWidget>

#include "expert/ExpertEngine.h"
#include "expert/ExpertKnowledgeReader.h"
#include "expert/ExpertTypes.h"

class QLabel;
class QLineEdit;
class QPushButton;
class QTextBrowser;
class QShowEvent;
class MainWindow;
class MEMSInterface;
class LocalAiClient;

// Conversational MEMS view. It only observes data already acquired by
// MEMSInterface; it never requests another ECU polling mode on its own.
// Package validation must exercise this exact integrated source state,
// including the main reference loader and lazy local-AI startup behavior.
// Final lab package validation must use this exact source revision.
class IaMemsTab final : public QWidget
{
    Q_OBJECT

public:
    explicit IaMemsTab(MainWindow *mainWindow, QWidget *parent = nullptr);
    ~IaMemsTab() override;

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void sendQuestion();
    void captureEcuSample();
    void onInjectionSample(double finalMs,
                           double baseMs,
                           double transientMs,
                           quint16 baseRaw,
                           quint16 transientRaw,
                           quint8 transientCounter);
    void onEcuConnected();
    void onEcuDisconnected();
    void onLocalAiResponse(const QString &text);
    void onLocalAiError(const QString &message);
    void onLocalAiStateChanged();

private:
    void startKnowledgeLoad();
    void finishKnowledgeLoad(bool ok, const QString &databasePath, const QString &errorText);
    void appendMessage(const QString &speaker, const QString &text);
    void appendSystemMessage(const QString &text);
    void updateStatus();
    void addObservation(const ExpertObservation &observation);
    void updateContextFromQuestion(const QString &question);

    QString answerQuestion(const QString &question);
    QString softwareAnswer(const QString &question) const;
    QString currentValuesAnswer() const;
    QString historyAnswer() const;
    QString analysisAnswer(bool explainPrevious = false);
    QString knowledgeAnswer(const QString &question) const;
    QString helpAnswer() const;

    QStringList knowledgeTerms(const QString &question) const;
    QString verificationLabel(const QString &level) const;

private:
    MainWindow *m_mainWindow = nullptr;
    MEMSInterface *m_mems = nullptr;

    QTextBrowser *m_transcript = nullptr;
    QLineEdit *m_question = nullptr;
    QPushButton *m_sendButton = nullptr;
    QLabel *m_status = nullptr;
    LocalAiClient *m_localAi = nullptr;

    ExpertEngine m_engine;
    ExpertKnowledgeReader m_reader;
    ExpertContext m_context;
    QList<ExpertObservation> m_history;
    ExpertAnalysisResult m_lastAnalysis;

    QString m_databasePath;
    QString m_knowledgeError;
    QString m_pendingGrounding;
    bool m_knowledgeLoading = false;
    bool m_knowledgeReady = false;
    bool m_connected = false;

    bool m_haveInjection = false;
    double m_injectionFinalMs = 0.0;
    double m_injectionBaseMs = 0.0;
    double m_injectionTransientMs = 0.0;
    quint8 m_injectionTransientCounter = 0;

    static const int kMaximumSamples = 600;
};

#endif // IAMEMSTAB_H
