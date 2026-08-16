#include "MemsGlobalSearchIndex.h"
#include "MemsReferenceDatabase.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPair>
#include <QRegularExpression>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QTimer>
#include <QUuid>
#include <QVariantMap>

namespace {

QString quoteIdentifier(QString value)
{
    value.replace(QLatin1Char('"'),QStringLiteral("\"\""));
    return QStringLiteral("\"%1\"").arg(value);
}

QString normalizeSearchText(const QString &input)
{
    const QString decomposed=input.normalized(QString::NormalizationForm_D).toCaseFolded();
    QString plain;
    plain.reserve(decomposed.size());
    bool previousSpace=true;
    for(const QChar ch:decomposed){
        const QChar::Category category=ch.category();
        if(category==QChar::Mark_NonSpacing ||
           category==QChar::Mark_SpacingCombining ||
           category==QChar::Mark_Enclosing)
            continue;
        if(ch.isLetterOrNumber()){
            plain+=ch;
            previousSpace=false;
        }else if(!previousSpace){
            plain+=QLatin1Char(' ');
            previousSpace=true;
        }
    }

    QStringList words=plain.simplified().split(QLatin1Char(' '),Qt::SkipEmptyParts);
    for(QString &word:words){
        bool hasDigit=false;
        for(const QChar ch:word){
            if(ch.isDigit()){hasDigit=true;break;}
        }
        if(hasDigit)
            word.replace(QLatin1Char('o'),QLatin1Char('0'));
    }
    return words.join(QLatin1Char(' '));
}

QString categoryForTable(const QString &tableName)
{
    const QString table=tableName.toLower();
    if(table.contains(QStringLiteral("dtc")) || table.contains(QStringLiteral("fault")))
        return QStringLiteral("dtc");
    if(table.contains(QStringLiteral("connector")) || table.contains(QStringLiteral("pinout")) ||
       table.contains(QStringLiteral("wiring")) || table.contains(QStringLiteral("wire")))
        return QStringLiteral("wiring");
    if(table.contains(QStringLiteral("actuator")) || table.contains(QStringLiteral("component")))
        return QStringLiteral("actuator");
    if(table.contains(QStringLiteral("protocol_command")) || table.contains(QStringLiteral("command")))
        return QStringLiteral("command");
    if(table.contains(QStringLiteral("protocol_data")) || table.contains(QStringLiteral("pid")) ||
       table.contains(QStringLiteral("field")))
        return QStringLiteral("data");
    if(table.contains(QStringLiteral("protocol_profile")) || table==QStringLiteral("protocol"))
        return QStringLiteral("protocol");
    if(table.contains(QStringLiteral("setting")) || table.contains(QStringLiteral("adaptation")))
        return QStringLiteral("setting");
    if(table.contains(QStringLiteral("capabil")))
        return QStringLiteral("capability");
    if(table.contains(QStringLiteral("fitment")) || table.contains(QStringLiteral("vehicle")))
        return QStringLiteral("vehicle");
    if(table.contains(QStringLiteral("rom")) || table.contains(QStringLiteral("file")) ||
       table.contains(QStringLiteral("calibration")) || table.contains(QStringLiteral("map")))
        return QStringLiteral("file");
    if(table.contains(QStringLiteral("ecu")))
        return QStringLiteral("ecu");
    if(table.contains(QStringLiteral("source")) || table.contains(QStringLiteral("document")) ||
       table.contains(QStringLiteral("reference")))
        return QStringLiteral("documentation");
    return QStringLiteral("technical");
}

QString aliasesForTable(const QString &tableName)
{
    const QString category=categoryForTable(tableName);
    if(category==QStringLiteral("ecu"))
        return QStringLiteral("ecu calculateur boitier boîtier engine control unit centralita centralina steuergeraet steuergerät");
    if(category==QStringLiteral("vehicle"))
        return QStringLiteral("vehicule véhicule voiture vehicle modelo modèle model veicolo fahrzeug application affectation");
    if(category==QStringLiteral("dtc"))
        return QStringLiteral("dtc code codes defaut défaut défauts defauts panne pannes fault faults error erreur fehler fallo guasto avaria diagnostic");
    if(category==QStringLiteral("wiring"))
        return QStringLiteral("connecteur connectique connector broche broches pin pins cablage câblage cable cableado cablaggio verkabelung fil fils wire wires wiring faisceau");
    if(category==QStringLiteral("actuator"))
        return QStringLiteral("actionneur actionneurs actuator actuators actuateur actuador attuatore stellglied composant component test");
    if(category==QStringLiteral("command"))
        return QStringLiteral("commande commandes command commands diagnostic requete requête response reponse réponse");
    if(category==QStringLiteral("data"))
        return QStringLiteral("donnee donnée donnees données data pid capteur sensor mesure measure trame frame packet octet byte offset champ field decodage décodage");
    if(category==QStringLiteral("protocol"))
        return QStringLiteral("protocole protocol protocolo protokoll communication liaison serial serie série diagnostic");
    if(category==QStringLiteral("setting"))
        return QStringLiteral("reglage réglage reglages réglages setting settings adaptation calibration ajustement");
    if(category==QStringLiteral("capability"))
        return QStringLiteral("capacite capacité capacites capacités capability capabilities fonction fonctions function features");
    if(category==QStringLiteral("file"))
        return QStringLiteral("fichier file rom cartographie map maps calibration firmware dump checksum");
    if(category==QStringLiteral("documentation"))
        return QStringLiteral("documentation document fiche manual manuel source reference référence pdf xml technique technical");
    return QStringLiteral("technique technical information");
}

QString aliasesForColumn(const QString &columnName)
{
    const QString column=columnName.toLower();
    QStringList aliases;
    if(column.contains(QStringLiteral("pin"))) aliases<<QStringLiteral("broche pin connecteur cablage câblage fil wire");
    if(column.contains(QStringLiteral("wire")) || column.contains(QStringLiteral("colour")) ||
       column.contains(QStringLiteral("color"))) aliases<<QStringLiteral("fil wire couleur color cablage câblage");
    if(column.contains(QStringLiteral("code"))) aliases<<QStringLiteral("code dtc defaut défaut panne fault");
    if(column.contains(QStringLiteral("command"))) aliases<<QStringLiteral("commande command diagnostic");
    if(column.contains(QStringLiteral("cause"))) aliases<<QStringLiteral("cause causes panne défaut defaut");
    if(column.contains(QStringLiteral("description")) || column.contains(QStringLiteral("note"))) aliases<<QStringLiteral("description notes information");
    if(column.contains(QStringLiteral("mems")) || column.contains(QStringLiteral("system")) ||
       column.contains(QStringLiteral("version")) || column.contains(QStringLiteral("generation")))
        aliases<<QStringLiteral("mems generation génération version systeme système");
    if(column.contains(QStringLiteral("vehicle")) || column.contains(QStringLiteral("model")) ||
       column.contains(QStringLiteral("make")) || column.contains(QStringLiteral("variant")))
        aliases<<QStringLiteral("vehicule véhicule voiture modele modèle marque version");
    if(column.contains(QStringLiteral("engine"))) aliases<<QStringLiteral("moteur engine");
    if(column.contains(QStringLiteral("field"))) aliases<<QStringLiteral("champ field donnee donnée");
    if(column.contains(QStringLiteral("offset")) || column.contains(QStringLiteral("byte"))) aliases<<QStringLiteral("offset position octet byte");
    if(column.contains(QStringLiteral("file")) || column.contains(QStringLiteral("rom"))) aliases<<QStringLiteral("fichier file rom cartographie");
    if(column.contains(QStringLiteral("signal"))) aliases<<QStringLiteral("signal fil wire broche pin");
    return aliases.join(QLatin1Char(' '));
}

QString extractGeneration(const QString &text)
{
    static const QRegularExpression expression(
        QStringLiteral("(?:MEMS\\s*)?(1\\.[2369])"),
        QRegularExpression::CaseInsensitiveOption);
    QSet<QString> seen;
    QStringList generations;
    QRegularExpressionMatchIterator it=expression.globalMatch(text);
    while(it.hasNext()){
        const QString generation=it.next().captured(1);
        if(!seen.contains(generation)){
            seen.insert(generation);
            generations.append(generation);
        }
    }
    return generations.join(QLatin1Char('/'));
}

QString preferredTitle(const QSqlRecord &record,const QSqlQuery &query)
{
    static const QStringList preferred={
        QStringLiteral("part_number"),QStringLiteral("code"),QStringLiteral("command_hex"),
        QStringLiteral("component_name"),QStringLiteral("field_name_fr"),QStringLiteral("capability"),
        QStringLiteral("setting_name"),QStringLiteral("name_fr"),QStringLiteral("name"),
        QStringLiteral("model"),QStringLiteral("filename"),QStringLiteral("title")
    };
    for(const QString &name:preferred){
        const int index=record.indexOf(name);
        if(index>=0){
            const QString value=query.value(index).toString().trimmed();
            if(!value.isEmpty()) return value;
        }
    }
    for(int i=0;i<record.count();++i){
        const QString value=query.value(i).toString().trimmed();
        if(!value.isEmpty()) return value.left(120);
    }
    return QString();
}

QString preferredKey(const QSqlRecord &record,const QSqlQuery &query,int rowNumber)
{
    static const QStringList preferred={
        QStringLiteral("id"),QStringLiteral("part_number"),QStringLiteral("code"),
        QStringLiteral("command_hex"),QStringLiteral("filename")
    };
    for(const QString &name:preferred){
        const int index=record.indexOf(name);
        if(index>=0){
            const QString value=query.value(index).toString().trimmed();
            if(!value.isEmpty()) return value;
        }
    }
    return QString::number(rowNumber);
}

bool insertDocument(QSqlDatabase &indexDatabase,
                    bool ftsEnabled,
                    const QString &category,
                    const QString &sourceTable,
                    const QString &sourceKey,
                    const QString &generation,
                    const QString &title,
                    const QString &content,
                    const QString &searchable)
{
    const QString normalized=normalizeSearchText(searchable);
    if(normalized.isEmpty()) return true;

    QSqlQuery insert(indexDatabase);
    insert.prepare(QStringLiteral(
        "INSERT INTO search_documents(category,source_table,source_key,generation,title,content,searchable,normalized) "
        "VALUES(:category,:source_table,:source_key,:generation,:title,:content,:searchable,:normalized)"));
    insert.bindValue(QStringLiteral(":category"),category);
    insert.bindValue(QStringLiteral(":source_table"),sourceTable);
    insert.bindValue(QStringLiteral(":source_key"),sourceKey);
    insert.bindValue(QStringLiteral(":generation"),generation);
    insert.bindValue(QStringLiteral(":title"),title);
    insert.bindValue(QStringLiteral(":content"),content);
    insert.bindValue(QStringLiteral(":searchable"),searchable);
    insert.bindValue(QStringLiteral(":normalized"),normalized);
    if(!insert.exec()) return false;
    const qlonglong documentId=insert.lastInsertId().toLongLong();

    QSet<QString> uniqueTerms;
    const QStringList terms=normalized.split(QLatin1Char(' '),Qt::SkipEmptyParts);
    for(const QString &term:terms){
        if(term.isEmpty() || term.size()>80) continue;
        uniqueTerms.insert(term);
    }
    QSqlQuery termInsert(indexDatabase);
    termInsert.prepare(QStringLiteral("INSERT OR IGNORE INTO search_terms(term,document_id) VALUES(:term,:document_id)"));
    for(const QString &term:uniqueTerms){
        termInsert.bindValue(QStringLiteral(":term"),term);
        termInsert.bindValue(QStringLiteral(":document_id"),documentId);
        if(!termInsert.exec()) return false;
    }

    if(ftsEnabled){
        QSqlQuery fts(indexDatabase);
        fts.prepare(QStringLiteral(
            "INSERT INTO search_fts(rowid,title,searchable,normalized,category,source_table,source_key,generation) "
            "VALUES(:rowid,:title,:searchable,:normalized,:category,:source_table,:source_key,:generation)"));
        fts.bindValue(QStringLiteral(":rowid"),documentId);
        fts.bindValue(QStringLiteral(":title"),title);
        fts.bindValue(QStringLiteral(":searchable"),searchable);
        fts.bindValue(QStringLiteral(":normalized"),normalized);
        fts.bindValue(QStringLiteral(":category"),category);
        fts.bindValue(QStringLiteral(":source_table"),sourceTable);
        fts.bindValue(QStringLiteral(":source_key"),sourceKey);
        fts.bindValue(QStringLiteral(":generation"),generation);
        if(!fts.exec()) return false;
    }
    return true;
}

bool indexSourceTables(QSqlDatabase &sourceDatabase,QSqlDatabase &indexDatabase,bool ftsEnabled)
{
    QStringList tables;
    {
        QSqlQuery tableQuery(sourceDatabase);
        if(!tableQuery.exec(QStringLiteral(
            "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%' ORDER BY name")))
            return false;
        while(tableQuery.next()){
            const QString table=tableQuery.value(0).toString();
            if(!table.startsWith(QStringLiteral("search_"),Qt::CaseInsensitive))
                tables.append(table);
        }
    }

    for(const QString &table:tables){
        QSqlQuery rows(sourceDatabase);
        if(!rows.exec(QStringLiteral("SELECT * FROM %1").arg(quoteIdentifier(table))))
            return false;
        int rowNumber=0;
        while(rows.next()){
            ++rowNumber;
            const QSqlRecord record=rows.record();
            const QString title=preferredTitle(record,rows);
            const QString sourceKey=preferredKey(record,rows,rowNumber);
            QStringList displayParts;
            QStringList searchParts;
            searchParts<<table<<categoryForTable(table)<<aliasesForTable(table);

            QString generationSource;
            for(int i=0;i<record.count();++i){
                const QVariant value=rows.value(i);
                if(value.isNull()) continue;
                QString text;
                if(value.type()==QVariant::ByteArray){
                    const QByteArray bytes=value.toByteArray();
                    if(bytes.size()>128) continue;
                    text=QString::fromLatin1(bytes.toHex(' '));
                }else{
                    text=value.toString().trimmed();
                }
                if(text.isEmpty()) continue;
                const QString column=record.fieldName(i);
                displayParts<<QStringLiteral("%1: %2").arg(column,text);
                searchParts<<column<<aliasesForColumn(column)<<text;

                const QString lower=column.toLower();
                if(lower.contains(QStringLiteral("mems")) || lower.contains(QStringLiteral("system")) ||
                   lower.contains(QStringLiteral("version")) || lower.contains(QStringLiteral("generation")))
                    generationSource+=QLatin1Char(' ')+text;
            }

            const QString content=displayParts.join(QStringLiteral("\n"));
            const QString searchable=searchParts.join(QLatin1Char(' '));
            QString generation=extractGeneration(generationSource);
            if(generation.isEmpty()) generation=extractGeneration(searchable);
            if(!insertDocument(indexDatabase,ftsEnabled,categoryForTable(table),table,sourceKey,
                               generation,title.isEmpty()?table:title,content,searchable))
                return false;
        }
    }
    return true;
}

bool indexXmlDocument(QSqlDatabase &indexDatabase,
                      bool ftsEnabled,
                      const QString &path,
                      const QString &generation)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly|QIODevice::Text)) return false;
    const QString raw=QString::fromUtf8(file.readAll());
    if(raw.trimmed().isEmpty()) return false;

    QString plain=raw;
    plain.replace(QRegularExpression(QStringLiteral("<[^>]+>")),QStringLiteral(" "));
    plain=plain.simplified();

    const QString title=QStringLiteral("MEMS %1 XML").arg(generation);
    const QString searchable=QStringLiteral(
        "documentation document fiche xml technique technical mems %1 brochage broche connecteur cablage câblage fil wire %2")
        .arg(generation,raw);
    return insertDocument(indexDatabase,ftsEnabled,QStringLiteral("documentation"),
                          QStringLiteral("xml_documentation"),generation,generation,
                          title,plain,searchable);
}

