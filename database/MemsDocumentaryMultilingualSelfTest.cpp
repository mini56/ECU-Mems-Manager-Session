#include "MemsDocumentaryRepository.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QVariantMap>
#include <cstdio>

static bool check(bool ok,const char *name)
{
    if(!ok) std::fprintf(stderr,"DOCUMENTARY_LANGUAGE_CONTRACT_FAIL %s\n",name);
    return ok;
}

static bool insert(QSqlDatabase &db,const QString &sql,const QVariantList &params={})
{
    QSqlQuery query(db);
    if(!query.prepare(sql))return false;
    for(const QVariant &param:params)query.addBindValue(param);
    if(!query.exec()){
        std::fprintf(stderr,"SQL_ERROR %s\n",qPrintable(query.lastError().text()));
        return false;
    }
    return true;
}

static bool createSource(const QString &path,const QString &lang,
                         const QString &doc,const QString &procedureKey,
                         const QString &title,const QString &subject)
{
    const QString connection=QStringLiteral("source_")+lang;
    bool success=false;
    {
        QSqlDatabase db=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),connection);
        db.setDatabaseName(path);
        if(db.open()){
            const QStringList schema={
                QStringLiteral("CREATE TABLE documentary_database("
                    "singleton INTEGER PRIMARY KEY,schema_version INTEGER,"
                    "language TEXT,publication_status TEXT)"),
                QStringLiteral("CREATE TABLE documentary_documents("
                    "document_id TEXT,source_revision TEXT,language TEXT)"),
                QStringLiteral("CREATE TABLE documentary_pages("
                    "document_id TEXT,source_revision TEXT,physical_page INTEGER)"),
                QStringLiteral("CREATE TABLE documentary_images("
                    "document_id TEXT,source_revision TEXT,physical_page INTEGER,"
                    "relative_path TEXT)"),
                QStringLiteral("CREATE TABLE documentary_entries("
                    "document_id TEXT,source_revision TEXT,entry_id TEXT,"
                    "entry_type TEXT,source_page INTEGER,title TEXT,"
                    "source_text TEXT,verification_status TEXT,"
                    "source_table TEXT,raw_json TEXT)"),
                QStringLiteral("CREATE VIRTUAL TABLE documentary_fts USING fts5("
                    "title,source_text,content='documentary_entries',"
                    "content_rowid='rowid',tokenize='unicode61 remove_diacritics 2')"),
                QStringLiteral("CREATE TRIGGER documentary_fts_insert AFTER INSERT "
                    "ON documentary_entries BEGIN "
                    "INSERT INTO documentary_fts(rowid,title,source_text) "
                    "VALUES(new.rowid,new.title,new.source_text); END")
            };
            success=true;
            for(const QString &sql:schema){
                QSqlQuery q(db);
                if(!q.exec(sql)){success=false;break;}
            }
            const auto row=[&](const QString &entry,const QString &kind,
                                int page,const QString &label,const QString &text,
                                const QString &table,const QJsonObject &raw){
                return insert(db,QStringLiteral(
                    "INSERT INTO documentary_entries VALUES(?,?,?,?,?,?,?,?,?,?)"),
                    {doc,QStringLiteral("rev1"),entry,kind,page,label,text,
                     QStringLiteral("review_required"),table,
                     QString::fromUtf8(QJsonDocument(raw).toJson(QJsonDocument::Compact))});
            };
            if(success)success=insert(db,QStringLiteral(
                "INSERT INTO documentary_database VALUES(1,1,?,'source_inventory')"),{lang});
            if(success)success=insert(db,QStringLiteral(
                "INSERT INTO documentary_documents VALUES(?,'rev1',?)"),{doc,lang});
            if(success)success=insert(db,QStringLiteral(
                "INSERT INTO documentary_pages VALUES(?,'rev1',1)"),{doc});
            if(success)success=insert(db,QStringLiteral(
                "INSERT INTO documentary_pages VALUES(?,'rev1',2)"),{doc});
            const bool english=lang==QStringLiteral("en");
            const QString keyField=english?QStringLiteral("procedure_id"):
                                          QStringLiteral("procedure_key");
            if(success)success=row(QStringLiteral("procedure:1"),
                QStringLiteral("procedure"),1,title,subject,
                QStringLiteral("source_procedure"),{{keyField,procedureKey}});
            for(int i=1;success&&i<=2;++i){
                const QString instruction=english
                    ? (i==1?QStringLiteral("Remove the cover"):
                             QStringLiteral("Inspect the gasket"))
                    : (i==1?QStringLiteral("Déposer le cache"):
                             QStringLiteral("Contrôler le joint"));
                QJsonObject raw{{keyField,procedureKey}};
                if(english){
                    raw.insert(QStringLiteral("step_number"),i);
                    raw.insert(QStringLiteral("source_text"),instruction);
                }else{
                    raw.insert(QStringLiteral("step_no"),i);
                    raw.insert(QStringLiteral("instruction_source"),instruction);
                }
                success=row(QStringLiteral("step:%1").arg(i),
                            QStringLiteral("step"),i,
                            QStringLiteral("Étape %1").arg(i),
                            instruction,english
                            ?QStringLiteral("akm7169eng_procedure_steps")
                            :QStringLiteral("mems_procedure_step"),raw);
            }
            // Both fixtures expose precisely the same normalized contract;
            // historical FR/EN key differences are not consulted by SESSION.
            const QStringList shared={
                QStringLiteral(
                    "CREATE TABLE documentary_operations("
                    "document_id TEXT,source_revision TEXT,operation_id TEXT,"
                    "source_entry_id TEXT,title TEXT,opening_page INTEGER,"
                    "verification_status TEXT)"),
                QStringLiteral(
                    "CREATE TABLE documentary_operation_steps("
                    "document_id TEXT,source_revision TEXT,operation_id TEXT,"
                    "source_entry_id TEXT,step_number INTEGER,phase TEXT,"
                    "source_page INTEGER,source_text TEXT,verification_status TEXT)"),
                QStringLiteral(
                    "CREATE TABLE documentary_operation_notices("
                    "document_id TEXT,source_revision TEXT,operation_id TEXT,"
                    "source_entry_id TEXT,source_page INTEGER,source_text TEXT,"
                    "verification_status TEXT)")
            };
            if(success){
                for(const QString &sql:shared){
                    QSqlQuery q(db);
                    if(!q.exec(sql)){success=false;break;}
                }
            }
            if(success)success=insert(db,QStringLiteral(
                "INSERT INTO documentary_operations VALUES(?,?,?,?,?,?,?)"),
                {doc,QStringLiteral("rev1"),procedureKey,
                 QStringLiteral("procedure:1"),title,1,
                 QStringLiteral("review_required")});
            for(int i=1;success&&i<=2;++i){
                const QString instruction=english
                    ? (i==1?QStringLiteral("Remove the cover"):
                             QStringLiteral("Inspect the gasket"))
                    : (i==1?QStringLiteral("Déposer le cache"):
                             QStringLiteral("Contrôler le joint"));
                success=insert(db,QStringLiteral(
                    "INSERT INTO documentary_operation_steps VALUES(?,?,?,?,?,?,?,?,?)"),
                    {doc,QStringLiteral("rev1"),procedureKey,
                     QStringLiteral("step:%1").arg(i),i,
                     QStringLiteral("remove"),i,instruction,
                     QStringLiteral("review_required")});
            }
            db.close();
        }
        db=QSqlDatabase();
    }
    QSqlDatabase::removeDatabase(connection);
    return success;
}

