#include "MemsGlobalSearchIndex.h"
#include "MemsReferenceDatabase.h"

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
#include <QUuid>
#include <QVariantMap>
#include <QXmlStreamReader>

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
        if(category==QChar::Mark_NonSpacing || category==QChar::Mark_SpacingCombining || category==QChar::Mark_Enclosing)
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
        if(hasDigit) word.replace(QLatin1Char('o'),QLatin1Char('0'));
    }
    return words.join(QLatin1Char(' '));
}

QString categoryForTable(const QString &tableName)
{
    const QString table=tableName.toLower();
    if(table.contains(QStringLiteral("dtc")) || table.contains(QStringLiteral("fault"))) return QStringLiteral("dtc");
    if(table.contains(QStringLiteral("connector")) || table.contains(QStringLiteral("pinout")) ||
       table.contains(QStringLiteral("wiring")) || table.contains(QStringLiteral("wire"))) return QStringLiteral("wiring");
    if(table.contains(QStringLiteral("actuator")) || table.contains(QStringLiteral("component"))) return QStringLiteral("actuator");
    if(table.contains(QStringLiteral("protocol_command")) || table.contains(QStringLiteral("command"))) return QStringLiteral("command");
    if(table.contains(QStringLiteral("protocol_data")) || table.contains(QStringLiteral("pid")) || table.contains(QStringLiteral("field"))) return QStringLiteral("data");
    if(table.contains(QStringLiteral("protocol_profile")) || table==QStringLiteral("protocol")) return QStringLiteral("protocol");
    if(table.contains(QStringLiteral("setting")) || table.contains(QStringLiteral("adaptation"))) return QStringLiteral("setting");
    if(table.contains(QStringLiteral("capabil"))) return QStringLiteral("capability");
    if(table.contains(QStringLiteral("fitment")) || table.contains(QStringLiteral("vehicle"))) return QStringLiteral("vehicle");
    if(table.contains(QStringLiteral("rom")) || table.contains(QStringLiteral("file")) ||
       table.contains(QStringLiteral("calibration")) || table.contains(QStringLiteral("map"))) return QStringLiteral("file");
    if(table.contains(QStringLiteral("ecu"))) return QStringLiteral("ecu");
    if(table.contains(QStringLiteral("source")) || table.contains(QStringLiteral("document")) || table.contains(QStringLiteral("reference"))) return QStringLiteral("documentation");
    return QStringLiteral("technical");
}

QString aliasesForTable(const QString &tableName)
{
    const QString category=categoryForTable(tableName);
    if(category==QStringLiteral("ecu")) return QStringLiteral("ecu calculateur boitier boîtier engine control unit");
    if(category==QStringLiteral("vehicle")) return QStringLiteral("vehicule véhicule voiture vehicle modele modèle model application affectation");
    if(category==QStringLiteral("dtc")) return QStringLiteral("dtc code codes defaut défaut défauts defauts panne pannes fault faults error erreur diagnostic");
    if(category==QStringLiteral("wiring")) return QStringLiteral("connecteur connectique connector broche broches pin pins cablage câblage fil fils wire wires wiring faisceau");
    if(category==QStringLiteral("actuator")) return QStringLiteral("actionneur actionneurs actuator actuators composant component test");
    if(category==QStringLiteral("command")) return QStringLiteral("commande commandes command commands diagnostic requete requête reponse réponse response");
    if(category==QStringLiteral("data")) return QStringLiteral("donnee donnée donnees données data pid capteur sensor mesure measure trame frame octet byte offset champ field decodage décodage");
    if(category==QStringLiteral("protocol")) return QStringLiteral("protocole protocol communication liaison serial serie série diagnostic");
    if(category==QStringLiteral("setting")) return QStringLiteral("reglage réglage reglages réglages setting settings adaptation calibration ajustement");
    if(category==QStringLiteral("capability")) return QStringLiteral("capacite capacité capacites capacités capability capabilities fonction fonctions function features");
    if(category==QStringLiteral("file")) return QStringLiteral("fichier file rom cartographie map maps calibration firmware dump checksum");
    if(category==QStringLiteral("documentation")) return QStringLiteral("documentation document fiche manual manuel source reference référence pdf xml technique technical");
    return QStringLiteral("technique technical information");
}

