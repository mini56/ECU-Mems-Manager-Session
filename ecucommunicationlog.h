#ifndef ECU_COMMUNICATION_LOG_H
#define ECU_COMMUNICATION_LOG_H
#include <QString>
/* GUI-thread lifecycle: provisional handshake trace, durable file only for an established session. */
class EcuCommunicationLog
{
public:
    static bool beginAttempt();
    static void confirmSession();
    static void identify(const QString &family,const QString &ecuId=QString());
    static void note(const QString &message);
    static void finish();
    static QString currentPath();
};
#endif
