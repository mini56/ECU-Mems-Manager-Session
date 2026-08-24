#include "MemsGlobalSearchIndex.h"
#include "MemsReferenceDatabase.h"

#include <QCoreApplication>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QUuid>

#include <cstdio>

namespace {

QString normalized(QString input)
{
    input=input.normalized(QString::NormalizationForm_D).toCaseFolded();
    QString out;
    bool space=true;
    for(const QChar ch:input){
        const QChar::Category category=ch.category();
        if(category==QChar::Mark_NonSpacing || category==QChar::Mark_SpacingCombining || category==QChar::Mark_Enclosing)
            continue;
        if(ch.isLetterOrNumber()){
            out+=ch;
            space=false;
        }else if(!space){
            out+=QLatin1Char(' ');
            space=true;
        }
    }
    return out.simplified();
}

QString quoteIdentifier(QString value)
{
    value.replace(QLatin1Char('"'),QStringLiteral("\"\""));
    return QStringLiteral("\"%1\"").arg(value);
}

QString rowText(const QVariantMap &row)
{
    return row.value(QStringLiteral("title")).toString()
        + QLatin1Char('\n')
        + row.value(QStringLiteral("content")).toString();
}

void printLine(const QString &line)
{
    const QByteArray bytes=line.toUtf8();
    std::fprintf(stderr,"%s\n",bytes.constData());
    std::fflush(stderr);
}

bool hasResult(const QVariantList &rows,const QStringList &needles)
{
    for(const QVariant &value:rows){
        const QString text=normalized(rowText(value.toMap()));
        bool all=true;
        for(const QString &needle:needles){
            if(!text.contains(normalized(needle))){
                all=false;
                break;
            }
        }
        if(all) return true;
    }
    return false;
}

bool requireSearch(const QString &query,const QString &category,const QStringList &needles)
{
    const QVariantList rows=MemsGlobalSearchIndex::search(query,category,200);
    if(rows.isEmpty()){
        printLine(QStringLiteral("FAIL search '%1' category '%2': no result").arg(query,category));
        return false;
    }
    if(!hasResult(rows,needles)){
        printLine(QStringLiteral("FAIL search '%1' category '%2': expected relationship not found").arg(query,category));
        for(int i=0;i<qMin(rows.size(),5);++i)
            printLine(QStringLiteral("  -> %1").arg(rowText(rows.at(i).toMap()).replace(QLatin1Char('\n'),QStringLiteral(" | "))));
        return false;
    }
    printLine(QStringLiteral("PASS search '%1' category '%2' (%3 result(s))").arg(query,category).arg(rows.size()));
    return true;
}

bool requireSourceTable(const QString &query,const QString &category,const QString &sourceTable,const QStringList &needles)
{
    const QVariantList rows=MemsGlobalSearchIndex::search(query,category,200);
    for(const QVariant &value:rows){
        const QVariantMap row=value.toMap();
        if(row.value(QStringLiteral("source_table")).toString()!=sourceTable) continue;
        if(hasResult(QVariantList{row},needles)){
            printLine(QStringLiteral("PASS resource '%1': table=%2 title=%3")
                      .arg(query,sourceTable,row.value(QStringLiteral("title")).toString()));
            return true;
        }
    }
    printLine(QStringLiteral("FAIL resource '%1': source table %2 not indexed/searchable")
              .arg(query,sourceTable));
    return false;
}

bool requireFirstResult(const QString &query,const QString &expectedCategory,const QStringList &needles)
{
    const QVariantList rows=MemsGlobalSearchIndex::search(query,QString(),50);
    if(rows.isEmpty()){
        printLine(QStringLiteral("FAIL relevance '%1': no result").arg(query));
        return false;
    }
    const QVariantMap first=rows.first().toMap();
    if(first.value(QStringLiteral("category")).toString()!=expectedCategory ||
       !hasResult(QVariantList{first},needles)){
        printLine(QStringLiteral("FAIL relevance '%1': first result is [%2] %3")
                  .arg(query,first.value(QStringLiteral("category")).toString(),first.value(QStringLiteral("title")).toString()));
        return false;
    }
    printLine(QStringLiteral("PASS relevance '%1': [%2] %3")
              .arg(query,expectedCategory,first.value(QStringLiteral("title")).toString()));
    return true;
}

bool requireCompleteSqliteCoverage()
{
    MemsReferenceDatabase reference;
    if(!reference.open()){
        printLine(QStringLiteral("FAIL coverage: cannot open MEMS reference database"));
        return false;
    }

    const QString sourceConnection=QStringLiteral("MEMS_SELFTEST_SOURCE_%1").arg(QUuid::createUuid().toString());
    const QString indexConnection=QStringLiteral("MEMS_SELFTEST_INDEX_%1").arg(QUuid::createUuid().toString());
    bool ok=true;
    int sourceRows=0;
    int indexedRows=0;

    {
        QSqlDatabase source=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),sourceConnection);
        source.setDatabaseName(reference.databasePath());
        QSqlDatabase index=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),indexConnection);
        index.setDatabaseName(MemsGlobalSearchIndex::indexPath());

        if(!source.open() || !index.open()){
            printLine(QStringLiteral("FAIL coverage: cannot open source/index SQLite files"));
            ok=false;
        }else{
            QSqlQuery tables(source);
            if(!tables.exec(QStringLiteral("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%' ORDER BY name"))){
                printLine(QStringLiteral("FAIL coverage: cannot enumerate source tables"));
                ok=false;
            }else{
                while(tables.next()){
                    const QString table=tables.value(0).toString();
                    if(table.startsWith(QStringLiteral("search_"),Qt::CaseInsensitive)) continue;

                    QSqlQuery sourceCount(source);
                    const QString countSql=QStringLiteral("SELECT COUNT(*) FROM %1").arg(quoteIdentifier(table));
                    if(!sourceCount.exec(countSql) || !sourceCount.next()){
                        printLine(QStringLiteral("FAIL coverage: cannot count source table %1").arg(table));
                        ok=false;
                        continue;
                    }
                    const int expected=sourceCount.value(0).toInt();
                    sourceRows+=expected;

                    QSqlQuery indexCount(index);
                    indexCount.prepare(QStringLiteral("SELECT COUNT(*) FROM search_documents WHERE source_table=:table"));
                    indexCount.bindValue(QStringLiteral(":table"),table);
                    if(!indexCount.exec() || !indexCount.next()){
                        printLine(QStringLiteral("FAIL coverage: cannot count indexed table %1").arg(table));
                        ok=false;
                        continue;
                    }
                    const int actual=indexCount.value(0).toInt();
                    indexedRows+=actual;
                    if(actual!=expected){
                        printLine(QStringLiteral("FAIL coverage table=%1 source_rows=%2 indexed_rows=%3")
                                  .arg(table).arg(expected).arg(actual));
                        ok=false;
                    }
                }
            }

            QSqlQuery xml(index);
            if(!xml.exec(QStringLiteral("SELECT DISTINCT generation FROM search_documents WHERE source_table='xml_documentation'"))){
                printLine(QStringLiteral("FAIL coverage: cannot inspect XML documentation index"));
                ok=false;
            }else{
                QSet<QString> generations;
                while(xml.next()) generations.insert(xml.value(0).toString());
                const QStringList required={QStringLiteral("1.2"),QStringLiteral("1.3"),QStringLiteral("1.6"),QStringLiteral("1.9")};
                for(const QString &generation:required){
                    if(!generations.contains(generation)){
                        printLine(QStringLiteral("FAIL coverage: MEMS %1 XML is not indexed").arg(generation));
                        ok=false;
                    }
                }
            }
        }

        if(source.isOpen()) source.close();
        if(index.isOpen()) index.close();
    }
    QSqlDatabase::removeDatabase(sourceConnection);
    QSqlDatabase::removeDatabase(indexConnection);
    reference.close();

    if(ok)
        printLine(QStringLiteral("PASS exhaustive SQLite coverage: source_rows=%1 indexed_rows=%2 + XML 1.2/1.3/1.6/1.9")
                  .arg(sourceRows).arg(indexedRows));
    return ok;
}

}

