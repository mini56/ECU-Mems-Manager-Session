// Connection dispatcher layered around the historical MEMSInterface implementation.
// The existing 1.3/1.6 code remains byte-for-byte in memsinterface.cpp; only the two
// connection entry points are renamed while this translation unit includes it.
#include "memsinterface.h"

#define connectToECU connectToECULegacy
#define onStartPollingRequest onStartPollingRequestLegacy
#include "memsinterface.cpp"
#undef onStartPollingRequest
#undef connectToECU

#include <QElapsedTimer>
#include <QSerialPort>
#include "serialadapterdetector.h"

bool MEMSInterface::tryRoscoConnect(const QString &devicePath)
{
    if (m_initComplete && mems_is_connected(&m_memsinfo))
        mems_disconnect(&m_memsinfo);

    m_deviceName = devicePath;
    return connectToECULegacy();
}

bool MEMSInterface::performMems19Wakeup(const QString &qtPortName, QString *detail)
{
    if (detail) detail->clear();

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

    auto sendSlowBit = [&port](bool high) -> bool {
        // BREAK = active low, therefore a logical high means BREAK disabled.
        if (!port.setBreakEnabled(!high))
            return false;
        QThread::msleep(200);
        return true;
    };

    if (!sendSlowBit(false))
    {
        if (detail) *detail = QStringLiteral("slow-init-start-failed");
        port.setBreakEnabled(false);
        port.close();
        return false;
    }

    for (int bit = 0; bit < 8; ++bit)
    {
        const bool high = ((address >> bit) & 0x01u) != 0u;
        if (!sendSlowBit(high))
        {
            if (detail) *detail = QStringLiteral("slow-init-data-failed");
            port.setBreakEnabled(false);
            port.close();
            return false;
        }
    }

    if (!sendSlowBit(true))
    {
        if (detail) *detail = QStringLiteral("slow-init-stop-failed");
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
    while (timer.elapsed() < 1600 && syncIndex < 0)
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
    while (timer.elapsed() < 1200 && !confirmed)
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
    const QString configuredDevice = m_deviceName;
    const QList<DetectedSerialAdapter> adapters =
        SerialAdapterDetector::availableAdapters(configuredDevice);

    // First use the historical ROSCO path on every candidate. This preserves
    // MEMS 1.3/1.6 behaviour and also fixes the common "wrong COM port" case:
    // the configured port is tried first, then the other detected serial ports.
    for (const DetectedSerialAdapter &adapter : adapters)
    {
        if (tryRoscoConnect(adapter.devicePath))
        {
            emit serialInterfaceDetected(adapter.portName,
                                         adapter.adapterFamily,
                                         QStringLiteral("ROSCO 1.3/1.6"));
            return true;
        }
        if (m_initComplete && mems_is_connected(&m_memsinfo))
            mems_disconnect(&m_memsinfo);
    }

    // Nothing answered the normal path: try the MEMS 1.9 ISO-9141 slow wake-up.
    // Only after a valid 0x55/key-byte handshake and 0xE9 address confirmation
    // do we hand the already-awake ECU to the existing 9600-baud ROSCO init.
    for (const DetectedSerialAdapter &adapter : adapters)
    {
        QString detail;
        if (!performMems19Wakeup(adapter.qtPortName, &detail))
            continue;

        if (tryRoscoConnect(adapter.devicePath))
        {
            emit serialInterfaceDetected(adapter.portName,
                                         adapter.adapterFamily,
                                         QStringLiteral("MEMS 1.9 K-Line"));
            return true;
        }
        if (m_initComplete && mems_is_connected(&m_memsinfo))
            mems_disconnect(&m_memsinfo);
    }

    m_deviceName = configuredDevice;
    return false;
}

void MEMSInterface::onStartPollingRequest()
{
    if (connectToECU())
    {
        emit connected();
        m_stopPolling = false;
        m_shutdownThread = false;
        runServiceLoop();
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
