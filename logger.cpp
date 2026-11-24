#include <QDir>
#include <QDateTime>
#include <QStandardPaths>
#include "logger.h"
#include "mems16na_log_schema.h"
#include <QFileInfo>

/**
 * Constructor. Sets the interface class pointer as
 * well as log directory and log file extension.
 */
Logger::Logger(MEMSInterface* memsiface):
m_logExtension(".txt")
{
  m_mems = memsiface;
  QString documents = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  if (documents.isEmpty())
    documents = QDir::homePath();
  m_logDir = QDir(documents).filePath(QStringLiteral("ECU MEMS Manager/logs"));
}

/**
 * Gets EcuId
 */
/* void Logger::EcuIdTransmitted(uint8_t* id)
{
	char idString[20];
	sprintf(idString, " %02X %02X %02X %02X", id[0], id[1], id[2], id[3]);
	ecuid = idString;
} */
bool Logger::EcuIdTransmitted(QString id)
{
	ecuid = id;
	return true;
}

/**
 * Attempts to open a log file with the name specified.
 * @return True on success, false otherwise
 */
bool Logger::openLog(QString fileName)
{
  if(m_recording)return false;
  m_requestedName=fileName;
  m_forceUnique=false;
  Mems16Na::SessionEvidence na;
  m_mems->getData(&na);
  m_recording=openSegment(na);
  return m_recording;
}

QString Logger::uniqueLogPath(const QString &stem) const
{
  const QDir dir(m_logDir);
  QString path=dir.filePath(stem+m_logExtension);
  for(int i=1;QFileInfo::exists(path);++i)
    path=dir.filePath(stem+QStringLiteral("_%1").arg(i)+m_logExtension);
  return path;
}

bool Logger::openNaSegment(const Mems16Na::SessionEvidence &na)
{
  const auto *profile=na.identifiedProfile();
  if(!profile || !QDir().mkpath(m_logDir))return false;
  const QString firmware=QString::fromLatin1(profile->firmware);
  const QString identity=QString::fromStdString(Mems16Na::LogV2::hex(na.identity().data(),na.identity().size()));
  const QDateTime now=QDateTime::currentDateTime();
  m_lastAttemptedLog=uniqueLogPath(m_requestedName+QStringLiteral("_MEMS16NA_")+firmware+
      QStringLiteral("_")+now.toString(QStringLiteral("yyyyMMdd_HHmmss_zzz")));
  m_logFile.setFileName(m_lastAttemptedLog);
  // Exclusive creation: even another process cannot cause an old file to be appended.
  if(!m_logFile.open(QIODevice::WriteOnly|QIODevice::NewOnly))return false;
  m_logFileStream.setDevice(&m_logFile);
  m_logFileStream.resetStatus();
  m_naFirmware=firmware;m_naIdentity=identity;
  m_logFileStream<<QString::fromLatin1(Mems16Na::LogV2::marker())<<"\n"
    <<"# ECU_ID="<<identity<<"\n"
    <<"# FIRMWARE="<<firmware<<"\n"
    <<"# STARTED="<<now.toString(Qt::ISODateWithMs)<<"\n"
    <<QString::fromStdString(Mems16Na::LogV2::header())<<endl;
  if(m_logFileStream.status()!=QTextStream::Ok || !m_logFile.flush()) {
    m_logFile.close();m_naFirmware.clear();m_naIdentity.clear();return false;
  }
  return true;
}

