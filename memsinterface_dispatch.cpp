// MEMS protocol dispatcher: mode/family negotiation and guarded commands.
// The low-level transport is compiled independently from memsinterface.cpp.
#include "memsinterface.h"
#include "ecucommunicationtraceio.h"

#include <QElapsedTimer>
#include <QThread>
#include <QVariant>
#include <QSerialPort>
#include "serialadapterdetector.h"
#include "mems19protocol.h"
#include "mems2jprotocol.h"
#include "mems3protocol.h"

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
    // An earlier actuator command might have left a forced output active.
    // Be conservative even on a missing acknowledgement; only an acknowledged
    // general cancellation clears the marker. Adjustment commands are excluded.
    switch(quint8(cmd)){
    case 0x11:case 0x01:case 0x12:case 0x02:case 0x13:case 0x03:
    case 0x18:case 0x08:case 0x19:case 0x09:case 0x1b:case 0x0b:
    case 0x1d:case 0x0d:case 0x1e:case 0x0e:case 0x6f:case 0x67:
    case 0xf7:case 0xef:case 0xf8:case 0xfd:case 0xfe:
        m_instrumentOtherForced.storeRelease(1);break;
    default:break;
    }
    uint8_t resetReply=0xff;
    uint8_t *reply=(quint8(cmd)==0xf4&&!data)?&resetReply:data;
    bool ok;
#ifdef MEMS_NATIVE_KLINE_19
    if (ecuFamily() == MemsEcuFamily::Mems19)
        ok=::mems_command_kline19(&m_memsinfo,quint8(cmd),m_kline19LocalEcho,reply);
    else
#endif
        ok=::mems_test_actuator(&m_memsinfo,cmd,reply);
    if(quint8(cmd)==0xf4&&ok&&reply&&*reply==0&&diagnosticMode()==MemsDiagnosticMode::Normal)
        m_instrumentOtherForced.storeRelease(0);
    return ok;
}

bool MEMSInterface::guardedMemsMoveIac(uint8_t desiredPos)
{
    if (!m_initComplete || !mems_is_connected(&m_memsinfo))
        return false;
    m_instrumentOtherForced.storeRelease(1);
#ifdef MEMS_NATIVE_KLINE_19
    if (ecuFamily() == MemsEcuFamily::Mems19)
        return ::mems_move_iac_kline19(&m_memsinfo, desiredPos,
                                        m_kline19LocalEcho);
#endif
    return ::mems_move_iac(&m_memsinfo, desiredPos);
}

bool MEMSInterface::guardedClearFaults()
{
    if (property("injectionRamTestRunning").toBool() ||
        property("mappedInjectionModeActive").toBool())
        return false;
    const auto cancelled=[this]() {
        return m_shutdownRequested.loadAcquire()!=0 ||
               m_disconnectRequested.loadAcquire()!=0;
    };
    // KWP-like MEMS 2J and MEMS 3 have their own framed clear-DTC service,
    // never pass them to the one-byte historical ROSCO implementation.
    if (ecuFamily()==MemsEcuFamily::Mems2J)
        return m_mems2j && m_mems2j->clearFaults(cancelled);
    if (ecuFamily()==MemsEcuFamily::Mems3)
        return m_mems3 && m_mems3->clearFaults(cancelled);
    if (!MemsProtocolSafety::allowsMutation(ecuFamily(), diagnosticMode(), 0xCCu))
        return false;
    #ifdef MEMS_NATIVE_KLINE_19
    if (ecuFamily() == MemsEcuFamily::Mems19)
        return ::mems_command_kline19(&m_memsinfo, 0xCCu,
                                       m_kline19LocalEcho, nullptr);
#endif
    return ::mems_clear_faults(&m_memsinfo);
}

bool MEMSInterface::guardedResetAdjustments()
{
    if (property("injectionRamTestRunning").toBool() ||
        property("mappedInjectionModeActive").toBool())
        return false;
    if (!MemsProtocolSafety::allowsMutation(ecuFamily(), diagnosticMode(), 0x0Fu))
        return false;
    #ifdef MEMS_NATIVE_KLINE_19
    if (ecuFamily() == MemsEcuFamily::Mems19)
        return ::mems_command_kline19(&m_memsinfo, 0x0Fu,
                                       m_kline19LocalEcho, nullptr);
#endif
    return ::mems_reset_adjustments(&m_memsinfo);
}

