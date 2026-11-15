#include "../database/MemsDocumentaryLocalInstaller.h"
#include "../database/MemsDocumentaryPackCatalog.h"
#include "../database/MemsDocumentaryRepository.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QVariantMap>
#include <cstdio>

static bool expect(bool result,const char *reason)
{
    if(!result)std::fprintf(stderr,"REAL_FRENCH_RAVE_INSTALL_FAILED %s\n",reason);
    return result;
}

int main(int argc,char**argv)
{
    QCoreApplication app(argc,argv);
    if(!expect(argc==2,"requires original authenticated private .encrypted pack"))
        return 1;
    QTemporaryDir staging;
    if(!expect(staging.isValid(),"needs temporary test directory"))
        return 1;
    QStandardPaths::setTestModeEnabled(true);
    qputenv("XDG_CONFIG_HOME",staging.path().toUtf8());
    qputenv("XDG_DATA_HOME",staging.path().toUtf8());
    // Only the ephemeral Windows CI runner sees the owner secret via
    // environment. No secret is embedded into SESSION or printed to logs.
    const QString password=QString::fromUtf8(
        qgetenv("MEMS_DOCUMENTARY_PUBLISH_PASSWORD"));
    if(!expect(password.size()>=12,"publisher password missing"))return 1;
    const QString asset=QString::fromLocal8Bit(argv[1]);
    if(!expect(QFileInfo(asset).isFile(),"real FR pack missing"))return 1;
    const QString destination=
        MemsDocumentaryPackCatalog::installedDatabasePath(QStringLiteral("fr"));
    if(!expect(!destination.isEmpty(),"app documentary directory missing"))return 1;
    const QString invalid=MemsDocumentaryLocalInstaller::installEncrypted(
        asset,QStringLiteral("fr"),QStringLiteral("wrong-password-ci-only"));
    if(!expect(!invalid.isEmpty()&&!QFile::exists(destination),
               "wrong password must never install anything"))return 1;
    const QString result=MemsDocumentaryLocalInstaller::installEncrypted(
        asset,QStringLiteral("fr"),password);
    if(!expect(result.isEmpty(),"real encrypted French install")){
        std::fprintf(stderr,"%s\n",qPrintable(result));
        return 1;
    }
    if(!expect(QFile::exists(destination),"French SQLite installed"))return 1;
    const QString connection=QStringLiteral("FR_797_REAL_INSTALL_VERIFY");
    bool complete=false;
    {
        QSqlDatabase db=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),connection);
        db.setDatabaseName(destination);
        if(!expect(db.open(),"installed sqlite open"))return 1;
        QSqlQuery q(db);
        complete=q.exec(QStringLiteral(
            "SELECT (SELECT COUNT(*) FROM documentary_documents),"
            "(SELECT COUNT(*) FROM documentary_pages),"
            "(SELECT COUNT(*) FROM documentary_images),"
            "(SELECT COUNT(*) FROM documentary_operations),"
            "(SELECT COUNT(*) FROM documentary_operation_steps),"
            "(SELECT COUNT(*) FROM documentary_entries "
            " WHERE source_table='original_rave_fr_ocr_scan'),"
            "(SELECT COUNT(*) FROM documentary_fts)")) && q.next() &&
            q.value(0).toInt()==6 &&
            q.value(1).toInt()==797 &&
            q.value(2).toInt()==751 &&
            q.value(3).toInt()==417 &&
            q.value(4).toInt()==3415 &&
            q.value(5).toInt()==371 &&
            q.value(6).toInt()==7164;
        QSqlQuery link(db);
        complete=complete &&
           link.exec(QStringLiteral(
             "SELECT COUNT(*) FROM documentary_entries "
             "WHERE source_table='original_rave_fr_ocr_scan' "
             "AND document_id='RCL0193FRE' "
             "AND verification_status='review_required'")) &&
           link.next() && link.value(0).toInt()==371;
        db.close();
        db=QSqlDatabase();
    }
    QSqlDatabase::removeDatabase(connection);
    if(!expect(complete,"six original French documents 797 pages and 3415 original steps intact"))
        return 1;

    qputenv("MEMS_DOCUMENTARY_DB",QFile::encodeName(destination));
    MemsDocumentaryRepository reader;
    if(!expect(reader.open(),"SESSION native reader opens the INSTALLED database"))
        return 1;
    const QVariantList matches=reader.search(
        QStringLiteral("joint de culasse"),30,false,QStringLiteral("fr"));
    if(!expect(!matches.isEmpty(),"French original scanned pages searchable"))
        return 1;
    bool ocrFound=false;
    for(const QVariant &value:matches){
        const QVariantMap hit=value.toMap();
        const QString id=hit.value(QStringLiteral("entry_id")).toString();
        if(id.startsWith(QStringLiteral("page-ocr:wmxn980f:"))){
            ocrFound=true;break;
        }
    }
    if(!expect(ocrFound,"OCR page visible in actual SESSION search"))
        return 1;
    if(!expect(reader.search(QStringLiteral("joint de culasse"),20,false,
                            QStringLiteral("en")).isEmpty(),
               "French scanner must never leak into English results"))
        return 1;
        const QVariantList newMatches=reader.search(
        QStringLiteral("autoradio"),90,false,QStringLiteral("fr"));
    bool fourSourceFound=false;
    for(const QVariant &item:newMatches){
        if(item.toMap().value(QStringLiteral("document_id")).toString()
           ==QStringLiteral("DOC_IR07301F_EEDC3668AA41")){
            fourSourceFound=true;
            break;
        }
    }
    if(!expect(fourSourceFound,
               "actual SESSION reader indexes the NEW French RAVE audio manual"))
        return 1;
    std::puts("PASS_REAL_FR_RAVE6_ENCRYPTED_WINDOWS_INSTALL_797_PAGES_751_IMAGES_417_PROCEDURES_3415_STEPS_AND_BOTH_SOURCE_SEARCHES");
    return 0;
}