bool Logger::openSegment(const Mems16Na::SessionEvidence &na)
{
  m_naFirmware.clear();m_naIdentity.clear();
  if(na.identifiedProfile())return openNaSegment(na);
  bool success = false;
  const QString stem=m_forceUnique?m_requestedName+QStringLiteral("_")+QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz")):m_requestedName;
  m_lastAttemptedLog = m_forceUnique?uniqueLogPath(stem):QDir(m_logDir).filePath(stem+m_logExtension);

  // Enregistre dans les Documents utilisateur, sans ecrire dans Program Files.
  if (!m_logFile.isOpen() && QDir().mkpath(m_logDir))
  {
    // set the name of the log file and open it for writing
    bool alreadyExists = QFileInfo(m_lastAttemptedLog).exists();

    m_logFile.setFileName(m_lastAttemptedLog);
    if (m_logFile.open(m_forceUnique?(QIODevice::WriteOnly|QIODevice::NewOnly):(QIODevice::WriteOnly|QIODevice::Append)))
    {
      m_logFileStream.setDevice(&m_logFile);
      m_logFileStream.resetStatus();

      if (!alreadyExists)
      {

       m_logFileStream << ecuid << endl;
	   m_logFileStream << "#time,"
                          "80x01-02_engine-rpm,"
                          "80x03_coolant_temp,"
                          "80x04_ambient_temp,"
                          "80x05_intake_air_temp,"
                          "80x06_fuel_temp,"
                          "80x07_map_kpa,"
                          "80x08_battery_voltage,"
                          "80x09_throttle_pot,"
                          "80x0A_idle_switch,"
                          "80x0B_uk1,"
                          "80x0C_park_neutral_switch,"
                          "80x0D-0E_fault_codes,"
                          "80x0F_idle_set_point,"
                          "80x10_idle_hot,"
                          "80x11_uk2,"
                          "80x12_iac_position,"
                          "80x13-14_idle_error,"
                          "80x15_ignition_advance_offset,"
                          "80x16_ignition_advance,"
                          "80x17-18_coil_time,"
                          "80x19_crankshaft_position_sensor,"
                          "80x1A_uk4,"
                          "80x1B_uk5,"
                          "7dx01_ignition_switch,"
                          "7dx02_throttle_angle,"
                          "7dx03_uk6,"
                          "7dx04_air_fuel_ratio,"
                          "7dx05_dtc2,"
                          "7dx06_lambda_voltage,"
                          "7dx07_lambda_sensor_frequency,"
                          "7dx08_lambda_sensor_dutycycle,"
                          "7dx09_lambda_sensor_status,"
                          "7dx0A_closed_loop,"
                          "7dx0B_long_term_fuel_trim,"
                          "7dx0C_short_term_fuel_trim,"
                          "7dx0D_carbon_canister_dutycycle,"
                          "7dx0E_dtc3,"
                          "7dx0F_idle_base_pos,"
                          "7dx10_uk7,"
                          "7dx11_dtc4,"
                          "7dx12_ignition_advance2,"
                          "7dx13_idle_speed_offset,"
                          "7dx14-15_idle_error_hot_corrected,"
                          "7dx14-15_raw,"
                          "7dx16_dtc5,"
                          "7dx17_uk11,"
                          "7dx18_uk12,"
                          "7dx19_uk13,"
                          "7dx1A_uk14,"
                          "7dx1B_uk15,"
                          "7dx1C_uk16,"
                          "7dx1D_uk17,"
                          "7dx1E_uk18,"
                          "7dx1F_uk19" 
						 /*  "80x0D_dtc0,"
						  "80x0E_dtc1" */ << endl;
    }
      success = true;
    }
  }

  return success;
}

/**
 * Close the log file.
 */
void Logger::closeLog()
{
  m_logFileStream.flush();
  m_logFile.close();
  m_recording=false;
  m_naFirmware.clear();m_naIdentity.clear();
}

void Logger::endNaSession()
{
  if(!m_recording||m_naFirmware.isEmpty())return;
  m_logFileStream.flush();
  m_logFile.close();
  m_naFirmware.clear();m_naIdentity.clear();
  m_forceUnique=true;
  // Retain the existing recording request; a later session gets a new file.
}

/**
 * Converts degrees F to degrees C if necessary
 */
/* uint8_t Logger::convertTemp(uint8_t degrees)
{
  if (m_tempUnits == Celsius)
  {
    return ((degrees - 32) / 1.8);
  }
  else
  {
    return degrees;
  }
} */

/**
 * Converts lambda signal to mV
 */
/* uint8_t Logger::convertLambda(uint8_t mV)
{
  if (m_LScale == _4mV_steps)
  {
    return (mV * 4);
  }
  else
  {
    return (mV * 5);
  }
} */

/**
 * Commands the logger to query the interface for the currently
 * buffered data, and write it to the file.
 */
void Logger::logData()
{
  Mems16Na::SessionEvidence na;
  const mems_data *data=m_mems->getData(&na);
  logData(data,na);
}

