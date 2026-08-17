#include <QSettings>
#include <QSerialPortInfo>
#include <QRegularExpression>

#include "optionsdialog.h"
#include "serialdevenumerator.h"
#include "desktopshortcut.h"
#include <QIcon>
#include "i18n.h"
#define tr I18n::text

namespace {

QString serialPortName(const QSerialPortInfo &info)
{
#ifdef Q_OS_WIN
  return info.portName().trimmed();
#else
  return info.systemLocation().trimmed();
#endif
}

bool looksLikeUsbSerial(const QSerialPortInfo &info)
{
  if (info.hasVendorIdentifier() || info.hasProductIdentifier())
    return true;

  const QString identity = (info.description() + QLatin1Char(' ') + info.manufacturer()).toLower();
  static const QRegularExpression usbSerial(
      QStringLiteral("usb|ftdi|ch340|ch341|cp210|prolific|serial|uart"),
      QRegularExpression::CaseInsensitiveOption);
  return usbSerial.match(identity).hasMatch();
}

void saveSerialIdentity(QSettings &settings, const QString &portName)
{
  settings.remove(QStringLiteral("SerialDeviceSerialNumber"));
  settings.remove(QStringLiteral("SerialDeviceVendorId"));
  settings.remove(QStringLiteral("SerialDeviceProductId"));

  for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts())
  {
    if (serialPortName(info).compare(portName, Qt::CaseInsensitive) != 0)
      continue;

    if (!info.serialNumber().trimmed().isEmpty())
      settings.setValue(QStringLiteral("SerialDeviceSerialNumber"), info.serialNumber().trimmed());
    if (info.hasVendorIdentifier())
      settings.setValue(QStringLiteral("SerialDeviceVendorId"), int(info.vendorIdentifier()));
    if (info.hasProductIdentifier())
      settings.setValue(QStringLiteral("SerialDeviceProductId"), int(info.productIdentifier()));
    return;
  }
}

QString resolveSerialDevice(QSettings &settings, const QString &savedPort)
{
  const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
  if (ports.isEmpty())
    return QString();

  const QString savedSerial = settings.value(QStringLiteral("SerialDeviceSerialNumber")).toString().trimmed();
  const bool hasSavedVid = settings.contains(QStringLiteral("SerialDeviceVendorId"));
  const bool hasSavedPid = settings.contains(QStringLiteral("SerialDeviceProductId"));
  const quint16 savedVid = quint16(settings.value(QStringLiteral("SerialDeviceVendorId"), 0).toUInt());
  const quint16 savedPid = quint16(settings.value(QStringLiteral("SerialDeviceProductId"), 0).toUInt());

  // First follow the physical adapter, not its COM number. Windows can assign
  // another COM number after a USB-port/driver change.
  if (!savedSerial.isEmpty())
  {
    for (const QSerialPortInfo &info : ports)
      if (!info.serialNumber().isEmpty() && info.serialNumber() == savedSerial)
        return serialPortName(info);
  }

  if (hasSavedVid && hasSavedPid)
  {
    QList<QSerialPortInfo> hardwareMatches;
    for (const QSerialPortInfo &info : ports)
      if (info.hasVendorIdentifier() && info.hasProductIdentifier() &&
          info.vendorIdentifier() == savedVid && info.productIdentifier() == savedPid)
        hardwareMatches.append(info);
    if (hardwareMatches.size() == 1)
      return serialPortName(hardwareMatches.first());
  }

  const QSerialPortInfo *savedInfo = nullptr;
  for (const QSerialPortInfo &info : ports)
  {
    if (serialPortName(info).compare(savedPort, Qt::CaseInsensitive) == 0)
    {
      savedInfo = &info;
      break;
    }
  }

  QList<QSerialPortInfo> usbCandidates;
  for (const QSerialPortInfo &info : ports)
    if (looksLikeUsbSerial(info))
      usbCandidates.append(info);

  if (savedInfo)
  {
    // A legacy configuration only stored "COMx". If that COM now names a
    // non-USB port while exactly one USB-serial adapter is present, the USB
    // adapter is the unambiguous diagnostic candidate.
    if (savedSerial.isEmpty() && !hasSavedVid && !hasSavedPid &&
        !looksLikeUsbSerial(*savedInfo) && usbCandidates.size() == 1)
      return serialPortName(usbCandidates.first());
    return serialPortName(*savedInfo);
  }

  // Never keep displaying or opening a COM number that no longer exists.
  if (usbCandidates.size() == 1)
    return serialPortName(usbCandidates.first());
  if (ports.size() == 1)
    return serialPortName(ports.first());

  return QString();
}

}

