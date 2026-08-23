#include "MemsReferenceDatabase.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUuid>

#include <algorithm>

namespace {

QString cacheReferenceRoot()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        + QStringLiteral("/reference");
}

int manifestDatabaseRevision(const QString &root)
{
    QFile file(QDir(root).filePath(QStringLiteral("manifest.json")));
    if(!file.open(QIODevice::ReadOnly|QIODevice::Text)) return 0;
    const QJsonDocument document=QJsonDocument::fromJson(file.readAll());
    if(!document.isObject()) return 0;
    return document.object().value(QStringLiteral("database_revision")).toInt(0);
}

int cachedDatabaseRevision(const QString &databasePath)
{
    if(!QFileInfo::exists(databasePath)) return 0;
    const QString connection=QStringLiteral("MEMS_REFERENCE_REV_%1").arg(QUuid::createUuid().toString());
    QSqlDatabase database=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),connection);
    database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
    database.setDatabaseName(databasePath);
    int revision=0;
    if(database.open()){
        QSqlQuery query(database);
        if(query.exec(QStringLiteral("PRAGMA user_version")) && query.next())
            revision=query.value(0).toInt();
        database.close();
    }
    database=QSqlDatabase();
    QSqlDatabase::removeDatabase(connection);
    return revision;
}

bool expandQz64(const QString &source,const QString &destination)
{
    if(QFileInfo::exists(destination)) return true;
    QFile input(source);
    if(!input.open(QIODevice::ReadOnly|QIODevice::Text)) return false;
    const QByteArray raw=qUncompress(QByteArray::fromBase64(input.readAll().trimmed()));
    if(raw.isEmpty()) return false;
    QDir().mkpath(QFileInfo(destination).absolutePath());
    QFile output(destination);
    if(!output.open(QIODevice::WriteOnly|QIODevice::Truncate)) return false;
    return output.write(raw)==raw.size();
}

bool executeQz64Sql(QSqlDatabase &database,const QString &path)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly|QIODevice::Text)) return false;
    const QByteArray sqlBytes=qUncompress(QByteArray::fromBase64(file.readAll().trimmed()));
    if(sqlBytes.isEmpty()) return false;

    QSqlQuery query(database);
    const QList<QByteArray> statements=sqlBytes.split('\n');
    for(const QByteArray &line:statements){
        const QString statement=QString::fromUtf8(line).trimmed();
        if(statement.isEmpty() || statement.startsWith(QStringLiteral("--"))) continue;
        if(!query.exec(statement)) return false;
    }
    return true;
}

