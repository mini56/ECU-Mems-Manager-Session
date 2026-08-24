#ifndef DIAGNOSTICLOGGER_H
#define DIAGNOSTICLOGGER_H

#include <QString>

namespace DiagnosticLogger
{
void initialize();
void log(const QString &message);
void checkpoint(const QString &name);
QString logPath();
}

#endif // DIAGNOSTICLOGGER_H
