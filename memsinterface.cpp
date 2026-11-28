#include <QThread>
#include <QDateTime>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <QMutexLocker>
#include <string.h>
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/select.h>
#endif
#include "memsinterface.h"
#include "mems16na_live_trace.h"
#include <QFile>
#include <QFileInfo>
#include "ecucommunicationtraceio.h"
#include "mems19protocol.h"
#include "mems2jprotocol.h"
#include "mems3protocol.h"

/**
 * Constructor. Sets the serial device and measurement units.
 * @param device Name of (or path to) the serial device used to comminucate
 *  with the ECU.
 */
MEMSInterface::MEMSInterface(QString device, QObject * parent):
QObject(parent), m_deviceName(device), m_stopPolling(false), m_shutdownThread(false), m_initComplete(false), m_serviceLoopRunning(false), m_connectionAttemptActive(false)
{
  memset(&m_data, 0, sizeof(mems_data));
  memset(m_d0_response_buffer, 0, 4);
  bindProtocolContextTracking();
}

/**
 * Destructor.
 */
MEMSInterface::~MEMSInterface()
{
  delete m_mems2j;
  delete m_mems3;
}

/**
 * Clears the block of fault codes.
 */
void MEMSInterface::onFaultCodesClearRequested()
{
  // A live 2J/3 QSerialPort session is not a librosco handle.
  // The user explicitly requested fault clearing; connection never does.
  if (!isConnected()) { emit notConnected(); return; }
  if (guardedClearFaults()) emit faultCodesClearSuccess();
  else emit errorSendingCommand();
}

void MEMSInterface::onSaveAdjustmentsRequested()
{
    if (!isConnected()) {
        emit notConnected();
        return;
    }

    const MemsEcuFamily family = ecuFamily();
    if (family != MemsEcuFamily::Rosco13_16 && family != MemsEcuFamily::Mems19) {
        emit errorSendingCommand();
        return;
    }

    // Preserve the proven ECU command. The MEMS 1.9 K-Line path remains
    // unchanged; this longer confirmation wait concerns only ROSCO 1.3/1.6.
    constexpr auto saveAdjustments = static_cast<actuator_cmd>(0x81);
    uint8_t reply = 0xff;
    if (family == MemsEcuFamily::Mems19) {
        if (guardedMemsTestActuator(saveAdjustments, &reply) && reply == 0x00)
            emit adjustmentsSaveAcknowledged();
        else
            emit errorSendingCommand();
        return;
    }

    // runServiceLoop() executes this slot through its worker-thread event
    // pump. While this synchronous transaction is running, no new 0x80/0x7D
    // measurement request can consume the late reply to 0x81.
#ifdef WIN32
    const HANDLE port = m_memsinfo.sd;
    COMMTIMEOUTS previousTimeouts = {};
    bool extended = false;
    if (port != nullptr && port != INVALID_HANDLE_VALUE &&
        GetCommTimeouts(port, &previousTimeouts)) {
        COMMTIMEOUTS saveTimeouts = previousTimeouts;
        // The observed 0x00 arrived about 145 ms after TX 0x81; the normal
        // 100 ms timeout is too short. Change the read only for this command.
        saveTimeouts.ReadTotalTimeoutMultiplier = 0;
        saveTimeouts.ReadTotalTimeoutConstant = 700;
        extended = SetCommTimeouts(port, &saveTimeouts) != FALSE;
    }
#endif

    QElapsedTimer transactionTimer;
    transactionTimer.start();
    const bool received = guardedMemsTestActuator(saveAdjustments, &reply);
    const qint64 durationMs = transactionTimer.elapsed();

#ifdef WIN32
    // Always restore the original serial timings before polling resumes.
    if (extended && !SetCommTimeouts(port, &previousTimeouts))
        emit adjustmentsSaveDiagnostic(QStringLiteral("SAVE81_RESTAURATION_TIMEOUT_ECHEC"));
#endif

    if (received && reply == 0x00) {
        emit adjustmentsSaveDiagnostic(
            QStringLiteral("SAVE81_ACK_00_DUREE_TRANSACTION_MS=%1").arg(durationMs));
        emit adjustmentsSaveAcknowledged();
    } else {
        const QString response = received
            ? QStringLiteral("%1").arg(reply, 2, 16, QLatin1Char('0')).toUpper()
            : QStringLiteral("ABSENTE");
        // No false modal 'command failed': missing ACK does not prove that
        // the ECU did not save. The raw TX/RX trace remains the authority.
        emit adjustmentsSaveDiagnostic(
            QStringLiteral("SAVE81_CONFIRMATION_NON_RECUE_DUREE_TRANSACTION_MS=%1_REPONSE=%2")
                .arg(durationMs).arg(response));
    }
}

