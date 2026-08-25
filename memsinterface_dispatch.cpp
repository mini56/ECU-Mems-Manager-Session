// Connection and safety dispatcher layered around the historical MEMSInterface implementation.
// The historical transport/command code stays in memsinterface.cpp. BUILD #27
// routes every mutating legacy call through a central family/mode policy before
// the historical C API can transmit it.
#include "memsinterface.h"

#define connectToECU connectToECULegacy
#define onStartPollingRequest onStartPollingRequestLegacy
#define onProtocolCommandRequested onProtocolCommandRequestedLegacy
#define mems_test_actuator(info, cmd, data) guardedMemsTestActuator((cmd), (data))
#define mems_move_iac(info, desiredPos) guardedMemsMoveIac((desiredPos))
#define mems_clear_faults(info) guardedClearFaults()
#define mems_reset_adjustments(info) guardedResetAdjustments()
#define mems_reset_ECU(info) guardedResetEcu()
#include "memsinterface.cpp"
#undef mems_reset_ECU
#undef mems_reset_adjustments
#undef mems_clear_faults
#undef mems_move_iac
#undef mems_test_actuator
#undef onProtocolCommandRequested
#undef onStartPollingRequest
#undef connectToECU

#include <QElapsedTimer>
#include <QVariant>
#include <QSerialPort>
#include "serialadapterdetector.h"

MemsEcuFamily MEMSInterface::ecuFamily() const
{
    return static_cast<MemsEcuFamily>(m_ecuFamily.loadAcquire());
}

MemsDiagnosticMode MEMSInterface::diagnosticMode() const
{
    return static_cast<MemsDiagnosticMode>(m_diagnosticMode.loadAcquire());
}

void MEMSInterface::setEcuFamily(MemsEcuFamily family)
{
    m_ecuFamily.storeRelease(static_cast<int>(family));
}

void MEMSInterface::setDiagnosticMode(MemsDiagnosticMode mode)
{
    m_diagnosticMode.storeRelease(static_cast<int>(mode));
}

void MEMSInterface::resetProtocolContext()
{
    setDiagnosticMode(MemsDiagnosticMode::Unknown);
    setEcuFamily(MemsEcuFamily::Unknown);
}

bool MEMSInterface::guardedMemsTestActuator(actuator_cmd cmd, uint8_t *data)
{
    if (property("injectionRamTestRunning").toBool() ||
        property("mappedInjectionModeActive").toBool())
        return false;

    const quint8 command = static_cast<quint8>(cmd);
    if (!MemsProtocolSafety::allowsMutation(ecuFamily(), diagnosticMode(), command))
        return false;
    return ::mems_test_actuator(&m_memsinfo, cmd, data);
}

bool MEMSInterface::guardedMemsMoveIac(uint8_t desiredPos)
{
    if (property("injectionRamTestRunning").toBool() ||
        property("mappedInjectionModeActive").toBool())
        return false;

    // mems_move_iac() can send either FD or FE depending on the current IAC
    // position, therefore both possible mutations must be legal first.
    if (!MemsProtocolSafety::allowsMutation(ecuFamily(), diagnosticMode(), 0xFDu) ||
        !MemsProtocolSafety::allowsMutation(ecuFamily(), diagnosticMode(), 0xFEu))
        return false;
    return ::mems_move_iac(&m_memsinfo, desiredPos);
}

bool MEMSInterface::guardedClearFaults()
{
    if (property("injectionRamTestRunning").toBool() ||
        property("mappedInjectionModeActive").toBool())
        return false;
    if (!MemsProtocolSafety::allowsMutation(ecuFamily(), diagnosticMode(), 0xCCu))
        return false;
    return ::mems_clear_faults(&m_memsinfo);
}

bool MEMSInterface::guardedResetAdjustments()
{
    if (property("injectionRamTestRunning").toBool() ||
        property("mappedInjectionModeActive").toBool())
        return false;
    if (!MemsProtocolSafety::allowsMutation(ecuFamily(), diagnosticMode(), 0x0Fu))
        return false;
    return ::mems_reset_adjustments(&m_memsinfo);
}

bool MEMSInterface::guardedResetEcu()
{
    if (property("injectionRamTestRunning").toBool() ||
        property("mappedInjectionModeActive").toBool())
        return false;
    if (!MemsProtocolSafety::allowsMutation(ecuFamily(), diagnosticMode(), 0xFAu))
        return false;
    return ::mems_reset_ECU(&m_memsinfo);
}

