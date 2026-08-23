#include "iamemstab.h"

#include "mainwindow.h"
#include "memsinterface.h"
#include "expert/ExpertRuntimeDatabase.h"
#include "expert/LocalAiClient.h"

#include <QApplication>
#include <QDateTime>
#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMetaObject>
#include <QPainter>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QShowEvent>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTabWidget>
#include <QTextBrowser>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

namespace {

static const int kTextRole = Qt::UserRole + 42;
static const int kTabPtrRole = Qt::UserRole + 60;

QString normalized(const QString &text)
{
    QString result = text.toLower().simplified();
    result.replace(QChar(0x00E0), QLatin1Char('a'));
    result.replace(QChar(0x00E2), QLatin1Char('a'));
    result.replace(QChar(0x00E4), QLatin1Char('a'));
    result.replace(QChar(0x00E7), QLatin1Char('c'));
    result.replace(QChar(0x00E8), QLatin1Char('e'));
    result.replace(QChar(0x00E9), QLatin1Char('e'));
    result.replace(QChar(0x00EA), QLatin1Char('e'));
    result.replace(QChar(0x00EB), QLatin1Char('e'));
    result.replace(QChar(0x00EE), QLatin1Char('i'));
    result.replace(QChar(0x00EF), QLatin1Char('i'));
    result.replace(QChar(0x00F4), QLatin1Char('o'));
    result.replace(QChar(0x00F6), QLatin1Char('o'));
    result.replace(QChar(0x00F9), QLatin1Char('u'));
    result.replace(QChar(0x00FB), QLatin1Char('u'));
    result.replace(QChar(0x00FC), QLatin1Char('u'));
    return result;
}

bool containsAny(const QString &text, const QStringList &needles)
{
    for (const QString &needle : needles) {
        if (text.contains(needle))
            return true;
    }
    return false;
}

QString number(double value, int decimals = 1)
{
    if (!std::isfinite(value))
        return QStringLiteral("—");
    return QString::number(value, 'f', decimals);
}

QIcon iaMemsIcon()
{
    QPixmap pixmap(22, 22);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(QColor(QStringLiteral("#ff8a1c")), 1.7,
                        Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(QRectF(3.0, 4.0, 16.0, 11.0), 3.0, 3.0);
    painter.drawLine(QPointF(8.0, 15.0), QPointF(6.0, 19.0));
    painter.drawLine(QPointF(8.0, 15.0), QPointF(12.0, 15.0));
    painter.drawEllipse(QPointF(8.0, 9.5), 0.7, 0.7);
    painter.drawEllipse(QPointF(11.0, 9.5), 0.7, 0.7);
    painter.drawEllipse(QPointF(14.0, 9.5), 0.7, 0.7);
    return QIcon(pixmap);
}

QWidget *realPage(QWidget *tab)
{
    if (!tab)
        return nullptr;
    if (QScrollArea *scroll = qobject_cast<QScrollArea*>(tab))
        return scroll->widget();
    return tab;
}

IaMemsTab *findIaTab(MainWindow *window)
{
    if (!window)
        return nullptr;
    QTabWidget *tabs = window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
    if (!tabs)
        return nullptr;
    for (int i = 0; i < tabs->count(); ++i) {
        QWidget *page = realPage(tabs->widget(i));
        if (page && page->objectName() == QStringLiteral("ia_mems_tab"))
            return qobject_cast<IaMemsTab*>(page);
    }
    return nullptr;
}

void ensureIaNavigation(MainWindow *window, IaMemsTab *tab)
{
    if (!window || !tab)
        return;

    QTabWidget *tabs = window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
    QListWidget *navigation = window->findChild<QListWidget*>(QStringLiteral("uiRebuildNav"));
    if (!tabs || !navigation)
        return;

    for (int row = 0; row < navigation->count(); ++row) {
        const qulonglong raw = navigation->item(row)->data(kTabPtrRole).toULongLong();
        QWidget *target = reinterpret_cast<QWidget*>(static_cast<quintptr>(raw));
        if (target == tab) {
            navigation->item(row)->setText(QStringLiteral("IA MEMS"));
            navigation->item(row)->setToolTip(QStringLiteral("IA MEMS"));
            if (tabs->currentWidget() == tab)
                navigation->setCurrentRow(row);
            return;
        }
    }

    QListWidgetItem *item = new QListWidgetItem(iaMemsIcon(), QStringLiteral("IA MEMS"));
    item->setData(kTextRole, QStringLiteral("IA MEMS"));
    item->setData(kTabPtrRole,
                  static_cast<qulonglong>(reinterpret_cast<quintptr>(static_cast<QWidget*>(tab))));
    item->setToolTip(QStringLiteral("IA MEMS"));

    // The final navigation ranks Diagnostic automatique at row 5 and Analyse at
    // row 6. IA MEMS belongs between both without altering the existing rank map.
    const int insertRow = qMin(6, navigation->count());
    navigation->insertItem(insertRow, item);
    if (tabs->currentWidget() == tab)
        navigation->setCurrentRow(insertRow);
}

IaMemsTab *ensureIaTab(MainWindow *window)
{
    if (!window || !window->memsInterface())
        return nullptr;

    if (IaMemsTab *existing = findIaTab(window))
        return existing;

    QTabWidget *tabs = window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
    if (!tabs)
        return nullptr;

    IaMemsTab *tab = new IaMemsTab(window, tabs);
    tab->setObjectName(QStringLiteral("ia_mems_tab"));
    tabs->addTab(tab, iaMemsIcon(), QStringLiteral("IA MEMS"));
    return tab;
}

class IaMemsInstaller final : public QObject
{
public:
    explicit IaMemsInstaller(QObject *parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        MainWindow *window = qobject_cast<MainWindow*>(watched);
        if (!window)
            return QObject::eventFilter(watched, event);

        if (event->type() == QEvent::Show) {
            QPointer<MainWindow> guard(window);
            QTimer::singleShot(0, window, [guard]() {
                if (!guard)
                    return;
                IaMemsTab *tab = ensureIaTab(guard);
                if (tab)
                    ensureIaNavigation(guard, tab);
            });
            // navigationorderpatch rebuilds the sidebar several times during
            // startup. Reinsert only this extra pointer-mapped item afterwards.
            const int delays[] = {500, 1800, 3900, 5200, 6500};
            for (int delay : delays) {
                QTimer::singleShot(delay, window, [guard]() {
                    if (!guard)
                        return;
                    IaMemsTab *tab = ensureIaTab(guard);
                    if (tab)
                        ensureIaNavigation(guard, tab);
                });
            }
        } else if (event->type() == QEvent::Resize || event->type() == QEvent::LanguageChange) {
            QPointer<MainWindow> guard(window);
            QTimer::singleShot(360, window, [guard]() {
                if (!guard)
                    return;
                if (IaMemsTab *tab = findIaTab(guard))
                    ensureIaNavigation(guard, tab);
            });
        }
        return QObject::eventFilter(watched, event);
    }
};

void installIaMemsTab()
{
    if (!qApp)
        return;
    IaMemsInstaller *installer = new IaMemsInstaller(qApp);
    qApp->installEventFilter(installer);
}

Q_COREAPP_STARTUP_FUNCTION(installIaMemsTab)

} // namespace

IaMemsTab::IaMemsTab(MainWindow *mainWindow, QWidget *parent)
    : QWidget(parent),
      m_mainWindow(mainWindow),
      m_mems(mainWindow ? mainWindow->memsInterface() : nullptr)
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

