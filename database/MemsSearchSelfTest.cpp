#include "MemsGlobalSearchIndex.h"

#include <QCoreApplication>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

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
    ok=requireSearch(QStringLiteral("vert rouge"),QStringLiteral("wiring"),
                     {QStringLiteral("IAT"),QStringLiteral("Vert / Rouge")}) && ok;
    ok=requireSearch(QStringLiteral("IAT"),QStringLiteral("wiring"),
                     {QStringLiteral("IAT")}) && ok;
    ok=requireSearch(QStringLiteral("temperature air"),QStringLiteral("wiring"),
                     {QStringLiteral("température air"),QStringLiteral("Vert / Rouge")}) && ok;
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
    ok=requireFirstResult(QStringLiteral("IAT"),QStringLiteral("wiring"),
                          {QStringLiteral("IAT"),QStringLiteral("Vert / Rouge")}) && ok;

    if(!ok){
        printLine(QStringLiteral("FAIL MEMS search semantic self-test"));
        return 3;
    }
    printLine(QStringLiteral("PASS MEMS search semantic self-test"));
    return 0;
}
