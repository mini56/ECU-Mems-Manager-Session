#ifndef IAMEMSTAB_H
#define IAMEMSTAB_H

#include <QString>
#include <QWidget>

#include "expert/ExpertTypes.h"

class QLabel;
class QLineEdit;
class QPushButton;
class QTextBrowser;
class QShowEvent;
class MainWindow;
class MEMSInterface;
class IaMemsService;

class IaMemsTab final : public QWidget
{
    Q_OBJECT

public:
    explicit IaMemsTab(MainWindow *mainWindow, QWidget *parent = nullptr);
    ~IaMemsTab() override = default;

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
    void onServiceResponse(const QString &text);
    void onServiceSystemMessage(const QString &text);
    void updateStatus();
    void openSuggestedDiagram();
    void openSuggestedDocument();

private:
    void appendMessage(const QString &speaker, const QString &text);
    void appendSystemMessage(const QString &text);
    void updateDiagramSuggestion(const QString &question);
    void updateDocumentSuggestion(const QString &question);
    QString resolveInductionFromKnownContext(const QString &question) const;
    QString clarificationPrompt(const QString &question) const;
    void answerLocally(const QString &text);

private:
    MainWindow *m_mainWindow = nullptr;
    MEMSInterface *m_mems = nullptr;
    IaMemsService *m_service = nullptr;

    QTextBrowser *m_transcript = nullptr;
    QLineEdit *m_question = nullptr;
    QPushButton *m_sendButton = nullptr;
    QPushButton *m_diagramButton = nullptr;
    QPushButton *m_documentButton = nullptr;
    QLabel *m_status = nullptr;
    QString m_diagramTitle;
    QString m_diagramQuestion;
    QString m_documentGeneration;
    QString m_pendingClarificationQuestion;
    QString m_detectedFamily;
    QString m_firmwareIdentifier;
    QString m_ecuIdHex;

    bool m_connected = false;
    bool m_haveInjection = false;
    double m_injectionFinalMs = 0.0;
    double m_injectionBaseMs = 0.0;
    double m_injectionTransientMs = 0.0;
    quint8 m_injectionTransientCounter = 0;
};

#endif // IAMEMSTAB_H
