#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include "ecucommunicationlog.h"
#include "ecucommunicationtraceio.h"
#include <cstdio>

int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv);
    QString docs=QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if(docs.isEmpty())docs=QDir::homePath();
    const QDir directory(QDir(docs).filePath(QStringLiteral("ECU MEMS Manager/logs")));
    if(!EcuCommunicationLog::beginAttempt())return 1;
    const QString firstTemp=EcuCommunicationLog::currentPath();
    if(!QFileInfo::exists(firstTemp))return 2;
    const uint8_t tx[]={0x81,0xA5};
    const uint8_t rx[]={0x81,0x00};
    ecu_trace_bytes("TX",tx,sizeof(tx));
    ecu_trace_bytes("RX",rx,sizeof(rx));
    EcuCommunicationLog::identify(QStringLiteral("MEMS-1.6"),QStringLiteral("AANMP002"));
    EcuCommunicationLog::finish();
    if(QFileInfo::exists(firstTemp))return 3;
    const QStringList firstFiles=directory.entryList(
       QStringList()<<QStringLiteral("*MEMS-1.6_AANMP002_Communications_ECU.txt"),QDir::Files);
    if(firstFiles.isEmpty())return 4;
    const QString firstPath=directory.filePath(firstFiles.last());
    QFile a(firstPath);
    if(!a.open(QIODevice::ReadOnly|QIODevice::Text))return 5;
    const QByteArray firstText=a.readAll();
    a.close();
    if(!firstText.contains("TX | 81 A5") || !firstText.contains("RX | 81 00"))return 6;
    if(!EcuCommunicationLog::beginAttempt())return 7;
    const QString secondTemp=EcuCommunicationLog::currentPath();
    if(firstPath==secondTemp)return 8;
    EcuCommunicationLog::finish();
    QFile b(firstPath);
    if(!b.open(QIODevice::ReadOnly|QIODevice::Text))return 9;
    if(b.readAll()!=firstText)return 10;
    if(!directory.entryList(QStringList()<<QStringLiteral("*ECU-Non-Identifie_Communications_ECU.txt"),
                             QDir::Files).size())return 11;
    std::puts("ECU_COMMUNICATION_LOG_CREATE_APPEND_IDENTIFY_NO_OVERWRITE_PASS");
    return 0;
}
