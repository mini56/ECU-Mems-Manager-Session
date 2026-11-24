#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include "memsinterface.h"


class Logger
{
public:
    Logger(MEMSInterface *memsiface);
    bool openLog(QString fileName);
    void closeLog();
    void logData();
    bool logData(const mems_data *data,const Mems16Na::SessionEvidence &na);
    void endNaSession();
    QString getLogPath();
    void setTemperatureUnits(TemperatureUnits type) { m_tempUnits = type; }
	bool EcuIdTransmitted(QString id);
	QString ecuid;
private:
    uint8_t convertTemp(uint8_t degrees);
    MEMSInterface *m_mems;
    QString m_logExtension;
    QString m_logDir;
    QFile m_logFile;
    QTextStream m_logFileStream;
    QString m_lastAttemptedLog;
    QString m_requestedName;
    QString m_naFirmware;
    QString m_naIdentity;
    bool m_recording = false;
    bool m_forceUnique = false;
    bool openSegment(const Mems16Na::SessionEvidence &na);
    bool openNaSegment(const Mems16Na::SessionEvidence &na);
    QString uniqueLogPath(const QString &stem) const;
    void writeLegacyRow(const mems_data *data);
    TemperatureUnits m_tempUnits;
};

#endif // LOGGER_H

