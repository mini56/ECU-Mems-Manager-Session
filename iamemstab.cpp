#include "iamemstab.h"

#include "mainwindow.h"
#include "memsinterface.h"
#include "expert/IaMemsService.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QShowEvent>
#include <QTextBrowser>
#include <QTimer>
#include <QVBoxLayout>

IaMemsTab::IaMemsTab(MainWindow *mainWindow, QWidget *parent)
    : QWidget(parent),
      m_mainWindow(mainWindow),
      m_mems(mainWindow ? mainWindow->memsInterface() : nullptr),
      m_service(IaMemsService::instance())
{
    setObjectName(QStringLiteral("ia_mems_tab"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 14, 18, 14);
    root->setSpacing(10);

    QHBoxLayout *header = new QHBoxLayout;
    QLabel *title = new QLabel(QStringLiteral("IA MEMS"), this);
    QFont titleFont = title->font();
    titleFont.setPointSize(qMax(titleFont.pointSize() + 4, 14));
    titleFont.setBold(true);
    title->setFont(titleFont);
    header->addWidget(title);
    header->addStretch(1);

    m_status = new QLabel(this);
    m_status->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    header->addWidget(m_status);
    root->addLayout(header);

    QLabel *subtitle = new QLabel(
        QStringLiteral("Dialogue local avec le moteur expert et la base de connaissances MEMS. "
                       "Les réponses distinguent mesures, hypothèses et niveau de preuve."),
        this);
    subtitle->setWordWrap(true);
    root->addWidget(subtitle);

    m_transcript = new QTextBrowser(this);
    m_transcript->setObjectName(QStringLiteral("iaMemsTranscript"));
    m_transcript->setOpenExternalLinks(false);
    m_transcript->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_transcript->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_transcript->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    if (QScrollBar *bar = m_transcript->verticalScrollBar())
        bar->setMinimumWidth(14);
    root->addWidget(m_transcript, 1);

    QHBoxLayout *input = new QHBoxLayout;
    m_question = new QLineEdit(this);
    m_question->setObjectName(QStringLiteral("iaMemsQuestion"));
    m_question->setPlaceholderText(QStringLiteral(
        "Posez une question : Qu'est-ce que tu vois d'anormal ? Mon avance est-elle normale ? Que sait-on sur AANMP002 ?"));
    m_question->setClearButtonEnabled(true);
    input->addWidget(m_question, 1);

    m_sendButton = new QPushButton(QStringLiteral("Envoyer"), this);
    m_sendButton->setObjectName(QStringLiteral("iaMemsSend"));
    input->addWidget(m_sendButton);
    root->addLayout(input);

    connect(m_sendButton, &QPushButton::clicked,
            this, &IaMemsTab::sendQuestion);
    connect(m_question, &QLineEdit::returnPressed,
            this, &IaMemsTab::sendQuestion);

    if (m_service) {
        connect(m_service, &IaMemsService::responseReady,
                this, &IaMemsTab::onServiceResponse);
        connect(m_service, &IaMemsService::systemMessage,
                this, &IaMemsTab::onServiceSystemMessage);
        connect(m_service, &IaMemsService::statusChanged,
                this, &IaMemsTab::updateStatus);
    }

    if (m_mems) {
        m_connected = m_mems->isConnected();
        if (m_service)
            m_service->setConnected(m_connected);

        connect(m_mems, &MEMSInterface::dataReady,
                this, &IaMemsTab::captureEcuSample, Qt::QueuedConnection);
        connect(m_mems, &MEMSInterface::injectionLiveSample,
                this, &IaMemsTab::onInjectionSample, Qt::QueuedConnection);
        connect(m_mems, &MEMSInterface::connected,
                this, &IaMemsTab::onEcuConnected, Qt::QueuedConnection);
        connect(m_mems, &MEMSInterface::disconnected,
                this, &IaMemsTab::onEcuDisconnected, Qt::QueuedConnection);
        connect(m_mems, &MEMSInterface::serialInterfaceDetected,
                this,
                [this](const QString &, const QString &, const QString &protocol) {
                    if (!m_service)
                        return;
                    ExpertContext context;
                    if (protocol.contains(QStringLiteral("1.9")))
                        context.family = QStringLiteral("1.9");
                    else if (protocol.contains(QStringLiteral("1.6")))
                        context.family = QStringLiteral("1.6");
                    else if (protocol.contains(QStringLiteral("1.3")))
                        context.family = QStringLiteral("1.3");
                    else if (protocol.contains(QStringLiteral("1.2")))
                        context.family = QStringLiteral("1.2");
                    if (!context.family.isEmpty())
                        m_service->setContext(context);
                },
                Qt::QueuedConnection);
    }

    appendSystemMessage(QStringLiteral(
        "Bonjour, je suis IA MEMS, l'assistant intégré à ECU MEMS Manager. "
        "Je peux expliquer le logiciel, les systèmes MEMS et les mesures déjà acquises par le programme. "
        "Je n'envoie aucune commande ECU et je n'invente pas une mesure absente."));

    updateStatus();
}

void IaMemsTab::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    // L'onglet est uniquement une vue. Le service IA est unique et appartient
    // à l'application ; il est activé après l'affichage, sans reparentage et
    // sans filtre global sur les événements QWidget.
    if (m_service) {
        IaMemsService *service = m_service;
        QTimer::singleShot(0, service, [service]() {
            service->activate();
        });
    }
}