QString aliasesForColumn(const QString &columnName)
{
    const QString column=columnName.toLower();
    QStringList aliases;
    if(column.contains(QStringLiteral("pin"))) aliases<<QStringLiteral("broche pin connecteur cablage câblage fil wire");
    if(column.contains(QStringLiteral("wire")) || column.contains(QStringLiteral("colour")) || column.contains(QStringLiteral("color"))) aliases<<QStringLiteral("fil wire couleur color cablage câblage");
    if(column.contains(QStringLiteral("code"))) aliases<<QStringLiteral("code dtc defaut défaut panne fault");
    if(column.contains(QStringLiteral("command"))) aliases<<QStringLiteral("commande command diagnostic");
    if(column.contains(QStringLiteral("cause"))) aliases<<QStringLiteral("cause causes panne défaut defaut");
    if(column.contains(QStringLiteral("description")) || column.contains(QStringLiteral("note"))) aliases<<QStringLiteral("description notes information");
    if(column.contains(QStringLiteral("mems")) || column.contains(QStringLiteral("system")) ||
       column.contains(QStringLiteral("version")) || column.contains(QStringLiteral("generation"))) aliases<<QStringLiteral("mems generation génération version systeme système");
    if(column.contains(QStringLiteral("vehicle")) || column.contains(QStringLiteral("model")) ||
       column.contains(QStringLiteral("make")) || column.contains(QStringLiteral("variant"))) aliases<<QStringLiteral("vehicule véhicule voiture modele modèle marque version");
    if(column.contains(QStringLiteral("engine"))) aliases<<QStringLiteral("moteur engine");
    if(column.contains(QStringLiteral("field"))) aliases<<QStringLiteral("champ field donnee donnée");
    if(column.contains(QStringLiteral("offset")) || column.contains(QStringLiteral("byte"))) aliases<<QStringLiteral("offset position octet byte");
    if(column.contains(QStringLiteral("file")) || column.contains(QStringLiteral("rom"))) aliases<<QStringLiteral("fichier file rom cartographie");
    if(column.contains(QStringLiteral("signal"))) aliases<<QStringLiteral("signal fil wire broche pin");
    return aliases.join(QLatin1Char(' '));
}

QString aliasesForCategory(const QString &category)
{
    if(category==QStringLiteral("wiring")) return QStringLiteral("broche broches pin pins connecteur connectique cablage câblage fil fils wire wires wiring couleur color faisceau");
    if(category==QStringLiteral("dtc")) return QStringLiteral("dtc code codes defaut défaut défauts panne pannes fault faults diagnostic");
    if(category==QStringLiteral("command")) return QStringLiteral("commande commandes command commands requete requête diagnostic");
    if(category==QStringLiteral("protocol")) return QStringLiteral("protocole protocol communication liaison diagnostic");
    if(category==QStringLiteral("actuator")) return QStringLiteral("actionneur actionneurs actuator actuators test composant component");
    if(category==QStringLiteral("data")) return QStringLiteral("trame trames mesure mesures pid capteur sensor donnee donnée data octet byte");
    return QStringLiteral("documentation document fiche xml technique technical");
}

QString extractGeneration(const QString &text)
{
    static const QRegularExpression expression(QStringLiteral("(?:MEMS\\s*)?(1\\.[2369])"),QRegularExpression::CaseInsensitiveOption);
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
    static const QStringList preferred={QStringLiteral("part_number"),QStringLiteral("code"),QStringLiteral("command_hex"),
        QStringLiteral("component_name"),QStringLiteral("field_name_fr"),QStringLiteral("capability"),QStringLiteral("setting_name"),
        QStringLiteral("name_fr"),QStringLiteral("name"),QStringLiteral("model"),QStringLiteral("filename"),QStringLiteral("title")};
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
    static const QStringList preferred={QStringLiteral("id"),QStringLiteral("part_number"),QStringLiteral("code"),QStringLiteral("command_hex"),QStringLiteral("filename")};
    for(const QString &name:preferred){
        const int index=record.indexOf(name);
        if(index>=0){
            const QString value=query.value(index).toString().trimmed();
            if(!value.isEmpty()) return value;
        }
    }
    return QString::number(rowNumber);
}

