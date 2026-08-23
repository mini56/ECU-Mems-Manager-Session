#ifndef LOCALAICLIENT_H
#define LOCALAICLIENT_H

#include <QObject>
#include <QPair>
#include <QPointer>
#include <QString>
#include <QVector>

class QNetworkAccessManager;
class QNetworkReply;
class QProcess;
class QTimer;

class LocalAiClient final : public QObject
{
    Q_OBJECT

public:
    enum State {
        NotStarted,
        MissingRuntime,
        MissingModel,
        Starting,
        Ready,
        Busy,
        Error
    };

    explicit LocalAiClient(QObject *parent = nullptr);
    ~LocalAiClient() override;

    void initialize();
    void shutdown();
    void ask(const QString &question, const QString &groundingContext);
    void clearConversation();

    State state() const { return m_state; }
    bool isReady() const { return m_state == Ready; }
    bool isBusy() const { return m_state == Busy; }
    QString statusText() const;
    QString lastError() const { return m_lastError; }
    QString runtimePath() const { return m_runtimePath; }
    QString modelPath() const { return m_modelPath; }

signals:
    void stateChanged();
    void responseReady(const QString &text);
    void responseError(const QString &message);

private:
    void setState(State state, const QString &error = QString());
    void discoverAssets();
    void startServer();
    void scheduleHealthCheck();
    void checkHealth();
    void handleHealthReply(QNetworkReply *reply);
    QString systemPrompt() const;
    QString cleanModelReply(QString text) const;

private:
    QNetworkAccessManager *m_network = nullptr;
    QProcess *m_server = nullptr;
    QTimer *m_healthTimer = nullptr;
    State m_state = NotStarted;
    QString m_runtimePath;
    QString m_modelPath;
    QString m_lastError;
    int m_healthAttempts = 0;
    bool m_startedServer = false;
    QVector<QPair<QString, QString>> m_conversation;
};

#endif // LOCALAICLIENT_H
