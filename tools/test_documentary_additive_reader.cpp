#include "../database/MemsDocumentaryRepository.h"
#include "../database/MemsDocumentaryLocalInstaller.h"
#include "../database/MemsDocumentaryPackCatalog.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QVariantMap>
#include <cstdio>

#ifndef MEMS_DOCUMENTARY_INSTALL_SELFTEST
#error "Fixture installer must never ship in the production build"
#endif

static bool expect(bool condition,const char *message)
{
    if(!condition) std::fprintf(stderr,"MULTIBASE_FAIL %s\n",message);
    return condition;
}

static QByteArray digest(const QString &path)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) return QByteArray();
    return QCryptographicHash::hash(file.readAll(),QCryptographicHash::Sha256);
}

int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv);
    if(argc!=2) return 2;
    QTemporaryDir profile;
    if(!expect(profile.isValid(),"test profile")) return 1;
    QStandardPaths::setTestModeEnabled(true);
    qputenv("XDG_CONFIG_HOME",profile.path().toUtf8());
    qputenv("XDG_DATA_HOME",profile.path().toUtf8());
    const QDir fixtures(QString::fromLocal8Bit(argv[1]));
    const QString first=fixtures.filePath("original");
    const QString second=fixtures.filePath("addition");
    const QString legacy=MemsDocumentaryPackCatalog::installedDatabasePath("fr");
    if(!expect(MemsDocumentaryLocalInstaller::installDirectory(first,"fr").isEmpty(),
               "first language database installed")) return 1;
    const QByteArray originalHash=digest(legacy);
    if(!expect(!originalHash.isEmpty(),"original SQLite digest")) return 1;

    if(!expect(MemsDocumentaryLocalInstaller::installDirectory(second,"fr").isEmpty(),
               "second independent language pack installed")) return 1;
    const QStringList paths=MemsDocumentaryPackCatalog::installedDatabasePaths("fr");
    if(!expect(paths.size()==2,"two separate SQLite databases exist") ||
       !expect(digest(legacy)==originalHash,"historical SQLite unchanged")) return 1;

    {
        MemsDocumentaryRepository reader;
        if(!expect(reader.open(),"multi-reader opens")) return 1;
        const QVariantList oldHits=reader.search("culasse",30,true,"fr");
        const QVariantList newHits=reader.search("freinage",30,true,"fr");
        if(!expect(!oldHits.isEmpty(),"historical base searchable") ||
           !expect(!newHits.isEmpty(),"new base searchable")) return 1;
        const QVariantMap newHit=newHits.first().toMap();
        if(!expect(newHit.value("document_id").toString()=="AKM7169_VOLUME1_TEST",
                   "new document provenance retained") ||
           !expect(!MemsDocumentaryRepository::imagePathForHit(newHit).isEmpty(),
                   "image from independent SQLite resolves")) return 1;
    }
    if(!expect(digest(legacy)==originalHash,"reader never changed original")) return 1;
    std::puts("DOCUMENTARY_MULTI_DATABASE_REAL_INSTALL_AND_SEARCH_PASS");
    return 0;
}
