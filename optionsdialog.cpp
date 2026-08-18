#include <QSettings>
#include "optionsdialog.h"
#include "serialdevenumerator.h"
#include "desktopshortcut.h"
#include <QIcon>
#include <QPalette>
#include <QColor>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#include "i18n.h"
#define tr I18n::text

namespace {

static void applyDarkNativeTitleBar(QWidget *widget)
{
#ifdef Q_OS_WIN
  if (!widget)
    return;

  HWND hwnd = reinterpret_cast<HWND>(widget->winId());
  HMODULE dwmApi = LoadLibraryW(L"dwmapi.dll");
  if (!dwmApi)
    return;

  typedef HRESULT (WINAPI *DwmSetWindowAttributeFn)(HWND, DWORD, LPCVOID, DWORD);
  DwmSetWindowAttributeFn setAttribute = reinterpret_cast<DwmSetWindowAttributeFn>(
      GetProcAddress(dwmApi, "DwmSetWindowAttribute"));
  if (setAttribute)
  {
    const BOOL dark = TRUE;
    const DWORD attributes[] = {20, 19};
    for (DWORD attribute : attributes)
    {
      if (SUCCEEDED(setAttribute(hwnd, attribute, &dark, sizeof(dark))))
        break;
    }
  }

  FreeLibrary(dwmApi);
#else
  Q_UNUSED(widget);
#endif
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
  QPalette dialogPalette = palette();
  dialogPalette.setColor(QPalette::Window, QColor(QStringLiteral("#090e13")));
  dialogPalette.setColor(QPalette::WindowText, QColor(QStringLiteral("#e7ecef")));
  setPalette(dialogPalette);
  setAutoFillBackground(true);
  readSettings();
  setupWidgets();
  applyDarkNativeTitleBar(this);
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
  
/*   m_lambdaScaleLabel = new QLabel("Lambda sensor scale:", this);
  m_lambdaScaleBox = new QComboBox(this); */

  m_horizontalLineA = new QFrame(this);
  m_horizontalLineA->setFrameShape(QFrame::HLine);
  m_horizontalLineA->setFrameShadow(QFrame::Sunken);

  m_okButton = new QPushButton("OK", this);
  m_cancelButton = new QPushButton(I18n::text(6103) /* EN: Cancel */, this);

  SerialDevEnumerator serialDevs;

  m_serialDeviceBox->addItems(serialDevs.getSerialDevList(m_serialDeviceName));
  m_serialDeviceBox->setEditable(true);
  m_serialDeviceBox->setMinimumWidth(150);
  m_serialDeviceBox->setMinimumHeight(30);
  if (m_serialDeviceBox->lineEdit())
    m_serialDeviceBox->lineEdit()->setMinimumHeight(22);

  m_temperatureUnitsBox->setEditable(false);
  m_temperatureUnitsBox->addItem("Fahrenheit");
  m_temperatureUnitsBox->addItem("Celsius");
  m_temperatureUnitsBox->setCurrentIndex((int)m_tempUnits);

  m_themeBox->setEditable(false);
  m_themeBox->addItem(I18n::text(6104) /* EN: Light */, "light");
  m_themeBox->addItem(QStringLiteral("StyleDark"), "dark");
  m_themeBox->setCurrentIndex(m_theme == "dark" || m_theme == "Sombre" ? 1 : 0);

  m_languageLabel = new QLabel(I18n::text(6106) /* EN: Language: */, this);
  m_languageBox = new QComboBox(this);
  // Keep language names native so they remain identifiable in every UI language.
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
  m_desktopShortcutBox->setStyleSheet(QStringLiteral("QCheckBox{color:#e7ecef;background:transparent;} QCheckBox:disabled{color:#aeb7bf;}"));
  m_desktopShortcutBox->setToolTip(
      I18n::text(6108) /* EN: The shortcut is created immediately after confirmation. If it is deleted, the software will automatically recreate it at the next startup. */);
  
/*   m_lambdaScaleBox->setEditable(false);
  m_lambdaScaleBox->addItem("_4mV_steps");
  m_lambdaScaleBox->addItem("_5mV_steps");
  m_lambdaScaleBox->setCurrentIndex((int)m_lambdaScale); */

  m_grid->addWidget(m_serialDeviceLabel, row, 0);
  m_grid->addWidget(m_serialDeviceBox, row++, 1);

  m_grid->addWidget(m_temperatureUnitsLabel, row, 0);
  m_grid->addWidget(m_temperatureUnitsBox, row++, 1);

  m_grid->addWidget(m_themeLabel, row, 0);
  m_grid->addWidget(m_themeBox, row++, 1);

  m_grid->addWidget(m_languageLabel, row, 0);
  m_grid->addWidget(m_languageBox, row++, 1);

  m_grid->addWidget(m_desktopShortcutBox, row++, 0, 1, 2);
  
/*   m_grid->addWidget(m_lambdaScaleLabel, row, 0);  
  m_grid->addWidget(m_lambdaScaleBox, row++, 1); */

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
  QString newSerialDeviceName = m_serialDeviceBox->currentText();

  // set a flag if the serial device has been changed;
  // the main application needs to know if it should
  // reconnect to the ECU
  if (m_serialDeviceName.compare(newSerialDeviceName) != 0)
  {
    m_serialDeviceName = newSerialDeviceName;
    m_serialDeviceChanged = true;
  }
  else
  {
    m_serialDeviceChanged = false;
  }

  m_tempUnits = (TemperatureUnits) (m_temperatureUnitsBox->currentIndex());
  // m_lambdaScale = (LambdaScale) (m_lambdaScaleBox->currentIndex());

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
  m_serialDeviceName = settings.value(m_settingSerialDev, "").toString();
  m_tempUnits = (TemperatureUnits) (settings.value(m_settingTemperatureUnits, Celsius).toInt());
  // m_lambdaScale = (LambdaScale) (settings.value(m_settingLambdaScale, _5mV_steps).toInt());
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
}

/**
 * Writes settings out to a file on disk.
 */
void OptionsDialog::writeSettings()
{
  QSettings settings(QSettings::IniFormat, QSettings::UserScope, PROJECTNAME);

  settings.beginGroup(m_settingsGroupName);
  settings.setValue(m_settingSerialDev, m_serialDeviceName);
  settings.setValue(m_settingTemperatureUnits, m_tempUnits);
  // settings.setValue(m_settingLambdaScale, m_lambdaScale);
  settings.setValue(m_settingTheme, m_theme);
  settings.setValue(m_settingLanguage, m_language);
  settings.setValue("LanguageConfigured", true);
  settings.setValue(m_settingDesktopShortcut, m_desktopShortcut);

  settings.endGroup();
}

/**
 * Returns the name of the serial device.
 */
QString OptionsDialog::getSerialDeviceName()
{
#ifdef WIN32
  return QString("\\\\.\\%1").arg(m_serialDeviceName);
#else
  return m_serialDeviceName;
#endif
}
