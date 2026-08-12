#include <QThread>
#include <QDateTime>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <string.h>
#include <QThread>
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/select.h>
#endif
#include "memsinterface.h"

/**
 * Constructor. Sets the serial device and measurement units.
 * @param device Name of (or path to) the serial device used to comminucate
 *  with the ECU.
 */
MEMSInterface::MEMSInterface(QString device, QObject * parent):
QObject(parent), m_deviceName(device), m_stopPolling(false), m_shutdownThread(false), m_initComplete(false)
{
  memset(&m_data, 0, sizeof(mems_data));
  memset(m_d0_response_buffer, 0, 4);
}

/**
 * Destructor.
 */
MEMSInterface::~MEMSInterface()
{
}

/**
 * Clears the block of fault codes.
 */
void MEMSInterface::onFaultCodesClearRequested()
{
  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    if (mems_clear_faults(&m_memsinfo))
    {
      emit faultCodesClearSuccess();
    }
    else
    {
      emit errorSendingCommand();
    }
  }
  else
  {
    emit notConnected();
  }
}

/**
 * Resets all adjustments.
 */
void MEMSInterface::onResetAdjustmentsRequested()
{
  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    if (mems_reset_adjustments(&m_memsinfo))
    {
      // emit adjustmentsResetSuccess();
    }
    else
    {
      emit errorSendingCommand();
    }
  }
  else
  {
    emit notConnected();
  }
}

/**
 * Resets complete ECU.
 */
void MEMSInterface::onResetECURequested()
{
  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    if (mems_reset_ECU(&m_memsinfo))
    {
      emit ECUResetSuccess();
    }
    else
    {
      emit errorSendingCommand();
    }
  }
  else
  {
    emit notConnected();
  }
}

/**
 * Responds to a signal requesting that the idle air control valve be moved.
 */
void MEMSInterface::onIdleAirControlMovementRequest(int desiredPos)
{
  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    if (!mems_move_iac(&m_memsinfo, desiredPos))
    {
      emit errorSendingCommand();
    }
  }
  else
  {
    emit notConnected();
  }

  emit moveIACComplete();
}

/**
 * Attempts to open the serial device that is connected to the ECU.
 * @return True if serial device was opened successfully and the
 *  ECU is responding to commands; false otherwise.
 */
bool MEMSInterface::connectToECU()
{
  bool status = mems_connect(&m_memsinfo, m_deviceName.toStdString().c_str()) &&
    mems_init_link(&m_memsinfo, m_d0_response_buffer);
  if (status)
  {
    emit gotEcuId(m_d0_response_buffer);
  }
  return status;
}

/**
 * Sets a flag that will cause us to stop polling and disconnect from the serial device.
 */
void MEMSInterface::disconnectFromECU()
{
  m_stopPolling = true;
}

/**
 * Cleans up and exits the worker thread.
 */
void MEMSInterface::onShutdownThreadRequest()
{
  if (m_serviceLoopRunning)
  {
    m_shutdownThread = true;
  }
  else
  {
    QThread::currentThread()->quit();
  }
}

/**
 * Indicates whether the serial device is currently open/connected.
 * @return True when the device is connected; false otherwise.
 */
bool MEMSInterface::isConnected()
{
  return (m_initComplete && mems_is_connected(&m_memsinfo));
}

/**
 * Responds to the parent thread being started by initializing the library
 * struct and emitting a signal indicating that the interface is ready.
 */
void MEMSInterface::onParentThreadStarted()
{
  // Initialize the interface state info struct here, so that
  // it's in the context of the thread that will use it.
  if (!m_initComplete)
  {
    mems_init(&m_memsinfo);
    m_initComplete = true;
  }

  emit interfaceThreadReady();
}

/**
 * Responds to a signal to start polling the ECU.
 */
void MEMSInterface::onStartPollingRequest()
{
  if (connectToECU())
  {
    emit connected();

    m_stopPolling = false;
    m_shutdownThread = false;
    runServiceLoop();
  }
  else
  {
#ifdef WIN32
    QString simpleDeviceName = m_deviceName;

    if (simpleDeviceName.indexOf("\\\\.\\") == 0)
    {
      simpleDeviceName.remove(0, 4);
    }
    emit failedToConnect(simpleDeviceName);
#else
    emit failedToConnect(m_deviceName);
#endif
  }
}

