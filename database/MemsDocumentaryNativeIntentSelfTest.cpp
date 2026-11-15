#include "MemsDocumentaryRepository.h"
#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QTemporaryDir>
#include <QVariantMap>
#include <cstdio>

static bool require(bool condition,const char* reason)
{
    if(!condition)
        std::fprintf(stderr,"MEMS_NATIVE_INTENT_SELFTEST_FAILED: %s\n",reason);
    return condition;
}

int main(int argc,char** argv)
{
    QCoreApplication app(argc,argv);
    QTemporaryDir dir;
    if(!require(dir.isValid(),"temporary root"))return 1;
    const QString path=QDir(dir.path()).filePath(QStringLiteral("native_fr.sqlite"));
    {
        QSqlDatabase db=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                                  QStringLiteral("native_intent_seed"));
        db.setDatabaseName(path);
        if(!require(db.open(),"seed sqlite"))return 1;
        const QStringList schema={
            QStringLiteral("CREATE TABLE documentary_database("
                           "singleton INTEGER PRIMARY KEY,schema_version INTEGER,"
                           "language TEXT,publication_status TEXT)"),
            QStringLiteral("CREATE TABLE documentary_documents("
                           "document_id TEXT,source_revision TEXT,language TEXT)"),
            QStringLiteral("CREATE TABLE documentary_pages("
                           "document_id TEXT,physical_page INTEGER)"),
            QStringLiteral("CREATE TABLE documentary_images("
                           "document_id TEXT,physical_page INTEGER,relative_path TEXT)"),
            QStringLiteral("CREATE TABLE documentary_entries("
                           "document_id TEXT,source_revision TEXT,entry_id TEXT,"
                           "entry_type TEXT,source_page INTEGER,title TEXT,"
                           "source_text TEXT,verification_status TEXT,"
                           "source_table TEXT,raw_json TEXT)"),
            QStringLiteral("CREATE VIRTUAL TABLE documentary_fts USING fts5("
                           "title,source_text,content='documentary_entries',"
                           "content_rowid='rowid',"
                           "tokenize='unicode61 remove_diacritics 2')"),
            QStringLiteral("INSERT INTO documentary_database VALUES(1,1,'fr','source_inventory')"),
            QStringLiteral("INSERT INTO documentary_documents VALUES('RCL0193FRE','v1','fr')"),
            QStringLiteral("INSERT INTO documentary_pages VALUES('RCL0193FRE',59)"),
            QStringLiteral("INSERT INTO documentary_pages VALUES('RCL0193FRE',60)"),
            QStringLiteral("INSERT INTO documentary_pages VALUES('RCL0193FRE',42)"),
            QStringLiteral("INSERT INTO documentary_entries VALUES("
                           "'RCL0193FRE','v1','mems_procedure:1','procedure',59,"
                           "'JOINT DE CULASSE - dépose',"
                           "'title_source: JOINT DE CULASSE - dépose\\n"
                           "Opération de réparation N° 12.29.02; début PDF p.59',"
                           "'source_checked','mems_procedure','{}')"),
            QStringLiteral("INSERT INTO documentary_entries VALUES("
                           "'RCL0193FRE','v1','mems_procedure:2','procedure',60,"
                           "'JOINT DE CULASSE - repose',"
                           "'title_source: JOINT DE CULASSE - repose\\n"
                           "Opération de réparation N° 12.29.02; début PDF p.60',"
                           "'source_checked','mems_procedure','{}')"),
            QStringLiteral("INSERT INTO documentary_entries VALUES("
                           "'RCL0193FRE','v1','mems_knowledge_item:1','reference',42,"
                           "'culasse écrous séquence de serrage',"
                           "'source_text: Écrous de culasse : 34 N.m puis 34 N.m de plus',"
                           "'source_checked','mems_knowledge_item','{}')"),
            QStringLiteral("INSERT INTO documentary_entries VALUES("
                           "'RCL0193FRE','v1','p3','procedure',70,"
                           "'MAITRE CYLINDRE DE FREIN - réparation',"
                           "'Consignes constructeur MAITRE CYLINDRE DE FREIN - réparation page 70',"
                           "'source_checked','mems_procedure','{}')"),
            QStringLiteral("INSERT INTO documentary_entries VALUES("
                           "'RCL0193FRE','v1','p4','procedure',71,"
                           "'MAITRE CYLINDRE DE FREIN - contrôle',"
                           "'Consignes constructeur MAITRE CYLINDRE DE FREIN - contrôle page 71',"
                           "'source_checked','mems_procedure','{}')"),
            QStringLiteral("INSERT INTO documentary_entries VALUES("
                           "'RCL0193FRE','v1','p5','procedure',72,"
                           "'MAITRE CYLINDRE DE FREIN - réglage',"
                           "'Consignes constructeur MAITRE CYLINDRE DE FREIN - réglage page 72',"
                           "'source_checked','mems_procedure','{}')"),
            QStringLiteral("INSERT INTO documentary_entries VALUES("
                           "'RCL0193FRE','v1','p6','procedure',73,"
                           "'MAITRE CYLINDRE DE FREIN - remplacement',"
                           "'Consignes constructeur MAITRE CYLINDRE DE FREIN - remplacement page 73',"
                           "'source_checked','mems_procedure','{}')"),
            QStringLiteral("INSERT INTO documentary_entries VALUES("
                           "'RCL0193FRE','v1','p7','procedure',74,"
                           "'MAITRE CYLINDRE DE FREIN - purge',"
                           "'Consignes constructeur MAITRE CYLINDRE DE FREIN - purge page 74',"
                           "'source_checked','mems_procedure','{}')"),
            QStringLiteral("INSERT INTO documentary_entries VALUES("
                           "'RCL0193FRE','v1','p8','procedure',75,"
                           "'MAITRE CYLINDRE DE FREIN - dépose',"
                           "'Consignes constructeur MAITRE CYLINDRE DE FREIN - dépose page 75',"
                           "'source_checked','mems_procedure','{}')"),
            QStringLiteral("INSERT INTO documentary_entries VALUES("
                           "'RCL0193FRE','v1','p9','procedure',76,"
                           "'MAITRE CYLINDRE DE FREIN - repose',"
                           "'Consignes constructeur MAITRE CYLINDRE DE FREIN - repose page 76',"
                           "'source_checked','mems_procedure','{}')"),
            QStringLiteral("INSERT INTO documentary_entries VALUES("
                           "'RCL0193FRE','v1','motor','procedure',77,"
                           "'MOTEUR ET BOITE - dépose',"
                           "'Débrancher les tuyaux de frein pendant la dépose du moteur',"
                           "'source_checked','mems_procedure','{}')"),
            QStringLiteral("INSERT INTO documentary_fts(documentary_fts) VALUES('rebuild')")
        };
        for(const QString& sql:schema){
            QSqlQuery query(db);
            if(!require(query.exec(sql),"fixture schema and data"))return 1;
        }
        db.close();
        db=QSqlDatabase();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("native_intent_seed"));
    QSettings settings(QSettings::IniFormat,QSettings::UserScope,
                       QStringLiteral("ECU_Mems_Manager"));
    settings.beginGroup(QStringLiteral("Settings"));
    settings.setValue(QStringLiteral("Language"),QStringLiteral("fr"));
    settings.endGroup();
    qputenv("MEMS_DOCUMENTARY_DB",path.toUtf8());
    MemsDocumentaryRepository repository;
    if(!require(repository.open(),"reader accepts native corpus"))return 1;

    const QStringList removalQuestions={
        QStringLiteral("Donne-moi la procédure de dépose du joint de culasse"),
        QStringLiteral("Explique-moi comment faire la dépose du joint de culasse"),
        QStringLiteral("Quelles sont les étapes pour déposer le joint de culasse ?"),
        QStringLiteral("dépose joint culasse")
    };
    for(const QString& question:removalQuestions){
        const QVariantList found=repository.search(question,12,true,QStringLiteral("fr"));
        if(found.isEmpty()) {
            std::fprintf(stderr,"NATIVE_QUERY_NO_HITS_FOR=%s\\n",qPrintable(question));
            QSqlDatabase debug=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),QStringLiteral("native-debug"));
            debug.setDatabaseName(path);
            if(debug.open()) {
                QSqlQuery fts(debug);
                fts.prepare(QStringLiteral("SELECT COUNT(*) FROM documentary_fts WHERE documentary_fts MATCH ?"));
                fts.addBindValue(QStringLiteral("\"joint\" AND \"culasse\""));
                const bool executed=fts.exec();
                const QString diagnostic=fts.lastError().text();
                std::fprintf(stderr,"NATIVE_PROBE_EXEC=%d ERROR=%s\\n",
                             int(executed),qPrintable(diagnostic));
                if(fts.next())std::fprintf(stderr,"NATIVE_PROBE_COUNT=%d\\n",fts.value(0).toInt());
                QSqlQuery all(debug);
                if(all.exec(QStringLiteral("SELECT COUNT(*) FROM documentary_fts")) && all.next())
                    std::fprintf(stderr,"NATIVE_FTS_ALL_ROWS=%d\\n",all.value(0).toInt());
                debug.close();
            }
            debug=QSqlDatabase();
            QSqlDatabase::removeDatabase(QStringLiteral("native-debug"));
            return 1;
        }
        const QVariantMap first=found.first().toMap();
        if(!require(first.value(QStringLiteral("title")).toString().contains(
             QStringLiteral("dépose")),"removal requested must prioritize removal"))return 1;
        if(!require(first.value(QStringLiteral("source_page")).toInt()==59,
                    "removal original source page 59"))return 1;
        if(!require(!found.first().toMap().value(QStringLiteral("source_key")).toString().isEmpty(),
                    "original source identity kept"))return 1;
    }
    const QStringList refitQuestions={
        QStringLiteral("Donne-moi la procédure de repose du joint de culasse"),
        QStringLiteral("Comment remonter le joint de culasse ?")
    };
    for(const QString& question:refitQuestions){
        const QVariantList found=repository.search(question,12,true,QStringLiteral("fr"));
        if(!require(!found.isEmpty() &&
                found.first().toMap().value(QStringLiteral("title")).toString()
                    .contains(QStringLiteral("repose")),
                    "refit requested must prioritize refit"))return 1;
        if(!require(found.first().toMap().value(QStringLiteral("source_page")).toInt()==60,
                    "refit original source page 60"))return 1;
    }
    const QVariantList direct=repository.search(QStringLiteral("joint de culasse"));
    if(!require(direct.size()>=2,"general topic returns both real operations"))return 1;
    // A broad component query must offer every real workshop operation,
    // including repairs, controls, settings, replacement and bleeding.
    const QVariantList brakeOffers=repository.search(
        QStringLiteral("maitre cylindre de frein"),100,true,QStringLiteral("fr"));
    const QString menu=MemsDocumentaryRepository::numberedMenu(
        QStringLiteral("maitre cylindre de frein"),brakeOffers,true);
    for (const QString &kind : {QStringLiteral("réparation"), QStringLiteral("contrôle"),
            QStringLiteral("réglage"), QStringLiteral("remplacement"),
            QStringLiteral("purge"), QStringLiteral("dépose"), QStringLiteral("repose")}) {
        if(!require(menu.contains(kind),"workshop subject lists all actual operations"))return 1;
    }
    const QVariantList repair=repository.search(
        QStringLiteral("réparation du maitre cylindre de frein"),100,true,QStringLiteral("fr"));
    if(!require(!repair.isEmpty() &&
                repair.first().toMap().value(QStringLiteral("title")).toString()
                    .contains(QStringLiteral("MAITRE CYLINDRE")),
                "repair request keeps the component without forcing removal/refit"))return 1;
    const QVariantList control=repository.search(
        QStringLiteral("contrôle maitre cylindre de frein"),100,true,QStringLiteral("fr"));
    if(!require(!control.isEmpty() &&
                control.first().toMap().value(QStringLiteral("title")).toString()
                    .contains(QStringLiteral("MAITRE CYLINDRE")),
                "control request stays with the intended brake component"))return 1;
    const QVariantList review=repository.search(
        QStringLiteral("révision du maitre cylindre de frein"),100,true,QStringLiteral("fr"));
    if(!require(!review.isEmpty(),
                "workshop operation word must not block otherwise available procedures"))return 1;
    const QVariantList unrelated=repository.search(QStringLiteral("reprogrammer le vaisseau spatial"));
    if(!require(unrelated.isEmpty(),"unrelated question must not return an index"))return 1;
    std::puts("MEMS_NATIVE_FRENCH_INTENT_AND_OPERATION_TEST_PASS");
    return 0;
}
