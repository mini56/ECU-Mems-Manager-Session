#ifndef MEMSINTERFACE_H
#define MEMSINTERFACE_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QByteArray>
#include <QAtomicInteger>
#include "rosco.h"
#include "commonunits.h"

class MEMSInterface : public QObject
{
    Q_OBJECT
public:
    explicit MEMSInterface(QString device, QObject *parent = 0);
    ~MEMSInterface();

    void setSerialDevice(QString device) { m_deviceName = device; }
    QString getSerialDevice() { return m_deviceName; }
    int getIntervalMsecs();
    bool isConnected();
    void disconnectFromECU();
    mems_data* getData()          { return &m_data; }
    librosco_version getVersion() { return mems_get_lib_version(); }
    void cancelRead();

public slots:
    void onParentThreadStarted();
    void onFaultCodesClearRequested();
    void onResetAdjustmentsRequested();
    void onResetECURequested();
    void onStartPollingRequest();
    void onShutdownThreadRequest();
    void onFuelPumpTest();
    void onFuelPumpOn();
    void onFuelPumpOff();
    void onPTCRelayTest();
    void onPTCRelayOn();
    void onPTCRelayOff();
    void onACRelayTest();
    void onACRelayOn();
    void onACRelayOff();
    void onIgnitionCoilTest();
    void onFuelInjectorTest();
    void onIdleAirControlMovementRequest(int desiredPos);
    void on_m_fuel_trim_plusButton_clicked();
    void on_m_fuel_trim_minusButton_clicked();
    void on_m_idle_decay_plusButton_clicked();
    void on_m_idle_decay_minusButton_clicked();
    void on_m_idle_speed_plusButton_clicked();
    void on_m_idle_speed_minusButton_clicked();
    void on_m_ignition_advance_plusButton_clicked();
    void on_m_ignition_advance_minusButton_clicked();
    void on_m_Purge_Valve_TestButton_clicked();
    void on_m_Purge_Valve_OnButton_clicked();
    void on_m_Purge_Valve_OffButton_clicked();
    void on_m_O2Heater_TestButton_clicked();
    void on_m_O2Heater_OnButton_clicked();
    void on_m_O2Heater_OffButton_clicked();
    void on_m_Boost_Valve_TestButton_clicked();
    void on_m_Boost_Valve_OnButton_clicked();
    void on_m_Boost_Valve_OffButton_clicked();
    void on_m_Fan1_TestButton_clicked();
    void on_m_Fan2_TestButton_clicked();
    void on_m_Fan3_TestButton_clicked();
    void on_m_Fan1_OffButton_clicked();
    void on_m_Fan2_OffButton_clicked();
    void on_m_Fan3_OffButton_clicked();
    void on_m_IACMinusButton_clicked();
    void on_m_IACPlusButton_clicked();
    void on_m_AllActuatorsOffButton_clicked();
    void on_interactive_push_button_clicked();
    void onProtocolCommandRequested(quint8 command);
    void onInjectionRamTestRequested();
    void onOverviewLiveModeRequested(bool enabled);
    void onMappedInjectionModeRequested(bool enabled, QString firmwareIdentifier);
    void onMappedInjectionStopRequested();
    void onMappedInjectionRequestState(bool enabled);

signals:
    void dataReady();
    void connected();
    void disconnected();
    void readError();
    void readSuccess();
    void faultCodesClearSuccess();
    void adjustmentsResetSuccess();
    void ECUResetSuccess();
    void failedToConnect(QString dev);
    void interfaceThreadReady();
    void notConnected();
    void gotEcuId(uint8_t* id_buffer);
    void errorSendingCommand();
    void turnO2HeaterOff();
    void fuelPumpTestComplete();
    void ptcRelayTestComplete();
    void acRelayTestComplete();
    void O2HeaterTestComplete();
    void BoostValveTestComplete();
    void PurgeValveTestComplete();
    void Fan1TestComplete();
    void Fan2TestComplete();
    void Fan3TestComplete();
    void moveIACComplete();
    void fuel_trim_plus();
    void fuel_trim_minus();
    void idle_decay_plus();
    void idle_decay_minus();
    void idle_speed_plus();
    void idle_speed_minus();
    void ignition_advance_plus();
    void ignition_advance_minus();
    void interactive_mode();
    void protocolResponse(quint8 command, QByteArray response);
    void injectionRamTestFinished(bool success,
                                  quint16 baseRaw,
                                  quint16 correctionRaw,
                                  QString status,
                                  QString log);
    void overviewInjectionReady(double injectionMs);
    void injectionLiveSample(double finalMs,
                             double baseMs,
                             double transientMs,
                             quint16 baseRaw,
                             quint16 transientRaw,
                             quint8 transientCounter);
    void readModeChanged(QString mode);

private:
    mems_data m_data;
    QString m_deviceName;
    mems_info m_memsinfo;
    bool m_stopPolling;
    bool m_shutdownThread;
    bool m_initComplete;
    bool m_serviceLoopRunning;
    QAtomicInteger<int> m_mappedInjectionRequested{0};
    uint8_t m_d0_response_buffer[4];
    void runServiceLoop();
    bool connectToECU();
    bool actuatorOnOffDelayTest(actuator_cmd onCmd, actuator_cmd offCmd);
};

#endif // MEMSINTERFACE_H