bool MEMSInterface::guardedResetEcu()
{
    if (property("injectionRamTestRunning").toBool() ||
        property("mappedInjectionModeActive").toBool())
        return false;
    if (!MemsProtocolSafety::allowsMutation(ecuFamily(), diagnosticMode(), 0xFAu))
        return false;
    #ifdef MEMS_NATIVE_KLINE_19
    if (ecuFamily() == MemsEcuFamily::Mems19)
        return ::mems_command_kline19(&m_memsinfo, 0xFAu,
                                       m_kline19LocalEcho, nullptr);
#endif
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

    { const uint8_t addr=0x16; ecu_trace_bytes("TX-5BAUD",&addr,1); }
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
        if (ecu_trace_read_file(handle, &byte, 1, &count, NULL) && count == 1)
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
    if (!ecu_trace_write_file(handle, &ack, 1, &written, NULL) || written != 1)
        return fail(QStringLiteral("key-ack-write-failed"));

    QByteArray confirmation;
    timer.restart();
    bool confirmed = false;
    while (timer.elapsed() < 1600 && !confirmed && !cancelled())
    {
        unsigned char byte = 0;
        DWORD count = 0;
        if (ecu_trace_read_file(handle, &byte, 1, &count, NULL) && count == 1)
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
    { const uint8_t addr=0x16; ecu_trace_bytes("TX-5BAUD",&addr,1); }

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
            { const QByteArray rx=port.readAll(); ecu_trace_bytes("RX",(const uint8_t*)rx.constData(),(size_t)rx.size()); wakeReply += rx; }
        else
            { const QByteArray rx=port.readAll(); ecu_trace_bytes("RX",(const uint8_t*)rx.constData(),(size_t)rx.size()); wakeReply += rx; }
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
    const qint64 keyWritten=port.write(&invertedKey,1);
    if(keyWritten>0)ecu_trace_bytes("TX",(const uint8_t*)&invertedKey,(size_t)keyWritten);
    if(keyWritten!=1 || !port.waitForBytesWritten(400))
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
            { const QByteArray rx=port.readAll(); ecu_trace_bytes("RX",(const uint8_t*)rx.constData(),(size_t)rx.size()); confirmation += rx; }
        else
            { const QByteArray rx=port.readAll(); ecu_trace_bytes("RX",(const uint8_t*)rx.constData(),(size_t)rx.size()); confirmation += rx; }

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
        if (m_mems2j)
        {
            m_mems2j->close();
            delete m_mems2j;
            m_mems2j=nullptr;
        }
        if (m_mems3)
        {
            m_mems3->close();
            delete m_mems3;
            m_mems3=nullptr;
        }
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

        // The successful CA/75/F4/D0 dialogue proves the legacy ROSCO
        // protocol family, even when its exact 1.2/1.3/1.6 subtype is unknown.
        // Keeping Unknown here made existing fault/reset buttons unusable.
        setEcuFamily(MemsEcuFamily::Rosco13_16);
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
        m_naSession.reset();
        m_naSession.tx({0xD0});
        m_naSession.rx({0xD0,m_d0_response_buffer[0],m_d0_response_buffer[1],m_d0_response_buffer[2],m_d0_response_buffer[3]});
        m_naSession.finish();
        { QMutexLocker locker(&m_dataMutex); m_naSnapshot.reset(); }
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

    // 2J is an independent K-Line/KWP2000 session, identified by a real ECU
    // diagnostic dialogue and a valid 21 09 reply; never by USB VID/PID.
    const auto connectMems2J=[this, &cancelled](
        const DetectedSerialAdapter &adapter) -> bool {
        Mems2JProtocol::Session *session = new Mems2JProtocol::Session;
        if (!session->connectTo(adapter.qtPortName, cancelled) || cancelled())
        {
            session->close();
            delete session;
            return false;
        }
        m_mems2j=session;
        m_deviceName=adapter.devicePath;
        {
            QMutexLocker locker(&m_dataMutex);
            m_mems2jValues=session->values();
            m_mems2jFaults=session->faults();
        }
        m_connectedState.storeRelease(1);
        setEcuFamily(MemsEcuFamily::Mems2J);
        setDiagnosticMode(MemsDiagnosticMode::Normal);
        emit ecuIdentityDetected(QStringLiteral("MEMS 2J"));
        emit serialInterfaceDetected(adapter.portName,
                                     adapter.adapterFamily,
                                     QStringLiteral("MEMS 2J K-Line"));
        return true;
    };

    // MEMS 3 is not MEMS 2J: 9600 baud, 8E1 and B8/13/F7 XOR packets.
    // Only its acknowledged diagnostic handshake identifies this family.
    const auto connectMems3=[this, &cancelled](
        const DetectedSerialAdapter &adapter) -> bool {
        Mems3Protocol::Session *session=new Mems3Protocol::Session;
        if (!session->connectTo(adapter.qtPortName,cancelled) || cancelled())
        {
            session->close();
            delete session;
            return false;
        }
        m_mems3=session;
        m_deviceName=adapter.devicePath;
        {
            QMutexLocker locker(&m_dataMutex);
            m_mems2jValues=session->values();
            m_mems2jFaults=session->faults();
        }
        m_connectedState.storeRelease(1);
        setEcuFamily(MemsEcuFamily::Mems3);
        setDiagnosticMode(MemsDiagnosticMode::Normal);
        emit ecuIdentityDetected(QStringLiteral("MEMS 3"));
        emit serialInterfaceDetected(adapter.portName,
                                     adapter.adapterFamily,
                                     QStringLiteral("MEMS 3 K-Line"));
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
            if (connectMems2J(adapter))
                return true;
            if (cancelled())
                break;
            if (connectMems3(adapter))
                return true;
            if (cancelled())
                break;
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
            if (connectMems2J(adapter))
                return true;
            if (cancelled())
                break;
            if (connectMems3(adapter))
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

    // Cancellation can arrive before the queued request reaches this thread.
    // Only the GUI's accepted new request may clear that cancellation flag.
    if(m_disconnectRequested.loadAcquire()!=0) {
        m_disconnectRequested.storeRelease(0);
        emit disconnected();
        return;
    }
    m_connectionAttemptActive = true;
    m_stopPolling = false;
    m_shutdownThread = false;
    resetProtocolContext();

    if (connectToECU())
    {
        const bool shutdown=m_shutdownRequested.loadAcquire()!=0;
        const bool disconnect=m_disconnectRequested.loadAcquire()!=0;
        if (shutdown || disconnect)
        {
            if (m_mems2j)
            {
                m_mems2j->close();
                delete m_mems2j;
                m_mems2j=nullptr;
            }
            if (m_mems3)
            {
                m_mems3->close();
                delete m_mems3;
                m_mems3=nullptr;
            }
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
