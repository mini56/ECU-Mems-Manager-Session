#ifndef MEMS19TESTTAB_H
#define MEMS19TESTTAB_H

#include <QByteArray>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QElapsedTimer>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMetaObject>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTextStream>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include "i18n.h"
#include "memsinterface.h"

class Mems19TestTab final : public QWidget
{
public:
    explicit Mems19TestTab(MEMSInterface *mems, QWidget *parent = nullptr)
        : QWidget(parent), m_mems(mems)
    {
        setObjectName(QStringLiteral("mems19_test_tab"));
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        QVBoxLayout *root = new QVBoxLayout(this);
        root->setContentsMargins(12, 10, 12, 10);
        root->setSpacing(8);

        m_title = new QLabel(this);
        QFont titleFont = m_title->font();
        titleFont.setBold(true);
        titleFont.setPointSizeF(titleFont.pointSizeF() + 2.0);
        m_title->setFont(titleFont);
        root->addWidget(m_title);

        m_description = new QLabel(this);
        m_description->setWordWrap(true);
        root->addWidget(m_description);

        m_safety = new QLabel(this);
        m_safety->setWordWrap(true);
        QFont safetyFont = m_safety->font();
        safetyFont.setBold(true);
        m_safety->setFont(safetyFont);
        root->addWidget(m_safety);

        m_overall = new QLabel(this);
        QFont overallFont = m_overall->font();
        overallFont.setBold(true);
        overallFont.setPointSizeF(overallFont.pointSizeF() + 1.0);
        m_overall->setFont(overallFont);
        m_overall->setWordWrap(true);
        root->addWidget(m_overall);

        m_instruction = new QLabel(this);
        m_instruction->setWordWrap(true);
        m_instruction->setMinimumHeight(34);
        root->addWidget(m_instruction);

        m_statusBox = new QGroupBox(this);
        QGridLayout *status = new QGridLayout(m_statusBox);
        status->setColumnStretch(0, 0);
        status->setColumnStretch(1, 1);
        int row = 0;
        addStatusRow(status, row++, m_connectionKey, m_connection);
        addStatusRow(status, row++, m_d0Key, m_d0);
        addStatusRow(status, row++, m_d1Key, m_d1);
        addStatusRow(status, row++, m_d2Key, m_d2);
        addStatusRow(status, row++, m_f0Key, m_f0);
        addStatusRow(status, row++, m_readsKey, m_reads);
        addStatusRow(status, row++, m_engineKey, m_engine);
        addStatusRow(status, row++, m_injectionKey, m_injection);
        addStatusRow(status, row++, m_restoreKey, m_restore);
        addStatusRow(status, row++, m_reconnectKey, m_reconnect);
        addStatusRow(status, row++, m_logKey, m_logPathLabel);
        root->addWidget(m_statusBox);

        QHBoxLayout *buttons = new QHBoxLayout();
        m_startButton = new QPushButton(this);
        m_stopButton = new QPushButton(this);
        m_skipEngineButton = new QPushButton(this);
        m_openFolderButton = new QPushButton(this);
        m_stopButton->setEnabled(false);
        m_skipEngineButton->setEnabled(false);
        m_skipEngineButton->setVisible(false);
        buttons->addWidget(m_startButton);
        buttons->addWidget(m_stopButton);
        buttons->addWidget(m_skipEngineButton);
        buttons->addWidget(m_openFolderButton);
        buttons->addStretch(1);
        root->addLayout(buttons);

        m_eventsTitle = new QLabel(this);
        QFont eventsFont = m_eventsTitle->font();
        eventsFont.setBold(true);
        m_eventsTitle->setFont(eventsFont);
        root->addWidget(m_eventsTitle);

        m_events = new QPlainTextEdit(this);
        m_events->setReadOnly(true);
        m_events->setMaximumBlockCount(5000);
        m_events->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        root->addWidget(m_events, 1);

        m_uiRefresh.start();
        retranslateUi();
        resetStatus();
        setOverall(I18n::text(19004), QStringLiteral("#ff9828"));
        setInstruction(I18n::text(19005).arg(I18n::text(19030)));

        connect(m_startButton, &QPushButton::clicked, this, [this]() { startTest(); });
        connect(m_stopButton, &QPushButton::clicked, this, [this]() { stopTest(); });
        connect(m_skipEngineButton, &QPushButton::clicked, this, [this]() {
            if (!m_running || m_step != Step::WaitEngine) return;
            m_partial = true;
            setState(m_engine, I18n::text(19044), StateColor::Warning);
            appendEvent(QStringLiteral("ENGINE"), QStringLiteral("engine-step-skipped"), true);
            m_skipEngineButton->setEnabled(false);
            m_skipEngineButton->setVisible(false);
            QTimer::singleShot(150, this, [this]() { beginInjection(); });
        });
        connect(m_openFolderButton, &QPushButton::clicked, this, [this]() {
            if (m_eventPath.isEmpty()) ensureSessionFiles();
            if (!m_eventPath.isEmpty())
                QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_eventPath).absolutePath()));
        });

        QTimer::singleShot(0, this, [this]() { updateTabTitle(); });

        if (!m_mems)
        {
            setOverall(I18n::text(19042).arg(I18n::text(19055)), QStringLiteral("#ff5b55"));
            return;
        }

        connect(m_mems, &MEMSInterface::serialInterfaceDetected, this,
                [this](const QString &portName, const QString &adapterFamily, const QString &protocolName) {
            const bool mems19 = protocolName.contains(QStringLiteral("MEMS 1.9"), Qt::CaseInsensitive);
            const QString detail = I18n::text(19029).arg(portName, adapterFamily, protocolName);
            setState(m_connection, detail, mems19 ? StateColor::Good : StateColor::Warning);
            appendEvent(QStringLiteral("INTERFACE"), QStringLiteral("port=%1 adapter=%2 protocol=%3").arg(portName, adapterFamily, protocolName), true);
            if (m_step == Step::Connecting)
            {
                m_mems19Detected = mems19;
                m_initialConnectionDetail = detail;
            }
            else if (m_step == Step::Reconnecting)
            {
                m_reconnectMems19Detected = mems19;
            }
        });

        connect(m_mems, &MEMSInterface::connected, this, [this]() {
            m_connected = true;
            appendEvent(QStringLiteral("CONNECTION"), QStringLiteral("connected"), true);
            if (!m_running) return;
            if (m_step == Step::Connecting)
                QTimer::singleShot(120, this, [this]() { validateInitialConnection(); });
            else if (m_step == Step::Reconnecting)
                QTimer::singleShot(120, this, [this]() { validateReconnect(); });
        });

        connect(m_mems, &MEMSInterface::disconnected, this, [this]() {
            m_connected = false;
            m_readMode = QStringLiteral("disconnected");
            appendEvent(QStringLiteral("CONNECTION"), QStringLiteral("disconnected"), true);
            flushAll();
            if (!m_running) return;
            if (m_step == Step::PreparingDisconnect)
                QTimer::singleShot(450, this, [this]() { beginInitialConnection(); });
            else if (m_step == Step::DisconnectForReconnect)
                QTimer::singleShot(450, this, [this]() { beginReconnectConnection(); });
            else if (m_step != Step::Connecting && m_step != Step::Reconnecting)
                failTest(I18n::text(19053));
        });

        connect(m_mems, &MEMSInterface::failedToConnect, this, [this](const QString &device) {
            appendEvent(QStringLiteral("CONNECTION"), QStringLiteral("failed device=%1").arg(device), true);
            if (m_running && (m_step == Step::Connecting || m_step == Step::Reconnecting))
                failTest(I18n::text(19049).arg(device));
        });

        connect(m_mems, &MEMSInterface::readError, this, [this]() {
            appendEvent(QStringLiteral("READ"), QStringLiteral("read-error"), true);
            if (m_running && m_step != Step::PreparingDisconnect && m_step != Step::DisconnectForReconnect)
                failTest(I18n::text(19053));
        });

        connect(m_mems, &MEMSInterface::errorSendingCommand, this, [this]() {
            appendEvent(QStringLiteral("COMMAND"), QStringLiteral("send-error"), true);
        });

        connect(m_mems, &MEMSInterface::gotEcuId, this, [this](uint8_t *id) {
            if (!id) return;
            const QByteArray raw(reinterpret_cast<const char*>(id), 4);
            const QString hex = QString::fromLatin1(raw.toHex(' ').toUpper());
            m_gotD0 = true;
            setState(m_d0, hex, StateColor::Good);
            appendEvent(QStringLiteral("D0"), QStringLiteral("ecu-id=%1").arg(hex), true);
        });

        connect(m_mems, &MEMSInterface::protocolResponse, this,
                [this](quint8 command, const QByteArray &response) { handleProtocolResponse(command, response); });
        connect(m_mems, &MEMSInterface::dataReady, this, [this]() { captureDataSample(); });

        connect(m_mems, &MEMSInterface::readModeChanged, this, [this](const QString &mode) {
            m_readMode = mode;
            appendEvent(QStringLiteral("MODE"), QStringLiteral("read-mode=%1").arg(mode), true);
            if (!m_running) return;
            if (m_step == Step::Injection && mode == QStringLiteral("injection"))
                m_injectionEntered = true;
            else if (m_step == Step::Restore && mode == QStringLiteral("diagnostic"))
            {
                setState(m_restore, I18n::text(19045), StateColor::Good);
                QTimer::singleShot(200, this, [this]() { beginReconnect(); });
            }
        });

        connect(m_mems, &MEMSInterface::injectionLiveSample, this,
                [this](double finalMs, double baseMs, double transientMs, quint16 baseRaw, quint16 transientRaw, quint8 counter) {
            if (!m_captureActive) return;
            setState(m_injection, I18n::text(19026)
                .arg(finalMs, 0, 'f', 3).arg(baseMs, 0, 'f', 3).arg(transientMs, 0, 'f', 3).arg(counter), StateColor::Good);
            ensureSessionFiles();
            if (m_injectionFile.isOpen())
            {
                m_injectionStream << now() << ',' << m_readMode << ',' << m_firmware << ','
                                  << QString::number(finalMs, 'f', 6) << ',' << QString::number(baseMs, 'f', 6) << ','
                                  << QString::number(transientMs, 'f', 6) << ',' << baseRaw << ',' << transientRaw << ',' << int(counter) << '\n';
                m_injectionStream.flush();
            }
            if (m_running && m_step == Step::Injection)
            {
                ++m_injectionSampleCount;
                if (m_injectionSampleCount >= 20) requestInjectionStop();
            }
        });
    }

    ~Mems19TestTab() override { closeSessionFiles(); }