bool Logger::logData(const mems_data *data,const Mems16Na::SessionEvidence &na)
{
  if(!m_recording||!data)return true;
  const auto *profile=na.identifiedProfile();
  const QString firmware=profile?QString::fromLatin1(profile->firmware):QString();
  const QString identity=profile?QString::fromStdString(Mems16Na::LogV2::hex(na.identity().data(),na.identity().size())):QString();
  if(!m_logFile.isOpen()||firmware!=m_naFirmware||identity!=m_naIdentity) {
    m_logFileStream.flush();m_logFile.close();m_forceUnique=true;
    if(!openSegment(na)){closeLog();return false;}
  }
  if(!firmware.isEmpty()) {
    const auto now=QDateTime::currentDateTime();
    const auto fields=Mems16Na::LogV2::fields(na,now.toString(Qt::ISODateWithMs).toStdString());
    m_logFileStream<<now.toString(QStringLiteral("hh:mm:ss.zzz"))
      <<QString::fromStdString(Mems16Na::LogV2::csv(fields,false))<<endl;
  } else writeLegacyRow(data);
  if(m_logFileStream.status()!=QTextStream::Ok||!m_logFile.flush()) {closeLog();return false;}
  return true;
}

void Logger::writeLegacyRow(const mems_data *data)
{
  if (m_logFile.isOpen() && (m_logFileStream.status() == QTextStream::Ok))
      {
        m_logFileStream << QDateTime::currentDateTime().toString("hh:mm:ss") << "," <<
      data->engine_rpm << "," <<
/*    convertTemp(data->coolant_temp) << "," <<
      convertTemp(data->ambient_temp)<< "," <<
      convertTemp(data->intake_air_temp)<< "," <<
      convertTemp(data->fuel_temp)<< "," << */
	  data->coolant_temp - 55<< "," <<
      data->ambient_temp - 55<< "," <<
      data->intake_air_temp - 55<< "," <<
      data->fuel_temp - 55<< "," <<
      data->map_kpa<< "," <<
      data->battery_voltage / 10.0<< "," <<
      data->throttle_pot * 0.02<< "," <<
      data->idle_switch<< "," <<
      data->uk1<< "," <<
      data->park_neutral_switch<< "," <<
      // data->fault_codes<< "," <<
/*    data->dtc0<< "," <<
	  data->dtc1<< "," << */
	  (((uint16_t)data->dtc1 << 8 ) | data->dtc0)<< "," <<
	  data->idle_set_point<< "," <<
      data->idle_hot<< "," <<
      data->uk2<< "," <<
      data->iac_position<< "," <<
      data->idle_error<< "," <<
      data->ignition_advance_offset<< "," <<
      (data->ignition_advance * 0.5) - 24<< "," <<
      data->coil_time * 0.002<< "," <<
      data->uk3<< "," <<
      data->uk4<< "," <<
      data->uk5<< "," <<
      data->ignition_switch<< "," <<
      data->throttle_angle<< "," <<
      data->uk6<< "," <<
      data->air_fuel_ratio<< "," <<
      data->dtc2<< "," <<
      // convertLambda(data->lambda_voltage)<< "," <<
	  data->lambda_voltage * 5<< "," <<
      data->lambda_sensor_frequency<< "," <<
      data->lambda_sensor_dutycycle<< "," <<
      data->lambda_sensor_status<< "," <<
      data->closed_loop<< "," <<
      data->long_term_fuel_trim<< "," <<
      data->short_term_fuel_trim<< "," <<
      data->carbon_canister_dutycycle<< "," <<
      data->dtc3<< "," <<
      data->idle_base_pos<< "," <<
      data->uk7<< "," <<
      data->dtc4<< "," <<
      data->ignition_advance2<< "," <<
      data->idle_speed_offset<< "," <<
      (((((uint16_t)data->idle_error2 << 8 ) | data->uk10) - 32768) +
       (static_cast<int>(data->idle_hot) - 35))<< "," <<
      (((uint16_t)data->idle_error2 << 8 ) | data->uk10)<< "," <<
      data->dtc5<< "," <<
      data->uk11<< "," <<
      data->uk12<< "," <<
      data->uk13<< "," <<
      data->uk14<< "," <<
      data->uk15<< "," <<
      data->uk16<< "," <<
      data->uk1A<< "," <<
      data->uk1B<< "," <<
      data->uk1C<</*  "," <<
	  data->dtc0<< "," <<
	  data->dtc1<< */ endl;
    }
}
/**
 * Returns the full path to the last log that we attempted to open.
 * @return Full path to last log file
 */
   QString Logger::getLogPath()
 {
   return m_lastAttemptedLog;
 }