/**
 * Resets all adjustments.
 */
void MEMSInterface::onResetAdjustmentsRequested()
{
  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    if (guardedResetAdjustments())
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
    if (guardedResetEcu())
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
    if (!guardedMemsMoveIac(desiredPos))
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
bool MEMSInterface::openRoscoTransport()
{
  m_naSession.reset();
  { QMutexLocker locker(&m_dataMutex); m_naSnapshot.reset(); }
  if (m_shutdownRequested.loadAcquire() != 0)
    return false;

  bool status = mems_connect(&m_memsinfo, m_deviceName.toStdString().c_str()) &&
    mems_init_link(&m_memsinfo, m_d0_response_buffer);
  m_connectedState.storeRelease(status ? 1 : 0);
  if (status)
  {
    // Identity returned by the existing initialization, not a new ECU request.
    m_naSession.tx({0xD0});
    m_naSession.rx({0xD0,m_d0_response_buffer[0],m_d0_response_buffer[1],
                   m_d0_response_buffer[2],m_d0_response_buffer[3]});
    m_naSession.finish();
    emit gotEcuId(m_d0_response_buffer);
  }
  return status;
}

/**
 * Sets a thread-safe flag that will cause polling/injection to stop and the
 * serial session to be disconnected by the worker thread.
 */
void MEMSInterface::disconnectFromECU()
{
  m_disconnectRequested.storeRelease(1);
  cancelInstrumentTest();
  m_mappedInjectionRequested.storeRelease(0);
}

/**
 * Thread-safe shutdown request used by MainWindow before waiting for the
 * worker thread. Only atomics are touched here, so it is safe from the GUI thread.
 */
void MEMSInterface::requestShutdown()
{
  m_shutdownRequested.storeRelease(1);
  cancelInstrumentTest();
  m_disconnectRequested.storeRelease(1);
  m_mappedInjectionRequested.storeRelease(0);
}

/**
 * Cleans up and exits the worker thread.
 */
void MEMSInterface::onShutdownThreadRequest()
{
  requestShutdown();
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
 * @return True when the device is connected; false otherwise
 */
bool MEMSInterface::isConnected()
{
  return m_connectedState.loadAcquire() != 0;
}

/**
 * Returns a stable per-calling-thread snapshot of the last complete ECU data
 * frame. The worker can continue polling without the GUI reading m_data while
 * librosco is updating it.
 */
mems_data* MEMSInterface::getData()
{
  return getData(nullptr);
}

mems_data* MEMSInterface::getData(Mems16Na::SessionEvidence *naEvidence)
{
  static thread_local mems_data snapshot;
  QMutexLocker locker(&m_dataMutex);
  snapshot = m_data;
  if (naEvidence) {
    if ((ecuFamily()==MemsEcuFamily::Rosco13_16 || ecuFamily()==MemsEcuFamily::Mems19) && isConnected())
      *naEvidence=m_naSnapshot;
    else naEvidence->reset();
  }
  return &snapshot;
}

QVariantMap MEMSInterface::mems2jValues() const
{
  QMutexLocker locker(&m_dataMutex);
  return m_mems2jValues;
}
QStringList MEMSInterface::mems2jFaults() const
{
  QMutexLocker locker(&m_dataMutex);
  return m_mems2jFaults;
}

/**
 * Responds to the parent thread being started by initializing the library
 * struct and emitting a signal indicating that the interface is ready.
 */
void MEMSInterface::onParentThreadStarted()
{
  if (m_shutdownRequested.loadAcquire() != 0)
  {
    QThread::currentThread()->quit();
    return;
  }

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
/**
 * Calls the library in a loop until commanded to stop.
 */
void MEMSInterface::runServiceLoop()
{
  const MemsEcuFamily family=ecuFamily();
  bool connected = family==MemsEcuFamily::Mems2J
      ? (m_mems2j && m_mems2j->isOpen())
      : (family==MemsEcuFamily::Mems3
             ? (m_mems3 && m_mems3->isOpen())
             : mems_is_connected(&m_memsinfo));
  m_connectedState.storeRelease(connected ? 1 : 0);

  m_serviceLoopRunning = true;
  m_instrumentOtherForced.storeRelease(0);
  // The instrument capabilities depend on the exact firmware, not the
  // vehicle of reference. Obtain D1 once in the normal diagnostic session
  // before publishing measurements, using the existing read transport.
  if (connected && (family==MemsEcuFamily::Rosco13_16 || family==MemsEcuFamily::Mems19) &&
      m_naSession.firmwareName().empty() &&
      MemsProtocolSafety::allowsGenericCommand(family,diagnosticMode(),0xD1u) &&
      m_disconnectRequested.loadAcquire()==0 && m_shutdownRequested.loadAcquire()==0)
      sendRoscoReadCommand(0xD1u);
  while (!m_stopPolling && !m_shutdownThread &&
         m_disconnectRequested.loadAcquire() == 0 &&
         m_shutdownRequested.loadAcquire() == 0 && connected)
  {
    if (ecuFamily()==MemsEcuFamily::Mems2J ||
        ecuFamily()==MemsEcuFamily::Mems3)
    {
      // Each non-ROSCO family owns its serial transport and packet decoder.
      const auto cancelled=[this]() {
        return m_disconnectRequested.loadAcquire()!=0 ||
               m_shutdownRequested.loadAcquire()!=0;
      };
      const bool ok=(family==MemsEcuFamily::Mems2J)
          ? (m_mems2j && m_mems2j->readCycle(cancelled))
          : (m_mems3 && m_mems3->readCycle(cancelled));
      if (!ok)
      {
        if (m_disconnectRequested.loadAcquire()==0 &&
            m_shutdownRequested.loadAcquire()==0)
          emit readError();
        break;
      }
      {
        QMutexLocker locker(&m_dataMutex);
        m_mems2jValues=family==MemsEcuFamily::Mems2J
            ? m_mems2j->values() : m_mems3->values();
        m_mems2jFaults=family==MemsEcuFamily::Mems2J
            ? m_mems2j->faults() : m_mems3->faults();
      }
      emit readSuccess();
      emit dataReady();
      QCoreApplication::processEvents();
      continue;
    }

    // Never keep m_dataMutex locked while librosco performs serial I/O.
    // mems_read() can block waiting for the ECU; holding the mutex here would
    // make GUI-side getData() wait on the serial timeout and can freeze the
    // whole window during the transition from diagnostic polling to Mode 4.
    mems_data nextData;
    {
      QMutexLocker locker(&m_dataMutex);
      nextData = m_data;
    }

    // Observe only the bytes already written by the existing communication trace.
    // No additional serial operation and no change to mems_read or its ABI.
    QString naTracePath;
    qint64 naTraceStart=-1;
    m_naSession.discardPair();
#ifdef WIN32
    if (family==MemsEcuFamily::Rosco13_16 || family==MemsEcuFamily::Mems19) {
      wchar_t path[4096];
      const DWORD n=GetEnvironmentVariableW(L"ECU_MEMS_COMM_TRACE_FILE",path,4096);
      if(n && n<4096) {
        naTracePath=QString::fromWCharArray(path,int(n));
        const QFileInfo info(naTracePath);
        if(info.isFile())naTraceStart=info.size();
      }
    }
#endif
    QElapsedTimer instrumentSampleClock;
    instrumentSampleClock.start();
    // The K-Line path reads the actual announced frame lengths and accounts
    // for the adapter's optional local echo. Leave legacy ROSCO untouched.
#ifdef MEMS_NATIVE_KLINE_19
    const bool readOk = (ecuFamily() == MemsEcuFamily::Mems19)
        ? mems_read_kline19(&m_memsinfo, &nextData, m_kline19LocalEcho)
        : mems_read(&m_memsinfo, &nextData);
#else
    const bool readOk = mems_read(&m_memsinfo, &nextData);
#endif

    if (readOk)
    {
      if (naTraceStart>=0) {
        QFile trace(naTracePath);
        if(trace.open(QIODevice::ReadOnly)) {
          const qint64 count=trace.size()-naTraceStart;
          if(count>0 && count<=8192 && trace.seek(naTraceStart)) {
            const QByteArray bytes=trace.read(count);
            if(bytes.size()==count && !m_naSession.identifiedProfile()) {
              Mems16Na::consumeExtendedPair(m_naSession,bytes.toStdString(),family==MemsEcuFamily::Mems19 && m_kline19LocalEcho,
                  nextData.engine_rpm,nextData.dtc2,nextData.dtc3,nextData.dtc4,nextData.dtc5);
            } else if(bytes.size()==count &&
               Mems16Na::consumeLivePair(m_naSession,bytes.toStdString())) {
              const auto a=m_naSession.frame80();
              const auto b=m_naSession.frame7d();
              // Verify the raw trace belongs to the same DLL sample.
              if(Mems16Na::word(a,1)!=nextData.engine_rpm ||
                 a.bytes[4]!=nextData.ambient_temp || a.bytes[6]!=nextData.fuel_temp ||
                 a.bytes[9]!=nextData.throttle_pot || a.bytes[13]!=nextData.dtc0 ||
                 a.bytes[14]!=nextData.dtc1 || a.bytes[16]!=nextData.idle_hot ||
                 b.bytes[2]!=nextData.throttle_angle || b.bytes[5]!=nextData.dtc2 ||
                 b.bytes[11]!=nextData.long_term_fuel_trim ||
                 b.bytes[20]!=nextData.idle_error2 || b.bytes[21]!=nextData.uk10 ||
                 b.bytes[31]!=nextData.uk1C)
                m_naSession.discardPair();
            }
          }
        }
      }
      m_naSession.observeCoolantFault((nextData.dtc0 & 0x01)!=0, QDateTime::currentMSecsSinceEpoch());
      const bool instrumentOk=serviceInstrumentTest(nextData,instrumentSampleClock.elapsed());
      {
        QMutexLocker locker(&m_dataMutex);
        m_data = nextData;
        m_naSnapshot=m_naSession;
      }
      if(!instrumentOk){
        emit readError();
        break;
      }
      emit readSuccess();
      emit dataReady();
    }
    else
    {
      // No more normal polling after a lost/invalid reply. The independent
      // firmware timeout will remove the force if F4 cannot be acknowledged.
      if(m_instrumentRequest.loadAcquire()!=0)finishInstrumentTest(2);
      if(m_disconnectRequested.loadAcquire()==0 && m_shutdownRequested.loadAcquire()==0)
        emit readError();
      // Une coupure du contact ou une perte série ne doit pas laisser
      // la boucle tourner indéfiniment. La déconnexion ci-dessous
      // libère proprement la session ; l'UI pourra retenter sans alerte.
      break;
    }
    // Do not dispatch queued adjustment/Mode 4/actuator requests while the
    // instrument is forced. Stop and disconnection use direct atomic flags.
    if(!m_instrumentActive)QCoreApplication::processEvents();
  }
  // Refuse a new GUI request before the final cancellation/transport close.
  m_connectedState.storeRelease(0);
  if(m_instrumentRequest.loadAcquire()!=0)finishInstrumentTest(2);
  m_serviceLoopRunning = false;

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
  if (connected && (family==MemsEcuFamily::Rosco13_16 ||
                    family==MemsEcuFamily::Mems19))
    mems_disconnect(&m_memsinfo);
  m_connectedState.storeRelease(0);
  if (m_shutdownRequested.loadAcquire() == 0)
    m_disconnectRequested.storeRelease(0);
  m_naSession.reset();
  { QMutexLocker locker(&m_dataMutex); m_naSnapshot.reset(); }
  emit disconnected();

  if (m_shutdownThread || m_shutdownRequested.loadAcquire() != 0)
  {
    QThread::currentThread()->quit();
  }
}

bool MEMSInterface::requestInstrumentTest(int action)
{
    if(action<1||action>3||!instrumentNormalMode()||instrumentOtherForced()||
       m_disconnectRequested.loadAcquire()!=0||m_shutdownRequested.loadAcquire()!=0||
       m_instrumentRequest.loadAcquire()!=0)return false;
    m_instrumentCancel.storeRelease(0);
    return m_instrumentRequest.testAndSetOrdered(0,action);
}

bool MEMSInterface::instrumentFamilyMatches(Mems218::InstrumentAction action) const
{
    return action==Mems218::InstrumentAction::Tachometer
        ?ecuFamily()==MemsEcuFamily::Mems19
        :ecuFamily()==MemsEcuFamily::Rosco13_16;
}

bool MEMSInterface::finishInstrumentTest(int result)
{
    const int action=m_instrumentRequest.loadAcquire();
    if(!action)return true;
    bool restored=true;
    if(m_instrumentActive){
        uint8_t reply=0xff;
        // The global cancellation is only used within our exclusive test,
        // and only in the verified normal protocol context.
        restored=m_initComplete&&mems_is_connected(&m_memsinfo)&&
            diagnosticMode()==MemsDiagnosticMode::Normal&&
            guardedMemsTestActuator(static_cast<actuator_cmd>(Mems218::instrumentRestoreCommand),&reply)&&reply==0;
        if(!restored)result=4;
    }
    m_instrumentActive=false;
    m_instrumentRequest.storeRelease(0);
    emit instrumentTestResult(action,result);
    // No traffic may refresh the ECU watchdog after an unconfirmed restore.
    if(!restored)m_stopPolling=true;
    return restored;
}

bool MEMSInterface::serviceInstrumentTest(const mems_data &sample,qint64 sampleMs)
{
    const int request=m_instrumentRequest.loadAcquire();
    if(!request)return true;
    const auto action=static_cast<Mems218::InstrumentAction>(request);
    const bool safe=instrumentNormalMode()&&instrumentFamilyMatches(action)&&
        !instrumentOtherForced()&&sampleMs<=Mems218::instrumentMaximumSampleMs&&
        sample.engine_rpm==0&&sample.uk3==0&&sample.ignition_switch==1&&
        Mems218::instrumentSafeSample(action,m_naSession)&&
        m_disconnectRequested.loadAcquire()==0&&m_shutdownRequested.loadAcquire()==0&&
        m_instrumentCancel.loadAcquire()==0;
    if(m_instrumentActive){
        if(!safe)return finishInstrumentTest(2);
        if(m_instrumentTimer.elapsed()>=Mems218::instrumentDurationMs)
            return finishInstrumentTest(1);
        return true;
    }
    if(!safe)return finishInstrumentTest(3);
    // Start only after a NEW complete pair read by this worker, never after
    // a cached GUI measurement. One start command; no automatic repetition.
    const unsigned command=Mems218::instrumentCommand(action);
    if(command>255)return finishInstrumentTest(3);
    uint8_t reply=0xff;
    m_instrumentActive=true; // even an absent reply may mean the force was applied
    m_instrumentTimer.start();
    if(!guardedMemsTestActuator(static_cast<actuator_cmd>(command),&reply)||reply!=0){
        finishInstrumentTest(5);
        // A failed start may have desynchronised the byte stream.
        m_stopPolling=true;
        return false;
    }
    return true;
}

bool MEMSInterface::actuatorOnOffDelayTest(actuator_cmd onCmd, actuator_cmd offCmd)
{
  bool status = false;

  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    const bool onOk = guardedMemsTestActuator( onCmd, NULL);
    if (onOk)
      QThread::sleep(1);

    // Toujours demander OFF par sécurité, même si l'accusé de la commande ON
    // a échoué. Le test n'est réussi que si ON et OFF ont tous les deux réussi.
    const bool offOk = guardedMemsTestActuator( offCmd, NULL);
    status = onOk && offOk;

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
    if (!guardedMemsTestActuator( MEMS_FuelPumpOn, NULL))
    {
      emit errorSendingCommand();
    }
  }
}

void MEMSInterface::onFuelPumpOff()
{
  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    if (!guardedMemsTestActuator( MEMS_FuelPumpOff, NULL))
    {
      emit errorSendingCommand();
    }
  }
}

void MEMSInterface::onPTCRelayOn()
{
  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    if (!guardedMemsTestActuator( MEMS_PTCRelayOn, NULL))
    {
      emit errorSendingCommand();
    }
  }
}

