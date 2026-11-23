#ifndef ECU_COMMUNICATION_LOG_H
#define ECU_COMMUNICATION_LOG_H
#include <QString>
/* GUI-thread lifecycle for an independent file per connection attempt. */
class EcuCommunicationLog
{
public:
    static bool beginAttempt();
    static void identify(const QString &family,const QString &ecuId=QString());
    static void note(const QString &message);
    static void finish();
    static QString currentPath();
};
#endif