void MEMSInterface::onProtocolCommandRequested(quint8 command)
{
    if (m_shutdownRequested.loadAcquire() != 0)
        return;

    if (!(m_initComplete && mems_is_connected(&m_memsinfo)))
    {
        emit notConnected();
        return;
    }

    // The dedicated RAM readers own the serial session while they are active.
    // No generic ECU/ROSCO byte is allowed to interleave with their controlled
    // Mode-4 transition/read/restore transaction.
    if (property("injectionRamTestRunning").toBool() ||
        property("mappedInjectionModeActive").toBool())
    {
        emit errorSendingCommand();
        return;
    }

    if (!MemsProtocolSafety::allowsGenericCommand(ecuFamily(), diagnosticMode(), command))
    {
        emit errorSendingCommand();
        return;
    }

    onProtocolCommandRequestedLegacy(command);
}

bool MEMSInterface::tryRoscoConnect(const QString &devicePath)
{
    if (m_shutdownRequested.loadAcquire() != 0)
        return false;

    if (m_initComplete && mems_is_connected(&m_memsinfo))
    {
        mems_disconnect(&m_memsinfo);
        m_connectedState.storeRelease(0);
    }

    resetProtocolContext();
    m_deviceName = devicePath;
    return connectToECULegacy();
}

bool MEMSInterface::performMems19Wakeup(const QString &qtPortName, QString *detail)
{
    if (detail) detail->clear();

    if (m_shutdownRequested.loadAcquire() != 0)
    {
        if (detail) *detail = QStringLiteral("shutdown-requested");
        return false;
    }

    QSerialPort port;
    port.setPortName(qtPortName);
    port.setBaudRate(QSerialPort::Baud9600);
    port.setDataBits(QSerialPort::Data8);
    port.setParity(QSerialPort::NoParity);
    port.setStopBits(QSerialPort::OneStop);
    port.setFlowControl(QSerialPort::NoFlowControl);

    if (!port.open(QIODevice::ReadWrite))
    {
        if (detail) *detail = QStringLiteral("port-open-failed");
        return false;
    }

    port.clear(QSerialPort::AllDirections);
    if (!port.setBreakEnabled(false))
    {
        if (detail) *detail = QStringLiteral("break-control-unavailable");
        port.close();
        return false;
    }

    // ISO 9141 slow initialisation for Rover MEMS 1.9.
    // ECU address is 0x16, sent at 5 baud: start bit, 8 data bits LSB first,
    // then stop bit. 5 baud == 200 ms per bit. Break drives K-line active-low
    // through a KKL transceiver; no direct TTL-to-K-line connection is assumed.
    QThread::msleep(300);
    const quint8 address = 0x16u;

    auto sendSlowBit = [this, &port](bool high) -> bool {
        if (m_shutdownRequested.loadAcquire() != 0)
            return false;
        // BREAK = active low, therefore a logical high means BREAK disabled.
        if (!port.setBreakEnabled(!high))
            return false;
        QThread::msleep(200);
        return m_shutdownRequested.loadAcquire() == 0;
    };

    if (!sendSlowBit(false))
    {
        if (detail) *detail = m_shutdownRequested.loadAcquire() != 0
            ? QStringLiteral("shutdown-requested")
            : QStringLiteral("slow-init-start-failed");
        port.setBreakEnabled(false);
        port.close();
        return false;
    }

    for (int bit = 0; bit < 8; ++bit)
    {
        const bool high = ((address >> bit) & 0x01u) != 0u;
        if (!sendSlowBit(high))
        {
            if (detail) *detail = m_shutdownRequested.loadAcquire() != 0
                ? QStringLiteral("shutdown-requested")
                : QStringLiteral("slow-init-data-failed");
            port.setBreakEnabled(false);
            port.close();
            return false;
        }
    }

    if (!sendSlowBit(true))
    {
        if (detail) *detail = m_shutdownRequested.loadAcquire() != 0
            ? QStringLiteral("shutdown-requested")
            : QStringLiteral("slow-init-stop-failed");
        port.setBreakEnabled(false);
        port.close();
        return false;
    }
    port.setBreakEnabled(false);

    // ECU reply is ISO 9141 sync + two key bytes. Typical MEMS 1.9 is
    // 55 76 83. Do not hard-code the key bytes: accept the 0x55 sync and
    // complement key byte 2, which is the ISO 9141 handshake operation.
    QByteArray wakeReply;
    QElapsedTimer timer;
    timer.start();
    int syncIndex = -1;
    while (timer.elapsed() < 1600 && syncIndex < 0 &&
           m_shutdownRequested.loadAcquire() == 0)
    {
        if (port.waitForReadyRead(120))
            wakeReply += port.readAll();
        else
            wakeReply += port.readAll();

        for (int i = 0; i + 2 < wakeReply.size(); ++i)
        {
            if (quint8(wakeReply.at(i)) == 0x55u)
            {
                syncIndex = i;
                break;
            }
        }
    }

    if (m_shutdownRequested.loadAcquire() != 0)
    {
        if (detail) *detail = QStringLiteral("shutdown-requested");
        port.close();
        return false;
    }

    if (syncIndex < 0 || syncIndex + 2 >= wakeReply.size())
    {
        if (detail) *detail = QStringLiteral("no-iso9141-key-bytes");
        port.close();
        return false;
    }

    const quint8 keyByte2 = quint8(wakeReply.at(syncIndex + 2));
    const char invertedKey = char(quint8(~keyByte2));
    if (port.write(&invertedKey, 1) != 1 || !port.waitForBytesWritten(400))
    {
        if (detail) *detail = QStringLiteral("key-ack-write-failed");
        port.close();
        return false;
    }

    // K-line echoes our own byte. The ECU then returns the complement of
    // address 0x16, i.e. 0xE9. Search the received stream rather than assuming
    // a fixed position so interfaces with or without echo are both tolerated.
    QByteArray confirmation;
    timer.restart();
    bool confirmed = false;
    while (timer.elapsed() < 1200 && !confirmed &&
           m_shutdownRequested.loadAcquire() == 0)
    {
        if (port.waitForReadyRead(100))
            confirmation += port.readAll();
        else
            confirmation += port.readAll();

        for (char c : confirmation)
        {
            if (quint8(c) == 0xE9u)
            {
                confirmed = true;
                break;
            }
        }
    }

    port.close();
    if (m_shutdownRequested.loadAcquire() != 0)
    {
        if (detail) *detail = QStringLiteral("shutdown-requested");
        return false;
    }
    if (!confirmed)
    {
        if (detail) *detail = QStringLiteral("address-confirmation-missing");
        return false;
    }

    if (detail)
        *detail = QStringLiteral("iso9141-mems19-wakeup-ok");
    return true;
}

