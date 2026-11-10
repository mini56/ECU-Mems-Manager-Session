#ifndef INJECTION_RAM_TEST_LOGGER_H
#define INJECTION_RAM_TEST_LOGGER_H

#include <QString>

class QGroupBox;
class QGridLayout;
class QPlainTextEdit;

// Native AANMP002 test reporting. The actual dialog signals call these
// functions directly, without scanning every open window or polling timers.
namespace InjectionRamTestLog {
QString directory();
bool isFinalLog(const QString &text);
bool writeFinalText(const QString &text,const QString &destinationDirectory,
                    QString *savedPath=nullptr);
void beginPhase(QPlainTextEdit *output);
void saveFinishedPhase(QPlainTextEdit *output);
void addFolderButton(QGroupBox *session,QGridLayout *grid);
}

#endif