QString sourceSignature(const QString &databasePath,const QStringList &xmlPaths)
{
    QFileInfo databaseInfo(databasePath);
    QStringList parts;
    parts<<QString::number(databaseInfo.size())
         <<QString::number(databaseInfo.lastModified().toMSecsSinceEpoch());
    for(const QString &path:xmlPaths){
        QFileInfo info(path);
        parts<<QString::number(info.size())
             <<QString::number(info.lastModified().toMSecsSinceEpoch());
    }
    return parts.join(QLatin1Char(':'));
}

bool metaMatches(const QString &path,const QString &signature)
{
    if(!QFileInfo::exists(path)) return false;
    const QString connection=QStringLiteral("MEMS_SEARCH_META_%1").arg(QUuid::createUuid().toString());
    QSqlDatabase database=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),connection);
    database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
    database.setDatabaseName(path);
    bool matches=false;
    if(database.open()){
        QSqlQuery query(database);
        query.prepare(QStringLiteral("SELECT value FROM search_meta WHERE key='source_signature'"));
        if(query.exec()&&query.next())
            matches=query.value(0).toString()==signature;
    }
    database.close();
    database=QSqlDatabase();
    QSqlDatabase::removeDatabase(connection);
    return matches;
}

bool rebuildIndex(const QString &sourcePath,
                  const QStringList &xmlPaths,
                  const QStringList &xmlGenerations,
                  const QString &destination,
                  const QString &signature,
                  QString *errorMessage)
{
    QFile::remove(destination);
    QDir().mkpath(QFileInfo(destination).absolutePath());

    const QString sourceConnection=QStringLiteral("MEMS_SEARCH_SOURCE_%1").arg(QUuid::createUuid().toString());
    const QString indexConnection=QStringLiteral("MEMS_SEARCH_INDEX_%1").arg(QUuid::createUuid().toString());

    QSqlDatabase source=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),sourceConnection);
    source.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
    source.setDatabaseName(sourcePath);

    QSqlDatabase index=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),indexConnection);
    index.setDatabaseName(destination);

    bool ok=source.open()&&index.open();
    if(ok){
        QSqlQuery setup(index);
        ok=setup.exec(QStringLiteral(
            "CREATE TABLE search_meta(key TEXT PRIMARY KEY,value TEXT NOT NULL)"));
        if(ok) ok=setup.exec(QStringLiteral(
            "CREATE TABLE search_documents("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "category TEXT NOT NULL,"
            "source_table TEXT NOT NULL,"
            "source_key TEXT,"
            "generation TEXT,"
            "title TEXT,"
            "content TEXT NOT NULL,"
            "searchable TEXT NOT NULL,"
            "normalized TEXT NOT NULL)"));
        if(ok) ok=setup.exec(QStringLiteral(
            "CREATE TABLE search_terms("
            "term TEXT NOT NULL,"
            "document_id INTEGER NOT NULL,"
            "PRIMARY KEY(term,document_id)) WITHOUT ROWID"));
        if(ok) ok=setup.exec(QStringLiteral("CREATE INDEX idx_search_documents_category ON search_documents(category)"));
        if(ok) ok=setup.exec(QStringLiteral("CREATE INDEX idx_search_documents_generation ON search_documents(generation)"));
        if(ok) ok=setup.exec(QStringLiteral("CREATE INDEX idx_search_documents_source ON search_documents(source_table,source_key)"));
        if(ok) ok=setup.exec(QStringLiteral("CREATE INDEX idx_search_terms_document ON search_terms(document_id)"));

        bool ftsEnabled=false;
        if(ok){
            QSqlQuery fts(index);
            ftsEnabled=fts.exec(QStringLiteral(
                "CREATE VIRTUAL TABLE search_fts USING fts5("
                "title,searchable,normalized,"
                "category UNINDEXED,source_table UNINDEXED,source_key UNINDEXED,generation UNINDEXED,"
                "tokenize='unicode61 remove_diacritics 2')"));
        }

        if(ok) ok=index.transaction();
        if(ok) ok=indexSourceTables(source,index,ftsEnabled);
        if(ok){
            for(int i=0;i<xmlPaths.size();++i){
                if(!QFileInfo::exists(xmlPaths.at(i))) continue;
                if(!indexXmlDocument(index,ftsEnabled,xmlPaths.at(i),xmlGenerations.value(i))){
                    ok=false;
                    break;
                }
            }
        }

        if(ok){
            QSqlQuery meta(index);
            meta.prepare(QStringLiteral("INSERT INTO search_meta(key,value) VALUES(:key,:value)"));
            const QList<QPair<QString,QString>> values={
                {QStringLiteral("schema_version"),QStringLiteral("1")},
                {QStringLiteral("source_signature"),signature},
                {QStringLiteral("fts5_enabled"),ftsEnabled?QStringLiteral("1"):QStringLiteral("0")}
            };
            for(const auto &entry:values){
                meta.bindValue(QStringLiteral(":key"),entry.first);
                meta.bindValue(QStringLiteral(":value"),entry.second);
                if(!meta.exec()){ok=false;break;}
            }
        }

        if(ok) ok=index.commit();
        else index.rollback();

        if(ok){
            QSqlQuery count(index);
            ok=count.exec(QStringLiteral("SELECT COUNT(*) FROM search_documents")) &&
               count.next() && count.value(0).toInt()>0;
        }
    }

    if(!ok && errorMessage)
        *errorMessage=QStringLiteral("global-search-index-build-failed");

    source.close();
    index.close();
    source=QSqlDatabase();
    index=QSqlDatabase();
    QSqlDatabase::removeDatabase(sourceConnection);
    QSqlDatabase::removeDatabase(indexConnection);

    if(!ok) QFile::remove(destination);
    return ok;
}

