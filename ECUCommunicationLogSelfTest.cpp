#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include "ecucommunicationlog.h"
#include "ecucommunicationtraceio.h"
#include "ecureconnectpolicy.h"
#include <cstdio>

static int fail(int stage,const QString &detail)
{
    const QByteArray utf8=detail.toUtf8();
    std::fprintf(stderr,"ECU_COMMUNICATION_LOG_SELFTEST_STAGE_%02d_FAILED: %s\n",stage,utf8.constData());
    std::fflush(stderr);
    return stage;
}

int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv);
    QStandardPaths::setTestModeEnabled(true);
    QString docs=QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if(docs.isEmpty())docs=QDir::homePath();
    const QDir directory(QDir(docs).filePath(QStringLiteral("ECU MEMS Manager/logs")));
    if(!EcuCommunicationLog::beginAttempt())return fail(1,QStringLiteral("First beginAttempt failed"));
    const QString firstTemp=EcuCommunicationLog::currentPath();
    if(!QFileInfo::exists(firstTemp))return fail(2,firstTemp);
    const uint8_t tx[]={0x81,0xA5};
    const uint8_t rx[]={0x81,0x00};
    ecu_trace_bytes("TX",tx,sizeof(tx));
    ecu_trace_bytes("RX",rx,sizeof(rx));
    EcuCommunicationLog::identify(QStringLiteral("MEMS-1.6"),QStringLiteral("AANMP002"));
    EcuCommunicationLog::confirmSession();
    EcuCommunicationLog::finish();
    if(QFileInfo::exists(firstTemp))return fail(3,firstTemp);
    const QStringList firstFiles=directory.entryList(
       QStringList()<<QStringLiteral("*MEMS-1.6_AANMP002_Communications_ECU.txt"),QDir::Files);
    if(firstFiles.isEmpty())return fail(4,directory.absolutePath()+QStringLiteral(" | ")+directory.entryList(QDir::Files).join(QStringLiteral(" ; ")));
    const QString firstPath=directory.filePath(firstFiles.last());
    QFile a(firstPath);
    if(!a.open(QIODevice::ReadOnly|QIODevice::Text))return fail(5,a.errorString());
    const QByteArray firstText=a.readAll();
    a.close();
    if(!firstText.contains("TX | 81 A5") || !firstText.contains("RX | 81 00"))return fail(6,QString::fromUtf8(firstText));
    if(!EcuCommunicationLog::beginAttempt())return fail(7,QStringLiteral("Second beginAttempt failed"));
    const QString secondTemp=EcuCommunicationLog::currentPath();
    if(firstPath==secondTemp)return fail(8,firstPath);
    EcuCommunicationLog::finish();
    QFile b(firstPath);
    if(!b.open(QIODevice::ReadOnly|QIODevice::Text))return fail(9,b.errorString());
    if(b.readAll()!=firstText)return fail(10,firstPath);
    if(QFileInfo::exists(secondTemp))return fail(11,QStringLiteral("Failed attempt left a provisional file"));
    const QStringList before=directory.entryList(QStringList()<<QStringLiteral("*Communications_ECU.txt"),QDir::Files);
    for(int i=0;i<44;++i){
        if(!EcuCommunicationLog::beginAttempt())return fail(12,QStringLiteral("Retry trace failed"));
        const QString pending=EcuCommunicationLog::currentPath();
        if(i%2==0)ecu_trace_bytes("TX",tx,sizeof(tx));
        EcuCommunicationLog::note(QStringLiteral("ECHEC_CONNEXION COM3"));
        EcuCommunicationLog::finish();
        if(QFileInfo::exists(pending))return fail(13,pending);
    }
    // Qt 5.15.2's QList equality instantiates a removed MSVC stdext iterator.
    // Compare the same ordered file names individually, not only the count.
    const QStringList after=directory.entryList(QStringList()<<QStringLiteral("*Communications_ECU.txt"),QDir::Files);
    if(after.size()!=before.size())
        return fail(14,QStringLiteral("Failed attempts created permanent journals"));
    for(int i=0;i<before.size();++i)
        if(after.at(i)!=before.at(i))
            return fail(14,QStringLiteral("Failed attempts changed permanent journal names"));
    using Outcome=EcuReconnectPolicy::Outcome;
    EcuReconnectPolicy policy;
    if(!policy.startManual() || policy.failed()!=Outcome::ManualFailure || policy.startRetry())
        return fail(15,QStringLiteral("Initial failure must not arm a retry"));
    if(!policy.startManual() || !policy.established() || !policy.cancel() ||
       policy.closed()!=Outcome::None || policy.startRetry())
        return fail(16,QStringLiteral("Manual disconnect must not arm a retry"));
    if(!policy.startManual() || !policy.established() || !policy.lost() || policy.startRetry() ||
       policy.closed()!=Outcome::Retry)
        return fail(17,QStringLiteral("Must await worker closure"));
    for(int i=1;i<=3;++i){
        if(!policy.startRetry() || policy.attempts()!=i ||
           policy.failed()!=(i==3?Outcome::Alert:Outcome::Retry))
            return fail(18,QStringLiteral("Exactly three retries then one alert"));
    }
    if(!policy.idle() || policy.startRetry() || policy.failed()!=Outcome::None)
        return fail(19,QStringLiteral("Retry exhaustion must remain stopped"));
    if(!policy.startManual() || !policy.established() || !policy.lost() ||
       policy.closed()!=Outcome::Retry || policy.cancel() || policy.startRetry())
        return fail(20,QStringLiteral("Cancel during waiting must stop retries"));
    if(!policy.startManual() || !policy.established() || !policy.lost() ||
       policy.closed()!=Outcome::Retry || !policy.startRetry() || !policy.cancel() ||
       policy.established() || policy.failed()!=Outcome::None || !policy.idle())
        return fail(21,QStringLiteral("Cancel during handshake must remain stopped"));
    if(!policy.startManual() || !policy.established() || !policy.lost() ||
       policy.closed()!=Outcome::Retry || !policy.startRetry() || !policy.established())
        return fail(22,QStringLiteral("Automatic reconnect must still work"));
    policy.receivedData();
    if(policy.attempts()!=0 || !policy.lost() || policy.closed()!=Outcome::Retry ||
       !policy.startRetry() || policy.attempts()!=1)
        return fail(23,QStringLiteral("New genuine loss must have fresh retry budget"));
    if(!policy.established() || !policy.lost() || policy.closed()!=Outcome::Retry ||
       !policy.startRetry() || policy.attempts()!=2 ||
       !policy.established() || !policy.lost() || policy.closed()!=Outcome::Retry ||
       !policy.startRetry() || policy.attempts()!=3 ||
       !policy.established() || !policy.lost() || policy.closed()!=Outcome::Alert)
        return fail(24,QStringLiteral("Handshake alone must not reset retry budget"));
    if(!policy.startManual() || !policy.cancel() || policy.established() ||
       policy.closed()!=Outcome::None || !policy.idle())
        return fail(25,QStringLiteral("Queued initial connection must remain cancelled"));
    if(!EcuCommunicationLog::beginAttempt())return fail(26,QStringLiteral("New successful session trace failed"));
    EcuCommunicationLog::identify(QStringLiteral("MEMS-1.6"),QStringLiteral("AANMP002"));
    ecu_trace_bytes("TX",tx,sizeof(tx));
    EcuCommunicationLog::confirmSession();
    EcuCommunicationLog::finish();
    if(directory.entryList(QStringList()<<QStringLiteral("*Communications_ECU.txt"),QDir::Files).size()!=before.size()+1)
        return fail(27,QStringLiteral("New successful session must create exactly one file"));
    QFile unchanged(firstPath);
    if(!unchanged.open(QIODevice::ReadOnly|QIODevice::Text) || unchanged.readAll()!=firstText)
        return fail(28,QStringLiteral("Previous successful session changed"));
    std::puts("ECU_RECONNECT_POLICY_THREE_RETRIES_CANCEL_NO_FILE_FLOOD_PASS");
    std::puts("ECU_COMMUNICATION_LOG_CREATE_APPEND_IDENTIFY_NO_OVERWRITE_PASS");
    return 0;
}
