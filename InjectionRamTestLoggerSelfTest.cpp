#include "injectionramtestlogger.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QDebug>
#include <cstdio>

static bool require(bool condition,const char *label)
{
    if(!condition) std::fprintf(stderr,"FAIL AANMP002 log self-test: %s\n",label);
    return condition;
}

int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv);
    QTemporaryDir tmp;
    if(!require(tmp.isValid(),"temporary directory creation")) return 1;

    const QString phase1=QStringLiteral("TEST RAM TEMPS INJECTION — AANMP002\nPHASE 1 OK");
    const QString phase2=QStringLiteral("TEST DYNAMIQUE RAM TEMPS INJECTION — AANMP002\nPHASE 2 OK");

    bool ok=true;
    QString first,second;
    ok=require(InjectionRamTestLog::writeFinalText(phase1,tmp.path(),&first),
               "first phase is saved")&&ok;
    ok=require(InjectionRamTestLog::writeFinalText(phase2,tmp.path(),&second),
               "second phase is saved")&&ok;
    ok=require(first!=second && QFileInfo::exists(first) && QFileInfo::exists(second),
               "each phase must have its own file")&&ok;

    QFile file1(first),file2(second);
    ok=require(file1.open(QIODevice::ReadOnly|QIODevice::Text) &&
               file2.open(QIODevice::ReadOnly|QIODevice::Text),
               "both logs must be readable")&&ok;
    if(file1.isOpen() && file2.isOpen()){
        ok=require(QString::fromUtf8(file1.readAll())==phase1+QLatin1Char('\n'),
                   "phase 1 contents changed")&&ok;
        ok=require(QString::fromUtf8(file2.readAll())==phase2+QLatin1Char('\n'),
                   "phase 2 contents changed")&&ok;
    }
    QString skipped;
    ok=require(!InjectionRamTestLog::writeFinalText(
                   QStringLiteral("Phase 2 : préparation de la capture rapide"),
                   tmp.path(),&skipped),
               "incomplete phase must not create a log")&&ok;
    ok=require(QDir(tmp.path()).entryList(QStringList()<<QStringLiteral("*.txt"),
                  QDir::Files).size()==2,
               "unexpected extra files")&&ok;

    if(ok) std::fprintf(stdout,"PASS AANMP002 two-phase journal files and portable text contents\n");
    return ok?0:1;
}
