#include <QStringList>
#include <QSerialPortInfo>
#include <QApplication>

#include "serialdevenumerator.h"
#include "splashprogress.h"

SerialDevEnumerator::SerialDevEnumerator()
{
}

QStringList SerialDevEnumerator::getSerialDevList(QString savedDevName)
{
  QStringList detected;
  const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();

  const int total = qMax(1, ports.size());
  for (int i = 0; i < ports.size(); ++i)
  {
    const QSerialPortInfo &info = ports.at(i);
#ifdef Q_OS_WIN
    const QString name = info.portName().trimmed();
#else
    const QString name = info.systemLocation().trimmed();
#endif
    if (!name.isEmpty())
      detected.append(name);

    if (g_splashProgressCallback)
      g_splashProgressCallback(((i + 1) * 100) / total);
    if (qApp)
      qApp->processEvents();
  }

  detected.removeDuplicates();
  detected.sort(Qt::CaseInsensitive);

  QStringList serialDevices;
  const int savedIndex = detected.indexOf(savedDevName);
  if (savedIndex >= 0)
  {
    // Keep the user's currently valid choice first, but never stop the scan:
    // every actually available port remains visible in the options dialog.
    serialDevices.append(savedDevName);
    detected.removeAt(savedIndex);
  }
  serialDevices.append(detected);

  // Do not re-inject a stale saved COM port: that was the source of cases
  // where the application started on COM4 while the USB cable had moved to COM3.
  if (serialDevices.isEmpty())
    serialDevices.append(QString());

  if (g_splashProgressCallback)
    g_splashProgressCallback(100);

  return serialDevices;
}