bool insertDocument(QSqlDatabase &indexDatabase,bool ftsEnabled,const QString &category,const QString &sourceTable,
                    const QString &sourceKey,const QString &generation,const QString &title,const QString &content,const QString &searchable)
{
    const QString normalized=normalizeSearchText(searchable);
    if(normalized.isEmpty()) return true;

    QSqlQuery insert(indexDatabase);
    insert.prepare(QStringLiteral("INSERT INTO search_documents(category,source_table,source_key,generation,title,content,searchable,normalized) VALUES(:category,:source_table,:source_key,:generation,:title,:content,:searchable,:normalized)"));
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
    for(const QString &term:normalized.split(QLatin1Char(' '),Qt::SkipEmptyParts))
        if(!term.isEmpty() && term.size()<=80) uniqueTerms.insert(term);

    QSqlQuery termInsert(indexDatabase);
    termInsert.prepare(QStringLiteral("INSERT OR IGNORE INTO search_terms(term,document_id) VALUES(:term,:document_id)"));
    for(const QString &term:uniqueTerms){
        termInsert.bindValue(QStringLiteral(":term"),term);
        termInsert.bindValue(QStringLiteral(":document_id"),documentId);
        if(!termInsert.exec()) return false;
    }

    if(ftsEnabled){
        QSqlQuery fts(indexDatabase);
        fts.prepare(QStringLiteral("INSERT INTO search_fts(rowid,title,searchable,normalized,category,source_table,source_key,generation) VALUES(:rowid,:title,:searchable,:normalized,:category,:source_table,:source_key,:generation)"));
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
    QSqlQuery tableQuery(sourceDatabase);
    if(!tableQuery.exec(QStringLiteral("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%' ORDER BY name"))) return false;
    while(tableQuery.next()){
        const QString table=tableQuery.value(0).toString();
        if(!table.startsWith(QStringLiteral("search_"),Qt::CaseInsensitive)) tables.append(table);
    }

    for(const QString &table:tables){
        QSqlQuery rows(sourceDatabase);
        if(!rows.exec(QStringLiteral("SELECT * FROM %1").arg(quoteIdentifier(table)))) return false;
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
                }else text=value.toString().trimmed();
                if(text.isEmpty()) continue;
                const QString column=record.fieldName(i);
                displayParts<<QStringLiteral("%1: %2").arg(column,text);
                searchParts<<column<<aliasesForColumn(column)<<text;
                const QString lower=column.toLower();
                if(lower.contains(QStringLiteral("mems")) || lower.contains(QStringLiteral("system")) ||
                   lower.contains(QStringLiteral("version")) || lower.contains(QStringLiteral("generation"))) generationSource+=QLatin1Char(' ')+text;
            }
            const QString searchable=searchParts.join(QLatin1Char(' '));
            QString generation=extractGeneration(generationSource);
            if(generation.isEmpty()) generation=extractGeneration(searchable);
            if(!insertDocument(indexDatabase,ftsEnabled,categoryForTable(table),table,sourceKey,generation,
                               title.isEmpty()?table:title,displayParts.join(QStringLiteral("\n")),searchable)) return false;
        }
    }
    return true;
}

struct XmlLine
{
    QString section;
    QStringList cells;
    QStringList tags;
    QString searchable;
};