void IaMemsTab::appendMessage(const QString &speaker, const QString &text)
{
    if (!m_transcript)
        return;

    const QString safeSpeaker = speaker.toHtmlEscaped();
    QString safeText = text.toHtmlEscaped();
    safeText.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
    m_transcript->append(QStringLiteral("<p><b>%1</b><br>%2</p>").arg(safeSpeaker, safeText));

    if (QScrollBar *bar = m_transcript->verticalScrollBar())
        bar->setValue(bar->maximum());
}

void IaMemsTab::appendSystemMessage(const QString &text)
{
    appendMessage(QStringLiteral("IA MEMS"), text);
}

void IaMemsTab::sendQuestion()
{
    if (!m_question || !m_service)
        return;

    const QString question = m_question->text().trimmed();
    if (question.isEmpty())
        return;

    m_question->clear();
    appendMessage(QStringLiteral("Vous"), question);

    if (m_sendButton)
        m_sendButton->setEnabled(false);
    m_question->setEnabled(false);

    m_service->ask(question);
}

void IaMemsTab::onServiceResponse(const QString &text)
{
    appendMessage(QStringLiteral("IA MEMS"), text);

    if (m_sendButton)
        m_sendButton->setEnabled(true);
    if (m_question) {
        m_question->setEnabled(true);
        m_question->setFocus();
    }
    updateStatus();
}

void IaMemsTab::onServiceSystemMessage(const QString &text)
{
    if (!text.trimmed().isEmpty())
        appendSystemMessage(text);
    updateStatus();
}

void IaMemsTab::updateStatus()
{
    if (!m_status)
        return;

    QStringList parts;
    parts << (m_connected ? QStringLiteral("ECU connecté") : QStringLiteral("ECU non connecté"));
    if (m_service)
        parts << m_service->statusText();

    m_status->setText(parts.join(QStringLiteral("  •  ")));
    m_status->setToolTip(m_service ? m_service->lastError() : QString());
}

void IaMemsTab::onEcuConnected()
{
    m_connected = true;
    if (m_service)
        m_service->setConnected(true);
    updateStatus();
}

void IaMemsTab::onEcuDisconnected()
{
    m_connected = false;
    if (m_service)
        m_service->setConnected(false);
    updateStatus();
}

