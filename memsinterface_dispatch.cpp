// MEMS protocol dispatcher: mode/family negotiation and guarded commands.
// The low-level transport is compiled independently from memsinterface.cpp.
#include "memsinterface.h"

#include <QElapsedTimer>
#include <QThread>
#include <QVariant>
#include <QSerialPort>
#include "serialadapterdetector.h"
#include "mems19protocol.h"

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
    // Les commandes historiques d'actionneurs exigent seulement une connexion ECU.
    // La recherche d'une reference dans une base n'est pas un prerequis.
    if (!m_initComplete || !mems_is_connected(&m_memsinfo))
        return false;
    return ::mems_test_actuator(&m_memsinfo, cmd, data);
}

bool MEMSInterface::guardedMemsMoveIac(uint8_t desiredPos)
{
    if (!m_initComplete || !mems_is_connected(&m_memsinfo))
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

    sendRoscoReadCommand(command);
}

bool MEMSInterface::tryRoscoConnect(const QString &devicePath)
{
    const auto cancelled=[this]() {
        return m_shutdownRequested.loadAcquire()!=0 || m_disconnectRequested.loadAcquire()!=0;
    };
    if (cancelled())
        return false;

    if (m_initComplete && mems_is_connected(&m_memsinfo))
    {
        mems_disconnect(&m_memsinfo);
        m_connectedState.storeRelease(0);
    }

    resetProtocolContext();
    m_deviceName = devicePath;
    const bool connected=openRoscoTransport();
    if (connected && cancelled())
    {
        if (m_initComplete && mems_is_connected(&m_memsinfo))
            mems_disconnect(&m_memsinfo);
        m_connectedState.storeRelease(0);
        resetProtocolContext();
        return false;
    }
    return connected;
}