QString categoryForXmlLine(const XmlLine &line)
{
    const QString section=normalizeSearchText(line.section);
    if(section.contains(QStringLiteral("brochage")) || section.contains(QStringLiteral("connecteur")) ||
       section.contains(QStringLiteral("prise diagnostic")) || section.contains(QStringLiteral("pinout")) ||
       section.contains(QStringLiteral("cablage"))) return QStringLiteral("wiring");
    if(section.contains(QStringLiteral("dtc")) || section.contains(QStringLiteral("defaut")) || section.contains(QStringLiteral("panne"))) return QStringLiteral("dtc");
    if(section.contains(QStringLiteral("commande")) || section.contains(QStringLiteral("command"))) return QStringLiteral("command");
    if(section.contains(QStringLiteral("protocole")) || section.contains(QStringLiteral("communication"))) return QStringLiteral("protocol");
    if(section.contains(QStringLiteral("actionneur")) || section.contains(QStringLiteral("actuator"))) return QStringLiteral("actuator");
    if(section.contains(QStringLiteral("trame")) || section.contains(QStringLiteral("mesure")) || section.contains(QStringLiteral("pid"))) return QStringLiteral("data");
    if(line.tags.contains(QStringLiteral("broche")) || line.tags.contains(QStringLiteral("couleur"))) return QStringLiteral("wiring");
    return QStringLiteral("documentation");
}

QList<XmlLine> parseXmlLines(const QString &path)
{
    QList<XmlLine> lines;
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly|QIODevice::Text)) return lines;
    QXmlStreamReader xml(&file);
    QString section;
    while(!xml.atEnd()){
        xml.readNext();
        if(!xml.isStartElement()) continue;
        if(xml.name()==QStringLiteral("section")){
            section=xml.attributes().value(QStringLiteral("titre")).toString().simplified();
            continue;
        }
        if(xml.name()!=QStringLiteral("ligne")) continue;
        XmlLine line;
        line.section=section;
        QStringList rawParts;
        while(!xml.atEnd()){
            xml.readNext();
            if(xml.isEndElement() && xml.name()==QStringLiteral("ligne")) break;
            if(!xml.isStartElement()) continue;
            const QString tag=xml.name().toString().toCaseFolded();
            line.tags.append(tag);
            rawParts.append(tag);
            const auto attrs=xml.attributes();
            for(const QXmlStreamAttribute &attr:attrs){
                rawParts.append(attr.name().toString());
                rawParts.append(attr.value().toString());
            }
            const QString text=xml.readElementText(QXmlStreamReader::IncludeChildElements).simplified();
            if(!text.isEmpty()){
                rawParts.append(text);
                if(tag==QStringLiteral("cellule") || tag==QStringLiteral("broche") || tag==QStringLiteral("fonction") || tag==QStringLiteral("couleur")) line.cells.append(text);
            }
        }
        line.searchable=rawParts.join(QLatin1Char(' '));
        if(!line.searchable.trimmed().isEmpty()) lines.append(line);
    }
    return lines;
}

bool indexXmlDocument(QSqlDatabase &indexDatabase,bool ftsEnabled,const QString &path,const QString &generation)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly|QIODevice::Text)) return false;
    const QString raw=QString::fromUtf8(file.readAll());
    if(raw.trimmed().isEmpty()) return false;
    QString plain=raw;
    plain.replace(QRegularExpression(QStringLiteral("<[^>]+>")),QStringLiteral(" "));
    plain=plain.simplified();
    if(!insertDocument(indexDatabase,ftsEnabled,QStringLiteral("documentation"),QStringLiteral("xml_documentation"),generation,generation,
                       QStringLiteral("MEMS %1 XML").arg(generation),plain,QStringLiteral("MEMS %1 documentation xml %2").arg(generation,raw))) return false;

    const QList<XmlLine> lines=parseXmlLines(path);
    int rowNumber=0;
    for(const XmlLine &line:lines){
        ++rowNumber;
        const QString category=categoryForXmlLine(line);
        const QString visible=line.cells.isEmpty()?line.searchable:line.cells.join(QStringLiteral(" — "));
        const QString title=visible.left(220);
        const QString content=QStringLiteral("MEMS %1\n%2\n%3").arg(generation,line.section,visible);
        const QString searchable=QStringLiteral("MEMS %1 %2 %3 %4 %5").arg(generation,line.section,line.tags.join(QLatin1Char(' ')),line.searchable,aliasesForCategory(category));
        if(!insertDocument(indexDatabase,ftsEnabled,category,QStringLiteral("xml_row"),QStringLiteral("%1:%2").arg(generation).arg(rowNumber),
                           generation,title,content,searchable)) return false;
    }
    return true;
}

