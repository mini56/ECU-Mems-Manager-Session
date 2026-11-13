#include "MemsDocumentaryLocalInstaller.h"
#include "MemsDocumentaryEncryptedPack.h"
#include "MemsDocumentaryPackCatalog.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QUuid>
#include <QTemporaryDir>

namespace {

QString normalizedLanguage(const QString &language)
{
    const QString code=language.left(2).toLower();
    return code==QStringLiteral("fr") || code==QStringLiteral("en")
        ? code : QString();
}

QByteArray fileHash(const QString &path)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))
        return QByteArray();
    QCryptographicHash digest(QCryptographicHash::Sha256);
    while(!file.atEnd()){
        const QByteArray chunk=file.read(1024*1024);
        if(chunk.isEmpty() && file.error()!=QFile::NoError)
            return QByteArray();
        digest.addData(chunk);
    }
    return digest.result().toHex();
}

bool metadataValid(const QString &path,const QString &language,QString *reason,
                   QStringList *images)
{
    const QString connection=QStringLiteral("DOCUMENTARY_INSTALL_CHECK_%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    bool ok=false;
    {
        QSqlDatabase database=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),connection);
        database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
        database.setDatabaseName(path);
        if(!database.open()){
            *reason=QStringLiteral("SQLite documentaire illisible.");
        }else{
            QSqlQuery integrity(database);
            if(!integrity.exec(QStringLiteral("PRAGMA integrity_check")) ||
               !integrity.next() || integrity.value(0).toString()!=QStringLiteral("ok")){
                *reason=QStringLiteral("Contrôle d'intégrité SQLite échoué.");
            }else{
                QSqlQuery foreignKeys(database);
                if(!foreignKeys.exec(QStringLiteral("PRAGMA foreign_key_check")) ||
                   foreignKeys.next()){
                    *reason=QStringLiteral("Relations SQLite invalides.");
                }else if(language==QStringLiteral("fr")){
                    QSqlQuery meta(database);
                    if(!meta.exec(QStringLiteral(
                        "SELECT language,publication_status FROM documentary_database "
                        "WHERE singleton=1")) || !meta.next() ||
                       meta.value(0).toString()!=QStringLiteral("fr") ||
                       !QStringList{QStringLiteral("source_inventory"),
                                    QStringLiteral("validated")}
                           .contains(meta.value(1).toString())){
                        *reason=QStringLiteral("Base documentaire non française.");
                    }else{
                        QSqlQuery docs(database);
                        if(!docs.exec(QStringLiteral(
                           "SELECT document_id,source_pages,source_sha256 FROM "
                           "documentary_documents"))){
                            *reason=QStringLiteral("Provenance documentaire absente.");
                        }else{
                            bool valid=true,hasRcl=false;
                            int docCount=0;
                            while(docs.next()){
                                ++docCount;
                                const QString id=docs.value(0).toString();
                                const int pages=docs.value(1).toInt();
                                const QString sha=docs.value(2).toString();
                                if(id==QStringLiteral("RCL0193FRE")){
                                    hasRcl=pages==371 &&
                                       sha==QStringLiteral(
                                       "0c7fef28d0d0f0673ba321d6625a019c005823103caa98afb3258114e1fec713");
                                    valid &= hasRcl;
                                }else if(id==QStringLiteral("AKM7169FRE")){
                                    valid &= pages==20 &&
                                       sha==QStringLiteral(
                                       "37cd0434347fe4009963d075e4b91b0b21ebc905bae5657519a8080c3026d4f5");
                                }else valid=false; // another edition needs a separate audit
                            }
                            QSqlQuery counts(database);
                            valid &= docs.lastError().type()==QSqlError::NoError &&
                                docCount>0 && hasRcl &&
                                counts.exec(QStringLiteral(
                                    "SELECT (SELECT COUNT(*) FROM documentary_pages),"
                                    "(SELECT COUNT(*) FROM documentary_entries),"
                                    "(SELECT COUNT(*) FROM documentary_fts),"
                                    "(SELECT COUNT(*) FROM documentary_images)")) &&
                                counts.next();
                            if(valid){
                                valid &= counts.value(0).toInt()==371+
                                    (docCount==2?20:0) &&
                                    counts.value(1).toInt()>=5958 &&
                                    counts.value(1)==counts.value(2) &&
                                    counts.value(3).toInt()==325+
                                    (docCount==2?20:0);
                            }
                            // Un corpus français ne doit pas être installé si
                            // une entrée est encore en cours de relecture.
                            QSqlQuery review(database);
                            valid &= review.exec(QStringLiteral(
                                "SELECT COUNT(*) FROM documentary_entries "
                                "WHERE verification_status<>'source_checked'")) &&
                                review.next() && review.value(0).toInt()==0;
                            QSqlQuery visual(database);
                            valid &= visual.exec(QStringLiteral(
                                "SELECT COUNT(*) FROM documentary_page_visual_status "
                                "WHERE document_id='RCL0193FRE' AND is_blank=0")) &&
                                visual.next() && visual.value(0).toInt()==325;
                            QSqlQuery imagesQuery(database);
                            valid &= imagesQuery.exec(QStringLiteral(
                                "SELECT relative_path,asset_sha256,asset_bytes "
                                "FROM documentary_images"));
                            int count=0;
                            const QRegularExpression imageName(QStringLiteral(
                                "^images/(?:RCL0193FRE|AKM7169FRE)_PDF_[0-9]{3}\\.png$"));
                            while(valid && imagesQuery.next()){
                                const QString relative=imagesQuery.value(0).toString();
                                const QString expected=imagesQuery.value(1).toString();
                                const qint64 size=imagesQuery.value(2).toLongLong();
                                const QString image=QDir(QFileInfo(path).absolutePath())
                                    .filePath(relative);
                                const QFileInfo info(image);
                                if(!imageName.match(relative).hasMatch() ||
                                   !info.isFile() || info.isSymLink() ||
                                   size<=0 || size>5*1024*1024 || info.size()!=size ||
                                   QString::fromLatin1(fileHash(image))!=expected){
                                    valid=false;
                                    break;
                                }
                                images->append(relative);
                                ++count;
                            }
                            valid &= count==325+(docCount==2?20:0);
                            ok=valid;
                            if(!valid) *reason=QStringLiteral(
                                "Base française non intégralement relue, ou provenance, pages ou images incohérentes.");
                        }
                    }
                }else{
                    QSqlQuery docs(database);
                    ok=docs.exec(QStringLiteral(
                         "SELECT total_pages,original_pdf_sha256 FROM source_documents")) &&
                       docs.next() && docs.value(0).toInt()==482 &&
                       docs.value(1).toString()==QStringLiteral(
                         "c8bbb30d7d5a52932e7f92723ba5dc70520012ac3ceac21d19ab0a39b4d4c4e0")
                       && !docs.next();
                    QSqlQuery counts(database);
                    ok &= counts.exec(QStringLiteral(
                        "SELECT (SELECT COUNT(*) FROM source_page_discovery),"
                        "(SELECT COUNT(*) FROM procedures),"
                        "(SELECT COUNT(*) FROM procedure_steps),"
                        "(SELECT COUNT(*) FROM technical_values),"
                        "(SELECT COUNT(*) FROM illustrations)")) && counts.next();
                    if(ok)
                        ok=counts.value(0).toInt()==482 &&
                           counts.value(1).toInt()==336 &&
                           counts.value(2).toInt()==5055 &&
                           counts.value(3).toInt()==630 &&
                           counts.value(4).toInt()==316;
                    if(!ok) *reason=QStringLiteral(
                        "Base anglaise ou références constructeur incohérentes.");
                }
            }
            database.close();
        }
    }
    QSqlDatabase::removeDatabase(connection);
    return ok;
}