bool MEMSInterface::performMems19Wakeup(const QString &devicePath, QString *detail)
{
#ifdef MEMS_NATIVE_KLINE_19
    if (detail) detail->clear();
    m_kline19LocalEcho = false;
    const auto cancelled=[this]() {
        return m_shutdownRequested.loadAcquire()!=0 || m_disconnectRequested.loadAcquire()!=0;
    };
    if (cancelled() || !m_initComplete)
    {
        if (detail) *detail = QStringLiteral("connection-cancelled");
        return false;
    }

    // Open once via librosco, and retain this SAME handle after ISO 9141.
    // Closing/reopening the COM port between E9 and CA can reset the K-Line
    // interface and discard the ECU's newly initialized diagnostic session.
    if (!mems_connect(&m_memsinfo, devicePath.toLocal8Bit().constData()))
    {
        if (detail) *detail = QStringLiteral("port-open-failed");
        return false;
    }
    const HANDLE handle = m_memsinfo.sd;
    const auto fail=[this, handle, detail](const QString &reason) -> bool {
        ClearCommBreak(handle);
        mems_disconnect(&m_memsinfo);
        m_connectedState.storeRelease(0);
        if (detail) *detail = reason;
        return false;
    };

    PurgeComm(handle, PURGE_RXCLEAR | PURGE_TXCLEAR);
    if (!ClearCommBreak(handle))
        return fail(QStringLiteral("break-control-unavailable"));

    // RoverMEMS 1.9: hold the bus idle for 2 s, then send 0x16 at
    // five baud (start, LSB-first data, stop; 200 ms per bit).
    QThread::msleep(2000);
    if (cancelled())
        return fail(QStringLiteral("connection-cancelled"));

    QElapsedTimer bitClock;
    bitClock.start();
    for (int bit = 0; bit < Mems19Protocol::kSlowBitCount; ++bit)
    {
        if (cancelled())
            return fail(QStringLiteral("connection-cancelled"));
        const BOOL changed = Mems19Protocol::slowBitHigh(bit)
            ? ClearCommBreak(handle) : SetCommBreak(handle);
        if (!changed)
            return fail(QStringLiteral("slow-init-break-control-failed"));

        const qint64 deadline =
            (bit + 1) * qint64(Mems19Protocol::kSlowBitDurationMs);
        const qint64 remaining = deadline - bitClock.elapsed();
        if (remaining > 0)
            QThread::msleep(static_cast<unsigned long>(remaining));
    }
    if (!ClearCommBreak(handle))
        return fail(QStringLiteral("break-clear-failed"));

    // Read sync 55 + two key bytes, accepting partial USB delivery.
    QByteArray reply;
    QElapsedTimer timer;
    timer.start();
    quint8 invertedKey = 0;
    bool gotKeys = false;
    while (timer.elapsed() < 2000 && !gotKeys && !cancelled())
    {
        unsigned char byte = 0;
        DWORD count = 0;
        if (ReadFile(handle, &byte, 1, &count, NULL) && count == 1)
        {
            reply.append(char(byte));
            gotKeys = Mems19Protocol::keyComplement(reply, &invertedKey);
        }
    }
    if (cancelled())
        return fail(QStringLiteral("connection-cancelled"));
    if (!gotKeys)
        return fail(QStringLiteral("no-iso9141-key-bytes"));

    QThread::msleep(50);
    const unsigned char ack = invertedKey;
    DWORD written = 0;
    if (!WriteFile(handle, &ack, 1, &written, NULL) || written != 1)
        return fail(QStringLiteral("key-ack-write-failed"));

    QByteArray confirmation;
    timer.restart();
    bool confirmed = false;
    while (timer.elapsed() < 1600 && !confirmed && !cancelled())
    {
        unsigned char byte = 0;
        DWORD count = 0;
        if (ReadFile(handle, &byte, 1, &count, NULL) && count == 1)
        {
            confirmation.append(char(byte));
            confirmed = Mems19Protocol::hasAddressConfirmation(confirmation);
        }
    }
    if (cancelled())
        return fail(QStringLiteral("connection-cancelled"));
    if (!confirmed)
        return fail(QStringLiteral("address-confirmation-missing"));

    // If 0x7C is echoed before ECU confirmation E9, this KKL transceiver
    // echoes local transmissions; every later diagnostic command will yield
    // this extra byte before the ECU reply.
    m_kline19LocalEcho = confirmation.contains(char(ack));
    if (detail)
        *detail = m_kline19LocalEcho
            ? QStringLiteral("iso9141-mems19-wakeup-ok-echo")
            : QStringLiteral("iso9141-mems19-wakeup-ok");
    return true;
#else
    // Compatibility fallback for builds using an external legacy DLL.
    if (detail) detail->clear();

    const auto cancelled=[this]() {
        return m_shutdownRequested.loadAcquire()!=0 || m_disconnectRequested.loadAcquire()!=0;
    };
    const auto cancelReason=[this]() {
        return m_shutdownRequested.loadAcquire()!=0
            ? QStringLiteral("shutdown-requested")
            : QStringLiteral("disconnect-requested");
    };

    if (cancelled())
    {
        if (detail) *detail = cancelReason();
        return false;
    }

    QSerialPort port;
    port.setPortName(SerialAdapterDetector::stripWindowsDevicePrefix(devicePath));
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
    if (cancelled())
    {
        if (detail) *detail = cancelReason();
        port.close();
        return false;
    }
    // Schedule all ten slow-init bits from one monotonic clock. Sleeping
    // 200 ms independently after each USB BREAK call adds driver overhead
    // to every bit and can cause cumulative timing drift on Windows.
    QElapsedTimer bitClock;
    bitClock.start();
    const auto sendSlowBit=[&port, &cancelled, &bitClock](int index) -> bool {
        if (cancelled())
            return false;
        if (!port.setBreakEnabled(!Mems19Protocol::slowBitHigh(index)))
            return false;
        const qint64 target =
            (index + 1) * qint64(Mems19Protocol::kSlowBitDurationMs);
        const qint64 remaining = target - bitClock.elapsed();
        if (remaining > 0)
            QThread::msleep(static_cast<unsigned long>(remaining));
        return !cancelled();
    };

    for (int index = 0; index < Mems19Protocol::kSlowBitCount; ++index)
    {
        if (sendSlowBit(index))
            continue;
        if (detail)
            *detail = cancelled() ? cancelReason() :
                QStringLiteral("slow-init-break-control-failed");
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
    quint8 complementedKey = 0;
    bool receivedKeys = false;
    while (timer.elapsed() < 1600 && !receivedKeys && !cancelled())
    {
        if (port.waitForReadyRead(120))
            wakeReply += port.readAll();
        else
            wakeReply += port.readAll();
        receivedKeys = Mems19Protocol::keyComplement(wakeReply,
                                                    &complementedKey);
    }

    if (cancelled())
    {
        if (detail) *detail = cancelReason();
        port.close();
        return false;
    }

    if (!receivedKeys)
    {
        if (detail) *detail = QStringLiteral("no-iso9141-key-bytes");
        port.close();
        return false;
    }

    const char invertedKey = char(complementedKey);
    if (cancelled())
    {
        if (detail) *detail = cancelReason();
        port.close();
        return false;
    }
    if (port.write(&invertedKey, 1) != 1 || !port.waitForBytesWritten(400))
    {
        if (detail) *detail = cancelled() ? cancelReason() : QStringLiteral("key-ack-write-failed");
        port.close();
        return false;
    }

    // K-line echoes our own byte. The ECU then returns the complement of
    // address 0x16, i.e. 0xE9. Search the received stream rather than assuming
    // a fixed position so interfaces with or without echo are both tolerated.
    QByteArray confirmation;
    timer.restart();
    bool confirmed = false;
    while (timer.elapsed() < 1200 && !confirmed && !cancelled())
    {
        if (port.waitForReadyRead(100))
            confirmation += port.readAll();
        else
            confirmation += port.readAll();

        confirmed = Mems19Protocol::hasAddressConfirmation(confirmation);
    }

    port.close();
    if (cancelled())
    {
        if (detail) *detail = cancelReason();
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
#endif
}

bool MEMSInterface::connectToECU()
{
    const auto cancelled=[this]() {
        return m_shutdownRequested.loadAcquire()!=0 || m_disconnectRequested.loadAcquire()!=0;
    };
    if (cancelled())
        return false;

    const QString configuredDevice = m_deviceName;
    const QList<DetectedSerialAdapter> adapters =
        SerialAdapterDetector::availableAdapters(configuredDevice);

    const auto resetFailedTransport=[this]() {
        if (m_initComplete && mems_is_connected(&m_memsinfo))
            mems_disconnect(&m_memsinfo);
        m_connectedState.storeRelease(0);
        resetProtocolContext();
    };

    // Do not change the historical ROSCO handshake or its ECU data path.
    const auto connectRosco=[this, &cancelled, &resetFailedTransport](
        const DetectedSerialAdapter &adapter) -> bool {
        if (!tryRoscoConnect(adapter.devicePath))
        {
            resetFailedTransport();
            return false;
        }
        if (cancelled())
        {
            resetFailedTransport();
            return false;
        }

        // A ROSCO handshake alone does not identify the exact ECU generation.
        setEcuFamily(MemsEcuFamily::Unknown);
        setDiagnosticMode(MemsDiagnosticMode::Normal);
        emit serialInterfaceDetected(adapter.portName,
                                     adapter.adapterFamily,
                                     QStringLiteral("ROSCO 1.3/1.6"));
        return true;
    };

    const auto connectMems19=[this, &cancelled, &resetFailedTransport](
        const DetectedSerialAdapter &adapter) -> bool {
        QString detail;
        if (!performMems19Wakeup(adapter.devicePath, &detail))
        {
            emit mems19WakeupDiagnostic(adapter.portName, detail);
            return false;
        }
        emit mems19WakeupDiagnostic(adapter.portName, detail);
        if (cancelled())
            return false;
#ifdef MEMS_NATIVE_KLINE_19
        // K-Line uses the same ECU command family but has a second (local)
        // echo on some adapters. Keep the port opened by the slow wake-up.
        if (!mems_init_link_kline19(&m_memsinfo, m_d0_response_buffer,
                                     m_kline19LocalEcho))
        {
            emit mems19WakeupDiagnostic(adapter.portName,
                                       QStringLiteral("kline-ecu-init-failed"));
            resetFailedTransport();
            return false;
        }
        m_connectedState.storeRelease(1);
        m_deviceName = adapter.devicePath;
        emit gotEcuId(m_d0_response_buffer);
#else
        if (!tryRoscoConnect(adapter.devicePath))
        {
            emit mems19WakeupDiagnostic(adapter.portName,
                                       QStringLiteral("rosco-init-after-wakeup-failed"));
            resetFailedTransport();
            return false;
        }
#endif
        if (cancelled())
        {
            resetFailedTransport();
            return false;
        }
        emit mems19WakeupDiagnostic(adapter.portName,
                                   QStringLiteral("iso9141-and-ecu-1x-link-ok"));

        setEcuFamily(MemsEcuFamily::Mems19);
        setDiagnosticMode(MemsDiagnosticMode::Normal);
        emit serialInterfaceDetected(adapter.portName,
                                     adapter.adapterFamily,
                                     QStringLiteral("MEMS 1.9 K-Line"));
        return true;
    };

    // Choose the first protocol attempt per physical USB cable rather than
    // trying ROSCO on every port before attempting MEMS 1.9 on any port.
    // VID:PID 1A86:7523 corresponds to the user's known CH340 KKL cable.
    // USB chip identifiers alone are not a universal connector identity:
    // keep the other protocol as a fallback, not a new compatibility lock.
    // The user's FTDI FT232 ROSCO cable keeps ROSCO as its first attempt.
    for (const DetectedSerialAdapter &adapter : adapters)
    {
        if (cancelled())
            break;

        const bool preferKLine = SerialAdapterDetector::prefersKLineFirst(adapter);

        if (preferKLine)
        {
            if (connectMems19(adapter))
                return true;
            if (cancelled())
                break;
            if (connectRosco(adapter))
                return true;
        }
        else
        {
            if (connectRosco(adapter))
                return true;
            if (cancelled())
                break;
            if (connectMems19(adapter))
                return true;
        }
    }

    m_deviceName = configuredDevice;
    m_connectedState.storeRelease(0);
    resetProtocolContext();
    return false;
}

// Per-instance tracking is connected exactly once during construction.
void MEMSInterface::bindProtocolContextTracking()
{
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

void MEMSInterface::onStartPollingRequest()
{
    if (m_shutdownRequested.loadAcquire() != 0)
    {
        resetProtocolContext();
        QThread::currentThread()->quit();
        return;
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
        const bool shutdown=m_shutdownRequested.loadAcquire()!=0;
        const bool disconnect=m_disconnectRequested.loadAcquire()!=0;
        if (shutdown || disconnect)
        {
            if (m_initComplete && mems_is_connected(&m_memsinfo))
                mems_disconnect(&m_memsinfo);
            m_connectedState.storeRelease(0);
            resetProtocolContext();
            m_connectionAttemptActive = false;
            if (shutdown)
                QThread::currentThread()->quit();
            else
            {
                m_disconnectRequested.storeRelease(0);
                emit disconnected();
            }
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
    if (m_disconnectRequested.loadAcquire() != 0)
    {
        m_connectedState.storeRelease(0);
        m_disconnectRequested.storeRelease(0);
        emit disconnected();
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