void MEMSInterface::onPTCRelayOff()
{
  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    if (!guardedMemsTestActuator( MEMS_PTCRelayOff, NULL))
    {
      emit errorSendingCommand();
    }
  }
}

void MEMSInterface::onACRelayOn()
{
  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    if (!guardedMemsTestActuator( MEMS_ACRelayOn, NULL))
    {
      emit errorSendingCommand();
    }
  }
}

void MEMSInterface::onACRelayOff()
{
  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    if (!guardedMemsTestActuator( MEMS_ACRelayOff, NULL))
    {
      emit errorSendingCommand();
    }
  }
}

void MEMSInterface::onIgnitionCoilTest()
{
  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    uint8_t reply=0;
    const bool received=guardedMemsTestActuator(MEMS_FireCoil, &reply);
    emit actuatorTestReply(true,received,int(reply),QString::fromStdString(m_naSession.firmwareName()));
    if(!received)emit errorSendingCommand();
  }
}

void MEMSInterface::onFuelInjectorTest()
{
  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    uint8_t reply=0;
    const bool received=guardedMemsTestActuator(MEMS_TestInjectors, &reply);
    emit actuatorTestReply(false,received,int(reply),QString::fromStdString(m_naSession.firmwareName()));
    if(!received)emit errorSendingCommand();
  }
}
void MEMSInterface::on_m_fuel_trim_plusButton_clicked()
{
 if (m_initComplete && mems_is_connected(&m_memsinfo))
 {
     if (!guardedMemsTestActuator(MEMS_FuelTrimPlus,NULL))
     {
       emit errorSendingCommand();
     }
}

}

 void MEMSInterface::on_m_fuel_trim_minusButton_clicked()
 {
  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
      if (!guardedMemsTestActuator( MEMS_FuelTrimMinus, NULL))
      {
        emit errorSendingCommand();
      }
 }
 }

  void MEMSInterface::on_m_idle_decay_plusButton_clicked()
  {
   if (m_initComplete && mems_is_connected(&m_memsinfo))
   {
       if (!guardedMemsTestActuator(MEMS_IdleDecayPlus, NULL))
       {
         emit errorSendingCommand();
       }
  }
  }

    void MEMSInterface::on_m_idle_decay_minusButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!guardedMemsTestActuator(MEMS_IdleDecayMinus, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }

    void MEMSInterface::on_m_idle_speed_plusButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!guardedMemsTestActuator(MEMS_IdleSpeedPlus, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }

    void MEMSInterface::on_m_idle_speed_minusButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!guardedMemsTestActuator(MEMS_IdleSpeedMinus, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
    void MEMSInterface::on_m_ignition_advance_plusButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!guardedMemsTestActuator(MEMS_IgnitionAdvancePlus, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
    void MEMSInterface::on_m_ignition_advance_minusButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!guardedMemsTestActuator(MEMS_IgnitionAdvanceMinus, NULL))
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
         if (!guardedMemsTestActuator(MEMS_PurgeValveOn, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
    void MEMSInterface::on_m_Purge_Valve_OffButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!guardedMemsTestActuator(MEMS_PurgeValveOff, NULL))
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
		if (!guardedMemsTestActuator(MEMS_O2HeaterOn, NULL))
         {
           emit errorSendingCommand();
         }
		/*  else 
		 {
			 QThread::sleep(5);
			 emit turnO2HeaterOff();
		 } */
		 /* if (guardedMemsTestActuator(MEMS_O2HeaterOn, NULL))
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
         if (!guardedMemsTestActuator(MEMS_O2HeaterOff, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
    void MEMSInterface::on_m_Boost_Valve_OnButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!guardedMemsTestActuator(MEMS_BoostValveOn, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
	void MEMSInterface::on_m_Boost_Valve_OffButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!guardedMemsTestActuator(MEMS_BoostValveOff, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
    void MEMSInterface::on_m_Fan1_OnButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!guardedMemsTestActuator(MEMS_Fan1On, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
    void MEMSInterface::on_m_Fan2_OnButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!guardedMemsTestActuator(MEMS_Fan2On, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
	void MEMSInterface::on_m_Fan3_OnButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!guardedMemsTestActuator(MEMS_Fan3On, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }

    void MEMSInterface::on_m_Fan1_OffButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!guardedMemsTestActuator(MEMS_Fan1Off, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
    void MEMSInterface::on_m_Fan2_OffButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!guardedMemsTestActuator(MEMS_Fan2Off, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
	void MEMSInterface::on_m_Fan3_OffButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!guardedMemsTestActuator(MEMS_Fan3Off, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
	
	void MEMSInterface::on_m_IACMinusButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!guardedMemsTestActuator(MEMS_CloseIAC, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
	void MEMSInterface::on_m_IACPlusButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!guardedMemsTestActuator(MEMS_OpenIAC, NULL))
         {
           emit errorSendingCommand();
         }
    }
    }
	
	void MEMSInterface::on_m_AllActuatorsOffButton_clicked()
    {
     if (m_initComplete && mems_is_connected(&m_memsinfo))
     {
         if (!guardedMemsTestActuator(MEMS_AllActuatorsOff, NULL))
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
void MEMSInterface::sendRoscoReadCommand(quint8 command)
{
  if (m_shutdownRequested.loadAcquire() != 0)
    return;

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
  if (!ecu_trace_write_file(h, &tx, 1, &written, NULL) || written != 1)
  {
    SetCommTimeouts(h, &oldTimeouts);
    emit errorSendingCommand();
    return;
  }

  // Responses are short single-frame ROSCO replies. Read until the
  // inter-byte timeout expires, with a hard upper bound.
  QElapsedTimer timer;
  timer.start();
  QElapsedTimer hardLimit;
  hardLimit.start();
  while (timer.elapsed() < 350 && hardLimit.elapsed() < 750 &&
         m_shutdownRequested.loadAcquire() == 0 && m_disconnectRequested.loadAcquire() == 0)
  {
    unsigned char b = 0;
    DWORD n = 0;
    if (ecu_trace_read_file(h, &b, 1, &n, NULL) && n == 1)
    {
      response.append(char(b));
      timer.restart();
    }
    else
    {
      // Do not dispatch queued serial operations in the middle of D1.
      if (command != 0xD1u) QCoreApplication::processEvents();
    }
  }

  SetCommTimeouts(h, &oldTimeouts);

  // Remove only the extra cable-local echo. Preserve the ECU response
  // (command echo + payload) for D1, D2 and F0 and the existing test page.
  if (ecuFamily() == MemsEcuFamily::Mems19)
      response = Mems19Protocol::normalizedReply(response, command,
                                                m_kline19LocalEcho);
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
  QElapsedTimer hardLimit;
  hardLimit.start();
  while (timer.elapsed() < 350 && hardLimit.elapsed() < 750 &&
         m_shutdownRequested.loadAcquire() == 0 && m_disconnectRequested.loadAcquire() == 0)
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

  if (m_shutdownRequested.loadAcquire() == 0) {
    if ((ecuFamily()==MemsEcuFamily::Rosco13_16 || ecuFamily()==MemsEcuFamily::Mems19) && (command==0xD0 || command==0xD1)) {
      m_naSession.tx({command});
      m_naSession.rx(std::vector<std::uint8_t>(response.begin(),response.end()));
      m_naSession.finish();
      // Do not publish the old measurement with a newly selected profile.
      // The next successful poll publishes a consistent new snapshot.
      QMutexLocker locker(&m_dataMutex);
      m_naSnapshot=m_naSession;
    }
    emit protocolResponse(command, response);
  }
}
