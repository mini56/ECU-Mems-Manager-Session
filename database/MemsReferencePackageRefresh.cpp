#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>

namespace {

QByteArray referencePackageSignature(const QString &referenceRoot)
{
    const QStringList relativeFiles={
        QStringLiteral("manifest.json"),
        QStringLiteral("mems_reference_seed_1.qz64"),
        QStringLiteral("mems_reference_seed_2.qz64"),
        QStringLiteral("mems_reference_seed_3.qz64"),
        QStringLiteral("mems_reference_seed_4.qz64"),
        QStringLiteral("fiches/mems_1_3.xml.qz64"),
        QStringLiteral("fiches/mems_1_6.xml.qz64"),
        QStringLiteral("fiches/mems_1_9.xml.qz64")
    };

    QCryptographicHash hash(QCryptographicHash::Sha256);
    for(const QString &relative:relativeFiles){
        QFile file(referenceRoot+QLatin1Char('/')+relative);
        if(!file.open(QIODevice::ReadOnly)) return QByteArray();
        hash.addData(relative.toUtf8());
        hash.addData("\0",1);
        while(!file.atEnd()) hash.addData(file.read(64*1024));
        hash.addData("\0",1);
    }
    return hash.result().toHex();
}

QByteArray readMarker(const QString &path)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly|QIODevice::Text)) return QByteArray();
    return file.readAll().trimmed();
}

bool writeMarker(const QString &path,const QByteArray &signature)
{
    QSaveFile file(path);
    if(!file.open(QIODevice::WriteOnly|QIODevice::Text)) return false;
    if(file.write(signature)!=signature.size()) return false;
    if(file.write("\n",1)!=1) return false;
    return file.commit();
}

bool cacheComplete(const QString &cacheRoot)
{
    QDir root(cacheRoot);
    if(root.entryList(QStringList()<<QStringLiteral("ecu_mems_reference_*.sqlite"),QDir::Files).isEmpty())
        return false;

    const QString fiches=cacheRoot+QStringLiteral("/fiches/");
    return QFileInfo::exists(fiches+QStringLiteral("mems_1_3.xml")) &&
           QFileInfo::exists(fiches+QStringLiteral("mems_1_6.xml")) &&
           QFileInfo::exists(fiches+QStringLiteral("mems_1_9.xml"));
}

void removeMatching(const QString &directory,const QStringList &patterns)
{
    QDir dir(directory);
    const QStringList files=dir.entryList(patterns,QDir::Files);
    for(const QString &file:files) QFile::remove(dir.filePath(file));
}

void invalidateReferenceCache()
{
    const QString referenceRoot=QCoreApplication::applicationDirPath()+QStringLiteral("/database/reference");
    const QByteArray signature=referencePackageSignature(referenceRoot);
    if(signature.isEmpty()) return;

    const QString cacheRoot=QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        +QStringLiteral("/reference");
    QDir().mkpath(cacheRoot);

    const QString markerPath=cacheRoot+QStringLiteral("/reference_package.sha256");
    const QByteArray cachedSignature=readMarker(markerPath);
    if(cachedSignature==signature && cacheComplete(cacheRoot)) return;

    removeMatching(cacheRoot,QStringList()
        <<QStringLiteral("ecu_mems_reference_*.sqlite")
        <<QStringLiteral("ecu_mems_reference_*.sqlite-*")
        <<QStringLiteral("ecu_mems_reference_*.sqlite.*")
        <<QStringLiteral("mems_global_search_*.sqlite")
        <<QStringLiteral("mems_global_search_*.sqlite-*")
        <<QStringLiteral("mems_global_search_*.sqlite.*"));

    const QString fiches=cacheRoot+QStringLiteral("/fiches");
    removeMatching(fiches,QStringList()<<QStringLiteral("mems_*.xml"));

    writeMarker(markerPath,signature);
}

void installReferenceCacheInvalidation()
{
    QCoreApplication *app=QCoreApplication::instance();
    if(!app) return;
    QTimer::singleShot(0,app,[](){invalidateReferenceCache();});
}

}

Q_COREAPP_STARTUP_FUNCTION(installReferenceCacheInvalidation)