QVariantList queryIndex(const QString &path,const QString &text,const QString &category,int limit)
{
    QVariantList rows;
    if(!QFileInfo::exists(path)) return rows;

    const QString connection=QStringLiteral("MEMS_SEARCH_QUERY_%1").arg(QUuid::createUuid().toString());
    QSqlDatabase database=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),connection);
    database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
    database.setDatabaseName(path);

    if(database.open()){
        const QString normalized=normalizeSearchText(text);
        const QStringList terms=normalized.split(QLatin1Char(' '),Qt::SkipEmptyParts);
        QString sql=QStringLiteral(
            "SELECT id,category,source_table,source_key,generation,title,content "
            "FROM search_documents WHERE 1=1");
        if(!category.trimmed().isEmpty())
            sql+=QStringLiteral(" AND category=:category");
        for(int i=0;i<terms.size();++i)
            sql+=QStringLiteral(" AND normalized LIKE :q%1").arg(i);
        sql+=QStringLiteral(" ORDER BY category,title LIMIT %1").arg(qBound(1,limit,500));

        QSqlQuery query(database);
        if(query.prepare(sql)){
            if(!category.trimmed().isEmpty())
                query.bindValue(QStringLiteral(":category"),category.trimmed());
            for(int i=0;i<terms.size();++i)
                query.bindValue(QStringLiteral(":q%1").arg(i),QStringLiteral("%%1%").arg(terms.at(i)));
            if(query.exec()){
                while(query.next()){
                    QVariantMap row;
                    const QSqlRecord record=query.record();
                    for(int i=0;i<record.count();++i)
                        row.insert(record.fieldName(i),query.value(i));
                    rows.append(row);
                }
            }
        }
    }

    database.close();
    database=QSqlDatabase();
    QSqlDatabase::removeDatabase(connection);
    return rows;
}