int numericSuffix(const QString &name)
{
    static const QRegularExpression rx(QStringLiteral("_(\\d+)\\.qz64$"),QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match=rx.match(name);
    return match.hasMatch()?match.captured(1).toInt():0;
}

QStringList seedFiles(const QString &root)
{
    QDir dir(root);
    QStringList names=dir.entryList(QStringList()<<QStringLiteral("mems_reference_seed_*.qz64"),QDir::Files,QDir::Name);
    std::sort(names.begin(),names.end(),[](const QString &a,const QString &b){
        const int na=numericSuffix(a),nb=numericSuffix(b);
        if(na!=nb) return na<nb;
        return a<b;
    });
    QStringList result;
    for(const QString &name:names) result.append(dir.filePath(name));
    return result;
}

QStringList enrichmentFiles(const QString &root)
{
    QStringList ordered;
    QSet<QString> seen;
    const QDir base(root);

    // Respect the manifest order first when it is present.
    QFile manifest(base.filePath(QStringLiteral("manifest.json")));
    if(manifest.open(QIODevice::ReadOnly|QIODevice::Text)){
        const QJsonDocument doc=QJsonDocument::fromJson(manifest.readAll());
        const QJsonArray batches=doc.object().value(QStringLiteral("research_enrichment_batches")).toArray();
        for(const QJsonValue &value:batches){
            const QString relative=QDir::cleanPath(value.toString()).replace(QLatin1Char('\\'),QLatin1Char('/'));
            if(relative.isEmpty() || relative.startsWith(QStringLiteral("../")) || relative==QStringLiteral("..")) continue;
            const QString absolute=base.filePath(relative);
            if(QFileInfo::exists(absolute) && !seen.contains(QFileInfo(absolute).canonicalFilePath())){
                ordered.append(absolute);
                seen.insert(QFileInfo(absolute).canonicalFilePath());
            }
        }
    }

    // Any future enrichment file is also discovered automatically even if
    // somebody forgot to extend the manifest. This prevents silent data loss.
    QStringList discovered;
    QDirIterator it(root,QStringList()<<QStringLiteral("research_enrichment*.qz64"),QDir::Files,QDirIterator::Subdirectories);
    while(it.hasNext()) discovered.append(it.next());
    std::sort(discovered.begin(),discovered.end(),[](const QString &a,const QString &b){
        const int na=numericSuffix(QFileInfo(a).fileName()),nb=numericSuffix(QFileInfo(b).fileName());
        if(na!=nb) return na<nb;
        return a<b;
    });
    for(const QString &absolute:discovered){
        const QString canonical=QFileInfo(absolute).canonicalFilePath();
        if(!seen.contains(canonical)){
            ordered.append(absolute);
            seen.insert(canonical);
        }
    }
    return ordered;
}

bool expandReferenceSheets(const QString &sourceRoot,const QString &cacheRoot)
{
    const QString fichesRoot=sourceRoot+QStringLiteral("/fiches");
    QDir dir(fichesRoot);
    if(!dir.exists()) return true;
    const QStringList files=dir.entryList(QStringList()<<QStringLiteral("*.qz64"),QDir::Files,QDir::Name);
    for(const QString &fileName:files){
        QString destinationName=fileName;
        destinationName.chop(QStringLiteral(".qz64").size());
        if(!expandQz64(dir.filePath(fileName),cacheRoot+QStringLiteral("/fiches/")+destinationName))
            return false;
    }
    return true;
}

QString assetGeneration(const QString &relativePath)
{
    const QString text=relativePath.toLower();
    const struct { const char *a; const char *b; const char *c; const char *value; } patterns[]={
        {"1_2","1.2","1-2","1.2"},
        {"1_3","1.3","1-3","1.3"},
        {"1_6","1.6","1-6","1.6"},
        {"1_9","1.9","1-9","1.9"}
    };
    for(const auto &pattern:patterns){
        if(text.contains(QString::fromLatin1(pattern.a)) || text.contains(QString::fromLatin1(pattern.b)) || text.contains(QString::fromLatin1(pattern.c)))
            return QString::fromLatin1(pattern.value);
    }
    if(text.contains(QStringLiteral("rosco")))
        return QStringLiteral("1.2/1.3/1.6");
    return QString();
}

QString humanStem(const QString &relativePath)
{
    QString text=QFileInfo(relativePath).completeBaseName();
    text.replace(QLatin1Char('_'),QLatin1Char(' '));
    text.replace(QLatin1Char('-'),QLatin1Char(' '));
    return text.simplified();
}

QString assetKind(const QString &relativePath,const QString &suffix)
{
    const QString lower=relativePath.toLower();
    if(lower.contains(QStringLiteral("rosco"))) return QStringLiteral("rosco");
    if(lower.contains(QStringLiteral("obd")) || lower.contains(QStringLiteral("j1962"))) return QStringLiteral("obd");
    if(lower.contains(QStringLiteral("connector")) || lower.contains(QStringLiteral("pinout"))) return QStringLiteral("ecu_connector");
    const QSet<QString> imageExtensions={QStringLiteral("svg"),QStringLiteral("png"),QStringLiteral("jpg"),QStringLiteral("jpeg"),QStringLiteral("webp"),QStringLiteral("gif")};
    if(imageExtensions.contains(suffix)) return QStringLiteral("diagram");
    return QStringLiteral("document");
}

QString localizedAssetName(const QString &kind,const QString &generation,const QString &fallback,const QString &language)
{
    QString label;
    if(language==QStringLiteral("fr")){
        if(kind==QStringLiteral("rosco")) label=QStringLiteral("Prise diagnostic Rover / ROSCO — 3 broches");
        else if(kind==QStringLiteral("obd")) label=QStringLiteral("Prise diagnostic OBD / J1962 — 16 broches");
        else if(kind==QStringLiteral("ecu_connector")) label=QStringLiteral("Connecteur ECU");
        else if(kind==QStringLiteral("diagram")) label=QStringLiteral("Schéma technique");
        else label=QStringLiteral("Document technique");
    }else if(language==QStringLiteral("es")){
        if(kind==QStringLiteral("rosco")) label=QStringLiteral("Conector de diagnóstico Rover / ROSCO — 3 pines");
        else if(kind==QStringLiteral("obd")) label=QStringLiteral("Conector de diagnóstico OBD / J1962 — 16 pines");
        else if(kind==QStringLiteral("ecu_connector")) label=QStringLiteral("Conector ECU");
        else if(kind==QStringLiteral("diagram")) label=QStringLiteral("Esquema técnico");
        else label=QStringLiteral("Documento técnico");
    }else if(language==QStringLiteral("it")){
        if(kind==QStringLiteral("rosco")) label=QStringLiteral("Connettore diagnostico Rover / ROSCO — 3 pin");
        else if(kind==QStringLiteral("obd")) label=QStringLiteral("Connettore diagnostico OBD / J1962 — 16 pin");
        else if(kind==QStringLiteral("ecu_connector")) label=QStringLiteral("Connettore ECU");
        else if(kind==QStringLiteral("diagram")) label=QStringLiteral("Schema tecnico");
        else label=QStringLiteral("Documento tecnico");
    }else if(language==QStringLiteral("pt")){
        if(kind==QStringLiteral("rosco")) label=QStringLiteral("Conector de diagnóstico Rover / ROSCO — 3 pinos");
        else if(kind==QStringLiteral("obd")) label=QStringLiteral("Conector de diagnóstico OBD / J1962 — 16 pinos");
        else if(kind==QStringLiteral("ecu_connector")) label=QStringLiteral("Conector ECU");
        else if(kind==QStringLiteral("diagram")) label=QStringLiteral("Esquema técnico");
        else label=QStringLiteral("Documento técnico");
    }else if(language==QStringLiteral("de")){
        if(kind==QStringLiteral("rosco")) label=QStringLiteral("Rover-/ROSCO-Diagnosestecker — 3-polig");
        else if(kind==QStringLiteral("obd")) label=QStringLiteral("OBD-/J1962-Diagnosestecker — 16-polig");
        else if(kind==QStringLiteral("ecu_connector")) label=QStringLiteral("ECU-Steckverbinder");
        else if(kind==QStringLiteral("diagram")) label=QStringLiteral("Technischer Schaltplan");
        else label=QStringLiteral("Technisches Dokument");
    }else{
        if(kind==QStringLiteral("rosco")) label=QStringLiteral("Rover / ROSCO diagnostic connector — 3 pin");
        else if(kind==QStringLiteral("obd")) label=QStringLiteral("OBD / J1962 diagnostic connector — 16 pin");
        else if(kind==QStringLiteral("ecu_connector")) label=QStringLiteral("ECU connector");
        else if(kind==QStringLiteral("diagram")) label=QStringLiteral("Technical diagram");
        else label=QStringLiteral("Technical document");
    }

    if(!generation.isEmpty() && kind!=QStringLiteral("rosco"))
        label=QStringLiteral("MEMS %1 — %2").arg(generation,label);
    if(kind==QStringLiteral("diagram") || kind==QStringLiteral("document"))
        label+=QStringLiteral(" — ")+fallback;
    return label;
}

bool registerReferenceAssets(QSqlDatabase &database,const QString &root)
{
    QSqlQuery setup(database);
    const QString schema=QStringLiteral(
        "(id INTEGER PRIMARY KEY AUTOINCREMENT,generation TEXT,name_fr TEXT,name_en TEXT,name_es TEXT,name_it TEXT,name_pt TEXT,name_de TEXT,"
        "relative_path TEXT NOT NULL UNIQUE,filename TEXT NOT NULL,file_type TEXT,keywords TEXT)");
    if(!setup.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS wiring_assets")+schema)) return false;
    if(!setup.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS document_assets")+schema)) return false;
    if(!setup.exec(QStringLiteral("DELETE FROM wiring_assets"))) return false;
    if(!setup.exec(QStringLiteral("DELETE FROM document_assets"))) return false;

    const QSet<QString> imageExtensions={QStringLiteral("svg"),QStringLiteral("png"),QStringLiteral("jpg"),QStringLiteral("jpeg"),QStringLiteral("webp"),QStringLiteral("gif")};
    const QSet<QString> documentExtensions={QStringLiteral("pdf"),QStringLiteral("html"),QStringLiteral("htm"),QStringLiteral("md"),QStringLiteral("txt"),QStringLiteral("csv"),QStringLiteral("xml")};
    QDir base(root);
    QDirIterator it(root,QDir::Files|QDir::NoDotAndDotDot,QDirIterator::Subdirectories);
    while(it.hasNext()){
        const QString absolute=it.next();
        const QFileInfo info(absolute);
        const QString suffix=info.suffix().toLower();
        if(!imageExtensions.contains(suffix) && !documentExtensions.contains(suffix)) continue;

        const QString relative=base.relativeFilePath(absolute).replace(QLatin1Char('\\'),QLatin1Char('/'));
        if(relative==QStringLiteral("manifest.json")) continue;
        const bool wiring=imageExtensions.contains(suffix);
        const QString table=wiring?QStringLiteral("wiring_assets"):QStringLiteral("document_assets");
        const QString generation=assetGeneration(relative);
        const QString kind=assetKind(relative,suffix);
        const QString fallback=humanStem(relative);
        const QString keywords=QStringLiteral(
            "MEMS %1 schema schéma diagram diagramme image illustration connector connecteur brochage pinout wiring cablage câblage "
            "diagnostic diagnostique obd j1962 rosco documentation document fiche source recurso recurso técnico esquema conector pines "
            "cablaggio connettore pinagem stecker pinbelegung schaltplan technical technique técnico tecnico technisch %2 %3")
            .arg(generation,relative,fallback);

        QSqlQuery insert(database);
        insert.prepare(QStringLiteral(
            "INSERT INTO %1(generation,name_fr,name_en,name_es,name_it,name_pt,name_de,relative_path,filename,file_type,keywords) "
            "VALUES(:generation,:fr,:en,:es,:it,:pt,:de,:path,:filename,:type,:keywords)").arg(table));
        insert.bindValue(QStringLiteral(":generation"),generation);
        insert.bindValue(QStringLiteral(":fr"),localizedAssetName(kind,generation,fallback,QStringLiteral("fr")));
        insert.bindValue(QStringLiteral(":en"),localizedAssetName(kind,generation,fallback,QStringLiteral("en")));
        insert.bindValue(QStringLiteral(":es"),localizedAssetName(kind,generation,fallback,QStringLiteral("es")));
        insert.bindValue(QStringLiteral(":it"),localizedAssetName(kind,generation,fallback,QStringLiteral("it")));
        insert.bindValue(QStringLiteral(":pt"),localizedAssetName(kind,generation,fallback,QStringLiteral("pt")));
        insert.bindValue(QStringLiteral(":de"),localizedAssetName(kind,generation,fallback,QStringLiteral("de")));
        insert.bindValue(QStringLiteral(":path"),relative);
        insert.bindValue(QStringLiteral(":filename"),info.fileName());
        insert.bindValue(QStringLiteral(":type"),suffix);
        insert.bindValue(QStringLiteral(":keywords"),keywords);
        if(!insert.exec()) return false;
    }
    return true;
}

}

MemsReferenceDatabase::MemsReferenceDatabase()
    : m_connectionName(QStringLiteral("MEMS_REFERENCE_%1").arg(QUuid::createUuid().toString()))
{
}

MemsReferenceDatabase::~MemsReferenceDatabase(){close();}

QString MemsReferenceDatabase::referenceRoot() const
{
    return QCoreApplication::applicationDirPath()+QStringLiteral("/database/reference");
}

bool MemsReferenceDatabase::open()
{
    if(m_database.isOpen()) return true;

    const QString cacheRoot=cacheReferenceRoot();
    QDir().mkpath(cacheRoot);
    m_databasePath=cacheRoot+QStringLiteral("/ecu_mems_reference_r5.sqlite");

    const int expectedRevision=manifestDatabaseRevision(referenceRoot());
    if(QFileInfo::exists(m_databasePath) && expectedRevision>0
       && cachedDatabaseRevision(m_databasePath)!=expectedRevision){
        if(!QFile::remove(m_databasePath))
            return false;
    }

    if(!QFileInfo::exists(m_databasePath)){
        const QString seedConnection=QStringLiteral("%1_SEED").arg(m_connectionName);
        QSqlDatabase buildDb=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),seedConnection);
        buildDb.setDatabaseName(m_databasePath);
        if(!buildDb.open()){
            buildDb=QSqlDatabase();
            QSqlDatabase::removeDatabase(seedConnection);
            return false;
        }

        QByteArray encoded;
        bool ok=true;
        const QStringList parts=seedFiles(referenceRoot());
        if(parts.isEmpty()) ok=false;
        for(const QString &partPath:parts){
            QFile seed(partPath);
            if(!seed.open(QIODevice::ReadOnly|QIODevice::Text)){ok=false;break;}
            encoded+=seed.readAll().trimmed();
        }
        QByteArray sqlBytes;
        if(ok) sqlBytes=qUncompress(QByteArray::fromBase64(encoded));
        if(sqlBytes.isEmpty()) ok=false;

        QSqlQuery query(buildDb);
        if(ok){
            const QList<QByteArray> statements=sqlBytes.split('\n');
            for(const QByteArray &line:statements){
                const QString statement=QString::fromUtf8(line).trimmed();
                if(statement.isEmpty()) continue;
                if(!query.exec(statement)){ok=false;break;}
            }
        }

        if(ok){
            const QStringList enrichments=enrichmentFiles(referenceRoot());
            for(const QString &path:enrichments){
                if(!executeQz64Sql(buildDb,path)){ok=false;break;}
            }
        }
        if(ok) ok=registerReferenceAssets(buildDb,referenceRoot());
        if(ok && expectedRevision>0)
            ok=query.exec(QStringLiteral("PRAGMA user_version=%1").arg(expectedRevision));

        buildDb.close();
        buildDb=QSqlDatabase();
        QSqlDatabase::removeDatabase(seedConnection);
        if(!ok){QFile::remove(m_databasePath);return false;}
    }

    if(!expandReferenceSheets(referenceRoot(),cacheRoot)) return false;

    m_database=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),m_connectionName);
    m_database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
    m_database.setDatabaseName(m_databasePath);
    if(!m_database.open()) return false;
    QSqlQuery query(m_database); query.exec(QStringLiteral("PRAGMA query_only = ON"));
    return true;
}