    connect(m_sendButton, &QPushButton::clicked, this, &IaMemsTab::sendQuestion);
    connect(m_question, &QLineEdit::returnPressed, this, &IaMemsTab::sendQuestion);

    if (m_mems) {
        m_connected = m_mems->isConnected();
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
                    if (protocol.contains(QStringLiteral("1.9"))) {
                        m_context.family = QStringLiteral("1.9");
                        m_engine.setContext(m_context);
                    }
                    updateStatus();
                },
                Qt::QueuedConnection);
    }

    m_engine.setContext(m_context);

    m_localAi = new LocalAiClient(this);
    connect(m_localAi, &LocalAiClient::responseReady,
            this, &IaMemsTab::onLocalAiResponse);
    connect(m_localAi, &LocalAiClient::responseError,
            this, &IaMemsTab::onLocalAiError);
    connect(m_localAi, &LocalAiClient::stateChanged,
            this, &IaMemsTab::onLocalAiStateChanged);

    appendSystemMessage(QStringLiteral(
        "Bonjour, je suis IA MEMS, l'assistant intégré à ECU MEMS Manager. "
        "Posez-moi votre question : je peux vous aider à utiliser le logiciel, "
        "expliquer les fonctions MEMS, analyser les mesures ECU et commenter les hypothèses du moteur expert. "
        "Si les informations disponibles ne permettent pas de conclure, je vous le dirai clairement."));

    updateStatus();
}

void IaMemsTab::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    // Keep the main ECU program light: the knowledge cache and the local
    // language model are started only when IA MEMS is actually opened.
    QTimer::singleShot(0, this, &IaMemsTab::startKnowledgeLoad);
    if (m_localAi)
        QTimer::singleShot(0, m_localAi, &LocalAiClient::initialize);
}

IaMemsTab::~IaMemsTab()
{
    m_reader.close();
}

