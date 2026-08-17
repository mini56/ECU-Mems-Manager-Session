#include "MemsReferenceDatabase.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUuid>

namespace {

QString cacheReferenceRoot()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        + QStringLiteral("/reference");
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
        for(int part=1;part<=4;++part){
            QFile seed(referenceRoot()+QStringLiteral("/mems_reference_seed_%1.qz64").arg(part));
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

        if(ok)
            ok=executeQz64Sql(buildDb,referenceRoot()+QStringLiteral("/research_enrichment.qz64"));
        if(ok)
            ok=executeQz64Sql(buildDb,referenceRoot()+QStringLiteral("/research_enrichment_500.qz64"));

        buildDb.close();
        buildDb=QSqlDatabase();
        QSqlDatabase::removeDatabase(seedConnection);
        if(!ok){QFile::remove(m_databasePath);return false;}
    }

    const QString fiches=cacheRoot+QStringLiteral("/fiches");
    expandQz64(referenceRoot()+QStringLiteral("/fiches/mems_1_2.xml.qz64"),fiches+QStringLiteral("/mems_1_2.xml"));
    expandQz64(referenceRoot()+QStringLiteral("/fiches/mems_1_3.xml.qz64"),fiches+QStringLiteral("/mems_1_3.xml"));
    expandQz64(referenceRoot()+QStringLiteral("/fiches/mems_1_6.xml.qz64"),fiches+QStringLiteral("/mems_1_6.xml"));
    expandQz64(referenceRoot()+QStringLiteral("/fiches/mems_1_9.xml.qz64"),fiches+QStringLiteral("/mems_1_9.xml"));

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