void MemsReferenceDatabase::close()
{
    if(!m_database.isValid()) return;
    if(m_database.isOpen()) m_database.close();
    const QString connection=m_connectionName;
    m_database=QSqlDatabase();
    QSqlDatabase::removeDatabase(connection);
}

bool MemsReferenceDatabase::isOpen() const{return m_database.isValid()&&m_database.isOpen();}
QString MemsReferenceDatabase::databasePath() const{return m_databasePath;}

QVariantList MemsReferenceDatabase::queryList(const QString &sql,const QVariantMap &bindings) const
{
    QVariantList rows;
    if(!isOpen()) return rows;
    QSqlQuery query(m_database);
    if(!query.prepare(sql)) return rows;
    for(auto it=bindings.constBegin();it!=bindings.constEnd();++it) query.bindValue(it.key(),it.value());
    if(!query.exec()) return rows;
    while(query.next()){
        QVariantMap row; const QSqlRecord record=query.record();
        for(int i=0;i<record.count();++i) row.insert(record.fieldName(i),query.value(i));
        rows.append(row);
    }
    return rows;
}

QVariantMap MemsReferenceDatabase::queryOne(const QString &sql,const QVariantMap &bindings) const
{
    const QVariantList rows=queryList(sql,bindings); return rows.isEmpty()?QVariantMap():rows.first().toMap();
}