/**
 * Constructor; sets up the options-dialog UI and sets settings-file field names.
 */
OptionsDialog::OptionsDialog(QString title, QWidget * parent):QDialog(parent),
m_serialDeviceChanged(false),
m_settingsGroupName("Settings"), m_settingSerialDev("SerialDevice"), m_settingTemperatureUnits("TemperatureUnits"), m_settingLambdaScale("LambdaScale"), m_settingTheme("Theme"), m_settingLanguage("Language"), m_settingDesktopShortcut("DesktopShortcut")
{
  this->setWindowTitle(title);
  readSettings();
  setupWidgets();
}

/**
 * Instantiates widgets, connects to their signals, and places them on the form.
 */
void OptionsDialog::setupWidgets()
{
  unsigned int row = 0;

  m_grid = new QGridLayout(this);

  m_serialDeviceLabel = new QLabel(I18n::text(6100) /* EN: Serial device name: */, this);
  m_serialDeviceBox = new QComboBox(this);

  m_temperatureUnitsLabel = new QLabel(I18n::text(6101) /* EN: Temperature units: */, this);
  m_temperatureUnitsBox = new QComboBox(this);

  m_themeLabel = new QLabel(I18n::text(6102) /* EN: Interface theme: */, this);
  m_themeBox = new QComboBox(this);

  m_horizontalLineA = new QFrame(this);
  m_horizontalLineA->setFrameShape(QFrame::HLine);
  m_horizontalLineA->setFrameShadow(QFrame::Sunken);

  m_okButton = new QPushButton("OK", this);
  m_cancelButton = new QPushButton(I18n::text(6103) /* EN: Cancel */, this);

  SerialDevEnumerator serialDevs;
  m_serialDeviceBox->addItems(serialDevs.getSerialDevList(m_serialDeviceName));
  m_serialDeviceBox->setEditable(true);
  m_serialDeviceBox->setMinimumWidth(150);
  const int currentPort = m_serialDeviceBox->findText(m_serialDeviceName, Qt::MatchFixedString);
  if (currentPort >= 0)
    m_serialDeviceBox->setCurrentIndex(currentPort);

  m_temperatureUnitsBox->setEditable(false);
  m_temperatureUnitsBox->addItem("Fahrenheit");
  m_temperatureUnitsBox->addItem("Celsius");
  m_temperatureUnitsBox->setCurrentIndex((int)m_tempUnits);

  m_themeBox->setEditable(false);
  m_themeBox->addItem(I18n::text(6104) /* EN: Light */, "light");
  m_themeBox->addItem(I18n::text(6105) /* EN: Dark */, "dark");
  m_themeBox->setCurrentIndex(m_theme == "dark" || m_theme == "Sombre" ? 1 : 0);

  m_languageLabel = new QLabel(I18n::text(6106) /* EN: Language: */, this);
  m_languageBox = new QComboBox(this);
  m_languageBox->addItem(QIcon(":/flags/fr.png"), QStringLiteral("Français"), "fr");
  m_languageBox->addItem(QIcon(":/flags/en.png"), QStringLiteral("English"), "en");
  m_languageBox->addItem(QIcon(":/flags/es.png"), QString::fromUtf8("Español"), "es");
  m_languageBox->addItem(QIcon(":/flags/it.png"), QStringLiteral("Italiano"), "it");
  m_languageBox->addItem(QIcon(":/flags/pt.png"), QString::fromUtf8("Português"), "pt");
  m_languageBox->addItem(QIcon(":/flags/de.png"), QStringLiteral("Deutsch"), "de");
  int languageIndex = m_languageBox->findData(m_language);
  if (languageIndex < 0) languageIndex = 0;
  m_languageBox->setCurrentIndex(languageIndex);

  m_desktopShortcutBox = new QCheckBox(
      I18n::text(6107) /* EN: Create and maintain a Desktop shortcut */, this);
  m_desktopShortcutBox->setChecked(m_desktopShortcut);
  m_desktopShortcutBox->setToolTip(
      I18n::text(6108) /* EN: The shortcut is created immediately after confirmation. If it is deleted, the software will automatically recreate it at the next startup. */);

  m_grid->addWidget(m_serialDeviceLabel, row, 0);
  m_grid->addWidget(m_serialDeviceBox, row++, 1);

  m_grid->addWidget(m_temperatureUnitsLabel, row, 0);
  m_grid->addWidget(m_temperatureUnitsBox, row++, 1);

  m_grid->addWidget(m_themeLabel, row, 0);
  m_grid->addWidget(m_themeBox, row++, 1);

  m_grid->addWidget(m_languageLabel, row, 0);
  m_grid->addWidget(m_languageBox, row++, 1);

  m_grid->addWidget(m_desktopShortcutBox, row++, 0, 1, 2);

  m_grid->addWidget(m_horizontalLineA, row++, 0, 1, 2);

  m_grid->addWidget(m_okButton, row, 0);
  m_grid->addWidget(m_cancelButton, row++, 1);

  connect(m_okButton, SIGNAL(clicked()), this, SLOT(accept()));
  connect(m_cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
}

/**
 * Reads the new settings from the form controls.
 */
void OptionsDialog::accept()
{
  QString newSerialDeviceName = m_serialDeviceBox->currentText().trimmed();

  if (m_serialDeviceName.compare(newSerialDeviceName, Qt::CaseInsensitive) != 0)
  {
    m_serialDeviceName = newSerialDeviceName;
    m_serialDeviceChanged = true;
  }
  else
  {
    m_serialDeviceChanged = false;
  }

  m_tempUnits = (TemperatureUnits) (m_temperatureUnitsBox->currentIndex());

  QString newTheme = m_themeBox->currentData().toString();
  QString newLanguage = m_languageBox->currentData().toString();
  m_languageChanged = (m_language != newLanguage);
  m_language = newLanguage;
  m_desktopShortcut = m_desktopShortcutBox->isChecked();
  m_themeChanged = (m_theme != newTheme);
  m_theme = newTheme;

  writeSettings();

#ifdef Q_OS_WIN
  if (m_desktopShortcut)
    DesktopShortcut::create();
#endif

  done(QDialog::Accepted);
}

/**
 * Reads values for all the settings (either from the settings file, or by return defaults.)
 */
void OptionsDialog::readSettings()
{
  QSettings settings(QSettings::IniFormat, QSettings::UserScope, PROJECTNAME);

  settings.beginGroup(m_settingsGroupName);
  const QString savedSerialDevice = settings.value(m_settingSerialDev, "").toString().trimmed();
  m_serialDeviceName = resolveSerialDevice(settings, savedSerialDevice);
  if (m_serialDeviceName != savedSerialDevice)
    settings.setValue(m_settingSerialDev, m_serialDeviceName);
  if (!m_serialDeviceName.isEmpty())
    saveSerialIdentity(settings, m_serialDeviceName);

  m_tempUnits = (TemperatureUnits) (settings.value(m_settingTemperatureUnits, Celsius).toInt());
  m_theme = settings.value(m_settingTheme, "light").toString();
  if (m_theme == "Clair") m_theme = "light";
  if (m_theme == "Sombre") m_theme = "dark";
  m_themeChanged = false;
  m_language = settings.value(m_settingLanguage, "fr").toString().toLower();
  const QStringList supportedLanguages = QStringList() << "fr" << "en" << "es" << "it" << "pt" << "de";
  if (!supportedLanguages.contains(m_language)) m_language = "fr";
  m_languageChanged = false;
  m_desktopShortcut = settings.value(m_settingDesktopShortcut, false).toBool();

  settings.endGroup();
  settings.sync();
}

/**
 * Writes settings out to a file on disk.
 */
void OptionsDialog::writeSettings()
{
  QSettings settings(QSettings::IniFormat, QSettings::UserScope, PROJECTNAME);

  settings.beginGroup(m_settingsGroupName);
  settings.setValue(m_settingSerialDev, m_serialDeviceName);
  saveSerialIdentity(settings, m_serialDeviceName);
  settings.setValue(m_settingTemperatureUnits, m_tempUnits);
  settings.setValue(m_settingTheme, m_theme);
  settings.setValue(m_settingLanguage, m_language);
  settings.setValue("LanguageConfigured", true);
  settings.setValue(m_settingDesktopShortcut, m_desktopShortcut);
  settings.endGroup();
  settings.sync();
}

QString OptionsDialog::getSerialDeviceName()
{
#ifdef WIN32
  if (m_serialDeviceName.isEmpty())
    return QString();
  return QString("\\\\.\\%1").arg(m_serialDeviceName);
#else
  return m_serialDeviceName;
#endif
}