QString locateSource(const QString &directory,const QString &code)
{
    const QDir root(directory);
    const QString base=QStringLiteral("mems_documentary_%1.sqlite").arg(code);
    const QString direct=root.filePath(base);
    if(QFileInfo(direct).isFile())
        return direct;
    QString found;
    QDirIterator iterator(directory,QStringList{base},QDir::Files,
                          QDirIterator::Subdirectories);
    int inspected=0;
    while(iterator.hasNext()){
        const QString next=iterator.next();
        if(++inspected>8 || !found.isEmpty())
            return QString();
        found=next;
    }
    return found;
}

QString installSource(const QString &source,const QString &code)
{
    // Private entrypoint: reached only after an authentic MEMSDOC1
    // decrypt-and-verify step (or in an explicitly compiled self-test).
    const QFileInfo input(source);
    if(!input.isFile() || input.isSymLink() || input.size()<4096 ||
       input.size()>512LL*1024*1024)
        return QStringLiteral("Fichier SQLite documentaire absent ou trop volumineux.");
    QString error;
    QStringList images;
    if(!metadataValid(input.absoluteFilePath(),code,&error,&images))
        return error;
    const QString installed=MemsDocumentaryPackCatalog::installedDatabasePath(code);
    if(installed.isEmpty())
        return QStringLiteral("Emplacement documentaire Windows indisponible.");
    const QFileInfo installedInfo(installed);
    const QFileInfo destinationInfo(installedInfo.absolutePath());
    if(!QDir().mkpath(destinationInfo.absolutePath()))
        return QStringLiteral("Impossible de créer le dossier documentaire.");
    const QString stamp=QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString destination=installedInfo.absolutePath();
    const QString incoming=destination+QStringLiteral(".incoming-")+stamp;
    const QString backup=destination+QStringLiteral(".backup-")+stamp;
    const QString staged=QDir(incoming).filePath(installedInfo.fileName());
    if(!QDir().mkpath(QDir(incoming).filePath(QStringLiteral("images"))) ||
       !QFile::copy(input.absoluteFilePath(),staged)){
        QDir(incoming).removeRecursively();
        return QStringLiteral("Impossible de préparer la nouvelle base.");
    }
    for(const QString &image : images){
        const QString inputImage=QDir(input.absolutePath()).filePath(image);
        const QString outputImage=QDir(incoming).filePath(image);
        if(!QFile::copy(inputImage,outputImage)){
            QDir(incoming).removeRecursively();
            return QStringLiteral("Copie des pages constructeur impossible.");
        }
    }
    QString recheck;
    QStringList verified;
    if(!metadataValid(staged,code,&recheck,&verified) ||
       verified.size()!=images.size()){
        QDir(incoming).removeRecursively();
        return QStringLiteral("Contrôle final du pack échoué : ")+recheck;
    }
    const bool existed=QFileInfo(destination).isDir();
    if(existed && !QDir().rename(destination,backup)){
        QDir(incoming).removeRecursively();
        return QStringLiteral("Sauvegarde de l'ancienne base impossible : installation annulée.");
    }
    if(!QDir().rename(incoming,destination)){
        if(existed) QDir().rename(backup,destination);
        QDir(incoming).removeRecursively();
        return QStringLiteral("Installation annulée. Ancienne base restaurée.");
    }
    return QString(); // Succès : éventuelle ancienne base conservée dans .backup-UUID
}

} // namespace

