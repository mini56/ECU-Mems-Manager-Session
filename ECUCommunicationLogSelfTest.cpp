#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include "ecucommunicationlog.h"
#include "ecucommunicationtraceio.h"
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
    if(!directory.entryList(QStringList()<<QStringLiteral("*ECU-Non-Identifie_Communications_ECU.txt"),
                             QDir::Files).size())return fail(11,directory.absolutePath()+QStringLiteral(" | ")+directory.entryList(QDir::Files).join(QStringLiteral(" ; ")));
    std::puts("ECU_COMMUNICATION_LOG_CREATE_APPEND_IDENTIFY_NO_OVERWRITE_PASS");
    return 0;
}