int MemsReferenceDatabase::scalarInt(const QString &sql) const
{
    if(!isOpen()) return 0; QSqlQuery query(m_database); if(!query.exec(sql)||!query.next()) return 0; return query.value(0).toInt();
}

QVariantList MemsReferenceDatabase::searchEcus(const QString &text,const QString &generation,int limit) const
{
    QString sql=QStringLiteral(
        "SELECT e.part_number,e.brand,e.system_family,e.injection,e.makes,e.models,e.year_from,e.year_to,e.n_fitments,e.n_files,e.notes "
        "FROM ecu_target e WHERE 1=1");
    QVariantMap bind;

    const QString generationText=generation.trimmed();
    if(!generationText.isEmpty()){
        sql+=QStringLiteral(" AND e.system_family LIKE :generation COLLATE NOCASE");
        bind.insert(QStringLiteral(":generation"),QStringLiteral("%%1%").arg(generationText));
    }

    const QString searchText=text.trimmed();
    if(!searchText.isEmpty()){
        const QString pattern=QStringLiteral("%%1%").arg(searchText);
        sql+=QStringLiteral(
            " AND ("
            "e.part_number LIKE :q0 COLLATE NOCASE OR "
            "COALESCE(e.brand,'') LIKE :q1 COLLATE NOCASE OR "
            "COALESCE(e.system_family,'') LIKE :q2 COLLATE NOCASE OR "
            "COALESCE(e.injection,'') LIKE :q3 COLLATE NOCASE OR "
            "COALESCE(e.makes,'') LIKE :q4 COLLATE NOCASE OR "
            "COALESCE(e.models,'') LIKE :q5 COLLATE NOCASE OR "
            "COALESCE(e.notes,'') LIKE :q6 COLLATE NOCASE OR "
            "EXISTS (SELECT 1 FROM ecu_fitment_target f WHERE f.part_number=e.part_number AND ("
            "COALESCE(f.make,'') LIKE :q7 COLLATE NOCASE OR "
            "COALESCE(f.model,'') LIKE :q8 COLLATE NOCASE OR "
            "COALESCE(f.variant,'') LIKE :q9 COLLATE NOCASE OR "
            "COALESCE(f.engine,'') LIKE :q10 COLLATE NOCASE OR "
            "COALESCE(f.market,'') LIKE :q11 COLLATE NOCASE OR "
            "COALESCE(f.alt_refs,'') LIKE :q12 COLLATE NOCASE)))");
        for(int i=0;i<=12;++i)
            bind.insert(QStringLiteral(":q%1").arg(i),pattern);
    }

    sql+=QStringLiteral(" ORDER BY e.system_family,e.part_number LIMIT %1")
        .arg(qBound(1,limit,1000));
    return queryList(sql,bind);
}