bool MEMSInterface::connectToECU()
{
    if (m_shutdownRequested.loadAcquire() != 0)
        return false;

    const QString configuredDevice = m_deviceName;
    const QList<DetectedSerialAdapter> adapters =
        SerialAdapterDetector::availableAdapters(configuredDevice);

    // First use the historical ROSCO path on every candidate. A successful
    // transport handshake proves a normal session, but it does NOT by itself
    // prove the ECU generation. Family therefore remains Unknown until an
    // identification path can establish it from evidence.
    for (const DetectedSerialAdapter &adapter : adapters)
    {
        if (m_shutdownRequested.loadAcquire() != 0)
            break;

        if (tryRoscoConnect(adapter.devicePath))
        {
            setEcuFamily(MemsEcuFamily::Unknown);
            setDiagnosticMode(MemsDiagnosticMode::Normal);
            emit serialInterfaceDetected(adapter.portName,
                                         adapter.adapterFamily,
                                         QStringLiteral("ROSCO 1.3/1.6"));
            return true;
        }
        if (m_initComplete && mems_is_connected(&m_memsinfo))
        {
            mems_disconnect(&m_memsinfo);
            m_connectedState.storeRelease(0);
            resetProtocolContext();
        }
    }

    // Nothing answered the normal path: try the MEMS 1.9 ISO-9141 slow wake-up.
    // Only after a valid 0x55/key-byte handshake and 0xE9 address confirmation
    // do we hand the already-awake ECU to the existing 9600-baud ROSCO init.
    for (const DetectedSerialAdapter &adapter : adapters)
    {
        if (m_shutdownRequested.loadAcquire() != 0)
            break;

        QString detail;
        if (!performMems19Wakeup(adapter.qtPortName, &detail))
            continue;

        if (m_shutdownRequested.loadAcquire() != 0)
            break;

        if (tryRoscoConnect(adapter.devicePath))
        {
            setEcuFamily(MemsEcuFamily::Mems19);
            setDiagnosticMode(MemsDiagnosticMode::Normal);
            emit serialInterfaceDetected(adapter.portName,
                                         adapter.adapterFamily,
                                         QStringLiteral("MEMS 1.9 K-Line"));
            return true;
        }
        if (m_initComplete && mems_is_connected(&m_memsinfo))
        {
            mems_disconnect(&m_memsinfo);
            m_connectedState.storeRelease(0);
            resetProtocolContext();
        }
    }

    m_deviceName = configuredDevice;
    m_connectedState.storeRelease(0);
    resetProtocolContext();
    return false;
}