QString sourceSignature(const QString &databasePath,const QStringList &xmlPaths)
{
    QFileInfo databaseInfo(databasePath);
    QStringList parts;
    parts<<QStringLiteral("global-search-v3")<<QString::number(databaseInfo.size())<<QString::number(databaseInfo.lastModified().toMSecsSinceEpoch());
#ifdef APP_BUILD_NUMBER
    parts<<QStringLiteral(APP_BUILD_NUMBER);
#endif
    for(const QString &path:xmlPaths){
        QFileInfo info(path);
        parts<<QString::number(info.size())<<QString::number(info.lastModified().toMSecsSinceEpoch());
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
        if(query.exec()&&query.next()) matches=query.value(0).toString()==signature;
    }
    database.close();
    database=QSqlDatabase();
    QSqlDatabase::removeDatabase(connection);
    return matches;
}

bool validateIndex(QSqlDatabase &index,const QStringList &xmlPaths,const QStringList &xmlGenerations)
{
    QSqlQuery q(index);
    if(!q.exec(QStringLiteral("SELECT COUNT(*) FROM search_documents")) || !q.next() || q.value(0).toInt()<=0) return false;
    if(!q.exec(QStringLiteral("SELECT COUNT(*) FROM search_terms")) || !q.next() || q.value(0).toInt()<=0) return false;
    for(int i=0;i<xmlPaths.size();++i){
        if(!QFileInfo::exists(xmlPaths.at(i))) continue;
        QSqlQuery xml(index);
        xml.prepare(QStringLiteral("SELECT COUNT(*) FROM search_documents WHERE source_table='xml_row' AND generation=:generation"));
        xml.bindValue(QStringLiteral(":generation"),xmlGenerations.value(i));
        if(!xml.exec() || !xml.next() || xml.value(0).toInt()<=0) return false;
    }
    return true;
}

