#include "ecucommunicationlog.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <string>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
namespace {
QString activePath,startStamp,familyName,ecuIdentifier;
bool sessionConfirmed=false;
QString safe(QString s){
    s=s.trimmed();
    for(QChar &c:s)if(!c.isLetterOrNumber()&&c!=QLatin1Char('-')&&c!=QLatin1Char('_')&&c!=QLatin1Char('.'))
        c=QLatin1Char('-');
    return s.left(80);
}
void setTrace(const QString &path){
#ifdef Q_OS_WIN
    const std::wstring w=path.toStdWString();
    SetEnvironmentVariableW(L"ECU_MEMS_COMM_TRACE_FILE",path.isEmpty()?nullptr:w.c_str());
#else
    if(path.isEmpty())qunsetenv("ECU_MEMS_COMM_TRACE_FILE");
    else qputenv("ECU_MEMS_COMM_TRACE_FILE",QFile::encodeName(path));
#endif
}
QString unique(const QDir &d,const QString &base,const QString &extension=QStringLiteral(".txt")){
    QString p=d.filePath(base+extension);
    for(int i=1;QFileInfo::exists(p);++i)
        p=d.filePath(base+QStringLiteral("_")+QString::number(i)+extension);
    return p;
}
}
bool EcuCommunicationLog::beginAttempt(){
    finish();
    QString docs=QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if(docs.isEmpty())docs=QDir::homePath();
    QDir dir(QDir(docs).filePath(QStringLiteral("ECU MEMS Manager/logs")));
    if(!dir.exists()&&!dir.mkpath(QStringLiteral(".")))return false;
    startStamp=QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss-zzz"));
    familyName=QStringLiteral("ECU-Non-Identifie");ecuIdentifier.clear();
    sessionConfirmed=false;
    // The DLL needs a real trace file before the handshake (including NA
    // frame validation). A provisional name keeps failed attempts out of
    // the user's saved TXT recordings.
    activePath=unique(dir,startStamp+QStringLiteral("_ECU-Non-Identifie_Communications_ECU"),QStringLiteral(".txt.part"));
    QFile f(activePath);
    if(!f.open(QIODevice::WriteOnly|QIODevice::NewOnly|QIODevice::Text)){
        activePath.clear();return false;
    }
    f.write("# ECU MEMS Manager - journal automatique des communications TX/RX\r\n");
    f.write("# Octets bruts; les echos de cable et ECU sont conserves.\r\n");
    f.close();
    setTrace(activePath);
    note(QStringLiteral("CONNEXION_DEMANDEE"));
    return true;
}
void EcuCommunicationLog::confirmSession(){
    if(!activePath.isEmpty()){sessionConfirmed=true;note(QStringLiteral("CONNEXION_ETABLIE"));}
}
void EcuCommunicationLog::identify(const QString &family,const QString &id){
    if(activePath.isEmpty())return;
    if(!family.trimmed().isEmpty())familyName=safe(family);
    if(!id.trimmed().isEmpty())ecuIdentifier=safe(id);
    note(QStringLiteral("ECU=%1 ID=%2").arg(familyName,
                     ecuIdentifier.isEmpty()?QStringLiteral("non-identifie"):ecuIdentifier));
}
void EcuCommunicationLog::note(const QString &message){
    if(activePath.isEmpty())return;
    QFile f(activePath);
    if(!f.open(QIODevice::Append|QIODevice::WriteOnly|QIODevice::Text))return;
    const QString line=QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"))
        +QStringLiteral(" | INFO | ")+QString(message).replace(QLatin1Char('\n'),QLatin1Char(' '))
        +QStringLiteral("\r\n");
    f.write(line.toUtf8());
}
void EcuCommunicationLog::finish(){
    if(activePath.isEmpty()){setTrace(QString());return;}
    note(QStringLiteral("SESSION_TERMINEE"));
    setTrace(QString());
    if(!sessionConfirmed){
        QFile::remove(activePath);
        activePath.clear();familyName.clear();ecuIdentifier.clear();
        return;
    }
    const QDir dir=QFileInfo(activePath).absoluteDir();
    const QString base=startStamp+QLatin1Char('_')+safe(familyName)
        +(ecuIdentifier.isEmpty()?QString():QStringLiteral("_")+safe(ecuIdentifier))
        +QStringLiteral("_Communications_ECU");
    // Promote the completed confirmed trace without overwriting older recordings.
    const QString natural=dir.filePath(base+QStringLiteral(".txt"));
    const QString target=natural==activePath?activePath:unique(dir,base);
    if(activePath!=target)QFile::rename(activePath,target);
    activePath.clear();familyName.clear();ecuIdentifier.clear();sessionConfirmed=false;
}
QString EcuCommunicationLog::currentPath(){return activePath;}
