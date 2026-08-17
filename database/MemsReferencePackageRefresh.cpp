#include "MemsReferencePackageRefresh.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>

#include <algorithm>

namespace {

QString referenceRoot()
{
    return QCoreApplication::applicationDirPath()+QStringLiteral("/database/reference");
}

QString cacheRoot()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        +QStringLiteral("/reference");
}

QStringList packagedReferenceFiles(const QString &root)
{
    QStringList relativeFiles;
    QDir base(root);
    if(!base.exists()) return relativeFiles;

    QDirIterator it(root,QDir::Files|QDir::NoDotAndDotDot,QDirIterator::Subdirectories);
    while(it.hasNext()){
        const QString absolute=it.next();
        const QString relative=base.relativeFilePath(absolute).replace(QLatin1Char('\\'),QLatin1Char('/'));
        if(!relative.isEmpty()) relativeFiles.append(relative);
    }
    std::sort(relativeFiles.begin(),relativeFiles.end(),[](const QString &a,const QString &b){
        return QString::compare(a,b,Qt::CaseInsensitive)<0;
    });
    return relativeFiles;
}

QByteArray referencePackageSignature(const QString &root)
{
    const QStringList relativeFiles=packagedReferenceFiles(root);
    if(relativeFiles.isEmpty()) return QByteArray();

    QCryptographicHash hash(QCryptographicHash::Sha256);

    // The cache belongs to one packaged reference set. The GitHub Actions
    // build number remains part of the signature so two executable builds
    // never accidentally share stale generated data.
    hash.addData("build:",6);
    hash.addData(QByteArray(APP_BUILD_NUMBER));
    hash.addData("\0",1);

    // IMPORTANT: hash every file below database/reference. No resource is
    // allowed to require a new hard-coded entry here. Adding an enrichment,
    // fiche, SVG, image, PDF, manifest entry or any future reference asset
    // automatically invalidates the generated database/search cache.
    for(const QString &relative:relativeFiles){
        QFile file(root+QLatin1Char('/')+relative);
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

bool cacheComplete(const QString &root)
{
    QDir dir(root);
    if(dir.entryList(QStringList()<<QStringLiteral("ecu_mems_reference_*.sqlite"),QDir::Files).isEmpty())
        return false;

    const QString fiches=root+QStringLiteral("/fiches/");
    return QFileInfo::exists(fiches+QStringLiteral("mems_1_2.xml")) &&
           QFileInfo::exists(fiches+QStringLiteral("mems_1_3.xml")) &&
           QFileInfo::exists(fiches+QStringLiteral("mems_1_6.xml")) &&
           QFileInfo::exists(fiches+QStringLiteral("mems_1_9.xml"));
}

bool cacheHasReferenceData(const QString &root)
{
    QDir dir(root);
    if(!dir.entryList(QStringList()<<QStringLiteral("ecu_mems_reference_*.sqlite"),QDir::Files).isEmpty())
        return true;

    const QDir fiches(root+QStringLiteral("/fiches"));
    return !fiches.entryList(QStringList()<<QStringLiteral("mems_*.xml"),QDir::Files).isEmpty();
}

void removeMatching(const QString &directory,const QStringList &patterns)
{
    QDir dir(directory);
    const QStringList files=dir.entryList(patterns,QDir::Files);
    for(const QString &file:files) QFile::remove(dir.filePath(file));
}

}

MemsReferencePackageAction memsReferencePackageAction()
{
    const QString sourceRoot=referenceRoot();
    const QByteArray signature=referencePackageSignature(sourceRoot);
    if(signature.isEmpty())
        return MemsReferencePackageAction::Unavailable;

    const QString destinationRoot=cacheRoot();
    const QByteArray cachedSignature=readMarker(destinationRoot+QStringLiteral("/reference_package.sha256"));
    const bool complete=cacheComplete(destinationRoot);

    if(complete && cachedSignature==signature)
        return MemsReferencePackageAction::None;

    if(cacheHasReferenceData(destinationRoot) || !cachedSignature.isEmpty())
        return MemsReferencePackageAction::Update;

    return MemsReferencePackageAction::Install;
}

bool refreshMemsReferencePackage(MemsReferencePackageAction *detectedAction)
{
    const MemsReferencePackageAction action=memsReferencePackageAction();
    if(detectedAction)
        *detectedAction=action;

    if(action==MemsReferencePackageAction::Unavailable)
        return false;
    if(action==MemsReferencePackageAction::None)
        return true;

    const QString sourceRoot=referenceRoot();
    const QByteArray signature=referencePackageSignature(sourceRoot);
    if(signature.isEmpty())
        return false;

    const QString destinationRoot=cacheRoot();
    QDir().mkpath(destinationRoot);

    // New/changed reference package: remove every generated artifact before
    // rebuilding. The next MemsReferenceDatabase/MemsGlobalSearchIndex open
    // recreates them from the complete packaged reference set.
    removeMatching(destinationRoot,QStringList()
        <<QStringLiteral("ecu_mems_reference_*.sqlite")
        <<QStringLiteral("ecu_mems_reference_*.sqlite-*")
        <<QStringLiteral("ecu_mems_reference_*.sqlite.*")
        <<QStringLiteral("mems_global_search_*.sqlite")
        <<QStringLiteral("mems_global_search_*.sqlite-*")
        <<QStringLiteral("mems_global_search_*.sqlite.*"));

    const QString fiches=destinationRoot+QStringLiteral("/fiches");
    removeMatching(fiches,QStringList()<<QStringLiteral("mems_*.xml"));

    return writeMarker(destinationRoot+QStringLiteral("/reference_package.sha256"),signature);
}
