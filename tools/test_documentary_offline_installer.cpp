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
        QStringLiteral("SyntheticTestOnlyPassword456!"));
    if(!demand(!failure.isEmpty(),"reject invalid fake data")) return 1;
    if(!demand(!MemsDocumentaryLocalInstaller::hasPassword(),
               "reject must not set installer password")) return 1;
    const QString absent=MemsDocumentaryPackCatalog::installedDatabasePath(
        QStringLiteral("fr"));
    if(!demand(!QFile::exists(absent),"reject must not modify local database"))
        return 1;
    const QString badLanguage=MemsDocumentaryLocalInstaller::installDirectory(
        folder.path(),QStringLiteral("tr"),QStringLiteral("SyntheticTestOnlyPassword456!"));
    if(!demand(!badLanguage.isEmpty(),"unrecognized source language rejected"))
        return 1;

    if(argc==2){
        const QString protectedPassword=QStringLiteral("SyntheticTestOnlyPassword456!");
        const QString validFolder=QString::fromLocal8Bit(argv[1]);
        const QString installed=MemsDocumentaryPackCatalog::
            installedDatabasePath(QStringLiteral("fr"));
        const QString installedResult=
            MemsDocumentaryLocalInstaller::installDirectory(
                validFolder,QStringLiteral("fr"),protectedPassword);
        if(!demand(installedResult.isEmpty(),
                   "validated French test fixture installs with password")){
            std::fprintf(stderr,"INSTALL_RETURNED %s\\n",
                         qPrintable(installedResult));
            return 1;
        }
        if(!demand(MemsDocumentaryLocalInstaller::hasPassword() &&
                   MemsDocumentaryLocalInstaller::passwordValid(protectedPassword) &&
                   !MemsDocumentaryLocalInstaller::passwordValid(
                       QStringLiteral("WRONG_PASSWORD")),
                   "password verified and wrong password rejected")) return 1;
        if(!demand(QFile::exists(installed),
                   "verified database exists in documentary storage")) return 1;
        const QString refusal=MemsDocumentaryLocalInstaller::installDirectory(
            validFolder,QStringLiteral("fr"),QStringLiteral("WRONG_PASSWORD"));
        if(!demand(!refusal.isEmpty() && QFile::exists(installed),
                   "failed upgrade keeps installed verified database")) return 1;
        const QString upgrade=MemsDocumentaryLocalInstaller::installDirectory(
            validFolder,QStringLiteral("fr"),protectedPassword);
        if(!demand(upgrade.isEmpty() && QFile::exists(installed),
                   "same verified pack can upgrade with backup")) return 1;
        if(argc==3){
            const QString encrypted=QString::fromLocal8Bit(argv[2]);
            const QString invalidPassword=MemsDocumentaryLocalInstaller::installEncrypted(
                encrypted,QStringLiteral("fr"),QStringLiteral("WrongPassword"));
            if(!demand(!invalidPassword.isEmpty() && QFile::exists(installed),
                       "wrong AES package password must preserve old database"))
                return 1;
            const QString newPackage=MemsDocumentaryLocalInstaller::installEncrypted(
                encrypted,QStringLiteral("fr"),protectedPassword);
            if(!demand(newPackage.isEmpty() && QFile::exists(installed),
                       "AES authenticated downloaded package installs successfully")){
                std::fprintf(stderr,"ENCRYPTED_INSTALL_ERROR %s\\n",
                             qPrintable(newPackage));
                return 1;
            }
            std::puts("DOCUMENTARY_WINDOWS_ENCRYPTED_PACK_REAL_INSTALL_PASS");
        }
        std::puts("DOCUMENTARY_OFFLINE_REAL_INSTALL_PASSWORD_BACKUP_PASS");
    }
    std::puts("DOCUMENTARY_OFFLINE_INVALID_PACKAGE_ROLLBACK_PASS");
    return 0;
}