int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv);
    QCoreApplication::setOrganizationName(QStringLiteral("ECU-Mems-Manager"));
    QCoreApplication::setApplicationName(QStringLiteral("ECU-Mems-Manager-Search-SelfTest"));

    printLine(QStringLiteral("START MEMS search semantic self-test"));

    QString error;
    if(!MemsGlobalSearchIndex::ensureBuilt(&error)){
        printLine(QStringLiteral("FAIL index build: %1").arg(error));
        return 2;
    }

    printLine(QStringLiteral("INDEX documents=%1").arg(MemsGlobalSearchIndex::documentCount()));

    bool ok=true;
    ok=requireCompleteSqliteCoverage() && ok;
    ok=requireSearch(QStringLiteral("vert rouge"),QStringLiteral("wiring"),
                     {QStringLiteral("IAT"),QStringLiteral("Vert / Rouge")}) && ok;
    ok=requireSearch(QStringLiteral("IAT"),QStringLiteral("wiring"),
                     {QStringLiteral("IAT")}) && ok;
    ok=requireSearch(QStringLiteral("temperature air"),QStringLiteral("wiring"),
                     {QStringLiteral("temperature air"),QStringLiteral("Vert / Rouge")}) && ok;
    ok=requireSearch(QStringLiteral("D0"),QStringLiteral("command"),
                     {QStringLiteral("D0")}) && ok;
    ok=requireSearch(QStringLiteral("P0115"),QStringLiteral("dtc"),
                     {QStringLiteral("P0115")}) && ok;
    ok=requireSearch(QStringLiteral("tension batterie"),QStringLiteral("data"),
                     {QStringLiteral("Tension batterie")}) && ok;
    ok=requireSearch(QStringLiteral("9600"),QStringLiteral("protocol"),
                     {QStringLiteral("ROSCO"),QStringLiteral("9600")}) && ok;
    ok=requireSearch(QStringLiteral("connecteur 36 voies"),QStringLiteral("wiring"),
                     {QStringLiteral("MEMS 1.2"),QStringLiteral("36 voies")}) && ok;

    // Regression guard for build 534: diagrams/resources must be part of the
    // global searchable database, not merely copied next to the executable.
    ok=requireSourceTable(QStringLiteral("rosco diagnostic connector"),QStringLiteral("wiring"),
                          QStringLiteral("wiring_assets"),
                          {QStringLiteral("ROSCO"),QStringLiteral("rover_rosco_3pin_black.svg")}) && ok;
    ok=requireSourceTable(QStringLiteral("obd j1962"),QStringLiteral("wiring"),
                          QStringLiteral("wiring_assets"),
                          {QStringLiteral("OBD"),QStringLiteral("mems_1_9_obd_16pin.svg")}) && ok;

    ok=requireFirstResult(QStringLiteral("IAT"),QStringLiteral("wiring"),
                          {QStringLiteral("IAT"),QStringLiteral("Vert / Rouge")}) && ok;

    if(!ok){
        printLine(QStringLiteral("FAIL MEMS search semantic self-test"));
        return 3;
    }
    printLine(QStringLiteral("PASS MEMS search semantic self-test"));
    return 0;
}