void IaMemsTab::startKnowledgeLoad()
{
    if (m_knowledgeLoading || m_knowledgeReady)
        return;

    m_knowledgeLoading = true;
    m_knowledgeError.clear();
    updateStatus();

    QPointer<IaMemsTab> guard(this);
    QThread *thread = QThread::create([guard]() {
        ExpertRuntimeDatabase reference;
        const bool ok = reference.buildOrOpen();
        const QString path = ok ? reference.databasePath() : QString();
        const QString error = ok ? QString() : reference.lastError();

        if (!guard)
            return;
        QMetaObject::invokeMethod(guard.data(), [guard, ok, path, error]() {
            if (guard)
                guard->finishKnowledgeLoad(ok, path, error);
        }, Qt::QueuedConnection);
    });
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

void IaMemsTab::finishKnowledgeLoad(bool ok,
                                    const QString &databasePath,
                                    const QString &errorText)
{
    m_knowledgeLoading = false;
    m_knowledgeReady = false;
    m_databasePath.clear();
    m_knowledgeError = errorText;

    if (ok && !databasePath.isEmpty() && m_reader.openReadOnly(databasePath)) {
        m_databasePath = databasePath;
        m_knowledgeReady = true;
        m_knowledgeError.clear();
        m_engine.setKnowledgeReader(&m_reader);
        appendSystemMessage(QStringLiteral("Base de connaissances MEMS prête en lecture seule."));
    } else if (ok) {
        m_knowledgeError = m_reader.lastError();
        appendSystemMessage(QStringLiteral(
            "La base MEMS a été créée mais la couche expert n'a pas pu l'ouvrir. "
            "La discussion sur les mesures reste disponible."));
    } else {
        appendSystemMessage(QStringLiteral(
            "La base de connaissances n'est pas disponible pour le moment. "
            "La discussion sur les mesures ECU reste disponible."));
    }
    updateStatus();
}

void IaMemsTab::appendMessage(const QString &speaker, const QString &text)
{
    if (!m_transcript)
        return;
    const QString safeSpeaker = speaker.toHtmlEscaped();
    QString safeText = text.toHtmlEscaped();
    safeText.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
    m_transcript->append(QStringLiteral("<p><b>%1</b><br>%2</p>").arg(safeSpeaker, safeText));
}

void IaMemsTab::appendSystemMessage(const QString &text)
{
    appendMessage(QStringLiteral("IA MEMS"), text);
}

void IaMemsTab::updateStatus()
{
    if (!m_status)
        return;

    QStringList parts;
    parts << (m_connected ? QStringLiteral("ECU connecté") : QStringLiteral("ECU non connecté"));
    if (m_knowledgeReady)
        parts << QStringLiteral("base prête");
    else if (m_knowledgeLoading)
        parts << QStringLiteral("base en chargement");
    else
        parts << QStringLiteral("base indisponible");

    if (!m_context.family.isEmpty())
        parts << QStringLiteral("MEMS %1").arg(m_context.family);
    if (!m_context.firmware.isEmpty())
        parts << m_context.firmware;
    if (m_localAi)
        parts << m_localAi->statusText();

    m_status->setText(parts.join(QStringLiteral("  •  ")));
}

void IaMemsTab::onEcuConnected()
{
    m_connected = true;
    updateStatus();
}

void IaMemsTab::onEcuDisconnected()
{
    m_connected = false;
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
    if (!m_mems)
        return;

    mems_data *data = m_mems->getData();
    if (!data)
        return;

    ExpertObservation observation;
    observation.timestampMs = QDateTime::currentMSecsSinceEpoch();

    // Use the same established conversions already used by MainWindow.
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
    const int rawHotIdleError = (static_cast<int>(data->idle_error2) << 8) | static_cast<int>(data->uk10);
    const int hotIdleCorrection = static_cast<int>(data->idle_hot) - 35;
    observation.values.insert(QStringLiteral("idle_error_hot_corrected"),
                              (rawHotIdleError - 32768) + hotIdleCorrection);
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

    addObservation(observation);
}

void IaMemsTab::addObservation(const ExpertObservation &observation)
{
    m_history.append(observation);
    if (m_history.size() > kMaximumSamples)
        m_history.removeFirst();

    if (m_engine.sampleCount() >= kMaximumSamples) {
        m_engine.clearSamples();
        for (const ExpertObservation &sample : m_history)
            m_engine.addSample(sample);
    } else {
        m_engine.addSample(observation);
    }
}

void IaMemsTab::sendQuestion()
{
    if (!m_question)
        return;
    const QString question = m_question->text().trimmed();
    if (question.isEmpty())
        return;

    m_question->clear();
    appendMessage(QStringLiteral("Vous"), question);
    updateContextFromQuestion(question);

    // The deterministic layer supplies the technical/software grounding.
    // The local model is responsible for natural language and conversation,
    // never for inventing ECU facts.
    m_pendingGrounding = answerQuestion(question);
    if (m_localAi && m_localAi->isReady()) {
        if (m_sendButton)
            m_sendButton->setEnabled(false);
        if (m_question)
            m_question->setEnabled(false);
        m_localAi->ask(question, m_pendingGrounding);
        return;
    }

    appendMessage(QStringLiteral("IA MEMS"), m_pendingGrounding);
}

void IaMemsTab::onLocalAiResponse(const QString &text)
{
    appendMessage(QStringLiteral("IA MEMS"), text);
    m_pendingGrounding.clear();
    if (m_sendButton)
        m_sendButton->setEnabled(true);
    if (m_question) {
        m_question->setEnabled(true);
        m_question->setFocus();
    }
    updateStatus();
}

void IaMemsTab::onLocalAiError(const QString &message)
{
    QString fallback = m_pendingGrounding.trimmed();
    if (fallback.isEmpty())
        fallback = QStringLiteral("Je ne peux pas répondre avec le moteur conversationnel local pour le moment.");
    appendMessage(QStringLiteral("IA MEMS"), fallback);
    appendSystemMessage(QStringLiteral("Moteur conversationnel local indisponible : %1").arg(message));
    m_pendingGrounding.clear();
    if (m_sendButton)
        m_sendButton->setEnabled(true);
    if (m_question) {
        m_question->setEnabled(true);
        m_question->setFocus();
    }
    updateStatus();
}

void IaMemsTab::onLocalAiStateChanged()
{
    updateStatus();
    if (!m_localAi)
        return;
    if (m_status)
        m_status->setToolTip(m_localAi->lastError());
    if ((m_localAi->state() == LocalAiClient::Error
         || m_localAi->state() == LocalAiClient::MissingRuntime
         || m_localAi->state() == LocalAiClient::MissingModel)
        && !m_localAi->lastError().trimmed().isEmpty()) {
        appendSystemMessage(QStringLiteral("Détail du moteur IA local : %1")
                                .arg(m_localAi->lastError().trimmed()));
    }
}

void IaMemsTab::updateContextFromQuestion(const QString &question)
{
    const QString text = normalized(question);
    if (text.contains(QStringLiteral("mems 1.2")) || text.contains(QStringLiteral("mems1.2")))
        m_context.family = QStringLiteral("1.2");
    else if (text.contains(QStringLiteral("mems 1.3")) || text.contains(QStringLiteral("mems1.3")))
        m_context.family = QStringLiteral("1.3");
    else if (text.contains(QStringLiteral("mems 1.6")) || text.contains(QStringLiteral("mems1.6")))
        m_context.family = QStringLiteral("1.6");
    else if (text.contains(QStringLiteral("mems 1.9")) || text.contains(QStringLiteral("mems1.9")))
        m_context.family = QStringLiteral("1.9");

    static const QRegularExpression firmwareRx(QStringLiteral("\\b([A-Z]{4,}[A-Z0-9]*[0-9]{3,})\\b"));
    QRegularExpressionMatchIterator iterator = firmwareRx.globalMatch(question.toUpper());
    while (iterator.hasNext()) {
        const QString candidate = iterator.next().captured(1);
        // MNE/MKC references are ECU part numbers rather than firmware IDs.
        if (!candidate.startsWith(QStringLiteral("MNE")) && !candidate.startsWith(QStringLiteral("MKC"))) {
            m_context.firmware = candidate;
            break;
        }
    }

    m_engine.setContext(m_context);
    updateStatus();
}

QString IaMemsTab::answerQuestion(const QString &question)
{
    const QString text = normalized(question);

    if (containsAny(text, {QStringLiteral("bonjour"), QStringLiteral("bonsoir"),
                           QStringLiteral("salut"), QStringLiteral("hello")}))
        return QStringLiteral(
            "Bonjour. Je suis IA MEMS. Je peux répondre à vos questions sur ECU MEMS Manager, "
            "sur les systèmes MEMS et sur les mesures de l'ECU connecté. Que souhaitez-vous savoir ?");

    if (containsAny(text, {QStringLiteral("faute"), QStringLiteral("fautes"),
                           QStringLiteral("orthographe"), QStringLiteral("mal ecrit"),
                           QStringLiteral("mal écrit"), QStringLiteral("erreur de frappe")}))
        return QStringLiteral(
            "Non, ce n'est pas grave. Vous pouvez écrire naturellement, même avec des fautes d'orthographe "
            "ou de frappe. J'essaie de comprendre le sens de votre question. Si une formulation est vraiment "
            "ambiguë, je vous demanderai simplement de préciser au lieu d'inventer une réponse.");

    const QString software = softwareAnswer(question);
    if (!software.isEmpty())
        return software;

    if (containsAny(text, {QStringLiteral("aide"), QStringLiteral("que peux tu"),
                           QStringLiteral("que peux-tu"), QStringLiteral("comment te parler")}))
        return helpAnswer();

    if (containsAny(text, {QStringLiteral("pourquoi"), QStringLiteral("explique")})) {
        if (!m_lastAnalysis.hypotheses.isEmpty())
            return analysisAnswer(true);
    }

    const bool asksGeneralReference = containsAny(text, {
        QStringLiteral("quelle avance"), QStringLiteral("quel avance"),
        QStringLiteral("avance correcte"), QStringLiteral("avance normal"),
        QStringLiteral("avance normale"), QStringLiteral("combien"),
        QStringLiteral("pour un moteur"), QStringLiteral("mini spi"),
        QStringLiteral("valeur de reference"), QStringLiteral("valeur de référence")
    });
    if (asksGeneralReference) {
        const QString reference = knowledgeAnswer(question);
        if (!reference.isEmpty())
            return reference;
        if (containsAny(text, {QStringLiteral("avance"), QStringLiteral("allumage")}))
            return QStringLiteral(
                "Il n'existe pas une seule valeur d'avance correcte pour tous les Mini SPi : elle dépend notamment "
                "du régime, de la charge, de la température et du firmware MEMS. Je ne donnerai pas un chiffre "
                "universel sans référence suffisamment fiable pour l'ECU concerné. Si vous me donnez le firmware "
                "ou l'identification ECU, je peux rechercher la donnée correspondante dans la base MEMS.");
    }

    if (containsAny(text, {QStringLiteral("anormal"), QStringLiteral("normal"),
                           QStringLiteral("normale"), QStringLiteral("coherent"),
                           QStringLiteral("coherente"), QStringLiteral("diagnostic"),
                           QStringLiteral("panne"), QStringLiteral("probleme"),
                           QStringLiteral("cause"), QStringLiteral("hypothese")}))
        return analysisAnswer(false);

    if (containsAny(text, {QStringLiteral("historique"), QStringLiteral("evolution"),
                           QStringLiteral("tendance"), QStringLiteral("oscill"),
                           QStringLiteral("derniere minute"), QStringLiteral("dernieres minutes"),
                           QStringLiteral("depuis")}))
        return historyAnswer();

    if (containsAny(text, {QStringLiteral("source"), QStringLiteral("certain"),
                           QStringLiteral("certaine"), QStringLiteral("fiable"),
                           QStringLiteral("preuve"), QStringLiteral("sait-on"), QStringLiteral("sais tu"),
                           QStringLiteral("sais-tu"), QStringLiteral("firmware"),
                           QStringLiteral("octet"), QStringLiteral("adresse"),
                           QStringLiteral("correspond"), QStringLiteral("mems 1."),
                           QStringLiteral("aanmp"), QStringLiteral("mne"),
                           QStringLiteral("mkc")})) {
        const QString answer = knowledgeAnswer(question);
        if (!answer.isEmpty())
            return answer;
    }

    if (containsAny(text, {QStringLiteral("valeur"), QStringLiteral("regime"),
                           QStringLiteral("rpm"), QStringLiteral("temperature"),
                           QStringLiteral("map"), QStringLiteral("lambda"),
                           QStringLiteral("avance"), QStringLiteral("bobine"),
                           QStringLiteral("dwell"), QStringLiteral("injection"),
                           QStringLiteral("batterie"), QStringLiteral("ralenti"),
                           QStringLiteral("papillon")}))
        return currentValuesAnswer();

    const QString knowledge = knowledgeAnswer(question);
    if (!knowledge.isEmpty())
        return knowledge;

    return QStringLiteral(
        "Je n'ai pas assez d'éléments pour relier cette question à une mesure ou à un fait MEMS précis. "
        "Vous pouvez me demander ce que je vois d'anormal, l'évolution des mesures, une valeur actuelle, "
        "ou une information technique sur un ECU, un firmware ou une famille MEMS.");
}

QString IaMemsTab::softwareAnswer(const QString &question) const
{
    const QString text = normalized(question);
    const bool softwareIntent = containsAny(text, {
        QStringLiteral("onglet"), QStringLiteral("mems manager"),
        QStringLiteral("logiciel"), QStringLiteral("programme"),
        QStringLiteral("a quoi sert"), QStringLiteral("comment fonctionne"),
        QStringLiteral("c'est quoi"), QStringLiteral("c est quoi")
    });

    auto wants = [&text, softwareIntent](const QStringList &terms) {
        if (!softwareIntent)
            return false;
        return containsAny(text, terms);
    };

    if (softwareIntent && containsAny(text, {QStringLiteral("que peut faire"),
                                              QStringLiteral("que peux faire"),
                                              QStringLiteral("fonctionnalite"),
                                              QStringLiteral("fonctionnalites"),
                                              QStringLiteral("possibilites du logiciel")}))
        return QStringLiteral(
            "ECU MEMS Manager sert à dialoguer avec les ECU Rover/Mini MEMS pris en charge, afficher les mesures "
            "en direct, lire les défauts, effectuer les tests d'actionneurs et réglages supportés, analyser des "
            "journaux CSV/TXT, consulter les données et la base technique, travailler avec les fonctions ROSCO, "
            "suivre l'injection lorsque ce mode est disponible et utiliser IA MEMS pour expliquer les mesures, "
            "l'historique, les hypothèses diagnostiques et le fonctionnement du logiciel.");

    if (softwareIntent && containsAny(text, {QStringLiteral("qui a concu"),
                                              QStringLiteral("qui a conçu"),
                                              QStringLiteral("qui a cree"),
                                              QStringLiteral("qui a créé"),
                                              QStringLiteral("auteur"),
                                              QStringLiteral("developpe par"),
                                              QStringLiteral("développé par")}))
        return QStringLiteral(
            "ECU MEMS Manager est le projet développé dans le dépôt mini56. Les travaux d'Andrew Revill, "
            "RoverMEMS et les autres sources intégrées servent de références techniques : ils ne sont pas les "
            "concepteurs de MEMS Manager.");

    if (wants({QStringLiteral("analyse")}))
        return QStringLiteral(
            "L'onglet Analyse sert à étudier des journaux de diagnostic enregistrés. "
            "Il peut lire les fichiers CSV/TXT pris en charge, vous laisser choisir les canaux de mesure "
            "et afficher ou superposer leurs courbes pour observer l'évolution des valeurs dans le temps.");

    if (wants({QStringLiteral("apercu"), QStringLiteral("vue d'ensemble")}))
        return QStringLiteral(
            "L'onglet Aperçu présente les principales mesures ECU en direct sous forme de cadrans et d'indicateurs : "
            "régime, MAP, températures, batterie, papillon, lambda, ralenti et allumage. Il sert au contrôle rapide du moteur.");

    if (wants({QStringLiteral("injection")}))
        return QStringLiteral(
            "L'onglet Injection regroupe les mesures d'injection acquises par le mode de lecture prévu pour cela, "
            "notamment le temps d'injection final, le temps de base, la correction transitoire et son état. "
            "L'IA n'active pas un autre mode de polling de son propre chef.");

    if (wants({QStringLiteral("reglage"), QStringLiteral("reglages")}))
        return QStringLiteral(
            "L'onglet Réglage contient les ajustements de service disponibles, notamment ceux liés au ralenti, "
            "au carburant ou à l'allumage selon l'ECU. Les réglages agissent sur l'ECU : ils doivent être utilisés avec prudence.");

    if (wants({QStringLiteral("erreur"), QStringLiteral("defaut")}))
        return QStringLiteral(
            "L'onglet Erreurs affiche les informations de défaut et les états associés renvoyés par l'ECU. "
            "Il faut lire et comprendre les défauts avant de les effacer.");

    if (wants({QStringLiteral("actionneur"), QStringLiteral("actionneurs")}))
        return QStringLiteral(
            "L'onglet Actionneurs permet d'exécuter les tests d'actionneurs supportés par l'ECU, par exemple certains relais, "
            "la pompe, le ventilateur ou la commande de ralenti selon le système. Ces tests peuvent réellement actionner des organes du véhicule.");

    if (wants({QStringLiteral("diagnostic automatique"), QStringLiteral("diagnostique automatique"),
               QStringLiteral("diagnostic auto"), QStringLiteral("diagnostique auto")}))
        return QStringLiteral(
            "L'onglet Diagnostic automatique contrôle les valeurs ECU courantes, signale les anomalies ou points à surveiller, "
            "peut capturer une référence et produire un rapport. IA MEMS va plus loin en croisant ces contrôles avec l'historique, "
            "la base de connaissances et le dialogue avec l'utilisateur.");

    if (wants({QStringLiteral("toutes les mesures")}))
        return QStringLiteral(
            "L'onglet Toutes les mesures rassemble les paramètres décodés dans une vue détaillée. "
            "Il permet de comparer le paramètre, son aide, la valeur reçue de l'ECU et sa valeur interprétée lorsque le décodage est connu.");

    if (wants({QStringLiteral("ecu/rosco"), QStringLiteral("ecu rosco"), QStringLiteral("rosco")}))
        return QStringLiteral(
            "L'onglet ECU/ROSCO permet d'observer et d'utiliser les commandes de session et de diagnostic ROSCO prises en charge. "
            "Les échanges TX/RX peuvent y être visualisés en hexadécimal. Les fonctions dangereuses ou non validées restent désactivées.");

    if (wants({QStringLiteral("toutes les donnees"), QStringLiteral("donnees brutes")}))
        return QStringLiteral(
            "L'onglet Toutes les données est la vue détaillée des champs MEMS et de leurs valeurs brutes ou décodées. "
            "Il est surtout utile pour comparer les octets du protocole avec les mesures interprétées.");

    if (wants({QStringLiteral("base donnees"), QStringLiteral("base de donnees"), QStringLiteral("base données")}))
        return QStringLiteral(
            "L'onglet Base de données donne accès à la base documentaire et technique intégrée à MEMS Manager. "
            "IA MEMS utilise également une partie structurée de cette connaissance avec son niveau de provenance et de confiance.");

    if (wants({QStringLiteral("capture"), QStringLiteral("captures")}))
        return QStringLiteral(
            "La fonction Capture enregistre une image de la fenêtre de MEMS Manager. Le visualiseur intégré permet ensuite "
            "de prévisualiser, ouvrir ou supprimer les captures enregistrées.");

    if (wants({QStringLiteral("option"), QStringLiteral("parametre"), QStringLiteral("parametres")}))
        return QStringLiteral(
            "Les Options regroupent les paramètres généraux du logiciel, notamment l'interface série, l'unité de température, "
            "le thème, la langue et certains réglages d'intégration au bureau.");

    if (wants({QStringLiteral("ia mems"), QStringLiteral("ia")}))
        return QStringLiteral(
            "L'onglet IA MEMS réunit le dialogue en langage naturel, les mesures ECU déjà acquises, leur historique, "
            "le moteur expert et la base de connaissances. Le modèle conversationnel formule les réponses, mais les faits techniques "
            "et les diagnostics restent ancrés dans les données et règles de MEMS Manager.");

    return QString();
}

QString IaMemsTab::currentValuesAnswer() const
{
    if (m_history.isEmpty())
        return QStringLiteral("Je n'ai encore reçu aucune mesure ECU. Connectez l'ECU et laissez MEMS Manager acquérir quelques trames.");

    const QHash<QString, double> &v = m_history.constLast().values;
    QStringList lines;
    lines << QStringLiteral("Dernière mesure disponible :")
          << QStringLiteral("• Régime : %1 tr/min").arg(number(v.value(QStringLiteral("rpm")), 0))
          << QStringLiteral("• Température liquide : %1 °C").arg(number(v.value(QStringLiteral("coolant_c")), 0))
          << QStringLiteral("• MAP : %1 kPa").arg(number(v.value(QStringLiteral("map_kpa")), 0))
          << QStringLiteral("• Batterie : %1 V").arg(number(v.value(QStringLiteral("battery_v")), 1))
          << QStringLiteral("• Lambda : %1 mV").arg(number(v.value(QStringLiteral("lambda_mv")), 0))
          << QStringLiteral("• Correction court terme : %1 %").arg(number(v.value(QStringLiteral("short_term_trim_pct")), 0))
          << QStringLiteral("• Avance : %1°").arg(number(v.value(QStringLiteral("ignition_advance_deg")), 1))
          << QStringLiteral("• Dwell bobine : %1 ms").arg(number(v.value(QStringLiteral("coil_time_ms")), 3))
          << QStringLiteral("• Boucle fermée : %1").arg(v.value(QStringLiteral("closed_loop")) != 0.0 ? QStringLiteral("oui") : QStringLiteral("non"));

    if (v.contains(QStringLiteral("injection_final_ms"))) {
        lines << QStringLiteral("• Injection finale : %1 ms").arg(number(v.value(QStringLiteral("injection_final_ms")), 2))
              << QStringLiteral("• Injection de base : %1 ms").arg(number(v.value(QStringLiteral("injection_base_ms")), 2))
              << QStringLiteral("• Correction transitoire : %1 ms").arg(number(v.value(QStringLiteral("injection_transient_ms")), 2));
    } else {
        lines << QStringLiteral("• Injection Mode 4 : aucune valeur récente disponible dans le mode de lecture actuel.");
    }

    return lines.join(QLatin1Char('\n'));
}

QString IaMemsTab::historyAnswer() const
{
    if (m_history.size() < 2)
        return QStringLiteral("Je n'ai pas encore assez d'historique pour décrire une évolution.");

    const ExpertAnalysisResult analysis = m_engine.analyze();
    const qint64 elapsedMs = m_history.constLast().timestampMs - m_history.constFirst().timestampMs;
    QStringList lines;
    lines << QStringLiteral("Historique analysé : %1 échantillons sur environ %2 s.")
             .arg(m_history.size())
             .arg(number(elapsedMs / 1000.0, 0));

    const struct Metric { const char *key; const char *label; const char *unit; int decimals; } metrics[] = {
        {"rpm", "Régime", "tr/min", 0},
        {"map_kpa", "MAP", "kPa", 1},
        {"coolant_c", "Température liquide", "°C", 1},
        {"battery_v", "Batterie", "V", 2},
        {"lambda_mv", "Lambda", "mV", 0},
        {"short_term_trim_pct", "Correction court terme", "%", 1},
        {"ignition_advance_deg", "Avance", "°", 1},
        {"coil_time_ms", "Dwell bobine", "ms", 3}
    };

    for (const Metric &metric : metrics) {
        const QString key = QString::fromLatin1(metric.key);
        if (!analysis.statistics.contains(key))
            continue;
        const ExpertSeriesStats stats = analysis.statistics.value(key);
        lines << QStringLiteral("• %1 : moyenne %2 %3, min %4, max %5, tendance %6 %3/s")
                 .arg(QString::fromUtf8(metric.label),
                      number(stats.mean, metric.decimals),
                      QString::fromUtf8(metric.unit),
                      number(stats.minimum, metric.decimals),
                      number(stats.maximum, metric.decimals),
                      number(stats.slopePerSecond, metric.decimals));
    }
    return lines.join(QLatin1Char('\n'));
}

QString IaMemsTab::analysisAnswer(bool explainPrevious)
{
    if (m_history.isEmpty())
        return QStringLiteral("Je ne peux pas faire d'analyse sans mesure ECU.");

    if (!explainPrevious)
        m_lastAnalysis = m_engine.analyze();

    if (m_lastAnalysis.hypotheses.isEmpty()) {
        QString text = QStringLiteral(
            "Je n'ai actuellement aucune hypothèse diagnostique suffisamment étayée par les règles chargées pour ce contexte. "
            "Je ne vais pas inventer une cause à partir d'une seule valeur.");
        if (!m_knowledgeReady)
            text += QStringLiteral(" La base de connaissances expert n'est pas encore prête.");
        else
            text += QStringLiteral(" Les mesures restent consultables et je peux en décrire l'évolution.");
        return text;
    }

    QStringList lines;
    if (explainPrevious)
        lines << QStringLiteral("Voici pourquoi j'avais retenu ces hypothèses :");
    else
        lines << QStringLiteral("Hypothèses actuelles du moteur expert :");

    const int maximum = qMin(5, m_lastAnalysis.hypotheses.size());
    for (int i = 0; i < maximum; ++i) {
        const ExpertHypothesis &hypothesis = m_lastAnalysis.hypotheses.at(i);
        lines << QStringLiteral("%1. %2 — confiance %3 % — preuve %4")
                 .arg(i + 1)
                 .arg(hypothesis.title)
                 .arg(number(hypothesis.confidence * 100.0, 0))
                 .arg(verificationLabel(hypothesis.verificationLevel));
        for (const QString &evidence : hypothesis.evidence)
            lines << QStringLiteral("   • %1").arg(evidence);
        if (!hypothesis.strongConclusionAllowed)
            lines << QStringLiteral("   • Conclusion forte interdite avec ce niveau de preuve.");
        for (const QString &recommendation : hypothesis.recommendations)
            lines << QStringLiteral("   → %1").arg(recommendation);
    }
    return lines.join(QLatin1Char('\n'));
}

QStringList IaMemsTab::knowledgeTerms(const QString &question) const
{
    QString text = normalized(question);
    text.replace(QRegularExpression(QStringLiteral("[^a-z0-9_.-]+")), QStringLiteral(" "));
    const QSet<QString> stopWords = {
        QStringLiteral("que"), QStringLiteral("quoi"), QStringLiteral("sur"), QStringLiteral("les"),
        QStringLiteral("des"), QStringLiteral("une"), QStringLiteral("dans"), QStringLiteral("pour"),
        QStringLiteral("avec"), QStringLiteral("est"), QStringLiteral("sais"), QStringLiteral("sait"),
        QStringLiteral("peux"), QStringLiteral("dire"), QStringLiteral("cette"), QStringLiteral("ce"),
        QStringLiteral("cet"), QStringLiteral("mon"), QStringLiteral("ma"), QStringLiteral("mes"),
        QStringLiteral("mems")
    };

    QStringList result;
    const QStringList words = text.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (const QString &word : words) {
        if (word.size() < 3 || stopWords.contains(word))
            continue;
        if (!result.contains(word))
            result.append(word);
        if (result.size() >= 5)
            break;
    }
    return result;
}

QString IaMemsTab::knowledgeAnswer(const QString &question) const
{
    if (!m_knowledgeReady || m_databasePath.isEmpty()) {
        if (m_knowledgeLoading)
            return QStringLiteral("La base de connaissances MEMS est encore en chargement. Vous pouvez déjà m'interroger sur les mesures ECU reçues.");
        return QString();
    }

    const QStringList terms = knowledgeTerms(question);
    if (terms.isEmpty())
        return QString();

    const QString connection = QStringLiteral("IA_MEMS_QUERY_%1").arg(QUuid::createUuid().toString());
    QStringList answers;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
        db.setDatabaseName(m_databasePath);
        if (db.open()) {
            QSqlQuery pragma(db);
            pragma.exec(QStringLiteral("PRAGMA query_only = ON"));

            QSqlQuery exists(db);
            exists.prepare(QStringLiteral("SELECT 1 FROM sqlite_master WHERE type='table' AND name=:name"));
            exists.bindValue(QStringLiteral(":name"), QStringLiteral("mems_expert_fact_external"));
            if (exists.exec() && exists.next()) {
                QStringList predicates;
                for (int i = 0; i < terms.size(); ++i) {
                    const QString p = QStringLiteral(":p%1").arg(i);
                    predicates << QStringLiteral("(lower(fact_key) LIKE %1 OR lower(family) LIKE %1 OR lower(firmware_code) LIKE %1 OR lower(topic) LIKE %1 OR lower(statement) LIKE %1 OR lower(notes) LIKE %1)").arg(p);
                }
                QSqlQuery query(db);
                query.prepare(QStringLiteral(
                    "SELECT statement,verification_level,source_key,notes FROM mems_expert_fact_external WHERE ")
                    + predicates.join(QStringLiteral(" OR "))
                    + QStringLiteral(" ORDER BY firmware_code,fact_key LIMIT 6"));
                for (int i = 0; i < terms.size(); ++i)
                    query.bindValue(QStringLiteral(":p%1").arg(i), QStringLiteral("%") + terms.at(i) + QStringLiteral("%"));
                if (query.exec()) {
                    while (query.next()) {
                        QString line = QStringLiteral("• %1 [preuve : %2; source : %3]")
                            .arg(query.value(0).toString(),
                                 verificationLabel(query.value(1).toString()),
                                 query.value(2).toString());
                        const QString notes = query.value(3).toString().trimmed();
                        if (!notes.isEmpty())
                            line += QStringLiteral(" — %1").arg(notes);
                        answers << line;
                    }
                }
            }

            if (answers.isEmpty()) {
                exists.finish();
                exists.prepare(QStringLiteral("SELECT 1 FROM sqlite_master WHERE type='table' AND name=:name"));
                exists.bindValue(QStringLiteral(":name"), QStringLiteral("mems_scalar_definition"));
                if (exists.exec() && exists.next()) {
                    QStringList predicates;
                    for (int i = 0; i < terms.size(); ++i) {
                        const QString p = QStringLiteral(":s%1").arg(i);
                        predicates << QStringLiteral("(lower(identifier) LIKE %1 OR lower(name) LIKE %1 OR lower(rover_comments) LIKE %1)").arg(p);
                    }
                    QSqlQuery query(db);
                    query.prepare(QStringLiteral(
                        "SELECT identifier,name,rover_comments,source_key FROM mems_scalar_definition WHERE ")
                        + predicates.join(QStringLiteral(" OR "))
                        + QStringLiteral(" LIMIT 6"));
                    for (int i = 0; i < terms.size(); ++i)
                        query.bindValue(QStringLiteral(":s%1").arg(i), QStringLiteral("%") + terms.at(i) + QStringLiteral("%"));
                    if (query.exec()) {
                        while (query.next()) {
                            QString description = query.value(1).toString().trimmed();
                            const QString comments = query.value(2).toString().trimmed();
                            if (!comments.isEmpty())
                                description += QStringLiteral(" — ") + comments;
                            answers << QStringLiteral("• %1 : %2 [source externe : %3]")
                                       .arg(query.value(0).toString(), description, query.value(3).toString());
                        }
                    }
                }
            }
        }
        db.close();
    }
    QSqlDatabase::removeDatabase(connection);

    if (answers.isEmpty())
        return QString();

    answers.prepend(QStringLiteral("Voici ce que la base MEMS contient de pertinent :"));
    return answers.join(QLatin1Char('\n'));
}