bool rebuildIndex(const QString &sourcePath,const QStringList &xmlPaths,const QStringList &xmlGenerations,
                  const QString &destination,const QString &signature,QString *errorMessage)
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
    bool ftsEnabled=false;
    if(ok){
        QSqlQuery setup(index);
        ok=setup.exec(QStringLiteral("CREATE TABLE search_meta(key TEXT PRIMARY KEY,value TEXT NOT NULL)"));
        if(ok) ok=setup.exec(QStringLiteral("CREATE TABLE search_documents(id INTEGER PRIMARY KEY AUTOINCREMENT,category TEXT NOT NULL,source_table TEXT NOT NULL,source_key TEXT,generation TEXT,title TEXT,content TEXT NOT NULL,searchable TEXT NOT NULL,normalized TEXT NOT NULL)"));
        if(ok) ok=setup.exec(QStringLiteral("CREATE TABLE search_terms(term TEXT NOT NULL,document_id INTEGER NOT NULL,PRIMARY KEY(term,document_id)) WITHOUT ROWID"));
        if(ok) ok=setup.exec(QStringLiteral("CREATE INDEX idx_search_documents_category ON search_documents(category)"));
        if(ok) ok=setup.exec(QStringLiteral("CREATE INDEX idx_search_documents_generation ON search_documents(generation)"));
        if(ok) ok=setup.exec(QStringLiteral("CREATE INDEX idx_search_documents_source ON search_documents(source_table,source_key)"));
        if(ok) ok=setup.exec(QStringLiteral("CREATE INDEX idx_search_terms_document ON search_terms(document_id)"));
        if(ok){
            QSqlQuery fts(index);
            ftsEnabled=fts.exec(QStringLiteral("CREATE VIRTUAL TABLE search_fts USING fts5(title,searchable,normalized,category UNINDEXED,source_table UNINDEXED,source_key UNINDEXED,generation UNINDEXED,tokenize='unicode61 remove_diacritics 2')"));
        }
        if(ok) ok=index.transaction();
        if(ok) ok=indexSourceTables(source,index,ftsEnabled);
        if(ok){
            for(int i=0;i<xmlPaths.size();++i){
                if(!QFileInfo::exists(xmlPaths.at(i))) continue;
                if(!indexXmlDocument(index,ftsEnabled,xmlPaths.at(i),xmlGenerations.value(i))){ok=false;break;}
            }
        }
        if(ok){
            QSqlQuery meta(index);
            meta.prepare(QStringLiteral("INSERT INTO search_meta(key,value) VALUES(:key,:value)"));
            const QList<QPair<QString,QString>> values={{QStringLiteral("schema_version"),QStringLiteral("3")},
                {QStringLiteral("source_signature"),signature},{QStringLiteral("fts5_enabled"),ftsEnabled?QStringLiteral("1"):QStringLiteral("0")}};
            for(const auto &entry:values){
                meta.bindValue(QStringLiteral(":key"),entry.first);
                meta.bindValue(QStringLiteral(":value"),entry.second);
                if(!meta.exec()){ok=false;break;}
            }
        }
        if(ok) ok=validateIndex(index,xmlPaths,xmlGenerations);
        if(ok) ok=index.commit(); else index.rollback();
    }

    if(!ok && errorMessage) *errorMessage=QStringLiteral("global-search-index-build-failed");
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
        const QStringList terms=normalizeSearchText(text).split(QLatin1Char(' '),Qt::SkipEmptyParts);
        QString sql=QStringLiteral("SELECT d.id,d.category,d.source_table,d.source_key,d.generation,d.title,d.content FROM search_documents d WHERE 1=1");
        if(!category.trimmed().isEmpty()) sql+=QStringLiteral(" AND d.category=:category");
        for(int i=0;i<terms.size();++i) sql+=QStringLiteral(" AND EXISTS(SELECT 1 FROM search_terms st%1 WHERE st%1.document_id=d.id AND st%1.term LIKE :term%1)").arg(i);
        sql+=QStringLiteral(" ORDER BY d.category,d.title LIMIT %1").arg(qBound(1,limit,500));
        QSqlQuery query(database);
        if(query.prepare(sql)){
            if(!category.trimmed().isEmpty()) query.bindValue(QStringLiteral(":category"),category.trimmed());
            for(int i=0;i<terms.size();++i) query.bindValue(QStringLiteral(":term%1").arg(i),terms.at(i)+QLatin1Char('%'));
            if(query.exec()){
                while(query.next()){
                    QVariantMap row;
                    const QSqlRecord record=query.record();
                    for(int i=0;i<record.count();++i) row.insert(record.fieldName(i),query.value(i));
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

}

QString MemsGlobalSearchIndex::indexPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+QStringLiteral("/reference/mems_global_search_r1.sqlite");
}

bool MemsGlobalSearchIndex::ensureBuilt(QString *errorMessage)
{
    MemsReferenceDatabase reference;
    if(!reference.open()){
        if(errorMessage) *errorMessage=QStringLiteral("reference-database-unavailable");
        return false;
    }
    const QStringList generations={QStringLiteral("1.3"),QStringLiteral("1.6"),QStringLiteral("1.9")};
    QStringList xmlPaths;
    for(const QString &generation:generations) xmlPaths<<reference.generationXmlPath(QStringLiteral("MEMS %1").arg(generation));
    const QString signature=sourceSignature(reference.databasePath(),xmlPaths);
    const QString destination=indexPath();
    if(metaMatches(destination,signature)) return true;
    return rebuildIndex(reference.databasePath(),xmlPaths,generations,destination,signature,errorMessage);
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
        if(query.exec(QStringLiteral("SELECT COUNT(*) FROM search_documents"))&&query.next()) count=query.value(0).toInt();
    }
    database.close();
    database=QSqlDatabase();
    QSqlDatabase::removeDatabase(connection);
    return count;
}

QVariantList MemsGlobalSearchIndex::search(const QString &text,const QString &category,int limit)
{
    if(!ensureBuilt()) return QVariantList();
    return queryIndex(indexPath(),text,category,limit);
}