QVariantMap MemsReferenceDatabase::ecu(const QString &partNumber) const
{
    QVariantMap b; b.insert(QStringLiteral(":part"),partNumber); return queryOne(QStringLiteral("SELECT * FROM ecu_target WHERE part_number=:part LIMIT 1"),b);
}

QVariantList MemsReferenceDatabase::fitments(const QString &partNumber) const
{
    QVariantMap b; b.insert(QStringLiteral(":part"),partNumber); return queryList(QStringLiteral("SELECT * FROM ecu_fitment_target WHERE part_number=:part ORDER BY make,model,year_from,market"),b);
}

QVariantList MemsReferenceDatabase::files(const QString &partNumber) const
{
    QVariantMap b; b.insert(QStringLiteral(":part"),partNumber); return queryList(QStringLiteral("SELECT part_number,system,vehicle,filename,url FROM ecu_file WHERE part_number=:part ORDER BY vehicle,filename"),b);
}

QString MemsReferenceDatabase::generationToken(const QString &systemFamily) const
{
    if(systemFamily.contains(QStringLiteral("1.9"))) return QStringLiteral("1.9");
    if(systemFamily.contains(QStringLiteral("1.6"))) return QStringLiteral("1.6");
    if(systemFamily.contains(QStringLiteral("1.3"))) return QStringLiteral("1.3");
    if(systemFamily.contains(QStringLiteral("1.2"))) return QStringLiteral("1.2");
    return QString();
}

