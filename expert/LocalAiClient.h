#ifndef LOCALAICLIENT_H
#define LOCALAICLIENT_H

#include <QObject>
#include <QPair>
#include <QString>
#include <QVector>

class QThread;
class LocalAiWorker;

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
    QString systemPrompt() const;
    QString cleanModelReply(QString text) const;

private:
    QThread *m_workerThread = nullptr;
    LocalAiWorker *m_worker = nullptr;
    State m_state = NotStarted;
    QString m_runtimePath;
    QString m_modelPath;
    QString m_lastError;
    quint64 m_epoch = 0;
    QVector<QPair<QString, QString>> m_conversation;
};

#endif // LOCALAICLIENT_H
