#ifndef SERIALADAPTERDETECTOR_H
#define SERIALADAPTERDETECTOR_H

#include <QList>
#include <QSerialPortInfo>
#include <QString>

struct DetectedSerialAdapter
{
    QString portName;       // COM5 / ttyUSB0
    QString qtPortName;     // name accepted by QSerialPort
    QString devicePath;     // path accepted by librosco
    QString adapterFamily;  // FTDI / Prolific / CH340 / CP210x / generic
    QString description;
    QString manufacturer;
    quint16 vendorId = 0;
    quint16 productId = 0;
    bool hasVendorId = false;
    bool hasProductId = false;

    QString displayName() const
    {
        QString text = portName;
        if (!adapterFamily.isEmpty())
            text += QStringLiteral(" — ") + adapterFamily;
        if (!description.isEmpty() && !text.contains(description, Qt::CaseInsensitive))
            text += QStringLiteral(" — ") + description;
        return text;
    }
};

namespace SerialAdapterDetector
{
inline QString stripWindowsDevicePrefix(QString name)
{
    name = name.trimmed();
    if (name.startsWith(QStringLiteral("\\\\.\\")))
        name.remove(0, 4);
    return name;
}

inline QString libroscoDevicePath(const QString &portName, const QString &systemLocation)
{
#ifdef Q_OS_WIN
    Q_UNUSED(systemLocation);
    return QStringLiteral("\\\\.\\") + stripWindowsDevicePrefix(portName);
#else
    return systemLocation.isEmpty() ? portName : systemLocation;
#endif
}

inline QString adapterFamily(const QSerialPortInfo &info)
{
    const QString haystack = (info.manufacturer() + QLatin1Char(' ') + info.description()).toUpper();
    const quint16 vid = info.hasVendorIdentifier() ? info.vendorIdentifier() : 0;

    if (vid == 0x0403u || haystack.contains(QStringLiteral("FTDI")) || haystack.contains(QStringLiteral("FT232")))
        return QStringLiteral("FTDI FT232");
    if (vid == 0x067Bu || haystack.contains(QStringLiteral("PROLIFIC")) || haystack.contains(QStringLiteral("PL2303")))
        return QStringLiteral("Prolific PL2303");
    if (vid == 0x1A86u || haystack.contains(QStringLiteral("CH340")) || haystack.contains(QStringLiteral("CH341")) || haystack.contains(QStringLiteral("WCH")))
        return QStringLiteral("WCH CH340/CH341");
    if (vid == 0x10C4u || haystack.contains(QStringLiteral("CP210")) || haystack.contains(QStringLiteral("SILICON LABS")))
        return QStringLiteral("Silicon Labs CP210x");
    if (info.hasVendorIdentifier())
        return QStringLiteral("USB série");
    return QStringLiteral("Port série");
}

inline QList<DetectedSerialAdapter> availableAdapters(const QString &preferredDevice = QString())
{
    QList<DetectedSerialAdapter> result;
    const QString preferredPort = stripWindowsDevicePrefix(preferredDevice);

    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports)
    {
        DetectedSerialAdapter item;
        item.portName = info.portName();
        item.qtPortName = info.portName();
        item.devicePath = libroscoDevicePath(info.portName(), info.systemLocation());
        item.adapterFamily = adapterFamily(info);
        item.description = info.description().trimmed();
        item.manufacturer = info.manufacturer().trimmed();
        item.hasVendorId = info.hasVendorIdentifier();
        item.hasProductId = info.hasProductIdentifier();
        if (item.hasVendorId) item.vendorId = info.vendorIdentifier();
        if (item.hasProductId) item.productId = info.productIdentifier();
        result.append(item);
    }

    // Keep the user's previously selected/manual port first when it still exists.
    if (!preferredPort.isEmpty())
    {
        for (int i = 0; i < result.size(); ++i)
        {
            if (result.at(i).portName.compare(preferredPort, Qt::CaseInsensitive) == 0 ||
                result.at(i).devicePath.compare(preferredDevice, Qt::CaseInsensitive) == 0)
            {
                if (i != 0) result.move(i, 0);
                return result;
            }
        }

        // A manually entered virtual/legacy COM port may not expose USB metadata.
        DetectedSerialAdapter manual;
        manual.portName = preferredPort;
        manual.qtPortName = preferredPort;
        manual.devicePath = libroscoDevicePath(preferredPort, QString());
        manual.adapterFamily = QStringLiteral("Port configuré");
        result.prepend(manual);
    }

    return result;
}
}

#endif // SERIALADAPTERDETECTOR_H
