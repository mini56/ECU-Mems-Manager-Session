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
#include <QSet>
#include <QStandardPaths>
#include <QUuid>
#include <QTemporaryDir>

namespace {

QString normalizedLanguage(const QString &language)
{
    const QString code=language.left(2).toLower();
    static const QStringList supported={
        QStringLiteral("fr"),QStringLiteral("en"),QStringLiteral("pt"),
        QStringLiteral("es"),QStringLiteral("it"),QStringLiteral("de")
    };
    return supported.contains(code) ? code : QString();
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
            QSqlQuery integrity(database),foreignKeys(database);
            bool valid=integrity.exec(QStringLiteral("PRAGMA integrity_check")) &&
                integrity.next() && integrity.value(0).toString()==QStringLiteral("ok") &&
                foreignKeys.exec(QStringLiteral("PRAGMA foreign_key_check")) &&
                !foreignKeys.next() && foreignKeys.lastError().type()==QSqlError::NoError;
            QSqlQuery nativeQuery(database);
            const bool native=nativeQuery.exec(QStringLiteral(
                "SELECT 1 FROM sqlite_master WHERE type='table' "
                "AND name='documentary_database'")) && nativeQuery.next();
            const QRegularExpression shaShape(QStringLiteral("^[0-9a-f]{64}$"));
            if(valid && native){
                QSqlQuery meta(database);
                valid &= meta.exec(QStringLiteral(
                    "SELECT schema_version,language,publication_status "
                    "FROM documentary_database WHERE singleton=1")) &&
                    meta.next() && meta.value(0).toInt()==1 &&
                    meta.value(1).toString()==language &&
                    QStringList{QStringLiteral("source_inventory"),
                                QStringLiteral("validated")}
                        .contains(meta.value(2).toString()) && !meta.next();
                QSqlQuery documents(database);
                valid &= documents.exec(QStringLiteral(
                    "SELECT d.document_id,d.source_revision,d.language,"
                    "d.source_pages,d.source_sha256,COUNT(p.physical_page),"
                    "MIN(p.physical_page),MAX(p.physical_page) "
                    "FROM documentary_documents d LEFT JOIN documentary_pages p "
                    "ON p.document_id=d.document_id AND p.source_revision=d.source_revision "
                    "GROUP BY d.document_id,d.source_revision,d.language,"
                    "d.source_pages,d.source_sha256"));
                int docCount=0;
                qint64 totalPages=0;
                while(valid && documents.next()){
                    const int pages=documents.value(3).toInt();
                    valid &= !documents.value(0).toString().isEmpty() &&
                        !documents.value(1).toString().isEmpty() &&
                        documents.value(2).toString()==language &&
                        pages>0 && pages<=50000 &&
                        shaShape.match(documents.value(4).toString()).hasMatch() &&
                        documents.value(5).toInt()==pages &&
                        documents.value(6).toInt()==1 &&
                        documents.value(7).toInt()==pages;
                    totalPages+=pages;
                    ++docCount;
                }
                valid &= docCount>0 && documents.lastError().type()==QSqlError::NoError;
                QSqlQuery counts(database);
                valid &= counts.exec(QStringLiteral(
                    "SELECT (SELECT COUNT(*) FROM documentary_pages),"
                    "(SELECT COUNT(*) FROM documentary_entries),"
                    "(SELECT COUNT(*) FROM documentary_fts),"
                    "(SELECT COUNT(*) FROM documentary_images)")) && counts.next();
                if(valid)
                    valid &= counts.value(0).toLongLong()==totalPages &&
                        counts.value(1).toInt()>0 &&
                        counts.value(1).toInt()==counts.value(2).toInt();
                QSqlQuery unchecked(database),unknownPages(database);
                // A complete manufacturer *inventory* can contain
                // review_required transcriptions, but such transcriptions
                // must NEVER be presented as a validated publication.
                // The source evidence remains searchable in Base de donnees;
                // IA MEMS withholds unreviewed repair instructions.
                valid &= unchecked.exec(QStringLiteral(
                    "SELECT COUNT(*) FROM documentary_entries "
                    "WHERE verification_status NOT IN "
                    "('source_checked','technical_reviewed','review_required') "
                    "OR (verification_status='review_required' AND EXISTS "
                    "(SELECT 1 FROM documentary_database "
                    "WHERE publication_status='validated'))")) &&
                    unchecked.next() && unchecked.value(0).toInt()==0;
                valid &= unknownPages.exec(QStringLiteral(
                    "SELECT COUNT(*) FROM documentary_entries e "
                    "WHERE e.source_page IS NOT NULL AND NOT EXISTS "
                    "(SELECT 1 FROM documentary_pages p "
                    "WHERE p.document_id=e.document_id AND "
                    "p.source_revision=e.source_revision AND "
                    "p.physical_page=e.source_page)")) &&
                    unknownPages.next() && unknownPages.value(0).toInt()==0;
                QSqlQuery imagesQuery(database);
                valid &= imagesQuery.exec(QStringLiteral(
                    "SELECT relative_path,asset_sha256,asset_bytes "
                    "FROM documentary_images"));
                const QRegularExpression imageName(QStringLiteral(
                    "^images/[A-Za-z0-9._/-]+\\.(png|jpg|jpeg|webp|svg)$"),
                    QRegularExpression::CaseInsensitiveOption);
                QSet<QString> known;
                while(valid && imagesQuery.next()){
                    const QString relative=imagesQuery.value(0).toString();
                    const QString expected=imagesQuery.value(1).toString();
                    const qint64 size=imagesQuery.value(2).toLongLong();
                    const QFileInfo info(QDir(QFileInfo(path).absolutePath())
                                         .filePath(relative));
                    valid &= imageName.match(relative).hasMatch() &&
                        !relative.split(QLatin1Char('/')).contains(QStringLiteral("..")) &&
                        QDir::cleanPath(relative)==relative &&
                        !known.contains(relative) &&
                        shaShape.match(expected).hasMatch() &&
                        info.isFile() && !info.isSymLink() &&
                        size>0 && size<=15*1024*1024 && info.size()==size &&
                        QString::fromLatin1(fileHash(info.absoluteFilePath()))==expected;
                    if(valid){
                        known.insert(relative);
                        images->append(relative);
                    }
                }
                valid &= imagesQuery.lastError().type()==QSqlError::NoError &&
                    counts.value(3).toInt()==known.size();
                ok=valid;
            }else if(valid && language==QStringLiteral("en") && !native){
                // The original English schema remains supported for its first pack.
                // Future English multi-source snapshots use documentary_database.
                QSqlQuery docs(database),counts(database);
                valid &= docs.exec(QStringLiteral(
                    "SELECT total_pages,original_pdf_sha256 FROM source_documents")) &&
                    docs.next() && docs.value(0).toInt()==482 &&
                    docs.value(1).toString()==QStringLiteral(
                      "c8bbb30d7d5a52932e7f92723ba5dc70520012ac3ceac21d19ab0a39b4d4c4e0")
                    && !docs.next();
                valid &= counts.exec(QStringLiteral(
                    "SELECT (SELECT COUNT(*) FROM source_page_discovery),"
                    "(SELECT COUNT(*) FROM procedures),"
                    "(SELECT COUNT(*) FROM procedure_steps),"
                    "(SELECT COUNT(*) FROM technical_values),"
                    "(SELECT COUNT(*) FROM illustrations)")) && counts.next();
                if(valid)
                    valid &= counts.value(0).toInt()==482 &&
                        counts.value(1).toInt()==336 &&
                        counts.value(2).toInt()==5055 &&
                        counts.value(3).toInt()==630 &&
                        counts.value(4).toInt()==316;
                ok=valid;
            }
            if(!ok)
                *reason=QStringLiteral(
                    "Base refusée : langue, provenance, pages, index, "
                    "intégrité ou illustrations non conformes.");
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
       archive.size()>256LL*1024*1024 || archive.size()<1024)
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
        if(file.isSymLink() || ++files>2500 ||
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