void installGlobalSearchIndex()
{
    QCoreApplication *application=QCoreApplication::instance();
    if(!application) return;
    QTimer::singleShot(1500,application,[](){
        MemsGlobalSearchIndex::ensureBuilt();
    });
}

}

QString MemsGlobalSearchIndex::indexPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        +QStringLiteral("/reference/mems_global_search_r1.sqlite");
}

bool MemsGlobalSearchIndex::ensureBuilt(QString *errorMessage)
{
    MemsReferenceDatabase reference;
    if(!reference.open()){
        if(errorMessage) *errorMessage=QStringLiteral("reference-database-unavailable");
        return false;
    }

    const QStringList generations={
        QStringLiteral("1.3"),QStringLiteral("1.6"),QStringLiteral("1.9")
    };
    QStringList xmlPaths;
    for(const QString &generation:generations)
        xmlPaths<<reference.generationXmlPath(QStringLiteral("MEMS %1").arg(generation));

    const QString signature=sourceSignature(reference.databasePath(),xmlPaths);
    const QString destination=indexPath();
    if(metaMatches(destination,signature))
        return true;

    return rebuildIndex(reference.databasePath(),xmlPaths,generations,
                        destination,signature,errorMessage);
}

int MemsGlobalSearchIndex::documentCount()
{
    if(!ensureBuilt()) return 0;
    const QString connection=QStringLiteral("MEMS_SEARCH_COUNT_%1").arg(QUuid::createUuid().toString());
    QSqlDatabase database=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),connection);
    database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
    database.setDatabaseName(indexPath());
    int count=0;
    if(database.open()){
        QSqlQuery query(database);
        if(query.exec(QStringLiteral("SELECT COUNT(*) FROM search_documents"))&&query.next())
            count=query.value(0).toInt();
    }
    database.close();
    database=QSqlDatabase();
    QSqlDatabase::removeDatabase(connection);
    return count;
}

QVariantList MemsGlobalSearchIndex::search(const QString &text,
                                           const QString &category,
                                           int limit)
{
    if(!ensureBuilt()) return QVariantList();
    return queryIndex(indexPath(),text,category,limit);
}

Q_COREAPP_STARTUP_FUNCTION(installGlobalSearchIndex)