void MEMSInterface::onStartPollingRequest()
{
    if (m_shutdownRequested.loadAcquire() != 0)
    {
        resetProtocolContext();
        QThread::currentThread()->quit();
        return;
    }

    // Bind once: F0 can recover/confirm the central diagnostic mode, while the
    // mapped Injection reader also publishes transition/Mode4/normal changes.
    if (!property("protocolContextTrackingBound").toBool())
    {
        setProperty("protocolContextTrackingBound", true);
        QObject::connect(this, &MEMSInterface::protocolResponse, this,
                         [this](quint8 command, const QByteArray &response) {
            if (command != quint8(0xF0) || response.isEmpty())
                return;
            for (int i = response.size() - 1; i >= 0; --i)
            {
                const quint8 value = quint8(static_cast<unsigned char>(response.at(i)));
                if (value == 0x50u)
                {
                    setDiagnosticMode(MemsDiagnosticMode::Normal);
                    return;
                }
                if (value == 0x14u)
                {
                    setDiagnosticMode(MemsDiagnosticMode::Mode3);
                    return;
                }
                if (value == 0x1Eu)
                {
                    setDiagnosticMode(MemsDiagnosticMode::Mode4);
                    return;
                }
            }
        }, Qt::DirectConnection);
        QObject::connect(this, &MEMSInterface::readModeChanged, this,
                         [this](const QString &mode) {
            if (mode == QStringLiteral("diagnostic"))
                setDiagnosticMode(MemsDiagnosticMode::Normal);
            else if (mode == QStringLiteral("injection"))
                setDiagnosticMode(MemsDiagnosticMode::Mode4);
            else if (mode == QStringLiteral("mode3"))
                setDiagnosticMode(MemsDiagnosticMode::Mode3);
            else if (mode == QStringLiteral("transition"))
                setDiagnosticMode(MemsDiagnosticMode::Transition);
        }, Qt::DirectConnection);
    }

    // Prevent a second queued connection request from entering while a slow
    // MEMS 1.9 initialisation or the service loop is already active.
    if (m_connectionAttemptActive || m_serviceLoopRunning)
        return;

    m_connectionAttemptActive = true;
    m_disconnectRequested.storeRelease(0);
    m_stopPolling = false;
    m_shutdownThread = false;
    resetProtocolContext();

    if (connectToECU())
    {
        if (m_shutdownRequested.loadAcquire() != 0)
        {
            if (m_initComplete && mems_is_connected(&m_memsinfo))
                mems_disconnect(&m_memsinfo);
            m_connectedState.storeRelease(0);
            resetProtocolContext();
            m_connectionAttemptActive = false;
            QThread::currentThread()->quit();
            return;
        }

        emit connected();
        runServiceLoop();
        resetProtocolContext();
        m_connectionAttemptActive = false;
        return;
    }

    m_connectionAttemptActive = false;
    resetProtocolContext();
    if (m_shutdownRequested.loadAcquire() != 0)
    {
        QThread::currentThread()->quit();
        return;
    }

#ifdef WIN32
    QString simpleDeviceName = m_deviceName;
    if (simpleDeviceName.indexOf(QStringLiteral("\\\\.\\")) == 0)
        simpleDeviceName.remove(0, 4);
    emit failedToConnect(simpleDeviceName);
#else
    emit failedToConnect(m_deviceName);
#endif
}