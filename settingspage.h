#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H
class QMainWindow;
class QWidget;
// Original settings controls, gauges and actuator adjustments.
void buildSettingsPage(QMainWindow *window);
void fitSettingsPage(QMainWindow *window);
QWidget *createActuatorTemperatureGauge(QWidget *parent);
QWidget *createActuatorRevCounterGauge(QWidget *parent);
#endif
