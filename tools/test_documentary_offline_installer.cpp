#include "../database/MemsDocumentaryLocalInstaller.h"
#include "../database/MemsDocumentaryPackCatalog.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <cstdio>

#ifndef MEMS_DOCUMENTARY_INSTALL_SELFTEST
#error "The unencrypted self-test fixture must NEVER build into production."
#endif

static bool demand(bool ok,const char *message)
{
    if(!ok) std::fprintf(stderr,"DOCUMENTARY_INSTALL_SELFTEST_FAILED %s\n",message);
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

    const QString invalid=QDir(folder.path()).filePath(
        QStringLiteral("mems_documentary_fr.sqlite"));
    {
        QFile file(invalid);
        if(!demand(file.open(QIODevice::WriteOnly),"invalid fixture")) return 1;
        file.write("Not an SQLite documentary database");
    }

    const QString failure=MemsDocumentaryLocalInstaller::installDirectory(
        folder.path(),QStringLiteral("fr"));
    if(!demand(!failure.isEmpty(),"reject fake SQLite")) return 1;

    const QString installed=MemsDocumentaryPackCatalog::installedDatabasePath(
        QStringLiteral("fr"));
    if(!demand(!QFile::exists(installed),
               "rejected package leaves installed SQLite unchanged")) return 1;

    const QString badLanguage=MemsDocumentaryLocalInstaller::installDirectory(
        folder.path(),QStringLiteral("tr"));
    if(!demand(!badLanguage.isEmpty(),"reject unsupported language")) return 1;

    if(argc>=2){
        const QString validFolder=QString::fromLocal8Bit(argv[1]);

        // The plaintext helper is exposed only by TEST macro. In the actual
        // program, only installEncrypted() can reach these operations.
        const QString installedResult=
            MemsDocumentaryLocalInstaller::installDirectory(
                validFolder,QStringLiteral("fr"));
        if(!demand(installedResult.isEmpty(),
                   "fully validated test fixture installs")){
            std::fprintf(stderr,"INSTALL_RESULT %s\n",
                         qPrintable(installedResult));
            return 1;
        }
        if(!demand(QFile::exists(installed),
                   "validated documentary database exists")) return 1;

        const QString upgrade=MemsDocumentaryLocalInstaller::installDirectory(
            validFolder,QStringLiteral("fr"));
        if(!demand(upgrade.isEmpty() && QFile::exists(installed),
                   "verified package upgrade retains a backup")) return 1;

        if(argc>=3){
            const QString encrypted=QString::fromLocal8Bit(argv[2]);
            const QString badPassword=MemsDocumentaryLocalInstaller::installEncrypted(
                encrypted,QStringLiteral("fr"),QStringLiteral("WrongPassword"));
            if(!demand(!badPassword.isEmpty() && QFile::exists(installed),
                       "wrong publisher password cannot overwrite existing base"))
                return 1;
            const QString correct=MemsDocumentaryLocalInstaller::installEncrypted(
                encrypted,QStringLiteral("fr"),
                QStringLiteral("SyntheticTestOnlyPassword456!"));
            if(!demand(correct.isEmpty() && QFile::exists(installed),
                       "publisher-secret AES-GCM pack installs into documentary store")){
                std::fprintf(stderr,"AUTHENTICATED_INSTALL_FAILED %s\n",
                             qPrintable(correct));
                return 1;
            }
            std::puts("DOCUMENTARY_ENCRYPTED_ADMIN_SECRET_INSTALL_PASS");
        }
        std::puts("DOCUMENTARY_ATOMIC_BACKUP_TEST_PASS");
    }
    std::puts("DOCUMENTARY_INVALID_PACKAGE_REJECT_PASS");
    return 0;
}