static bool verifyLanguage(const QString &path,const QString &lang,
                           const QString &question,const QString &expected,
                           const QString &document)
{
    QSettings settings(QSettings::IniFormat,QSettings::UserScope,
                       QStringLiteral("ECU_Mems_Manager"));
    settings.beginGroup(QStringLiteral("Settings"));
    settings.setValue(QStringLiteral("Language"),lang);
    settings.endGroup();
    settings.sync();
    qputenv("MEMS_DOCUMENTARY_DB",QFile::encodeName(path));
    MemsDocumentaryRepository repository;
    if(!check(repository.open(),"OPEN_NATIVE_LANGUAGE_DATABASE")){
        std::fprintf(stderr,"%s\n",qPrintable(repository.lastError()));
        return false;
    }
    QVariantMap procedure;
    for(const QVariant &value:repository.search(question,12,true,lang)){
        const QVariantMap hit=value.toMap();
        if(hit.value(QStringLiteral("entry_type")).toString()==QStringLiteral("procedure")){
            procedure=hit;
            break;
        }
    }
    if(!check(!procedure.isEmpty(),"SEARCH_FINDS_PROCEDURE"))return false;
    QStringList images;
    const QString result=repository.procedureAnswer(procedure,&images);
    if(!check(result.contains(expected) &&
              result.contains(QStringLiteral("2 étapes")) &&
              result.contains(document) &&
              result.contains(QStringLiteral("transcription à vérifier")),
              "SAME_PROCEDURE_CONTRACT_FR_EN"))return false;
    const QString opposite=lang==QStringLiteral("fr")
        ?QStringLiteral("en"):QStringLiteral("fr");
    if(!check(repository.search(question,12,true,opposite).isEmpty(),
              "NO_SILENT_FALLBACK_ACROSS_LANGUAGES"))return false;
    return true;
}

int main(int argc,char**argv)
{
    QCoreApplication app(argc,argv);
    QTemporaryDir dir;
    if(!check(dir.isValid(),"TEMPORARY_DIR"))return 1;
    const QString fr=QDir(dir.path()).filePath(QStringLiteral("fr.sqlite"));
    const QString en=QDir(dir.path()).filePath(QStringLiteral("en.sqlite"));
    if(!check(createSource(fr,QStringLiteral("fr"),QStringLiteral("RCL0193FRE"),
        QStringLiteral("PROC-FR"),QStringLiteral("Dépose joint culasse"),
        QStringLiteral("joint culasse")),"FR_TEST_SOURCE"))return 1;
    if(!check(createSource(en,QStringLiteral("en"),QStringLiteral("AKM7169ENG"),
        QStringLiteral("rover-akm7169eng-12.65.12"),
        QStringLiteral("Timing chain cover"),
        QStringLiteral("Timing chain")),"EN_TEST_SOURCE"))return 1;
    if(!verifyLanguage(fr,QStringLiteral("fr"),QStringLiteral("joint culasse"),
        QStringLiteral("Déposer le cache"),QStringLiteral("RCL0193FRE")))return 1;
    if(!verifyLanguage(en,QStringLiteral("en"),QStringLiteral("Timing chain"),
        QStringLiteral("Remove the cover"),QStringLiteral("AKM7169ENG")))return 1;
    std::puts("PASS_FR_EN_SHARED_PROCEDURE_READER_2_LANGUAGES");
    return 0;
}
