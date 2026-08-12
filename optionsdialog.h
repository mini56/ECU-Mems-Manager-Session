#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QString>
#include <QHash>
#include <QFrame>
#include "commonunits.h"

class OptionsDialog : public QDialog
{
    Q_OBJECT

public:
    OptionsDialog(QString title, QWidget *parent = 0);
    QString getSerialDeviceName();
    bool getSerialDeviceChanged() { return m_serialDeviceChanged; }
    TemperatureUnits getTemperatureUnits() { return m_tempUnits; }
	LambdaScale getLambdaScale() { return m_lambdaScale; }
    QString getTheme() { return m_theme; }
    bool getThemeChanged() { return m_themeChanged; }
    QString getLanguage() const { return m_language; }
    bool getLanguageChanged() const { return m_languageChanged; }

protected:
    void accept();

private:
    QGridLayout *m_grid;
    QLabel *m_serialDeviceLabel;
    QComboBox *m_serialDeviceBox;

    QLabel *m_temperatureUnitsLabel;
    QComboBox *m_temperatureUnitsBox;
	
	QLabel *m_lambdaScaleLabel;
	QComboBox *m_lambdaScaleBox;

    QLabel *m_themeLabel;
    QComboBox *m_themeBox;

    QLabel *m_languageLabel;
    QComboBox *m_languageBox;
    QCheckBox *m_desktopShortcutBox;

    QFrame *m_horizontalLineA;

    QCheckBox *m_refreshFuelMapCheckbox;

    QPushButton *m_okButton;
    QPushButton *m_cancelButton;

    QString m_serialDeviceName;
    TemperatureUnits m_tempUnits;
	LambdaScale m_lambdaScale;
    QString m_theme;
    bool m_themeChanged;
    QString m_language;
    bool m_languageChanged;
    bool m_desktopShortcut;

    bool m_serialDeviceChanged;

    const QString m_settingsFileName;
    const QString m_settingsGroupName;

    const QString m_settingSerialDev;
    const QString m_settingTemperatureUnits;
	const QString m_settingLambdaScale;
    const QString m_settingTheme;
    const QString m_settingLanguage;
    const QString m_settingDesktopShortcut;

    void setupWidgets();
    void readSettings();
    void writeSettings();
};

#endif // OPTIONSDIALOG_H