/**
 * Calls the library in a loop until commanded to stop.
 */
void MEMSInterface::runServiceLoop()
{
  bool connected = mems_is_connected(&m_memsinfo);

  m_serviceLoopRunning = true;
  while (!m_stopPolling && !m_shutdownThread && connected)
  {
    if (mems_read(&m_memsinfo, &m_data))
    {
      emit readSuccess();
      emit dataReady();
    }
    else
    {
      emit readError();

      // Important : read() peut échouer lorsque le contact de la voiture
      // est coupé. L'ancien code conservait « connected == true » et
      // continuait donc à boucler indéfiniment. On vérifie immédiatement
      // l'état réel de la connexion afin de sortir proprement.
      connected = mems_is_connected(&m_memsinfo);
      if (!connected)
        break;
    }
    QCoreApplication::processEvents();
  }
  m_serviceLoopRunning = false;

  if (connected)
    mems_disconnect(&m_memsinfo);

  emit disconnected();

  if (m_shutdownThread)
  {
    QThread::currentThread()->quit();
  }
}

bool MEMSInterface::actuatorOnOffDelayTest(actuator_cmd onCmd, actuator_cmd offCmd)
{
  bool status = false;
  void sleep();

  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    if (mems_test_actuator(&m_memsinfo, onCmd, NULL))
		QThread::sleep(1);
    {
      if (mems_test_actuator(&m_memsinfo, offCmd, NULL))
      {
        status = true;
      }
    }

    if (!status)
    {
      emit errorSendingCommand();
    }
  }
  else
  {
    emit notConnected();
  }

  return status;
}

void MEMSInterface::onFuelPumpTest()
{
  actuatorOnOffDelayTest(MEMS_FuelPumpOn, MEMS_FuelPumpOff);
  emit fuelPumpTestComplete();
}

void MEMSInterface::on_m_Purge_Valve_TestButton_clicked()
{
  actuatorOnOffDelayTest(MEMS_PurgeValveOn, MEMS_PurgeValveOff);
  emit PurgeValveTestComplete();
}

void MEMSInterface::onPTCRelayTest()
{
  actuatorOnOffDelayTest(MEMS_PTCRelayOn, MEMS_PTCRelayOff);
  emit ptcRelayTestComplete();
}

void MEMSInterface::onACRelayTest()
{
  actuatorOnOffDelayTest(MEMS_ACRelayOn, MEMS_ACRelayOff);
  emit acRelayTestComplete();
}

void MEMSInterface::on_m_O2Heater_TestButton_clicked()
{
  actuatorOnOffDelayTest(MEMS_O2HeaterOn, MEMS_O2HeaterOff);
  emit O2HeaterTestComplete();
}

void MEMSInterface::on_m_Boost_Valve_TestButton_clicked()
{
  actuatorOnOffDelayTest(MEMS_BoostValveOn, MEMS_BoostValveOff);
  emit BoostValveTestComplete();
}

void MEMSInterface::on_m_Fan1_TestButton_clicked()
{
  actuatorOnOffDelayTest(MEMS_Fan1On, MEMS_Fan1Off);
  emit Fan1TestComplete();
}

void MEMSInterface::on_m_Fan2_TestButton_clicked()
{
  actuatorOnOffDelayTest(MEMS_Fan2On, MEMS_Fan2Off);
  emit Fan2TestComplete();
}

void MEMSInterface::on_m_Fan3_TestButton_clicked()
{
  actuatorOnOffDelayTest(MEMS_Fan3On, MEMS_Fan3Off);
  emit Fan3TestComplete();
}

void MEMSInterface::onFuelPumpOn()
{
  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    if (!mems_test_actuator(&m_memsinfo, MEMS_FuelPumpOn, NULL))
    {
      emit errorSendingCommand();
    }
  }
}

void MEMSInterface::onFuelPumpOff()
{
  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    if (!mems_test_actuator(&m_memsinfo, MEMS_FuelPumpOff, NULL))
    {
      emit errorSendingCommand();
    }
  }
}

void MEMSInterface::onPTCRelayOn()
{
  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    if (!mems_test_actuator(&m_memsinfo, MEMS_PTCRelayOn, NULL))
    {
      emit errorSendingCommand();
    }
  }
}