QString IaMemsTab::verificationLabel(const QString &level) const
{
    if (level == QStringLiteral("verifie_constructeur")) return QStringLiteral("constructeur");
    if (level == QStringLiteral("decoded_by_project")) return QStringLiteral("décodé par le projet");
    if (level == QStringLiteral("recoupee")) return QStringLiteral("recoupée");
    if (level == QStringLiteral("source_externe")) return QStringLiteral("source externe");
    if (level == QStringLiteral("plausible")) return QStringLiteral("plausible");
    if (level == QStringLiteral("non_verifie")) return QStringLiteral("non vérifiée");
    if (level == QStringLiteral("conflit_a_verifier")) return QStringLiteral("conflit à vérifier");
    return level.isEmpty() ? QStringLiteral("inconnu") : level;
}

QString IaMemsTab::helpAnswer() const
{
    return QStringLiteral(
        "Vous pouvez me demander par exemple :\n"
        "• Qu'est-ce que tu vois d'anormal ?\n"
        "• Quelles sont les valeurs actuelles ?\n"
        "• Comment le régime et le MAP ont-ils évolué ?\n"
        "• Pourquoi as-tu retenu cette hypothèse ?\n"
        "• Que sait-on sur AANMP002 ?\n"
        "• C'est quoi l'onglet Analyse ?\n"
        "• À quoi sert l'onglet Injection ?\n"
        "• Cette information est-elle certaine ?\n"
        "Je peux également expliquer le fonctionnement de MEMS Manager. "
        "Je distingue toujours les mesures réelles, les hypothèses et les connaissances externes.");
}