QVariantList MemsReferenceDatabase::protocolProfiles(const QString &systemFamily) const
{
    const QString token=generationToken(systemFamily); if(token.isEmpty()) return QVariantList(); QVariantMap b; b.insert(QStringLiteral(":token"),token); return queryList(QStringLiteral("SELECT * FROM protocol_profiles WHERE mems_version LIKE '%' || :token || '%' ORDER BY mems_version"),b);
}

QVariantList MemsReferenceDatabase::protocolCommands(const QString &systemFamily,int limit) const
{
    const QString token=generationToken(systemFamily); if(token.isEmpty()) return QVariantList(); QVariantMap b; b.insert(QStringLiteral(":token"),token); b.insert(QStringLiteral(":limit"),qBound(1,limit,500)); return queryList(QStringLiteral("SELECT command_hex,category,name_fr,response_format,confidence,safety_level,notes FROM protocol_commands WHERE mems_versions LIKE '%' || :token || '%' ORDER BY category,command_hex LIMIT :limit"),b);
}

QVariantList MemsReferenceDatabase::protocolCapabilities(const QString &systemFamily,int limit) const
{
    const QString token=generationToken(systemFamily); if(token.isEmpty()) return QVariantList(); QVariantMap b; b.insert(QStringLiteral(":token"),token); b.insert(QStringLiteral(":limit"),qBound(1,limit,500)); return queryList(QStringLiteral("SELECT capability,availability,access_method,safety_level,notes FROM protocol_capabilities WHERE mems_version LIKE '%' || :token || '%' ORDER BY capability LIMIT :limit"),b);
}

