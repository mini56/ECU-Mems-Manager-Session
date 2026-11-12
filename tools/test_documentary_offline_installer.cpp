#include "../database/MemsDocumentaryLocalInstaller.h"
#include "../database/MemsDocumentaryPackCatalog.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <cstdio>

static bool demand(bool ok,const char *message)
{
    if(!ok) std::fprintf(stderr,"DOCUMENTARY_OFFLINE_FAILED %s\n",message);
    return ok;
}

int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv);
    QTemporaryDir folder;
    if(!demand(folder.isValid(),"test folder")) return 1;
    QStandardPaths::setTestModeEnabled(true);
    qputenv("XDG_CONFIG_HOME",folder.path().toUtf8());
    qputenv("XDG_DATA_HOME",folder.path().toUtf8());

    if(!demand(!MemsDocumentaryLocalInstaller::hasPassword(),
               "no password initially")) return 1;
    if(!demand(!MemsDocumentaryLocalInstaller::passwordValid(
                   QStringLiteral("incorrect password")),
               "reject unknown password")) return 1;
    const QString invalid=QDir(folder.path()).filePath(
        QStringLiteral("mems_documentary_fr.sqlite"));
    {
        QFile file(invalid);
        if(!demand(file.open(QIODevice::WriteOnly),"create invalid fixture"))
            return 1;
        file.write("Not an SQLite documentary database");
    }
    const QString failure=MemsDocumentaryLocalInstaller::installDirectory(
        folder.path(),QStringLiteral("fr"),
        QStringLiteral("TestPassword12345!"));
    if(!demand(!failure.isEmpty(),"reject invalid fake data")) return 1;
    if(!demand(!MemsDocumentaryLocalInstaller::hasPassword(),
               "reject must not set installer password")) return 1;
    const QString absent=MemsDocumentaryPackCatalog::installedDatabasePath(
        QStringLiteral("fr"));
    if(!demand(!QFile::exists(absent),"reject must not modify local database"))
        return 1;
    const QString badLanguage=MemsDocumentaryLocalInstaller::installDirectory(
        folder.path(),QStringLiteral("tr"),QStringLiteral("TestPassword12345!"));
    if(!demand(!badLanguage.isEmpty(),"unrecognized source language rejected"))
        return 1;
    std::puts("DOCUMENTARY_OFFLINE_INVALID_PACKAGE_ROLLBACK_PASS");
    return 0;
}