QString MemsDocumentaryLocalInstaller::installDirectory(
    const QString &sourceDirectory,const QString &language)
{
    const QString code=normalizedLanguage(language);
    if(code.isEmpty())
        return QStringLiteral("Langue de base non prise en charge.");
    const QString source=locateSource(sourceDirectory,code);
    if(source.isEmpty())
        return QStringLiteral("Fichier documentaire %1 introuvable ou ambigu.").arg(code);
    return installSource(source,code);
}

QString MemsDocumentaryLocalInstaller::installZip(
    const QString &archivePath,const QString &language)
{
    const QFileInfo archive(archivePath);
    if(!archive.isFile() || archive.isSymLink() ||
       archive.suffix().toLower()!=QStringLiteral("zip") ||
       archive.size()>160LL*1024*1024 || archive.size()<1024)
        return QStringLiteral("Archive ZIP documentaire invalide.");
    const QString code=normalizedLanguage(language);
    if(code.isEmpty())
        return QStringLiteral("Langue non prise en charge.");
#ifdef Q_OS_WIN
    QTemporaryDir temporary;
    if(!temporary.isValid())
        return QStringLiteral("Dossier temporaire indisponible.");
    QProcess process;
    QProcessEnvironment env=QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("MEMS_DOC_ARCHIVE"),archive.absoluteFilePath());
    env.insert(QStringLiteral("MEMS_DOC_TARGET"),temporary.path());
    process.setProcessEnvironment(env);
    const QString shell=QDir::cleanPath(qEnvironmentVariable("WINDIR")+
        QStringLiteral("/System32/WindowsPowerShell/v1.0/powershell.exe"));
    if(!QFileInfo(shell).isFile())
        return QStringLiteral("Extracteur ZIP Windows introuvable.");
    process.start(shell, QStringList{
        QStringLiteral("-NoProfile"),QStringLiteral("-NonInteractive"),
        QStringLiteral("-Command"),
        QStringLiteral(
            "$ErrorActionPreference='Stop'; "
            "Expand-Archive -LiteralPath $env:MEMS_DOC_ARCHIVE "
            "-DestinationPath $env:MEMS_DOC_TARGET -Force")});
    if(!process.waitForStarted(10000) || !process.waitForFinished(180000) ||
       process.exitStatus()!=QProcess::NormalExit || process.exitCode()!=0)
        return QStringLiteral("Extraction ZIP refusée ; aucune base modifiée.");
    qint64 total=0;
    int files=0;
    QDirIterator iterator(temporary.path(),QDir::Files|QDir::NoDotAndDotDot,
                          QDirIterator::Subdirectories);
    while(iterator.hasNext()){
        const QFileInfo file(iterator.next());
        if(file.isSymLink() || ++files>1000 ||
           (total+=file.size())>700LL*1024*1024)
            return QStringLiteral("Archive contenant trop de fichiers ou de données.");
    }
    return installDirectory(temporary.path(),code);
#else
    return QStringLiteral("Import ZIP intégré uniquement sous Windows ; "
                          "utiliser un dossier déjà extrait pour le test.");
#endif
}

QString MemsDocumentaryLocalInstaller::installEncrypted(
    const QString &encryptedPath,const QString &language,
    const QString &password)
{
    if(password.isEmpty())
        return QStringLiteral("Mot de passe obligatoire.");
    QTemporaryDir temporary;
    if(!temporary.isValid())
        return QStringLiteral("Dossier temporaire inaccessible.");
    const QString decrypted=QDir(temporary.path()).filePath(
        QStringLiteral("source-documentaire-dechiffree.zip"));
    const QString failure=MemsDocumentaryEncryptedPack::decryptToZip(
        encryptedPath,password,decrypted);
    if(!failure.isEmpty())
        return failure;
    // AES-GCM verified. The same production ZIP importer checks file names,
    // documentation provenance, language, pages, images, SQLite and backups.
    // QTemporaryDir wipes the decrypted ZIP on both success and failure.
    return installZip(decrypted,language);
}