QVariantList MemsReferenceDatabase::protocolDataFields(const QString &systemFamily,int limit) const
{
    const QString token=generationToken(systemFamily); if(token.isEmpty()) return QVariantList(); QVariantMap b; b.insert(QStringLiteral(":token"),token); b.insert(QStringLiteral(":limit"),qBound(1,limit,500)); return queryList(QStringLiteral("SELECT packet_command,byte_offset,field_name_fr,bit_definition,decoding_formula,unit,confidence,notes FROM protocol_data_fields WHERE mems_versions LIKE '%' || :token || '%' ORDER BY packet_command,byte_offset LIMIT :limit"),b);
}

QVariantList MemsReferenceDatabase::protocolSettings(const QString &systemFamily,int limit) const
{
    const QString token=generationToken(systemFamily); if(token.isEmpty()) return QVariantList(); QVariantMap b; b.insert(QStringLiteral(":token"),token); b.insert(QStringLiteral(":limit"),qBound(1,limit,500)); return queryList(QStringLiteral("SELECT setting_name,default_hex,minimum_hex,maximum_hex,increment_command,decrement_command,scaling,confidence FROM protocol_settings WHERE mems_versions LIKE '%' || :token || '%' ORDER BY setting_name LIMIT :limit"),b);
}

QVariantList MemsReferenceDatabase::dtcs(const QString &systemFamily) const
{
    const QString token=generationToken(systemFamily); QVariantMap b; b.insert(QStringLiteral(":token"),token); return queryList(QStringLiteral("SELECT code,system,description,possible_causes FROM dtc_codes WHERE system LIKE '%tous%' OR (:token<>'' AND system LIKE '%' || :token || '%') ORDER BY code"),b);
}

QVariantList MemsReferenceDatabase::actuatorSpecs() const{return queryList(QStringLiteral("SELECT component_name,nominal_resistance_ohms,control_pinout,testing_notes FROM actuator_specs ORDER BY component_name"));}
int MemsReferenceDatabase::targetEcuCount() const{return scalarInt(QStringLiteral("SELECT COUNT(*) FROM ecu_target"));}
int MemsReferenceDatabase::targetFitmentCount() const{return scalarInt(QStringLiteral("SELECT COUNT(*) FROM ecu_fitment_target"));}
int MemsReferenceDatabase::targetFileCount() const{return scalarInt(QStringLiteral("SELECT COUNT(*) FROM ecu_file WHERE part_number IN (SELECT part_number FROM ecu_target)"));}
int MemsReferenceDatabase::commandCount() const{return scalarInt(QStringLiteral("SELECT COUNT(*) FROM protocol_commands"));}
int MemsReferenceDatabase::dataFieldCount() const{return scalarInt(QStringLiteral("SELECT COUNT(*) FROM protocol_data_fields"));}
int MemsReferenceDatabase::capabilityCount() const{return scalarInt(QStringLiteral("SELECT COUNT(*) FROM protocol_capabilities"));}

QString MemsReferenceDatabase::generationXmlPath(const QString &systemFamily) const
{
    const QString token=generationToken(systemFamily); const QString root=cacheReferenceRoot()+QStringLiteral("/fiches/");
    if(token==QStringLiteral("1.2")) return root+QStringLiteral("mems_1_2.xml");
    if(token==QStringLiteral("1.3")) return root+QStringLiteral("mems_1_3.xml");
    if(token==QStringLiteral("1.6")) return root+QStringLiteral("mems_1_6.xml");
    if(token==QStringLiteral("1.9")) return root+QStringLiteral("mems_1_9.xml");
    return QString();
}

QString MemsReferenceDatabase::generationPdfPath(const QString &systemFamily) const
{
    const QString token=generationToken(systemFamily);
    if(token==QStringLiteral("1.3")) return referenceRoot()+QStringLiteral("/pdf/mems1.3_source.pdf");
    if(token==QStringLiteral("1.6")) return referenceRoot()+QStringLiteral("/pdf/mems1.6_source.pdf");
    if(token==QStringLiteral("1.9")) return referenceRoot()+QStringLiteral("/pdf/mems1.9_source.pdf");
    return QString();
}