void MEMSInterface::onPTCRelayOff()
{
  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    if (!mems_test_actuator(&m_memsinfo, MEMS_PTCRelayOff, NULL))
    {
      emit errorSendingCommand();
    }
  }
}

void MEMSInterface::onACRelayOn()
{
  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    if (!mems_test_actuator(&m_memsinfo, MEMS_ACRelayOn, NULL))
    {
      emit errorSendingCommand();
    }
  }
}

void MEMSInterface::onACRelayOff()
{
  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    if (!mems_test_actuator(&m_memsinfo, MEMS_ACRelayOff, NULL))
    {
      emit errorSendingCommand();
    }
  }
}

void MEMSInterface::onIgnitionCoilTest()
{
  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    if (!mems_test_actuator(&m_memsinfo, MEMS_FireCoil, NULL))
    {
      emit errorSendingCommand();
    }
  }
}

void MEMSInterface::onFuelInjectorTest()
{
  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    if (!mems_test_actuator(&m_memsinfo, MEMS_TestInjectors, NULL))
    {
      emit errorSendingCommand();
    }
  }
}
void MEMSInterface::on_m_fuel_trim_plusButton_clicked()
{
 if (m_initComplete && mems_is_connected(&m_memsinfo))
 {
     if (!mems_test_actuator(&m_memsinfo,MEMS_FuelTrimPlus,NULL))
     {
       emit errorSendingCommand();
     }
}

}

 void MEMSInterface::on_m_fuel_trim_minusButton_clicked()
 {
  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
      if (!mems_test_actuator(&m_memsinfo, MEMS_FuelTrimMinus, NULL))
      {
        emit errorSendingCommand();
      }
 }
 }

  void MEMSInterface::on_m_idle_decay_plusButton_clicked()
  {
   if (m_initComplete && mems_is_connected(&m_memsinfo))
   {
       if (!mems_test_actuator(&m_memsinfo,MEMS_IdleDecayPlus, NULL))
       {
         emit errorSendingCommand();
       }
  }
  }

    void MEMSInterface::on_m_idle_decay_minusButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!mems_test_actuator(&m_memsinfo,MEMS_IdleDecayMinus, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }

    void MEMSInterface::on_m_idle_speed_plusButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!mems_test_actuator(&m_memsinfo,MEMS_IdleSpeedPlus, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }

    void MEMSInterface::on_m_idle_speed_minusButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!mems_test_actuator(&m_memsinfo,MEMS_IdleSpeedMinus, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
    void MEMSInterface::on_m_ignition_advance_plusButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!mems_test_actuator(&m_memsinfo,MEMS_IgnitionAdvancePlus, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
    void MEMSInterface::on_m_ignition_advance_minusButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!mems_test_actuator(&m_memsinfo,MEMS_IgnitionAdvanceMinus, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }

//mems_Purge_Valve_On(mems_info* info, uint8_t* data)
    void MEMSInterface::on_m_Purge_Valve_OnButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!mems_test_actuator(&m_memsinfo,MEMS_PurgeValveOn, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
    void MEMSInterface::on_m_Purge_Valve_OffButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!mems_test_actuator(&m_memsinfo,MEMS_PurgeValveOff, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
    void MEMSInterface::on_m_O2Heater_OnButton_clicked()
    {
		// void sleep();
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
		if (!mems_test_actuator(&m_memsinfo,MEMS_O2HeaterOn, NULL))
         {
           emit errorSendingCommand();
         }
		/*  else 
		 {
			 QThread::sleep(5);
			 emit turnO2HeaterOff();
		 } */
		 /* if (mems_test_actuator(&m_memsinfo,MEMS_O2HeaterOn, NULL))
		 {
			 QThread::sleep(2);
			 emit turnO2HeaterOff();
		 } */
		  
    }
    }
    void MEMSInterface::on_m_O2Heater_OffButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!mems_test_actuator(&m_memsinfo,MEMS_O2HeaterOff, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
    void MEMSInterface::on_m_Boost_Valve_OnButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!mems_test_actuator(&m_memsinfo,MEMS_BoostValveOn, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
	void MEMSInterface::on_m_Boost_Valve_OffButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!mems_test_actuator(&m_memsinfo,MEMS_BoostValveOff, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
    void MEMSInterface::on_m_Fan1_OnButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!mems_test_actuator(&m_memsinfo,MEMS_Fan1On, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
    void MEMSInterface::on_m_Fan2_OnButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!mems_test_actuator(&m_memsinfo,MEMS_Fan2On, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
	void MEMSInterface::on_m_Fan3_OnButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!mems_test_actuator(&m_memsinfo,MEMS_Fan3On, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }

    void MEMSInterface::on_m_Fan1_OffButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!mems_test_actuator(&m_memsinfo,MEMS_Fan1Off, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
    void MEMSInterface::on_m_Fan2_OffButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!mems_test_actuator(&m_memsinfo,MEMS_Fan2Off, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
	void MEMSInterface::on_m_Fan3_OffButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!mems_test_actuator(&m_memsinfo,MEMS_Fan3Off, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
	
	void MEMSInterface::on_m_IACMinusButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!mems_test_actuator(&m_memsinfo,MEMS_CloseIAC, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
	void MEMSInterface::on_m_IACPlusButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!mems_test_actuator(&m_memsinfo,MEMS_OpenIAC, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
	
	void MEMSInterface::on_m_AllActuatorsOffButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!mems_test_actuator(&m_memsinfo,MEMS_AllActuatorsOff, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }

    void MEMSInterface::on_interactive_push_button_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {


           emit errorSendingCommand();

    }
    }



/**
 * Sends a single ROSCO command that is not otherwise exposed by librosco.
 *
 * This is deliberately limited to read/session commands.  Configuration
 * writes (for example D3) are not exposed here because their semantics are
 * not sufficiently documented for safe automatic use.
 */
void MEMSInterface::onProtocolCommandRequested(quint8 command)
{
  if (!(m_initComplete && mems_is_connected(&m_memsinfo)))
  {
    emit notConnected();
    return;
  }

  QByteArray response;

#ifdef WIN32
  HANDLE h = m_memsinfo.sd;
  if (h == INVALID_HANDLE_VALUE || h == NULL)
  {
    emit notConnected();
    return;
  }

  COMMTIMEOUTS oldTimeouts;
  COMMTIMEOUTS timeouts;
  if (!GetCommTimeouts(h, &oldTimeouts))
    memset(&oldTimeouts, 0, sizeof(oldTimeouts));

  memset(&timeouts, 0, sizeof(timeouts));
  timeouts.ReadIntervalTimeout = 20;
  timeouts.ReadTotalTimeoutConstant = 120;
  timeouts.ReadTotalTimeoutMultiplier = 5;
  timeouts.WriteTotalTimeoutConstant = 120;
  timeouts.WriteTotalTimeoutMultiplier = 5;
  SetCommTimeouts(h, &timeouts);

  PurgeComm(h, PURGE_RXCLEAR);

  DWORD written = 0;
  unsigned char tx = command;
  if (!WriteFile(h, &tx, 1, &written, NULL) || written != 1)
  {
    SetCommTimeouts(h, &oldTimeouts);
    emit errorSendingCommand();
    return;
  }

  // Responses are short single-frame ROSCO replies. Read until the
  // inter-byte timeout expires, with a hard upper bound.
  QElapsedTimer timer;
  timer.start();
  while (timer.elapsed() < 350)
  {
    unsigned char b = 0;
    DWORD n = 0;
    if (ReadFile(h, &b, 1, &n, NULL) && n == 1)
    {
      response.append(char(b));
      timer.restart();
    }
    else
    {
      QCoreApplication::processEvents();
    }
  }

  SetCommTimeouts(h, &oldTimeouts);
#else
  int fd = m_memsinfo.sd;
  if (fd < 0)
  {
    emit notConnected();
    return;
  }

  unsigned char tx = command;
  if (write(fd, &tx, 1) != 1)
  {
    emit errorSendingCommand();
    return;
  }

  QElapsedTimer timer;
  timer.start();
  while (timer.elapsed() < 350)
  {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(fd, &set);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 30000;
    int ready = select(fd + 1, &set, NULL, NULL, &tv);
    if (ready > 0 && FD_ISSET(fd, &set))
    {
      unsigned char buf[64];
      ssize_t n = read(fd, buf, sizeof(buf));
      if (n > 0)
      {
        response.append(reinterpret_cast<const char*>(buf), int(n));
        timer.restart();
      }
    }
  }
#endif

  emit protocolResponse(command, response);
}