protected:
    void changeEvent(QEvent *event) override
    {
        QWidget::changeEvent(event);
        if (event && event->type() == QEvent::LanguageChange)
        {
            retranslateUi();
            updateTabTitle();
        }
    }

private:
    enum class Step { Idle, PreparingDisconnect, Connecting, D1, D2, F0, NormalReads, WaitEngine, Injection, Restore, DisconnectForReconnect, Reconnecting, Finished };
    enum class StateColor { Neutral, Good, Warning, Bad };

    static QString colorFor(StateColor state)
    {
        if (state == StateColor::Good) return QStringLiteral("#58d27a");
        if (state == StateColor::Warning) return QStringLiteral("#ff9828");
        if (state == StateColor::Bad) return QStringLiteral("#ff5b55");
        return QString();
    }

    void addStatusRow(QGridLayout *layout, int row, QLabel *&key, QLabel *&value)
    {
        key = new QLabel(this);
        QFont keyFont = key->font(); keyFont.setBold(true); key->setFont(keyFont);
        value = new QLabel(this); value->setTextInteractionFlags(Qt::TextSelectableByMouse); value->setWordWrap(true);
        layout->addWidget(key, row, 0, Qt::AlignTop); layout->addWidget(value, row, 1);
    }

    void setState(QLabel *label, const QString &text, StateColor state)
    {
        if (!label) return;
        label->setText(text);
        const QString color = colorFor(state);
        label->setStyleSheet(color.isEmpty() ? QStringLiteral("font-weight:600;") : QStringLiteral("font-weight:600;color:%1;").arg(color));
    }

    void setOverall(const QString &text, const QString &color)
    {
        if (!m_overall) return;
        m_overall->setText(text);
        m_overall->setStyleSheet(QStringLiteral("font-weight:700;color:%1;").arg(color));
    }

    void setInstruction(const QString &text) { if (m_instruction) m_instruction->setText(text); }

    void retranslateUi()
    {
        if (m_title) m_title->setText(I18n::text(19001));
        if (m_description) m_description->setText(I18n::text(19002));
        if (m_safety) m_safety->setText(I18n::text(19003));
        if (m_statusBox) m_statusBox->setTitle(I18n::text(19001));
        if (m_connectionKey) m_connectionKey->setText(I18n::text(19006));
        if (m_d0Key) m_d0Key->setText(I18n::text(19007));
        if (m_d1Key) m_d1Key->setText(I18n::text(19008));
        if (m_d2Key) m_d2Key->setText(I18n::text(19009));
        if (m_f0Key) m_f0Key->setText(I18n::text(19010));
        if (m_readsKey) m_readsKey->setText(I18n::text(19011));
        if (m_engineKey) m_engineKey->setText(I18n::text(19012));
        if (m_injectionKey) m_injectionKey->setText(I18n::text(19013));
        if (m_restoreKey) m_restoreKey->setText(I18n::text(19014));
        if (m_reconnectKey) m_reconnectKey->setText(I18n::text(19015));
        if (m_logKey) m_logKey->setText(I18n::text(19016));
        if (m_startButton) m_startButton->setText(I18n::text(19020));
        if (m_stopButton) m_stopButton->setText(I18n::text(19021));
        if (m_skipEngineButton) m_skipEngineButton->setText(I18n::text(19022));
        if (m_openFolderButton) m_openFolderButton->setText(I18n::text(19023));
        if (m_eventsTitle) m_eventsTitle->setText(I18n::text(19024));
    }

    void updateTabTitle()
    {
        QWidget *w = window(); if (!w) return;
        QTabWidget *tabs = w->findChild<QTabWidget*>(QStringLiteral("Tab_main")); if (!tabs) return;
        const int index = tabs->indexOf(this); if (index >= 0) tabs->setTabText(index, I18n::text(19000));
    }

    void resetStatus()
    {
        m_connected = m_mems && m_mems->isConnected();
        m_readMode = m_connected ? QStringLiteral("diagnostic") : QStringLiteral("disconnected");
        setState(m_connection, m_connected ? I18n::text(19019) : I18n::text(19018), StateColor::Neutral);
        setState(m_d0, I18n::text(19017), StateColor::Neutral); setState(m_d1, I18n::text(19017), StateColor::Neutral);
        setState(m_d2, I18n::text(19017), StateColor::Neutral); setState(m_f0, I18n::text(19017), StateColor::Neutral);
        setState(m_reads, QStringLiteral("0"), StateColor::Neutral); setState(m_engine, I18n::text(19017), StateColor::Neutral);
        setState(m_injection, I18n::text(19017), StateColor::Neutral); setState(m_restore, I18n::text(19017), StateColor::Neutral);
        setState(m_reconnect, I18n::text(19017), StateColor::Neutral);
        setState(m_logPathLabel, m_eventPath.isEmpty() ? I18n::text(19027) : I18n::text(19028).arg(m_eventPath), StateColor::Neutral);
    }

    void resetRunState()
    {
        m_step = Step::Idle; m_partial = false; m_gotD0 = false; m_mems19Detected = false; m_reconnectMems19Detected = false;
        m_injectionEntered = false; m_injectionSampleCount = 0; m_readCount = 0; m_normalReadStart = 0; m_dataRowsSinceFlush = 0;
        m_lastRpm = 0; m_firmware.clear(); m_initialConnectionDetail.clear(); m_readMode = QStringLiteral("disconnected");
    }

    void startTest()
    {
        if (m_running || !m_mems) return;
        closeSessionFiles(); if (m_events) m_events->clear(); resetRunState(); resetStatus(); m_captureActive = true;
        if (!ensureSessionFiles())
        {
            m_captureActive = false; setOverall(I18n::text(19042).arg(I18n::text(19054)), QStringLiteral("#ff5b55"));
            setInstruction(I18n::text(19054)); return;
        }
        m_running = true; m_startButton->setEnabled(false); m_stopButton->setEnabled(true);
        m_skipEngineButton->setEnabled(false); m_skipEngineButton->setVisible(false);
        setOverall(I18n::text(19030), QStringLiteral("#ff9828")); setInstruction(I18n::text(19005).arg(I18n::text(19031)));
        appendEvent(QStringLiteral("TEST"), QStringLiteral("guided-test-start"), true);
        if (m_mems->isConnected())
        {
            m_step = Step::PreparingDisconnect; setInstruction(I18n::text(19005).arg(I18n::text(19056)));
            if (!clickTopButton(QStringLiteral("m_disconnectButton"))) failTest(I18n::text(19057)); else armTimeout(Step::PreparingDisconnect, 7000);
        }
        else beginInitialConnection();
    }

    void stopTest()
    {
        if (!m_running) return;
        if (m_mems) m_mems->onMappedInjectionRequestState(false);
        appendEvent(QStringLiteral("TEST"), QStringLiteral("stopped-by-user"), true);
        m_running = false; m_captureActive = false; m_step = Step::Finished;
        m_startButton->setEnabled(true); m_stopButton->setEnabled(false); m_skipEngineButton->setEnabled(false); m_skipEngineButton->setVisible(false);
        setOverall(I18n::text(19047), QStringLiteral("#ff9828")); setInstruction(I18n::text(19005).arg(I18n::text(19058))); flushAll();
    }

    bool clickTopButton(const QString &objectName)
    {
        QWidget *w = window(); if (!w) return false;
        QPushButton *button = w->findChild<QPushButton*>(objectName); if (!button || !button->isEnabled()) return false;
        QTimer::singleShot(0, button, [button]() { button->click(); }); return true;
    }

    void beginInitialConnection()
    {
        if (!m_running) return;
        m_step = Step::Connecting; m_mems19Detected = false; m_initialConnectionDetail.clear();
        setState(m_connection, I18n::text(19059), StateColor::Warning); setInstruction(I18n::text(19005).arg(I18n::text(19031)));
        appendEvent(QStringLiteral("STEP"), QStringLiteral("connect-mems19"), true);
        if (!clickTopButton(QStringLiteral("m_connectButton"))) { failTest(I18n::text(19057)); return; }
        armTimeout(Step::Connecting, 45000);
    }

    void validateInitialConnection()
    {
        if (!m_running || m_step != Step::Connecting) return;
        if (!m_mems19Detected) { failTest(I18n::text(19048)); return; }
        setState(m_connection, m_initialConnectionDetail.isEmpty() ? I18n::text(19019) : m_initialConnectionDetail, StateColor::Good);
        if (!m_gotD0) { m_partial = true; setState(m_d0, I18n::text(19051), StateColor::Warning); }
        requestD1();
    }

    void requestD1() { m_step = Step::D1; setState(m_d1, I18n::text(19059), StateColor::Warning); setInstruction(I18n::text(19005).arg(I18n::text(19032))); queueProtocolCommand(0xD1); armTimeout(Step::D1, 3000); }
    void requestD2() { m_step = Step::D2; setState(m_d2, I18n::text(19059), StateColor::Warning); setInstruction(I18n::text(19005).arg(I18n::text(19033))); queueProtocolCommand(0xD2); armTimeout(Step::D2, 3000); }
    void requestF0() { m_step = Step::F0; setState(m_f0, I18n::text(19059), StateColor::Warning); setInstruction(I18n::text(19005).arg(I18n::text(19034))); queueProtocolCommand(0xF0); armTimeout(Step::F0, 3000); }

    void queueProtocolCommand(quint8 command)
    {
        if (!m_mems) return;
        QMetaObject::invokeMethod(m_mems, [this, command]() { if (m_mems) m_mems->onProtocolCommandRequested(command); }, Qt::QueuedConnection);
    }

    void beginNormalReads()
    {
        if (!m_running) return;
        m_step = Step::NormalReads; m_normalReadStart = m_readCount;
        setState(m_reads, I18n::text(19059), StateColor::Warning); setInstruction(I18n::text(19005).arg(I18n::text(19035)));
        appendEvent(QStringLiteral("STEP"), QStringLiteral("normal-polling-validation"), true); armTimeout(Step::NormalReads, 12000);
    }

    void beginInjection()
    {
        if (!m_running) return;
        if (m_firmware.isEmpty())
        {
            m_partial = true; setState(m_injection, I18n::text(19060), StateColor::Warning); setState(m_restore, I18n::text(19061), StateColor::Good);
            QTimer::singleShot(200, this, [this]() { beginReconnect(); }); return;
        }
        m_step = Step::Injection; m_injectionEntered = false; m_injectionSampleCount = 0;
        setState(m_injection, I18n::text(19059), StateColor::Warning); setInstruction(I18n::text(19005).arg(I18n::text(19037)));
        appendEvent(QStringLiteral("STEP"), QStringLiteral("mode4-injection-start firmware=%1").arg(m_firmware), true);
        m_mems->onMappedInjectionRequestState(true);
        const QString firmware = m_firmware;
        QMetaObject::invokeMethod(m_mems, [this, firmware]() { if (m_mems) m_mems->onMappedInjectionModeRequested(true, firmware); }, Qt::QueuedConnection);
        armTimeout(Step::Injection, 10000);
    }

    void requestInjectionStop()
    {
        if (!m_running || m_step != Step::Injection) return;
        m_step = Step::Restore; setState(m_restore, I18n::text(19059), StateColor::Warning);
        setInstruction(I18n::text(19005).arg(I18n::text(19038))); appendEvent(QStringLiteral("STEP"), QStringLiteral("restore-diagnostic"), true);
        m_mems->onMappedInjectionRequestState(false); armTimeout(Step::Restore, 7000);
    }

    void injectionTimeout()
    {
        if (!m_running || m_step != Step::Injection) return;
        m_partial = true; setState(m_injection, I18n::text(19062), StateColor::Warning);
        appendEvent(QStringLiteral("INJECTION"), QStringLiteral("no-mode4-sample-before-timeout"), true); m_mems->onMappedInjectionRequestState(false);
        if (m_injectionEntered)
        {
            m_step = Step::Restore; setState(m_restore, I18n::text(19059), StateColor::Warning);
            setInstruction(I18n::text(19005).arg(I18n::text(19038))); armTimeout(Step::Restore, 7000);
        }
        else
        {
            setState(m_restore, I18n::text(19061), StateColor::Good); QTimer::singleShot(250, this, [this]() { beginReconnect(); });
        }
    }

    void beginReconnect()
    {
        if (!m_running) return;
        m_step = Step::DisconnectForReconnect; setState(m_reconnect, I18n::text(19059), StateColor::Warning);
        setInstruction(I18n::text(19005).arg(I18n::text(19039))); appendEvent(QStringLiteral("STEP"), QStringLiteral("disconnect-for-reconnect"), true);
        if (!m_mems->isConnected()) { QTimer::singleShot(250, this, [this]() { beginReconnectConnection(); }); return; }
        if (!clickTopButton(QStringLiteral("m_disconnectButton"))) { failTest(I18n::text(19057)); return; }
        armTimeout(Step::DisconnectForReconnect, 7000);
    }

    void beginReconnectConnection()
    {
        if (!m_running) return;
        m_step = Step::Reconnecting; m_reconnectMems19Detected = false; setInstruction(I18n::text(19005).arg(I18n::text(19039)));
        appendEvent(QStringLiteral("STEP"), QStringLiteral("reconnect-mems19"), true);
        if (!clickTopButton(QStringLiteral("m_connectButton"))) { failTest(I18n::text(19057)); return; }
        armTimeout(Step::Reconnecting, 45000);
    }

    void validateReconnect()
    {
        if (!m_running || m_step != Step::Reconnecting) return;
        if (!m_reconnectMems19Detected) { failTest(I18n::text(19063)); return; }
        setState(m_reconnect, I18n::text(19046), StateColor::Good); finishTest();
    }

    void finishTest()
    {
        if (!m_running) return;
        appendEvent(QStringLiteral("TEST"), m_partial ? QStringLiteral("finished-partial") : QStringLiteral("finished-ok"), true);
        m_running = false; m_captureActive = false; m_step = Step::Finished;
        m_startButton->setEnabled(true); m_stopButton->setEnabled(false); m_skipEngineButton->setEnabled(false); m_skipEngineButton->setVisible(false);
        setOverall(m_partial ? I18n::text(19041) : I18n::text(19040), m_partial ? QStringLiteral("#ff9828") : QStringLiteral("#58d27a"));
        setInstruction(I18n::text(19005).arg(I18n::text(19064))); flushAll();
    }

    void failTest(const QString &reason)
    {
        if (!m_running) return;
        if (m_mems) m_mems->onMappedInjectionRequestState(false);
        appendEvent(QStringLiteral("TEST"), QStringLiteral("failed reason=%1").arg(reason), true);
        m_running = false; m_captureActive = false; m_step = Step::Finished;
        m_startButton->setEnabled(true); m_stopButton->setEnabled(false); m_skipEngineButton->setEnabled(false); m_skipEngineButton->setVisible(false);
        setOverall(I18n::text(19042).arg(reason), QStringLiteral("#ff5b55")); setInstruction(I18n::text(19005).arg(I18n::text(19065))); flushAll();
    }

    void armTimeout(Step expected, int milliseconds)
    {
        QTimer::singleShot(milliseconds, this, [this, expected]() {
            if (!m_running || m_step != expected) return;
            if (expected == Step::D1) { m_partial = true; setState(m_d1, I18n::text(19051), StateColor::Warning); requestD2(); }
            else if (expected == Step::D2) { m_partial = true; setState(m_d2, I18n::text(19051), StateColor::Warning); requestF0(); }
            else if (expected == Step::F0) { m_partial = true; setState(m_f0, I18n::text(19051), StateColor::Warning); beginNormalReads(); }
            else if (expected == Step::Injection) injectionTimeout();
            else if (expected == Step::Connecting) failTest(I18n::text(19050));
            else if (expected == Step::Reconnecting) failTest(I18n::text(19063));
            else if (expected == Step::NormalReads) failTest(I18n::text(19066));
            else if (expected == Step::Restore) failTest(I18n::text(19067));
            else if (expected == Step::PreparingDisconnect || expected == Step::DisconnectForReconnect) failTest(I18n::text(19068));
        });
    }

    QString now() const { return QDateTime::currentDateTime().toString(Qt::ISODateWithMs); }

    QString sessionDirectory() const
    {
        QString root = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation); if (root.isEmpty()) root = QDir::homePath();
        return QDir(root).filePath(QStringLiteral("ECU Mems Manager/MEMS19-tests"));
    }

    bool ensureSessionFiles()
    {
        if (m_eventFile.isOpen() && m_dataFile.isOpen() && m_injectionFile.isOpen()) return true;
        const QString dirPath = sessionDirectory(); if (!QDir().mkpath(dirPath)) return false;
        const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss-zzz"));
        const QString base = QDir(dirPath).filePath(QStringLiteral("MEMS19_test_%1").arg(stamp));
        m_eventPath = base + QStringLiteral("_events.log"); m_dataPath = base + QStringLiteral("_data.csv"); m_injectionPath = base + QStringLiteral("_injection.csv");
        m_eventFile.setFileName(m_eventPath); m_dataFile.setFileName(m_dataPath); m_injectionFile.setFileName(m_injectionPath);
        if (!m_eventFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text) ||
            !m_dataFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text) ||
            !m_injectionFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        { closeSessionFiles(); return false; }
        m_eventStream.setDevice(&m_eventFile); m_dataStream.setDevice(&m_dataFile); m_injectionStream.setDevice(&m_injectionFile);
        m_eventStream.setCodec("UTF-8"); m_dataStream.setCodec("UTF-8"); m_injectionStream.setCodec("UTF-8");
        m_eventStream << "# ECU Mems Manager - MEMS 1.9 guided validation\n" << "# version=" << APP_VERSION << " build=" << APP_BUILD_NUMBER
                      << " commit=" << APP_COMMIT_SHA << '\n' << "# started=" << now() << '\n'; m_eventStream.flush();
        m_dataStream << "timestamp,engine_rpm,coolant_raw,intake_raw,map_kpa,battery_raw,throttle_raw,idle_switch,iac_position,idle_error,ignition_advance,coil_time,lambda_voltage,closed_loop,long_term_fuel_trim,short_term_fuel_trim,dtc0,dtc1,dtc2,dtc3,dtc4,dtc5,battery_v,coolant_c,intake_c,coil_ms,ignition_deg\n"; m_dataStream.flush();
        m_injectionStream << "timestamp,read_mode,firmware,final_ms,base_ms,transient_ms,base_raw,transient_raw,counter\n"; m_injectionStream.flush();
        setState(m_logPathLabel, I18n::text(19028).arg(m_eventPath), StateColor::Good); return true;
    }

    void closeSessionFiles()
    {
        flushAll(); if (m_eventFile.isOpen()) m_eventFile.close(); if (m_dataFile.isOpen()) m_dataFile.close(); if (m_injectionFile.isOpen()) m_injectionFile.close();
        m_eventStream.setDevice(nullptr); m_dataStream.setDevice(nullptr); m_injectionStream.setDevice(nullptr);
        m_eventPath.clear(); m_dataPath.clear(); m_injectionPath.clear();
    }

    void flushAll()
    {
        if (m_eventFile.isOpen()) m_eventStream.flush(); if (m_dataFile.isOpen()) m_dataStream.flush(); if (m_injectionFile.isOpen()) m_injectionStream.flush();
    }

    void appendEvent(const QString &type, const QString &detail, bool flush)
    {
        if (!m_captureActive && !m_running && !m_eventFile.isOpen()) return;
        if (!m_eventFile.isOpen() && !ensureSessionFiles()) return;
        const QString line = QStringLiteral("%1\t%2\t%3").arg(now(), type, detail);
        if (m_eventFile.isOpen()) { m_eventStream << line << '\n'; if (flush) m_eventStream.flush(); }
        if (m_events) m_events->appendPlainText(line);
    }

    void captureDataSample()
    {
        if (!m_mems || !m_captureActive) return;
        mems_data *d = m_mems->getData(); if (!d) return; ensureSessionFiles(); ++m_readCount; m_lastRpm = d->engine_rpm;
        if (m_dataFile.isOpen())
        {
            m_dataStream << now() << ',' << d->engine_rpm << ',' << int(d->coolant_temp) << ',' << int(d->intake_air_temp) << ',' << int(d->map_kpa) << ','
                << int(d->battery_voltage) << ',' << int(d->throttle_pot) << ',' << int(d->idle_switch) << ',' << int(d->iac_position) << ',' << d->idle_error << ','
                << int(d->ignition_advance) << ',' << d->coil_time << ',' << int(d->lambda_voltage) << ',' << int(d->closed_loop) << ',' << int(d->long_term_fuel_trim) << ','
                << int(d->short_term_fuel_trim) << ',' << int(d->dtc0) << ',' << int(d->dtc1) << ',' << int(d->dtc2) << ',' << int(d->dtc3) << ',' << int(d->dtc4) << ',' << int(d->dtc5) << ','
                << QString::number(d->battery_voltage / 10.0, 'f', 2) << ',' << (int(d->coolant_temp) - 55) << ',' << (int(d->intake_air_temp) - 55) << ','
                << QString::number(d->coil_time * 0.002, 'f', 3) << ',' << QString::number((d->ignition_advance * 0.5) - 24.0, 'f', 2) << '\n';
            if (++m_dataRowsSinceFlush >= 16) { m_dataStream.flush(); m_dataRowsSinceFlush = 0; }
        }
        if (m_uiRefresh.elapsed() >= 250)
        {
            setState(m_reads, I18n::text(19025).arg(m_readCount).arg(d->engine_rpm).arg(d->battery_voltage / 10.0, 0, 'f', 1).arg(int(d->map_kpa)), StateColor::Good);
            m_uiRefresh.restart();
        }
        if (!m_running) return;
        if (m_step == Step::NormalReads && (m_readCount - m_normalReadStart) >= 12)
        {
            setState(m_reads, I18n::text(19025).arg(m_readCount).arg(d->engine_rpm).arg(d->battery_voltage / 10.0, 0, 'f', 1).arg(int(d->map_kpa)), StateColor::Good);
            if (d->engine_rpm > 0)
            {
                setState(m_engine, I18n::text(19043).arg(d->engine_rpm), StateColor::Good); QTimer::singleShot(150, this, [this]() { beginInjection(); });
            }
            else
            {
                m_step = Step::WaitEngine; setState(m_engine, I18n::text(19069), StateColor::Warning); setInstruction(I18n::text(19005).arg(I18n::text(19036)));
                m_skipEngineButton->setVisible(true); m_skipEngineButton->setEnabled(true);
            }
        }
        else if (m_step == Step::WaitEngine && d->engine_rpm > 0)
        {
            setState(m_engine, I18n::text(19043).arg(d->engine_rpm), StateColor::Good); m_skipEngineButton->setEnabled(false); m_skipEngineButton->setVisible(false);
            QTimer::singleShot(150, this, [this]() { beginInjection(); });
        }
    }

    static QString firmwareFromReply(const QByteArray &response)
    {
        QString printable; printable.reserve(response.size());
        for (char c : response)
        {
            const unsigned char u = static_cast<unsigned char>(c);
            if ((u >= '0' && u <= '9') || (u >= 'A' && u <= 'Z') || (u >= 'a' && u <= 'z')) printable.append(QChar(u).toUpper()); else printable.append(QLatin1Char(' '));
        }
        const QRegularExpressionMatch match = QRegularExpression(QStringLiteral("([A-Z0-9]{8})")).match(printable);
        return match.hasMatch() ? match.captured(1) : QString();
    }

    void handleProtocolResponse(quint8 command, const QByteArray &response)
    {
        const QString cmd = QStringLiteral("%1").arg(command, 2, 16, QLatin1Char('0')).toUpper();
        const QString hex = response.isEmpty() ? QStringLiteral("<empty>") : QString::fromLatin1(response.toHex(' ').toUpper());
        appendEvent(QStringLiteral("RX"), QStringLiteral("cmd=0x%1 bytes=%2").arg(cmd, hex), true);
        if (command == 0xD0)
        {
            if (!response.isEmpty()) { m_gotD0 = true; setState(m_d0, hex, StateColor::Good); }
        }
        else if (command == 0xD1)
        {
            m_firmware = firmwareFromReply(response);
            if (response.isEmpty()) setState(m_d1, I18n::text(19051), StateColor::Warning);
            else if (m_firmware.isEmpty()) { m_partial = true; setState(m_d1, hex, StateColor::Warning); }
            else setState(m_d1, QStringLiteral("%1  [%2]").arg(m_firmware, hex), StateColor::Good);
            if (m_running && m_step == Step::D1) requestD2();
        }
        else if (command == 0xD2)
        {
            setState(m_d2, response.isEmpty() ? I18n::text(19051) : hex, response.isEmpty() ? StateColor::Warning : StateColor::Good);
            if (response.isEmpty()) m_partial = true; if (m_running && m_step == Step::D2) requestF0();
        }
        else if (command == 0xF0)
        {
            QString decoded;
            for (char c : response)
            {
                const quint8 v = quint8(c); if (v == 0x14u) decoded = QStringLiteral("mode3"); else if (v == 0x1Eu) decoded = QStringLiteral("injection"); else if (v == 0x50u) decoded = QStringLiteral("diagnostic");
            }
            if (!decoded.isEmpty()) m_readMode = decoded;
            const bool diagnostic = decoded == QStringLiteral("diagnostic");
            setState(m_f0, decoded.isEmpty() ? hex : QStringLiteral("%1  [%2]").arg(decoded, hex), diagnostic ? StateColor::Good : StateColor::Warning);
            if (!diagnostic) m_partial = true; if (m_running && m_step == Step::F0) beginNormalReads();
        }
    }

    MEMSInterface *m_mems = nullptr;
    QLabel *m_title = nullptr, *m_description = nullptr, *m_safety = nullptr, *m_overall = nullptr, *m_instruction = nullptr;
    QGroupBox *m_statusBox = nullptr;
    QLabel *m_connectionKey = nullptr, *m_connection = nullptr, *m_d0Key = nullptr, *m_d0 = nullptr, *m_d1Key = nullptr, *m_d1 = nullptr;
    QLabel *m_d2Key = nullptr, *m_d2 = nullptr, *m_f0Key = nullptr, *m_f0 = nullptr, *m_readsKey = nullptr, *m_reads = nullptr;
    QLabel *m_engineKey = nullptr, *m_engine = nullptr, *m_injectionKey = nullptr, *m_injection = nullptr, *m_restoreKey = nullptr, *m_restore = nullptr;
    QLabel *m_reconnectKey = nullptr, *m_reconnect = nullptr, *m_logKey = nullptr, *m_logPathLabel = nullptr, *m_eventsTitle = nullptr;
    QPushButton *m_startButton = nullptr, *m_stopButton = nullptr, *m_skipEngineButton = nullptr, *m_openFolderButton = nullptr;
    QPlainTextEdit *m_events = nullptr;
    QFile m_eventFile, m_dataFile, m_injectionFile; QTextStream m_eventStream, m_dataStream, m_injectionStream;
    QString m_eventPath, m_dataPath, m_injectionPath, m_firmware, m_readMode, m_initialConnectionDetail;
    quint64 m_readCount = 0, m_normalReadStart = 0; int m_dataRowsSinceFlush = 0, m_lastRpm = 0, m_injectionSampleCount = 0;
    bool m_connected = false, m_running = false, m_captureActive = false, m_partial = false, m_gotD0 = false;
    bool m_mems19Detected = false, m_reconnectMems19Detected = false, m_injectionEntered = false;
    Step m_step = Step::Idle; QElapsedTimer m_uiRefresh;
};

#endif // MEMS19TESTTAB_H