void IaMemsTab::onInjectionSample(double finalMs,
                                  double baseMs,
                                  double transientMs,
                                  quint16,
                                  quint16,
                                  quint8 transientCounter)
{
    m_haveInjection = true;
    m_injectionFinalMs = finalMs;
    m_injectionBaseMs = baseMs;
    m_injectionTransientMs = transientMs;
    m_injectionTransientCounter = transientCounter;
}

void IaMemsTab::captureEcuSample()
{
    if (!m_mems || !m_service)
        return;

    mems_data *data = m_mems->getData();
    if (!data)
        return;

    ExpertObservation observation;
    observation.timestampMs = QDateTime::currentMSecsSinceEpoch();

    observation.values.insert(QStringLiteral("rpm"), data->engine_rpm);
    observation.values.insert(QStringLiteral("coolant_c"), static_cast<int>(data->coolant_temp) - 55.0);
    observation.values.insert(QStringLiteral("intake_air_c"), static_cast<int>(data->intake_air_temp) - 55.0);
    observation.values.insert(QStringLiteral("map_kpa"), data->map_kpa);
    observation.values.insert(QStringLiteral("battery_v"), data->battery_voltage / 10.0);
    observation.values.insert(QStringLiteral("lambda_mv"), data->lambda_voltage * 5.0);
    observation.values.insert(QStringLiteral("short_term_trim_pct"), static_cast<int>(data->short_term_fuel_trim) - 100.0);
    observation.values.insert(QStringLiteral("long_term_trim_raw"), data->long_term_fuel_trim);
    observation.values.insert(QStringLiteral("long_term_trim_pct"), static_cast<int>(data->long_term_fuel_trim) - 128.0);
    observation.values.insert(QStringLiteral("ignition_advance_deg"), data->ignition_advance * 0.5 - 24.0);
    observation.values.insert(QStringLiteral("coil_time_ms"), data->coil_time * 0.002);
    observation.values.insert(QStringLiteral("iac_position"), data->iac_position);
    observation.values.insert(QStringLiteral("idle_error_raw"), data->idle_error);

    const int rawHotIdleError = (static_cast<int>(data->idle_error2) << 8)
                              | static_cast<int>(data->uk10);
    const int hotIdleCorrection = static_cast<int>(data->idle_hot) - 35;
    observation.values.insert(QStringLiteral("idle_error_hot_corrected"),
                              rawHotIdleError - 32768 - hotIdleCorrection);

    observation.values.insert(QStringLiteral("uk3_raw"), data->uk3);
    observation.values.insert(QStringLiteral("closed_loop"), data->closed_loop != 0 ? 1.0 : 0.0);
    observation.values.insert(QStringLiteral("idle_switch_closed"), data->idle_switch == 0 ? 1.0 : 0.0);
    observation.values.insert(QStringLiteral("throttle_pot_raw"), data->throttle_pot);
    observation.values.insert(QStringLiteral("lambda_fault_active"),
                              ((data->dtc2 & 0x04) || (data->dtc2 & 0x08)) ? 1.0 : 0.0);
    observation.values.insert(QStringLiteral("tps_fault_active"),
                              ((data->dtc1 & 0x80) || (data->dtc2 & 0x01)) ? 1.0 : 0.0);
    observation.values.insert(QStringLiteral("fault_mask"),
                              static_cast<double>(static_cast<quint32>(data->dtc0)
                              | (static_cast<quint32>(data->dtc1) << 8)
                              | (static_cast<quint32>(data->dtc2) << 16)));

    if (m_haveInjection) {
        observation.values.insert(QStringLiteral("injection_final_ms"), m_injectionFinalMs);
        observation.values.insert(QStringLiteral("injection_base_ms"), m_injectionBaseMs);
        observation.values.insert(QStringLiteral("injection_transient_ms"), m_injectionTransientMs);
        observation.values.insert(QStringLiteral("injection_transient_state"), m_injectionTransientCounter);
    }

    m_service->addObservation(observation);
}
