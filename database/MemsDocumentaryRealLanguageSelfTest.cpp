#include "MemsDocumentaryRepository.h"
#include "MemsDocumentaryPackCatalog.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>
#include <QVariantMap>
#include <cstdio>

static bool expect(bool ok,const char *message)
{
    if(!ok) std::fprintf(stderr,"REAL_DOCUMENTARY_LANGUAGE_FAIL %s\n",message);
    return ok;
}

static bool verify(const QString &path,const QString &language,
                   int documents,int operations,int steps,
                   const QStringList &requiredSources)
{
    if(!expect(QFileInfo(path).isFile(),"ORIGINAL_LANGUAGE_SQLITE_MISSING"))
        return false;
    QSettings settings(QSettings::IniFormat,QSettings::UserScope,
                       QStringLiteral("ECU_Mems_Manager"));
    settings.beginGroup(QStringLiteral("Settings"));
    settings.setValue(QStringLiteral("Language"),language);
    settings.endGroup();
    settings.sync();
    qputenv("MEMS_DOCUMENTARY_DB",QFile::encodeName(path));

    const QString connection=QStringLiteral("real-language-")+language;
    bool databaseOk=false;
    {
        QSqlDatabase db=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),connection);
        db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
        db.setDatabaseName(path);
        if(!expect(db.open(),"REAL_SQLITE_OPEN"))return false;
        QSqlQuery query(db);
        const auto count=[&query,&db](const QString &table)->int{
            if(!query.exec(QStringLiteral("SELECT COUNT(*) FROM %1").arg(table))
                || !query.next())return -1;
            return query.value(0).toInt();
        };
        databaseOk=expect(count(QStringLiteral("documentary_documents"))==documents,
                          "EXPECTED_MANUAL_COUNT") &&
                   expect(count(QStringLiteral("documentary_import_history"))==documents,
                          "IMMUTABLE_IMPORT_HISTORY_COUNT") &&
                   expect(count(QStringLiteral("documentary_operations"))==operations,
                          "ONE_OPERATION_CONTRACT") &&
                   expect(count(QStringLiteral("documentary_operation_steps"))==steps,
                          "ONE_ORDERED_STEP_CONTRACT") &&
                   expect(count(QStringLiteral("documentary_fts"))>0,"SOURCE_SEARCHABLE");
        db.close();
        db=QSqlDatabase();
    }
    QSqlDatabase::removeDatabase(connection);
    if(!databaseOk)return false;

    MemsDocumentaryRepository reader;
    if(!expect(reader.open(),"SESSION_NATIVE_DOCUMENTARY_READER")){
        std::fprintf(stderr,"%s\n",qPrintable(reader.lastError()));
        return false;
    }
    if(!expect(reader.search(QString(),3,true,language).size()>0,
               "DOCUMENTARY_SOURCE_BROWSE"))return false;
    const QString opposite=language==QStringLiteral("fr")
        ?QStringLiteral("en"):QStringLiteral("fr");
    if(!expect(reader.search(QString(),3,true,opposite).isEmpty(),
               "NO_OTHER_LANGUAGE_SUBSTITUTION"))return false;

    const QString readConn=QStringLiteral("operation-check-")+language;
    bool sourcesOk=true;
    {
        QSqlDatabase db=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),readConn);
        db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
        db.setDatabaseName(path);
        if(!expect(db.open(),"REAL_OPERATION_SOURCE_OPEN"))return false;
        for(const QString &source:requiredSources){
            QSqlQuery q(db);
            q.prepare(QStringLiteral(
                "SELECT source_entry_id FROM documentary_operations "
                "WHERE document_id=? ORDER BY source_entry_id LIMIT 1"));
            q.addBindValue(source);
            if(!expect(q.exec()&&q.next(),"PROCEDURE_FOR_EACH_ORIGINAL_MANUAL")){
                sourcesOk=false;break;
            }
            QVariantMap hit;
            hit.insert(QStringLiteral("entry_type"),QStringLiteral("procedure"));
            hit.insert(QStringLiteral("document_id"),source);
            hit.insert(QStringLiteral("entry_id"),q.value(0).toString());
            QStringList pictures;
            const QString answer=reader.procedureAnswer(hit,&pictures);
            if(!expect(answer.contains(source) &&
                       answer.contains(QStringLiteral("Étapes de l'opération")) &&
                       !answer.contains(QStringLiteral("Aucune procédure")),
                       "ACTUAL_MANUFACTURER_STEPS_RENDER_IN_SESSION")){
                sourcesOk=false;break;
            }
        }
        db.close();
        db=QSqlDatabase();
    }
    QSqlDatabase::removeDatabase(readConn);
    return sourcesOk;
}

int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv);
    if(!expect(argc==3,"REQUIRES_BOTH_EXISTING_LANGUAGE_BASES"))return 1;
    const QString fr=QString::fromLocal8Bit(argv[1]);
    const QString en=QString::fromLocal8Bit(argv[2]);
    if(!verify(fr,QStringLiteral("fr"),2,417,3415,{
        QStringLiteral("RCL0193FRE"),QStringLiteral("AKM7169FRE")}))return 1;
    if(!verify(en,QStringLiteral("en"),48,336,5055,{
        QStringLiteral("AKM7169ENG")}))return 1;
    std::puts("PASS_REAL_FR_EN_DOCUMENTARY_DATASETS_IN_SAME_SESSION_READER");
    return 0;
}
