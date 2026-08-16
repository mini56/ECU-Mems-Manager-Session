#include "MemsGlobalSearchIndex.h"

#include <QCoreApplication>
#include <QDebug>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

namespace {

QString rowText(const QVariantMap &row)
{
    return row.value(QStringLiteral("title")).toString()
        + QLatin1Char('\n')
        + row.value(QStringLiteral("content")).toString();
}

bool hasResult(const QVariantList &rows,const QStringList &needles)
{
    for(const QVariant &value:rows){
        const QString text=rowText(value.toMap());
        bool all=true;
        for(const QString &needle:needles){
            if(!text.contains(needle,Qt::CaseInsensitive)){
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
        qCritical().noquote()<<QStringLiteral("FAIL search '%1' category '%2': no result").arg(query,category);
        return false;
    }
    if(!hasResult(rows,needles)){
        qCritical().noquote()<<QStringLiteral("FAIL search '%1' category '%2': expected relationship not found").arg(query,category);
        for(int i=0;i<qMin(rows.size(),5);++i)
            qCritical().noquote()<<QStringLiteral("  -> %1").arg(rowText(rows.at(i).toMap()).replace(QLatin1Char('\n'),QStringLiteral(" | ")));
        return false;
    }
    qInfo().noquote()<<QStringLiteral("PASS search '%1' category '%2' (%3 result(s))").arg(query,category).arg(rows.size());
    return true;
}

}

int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv);
    QCoreApplication::setOrganizationName(QStringLiteral("ECU-Mems-Manager"));
    QCoreApplication::setApplicationName(QStringLiteral("ECU-Mems-Manager-Search-SelfTest"));

    QString error;
    if(!MemsGlobalSearchIndex::ensureBuilt(&error)){
        qCritical().noquote()<<QStringLiteral("FAIL index build: %1").arg(error);
        return 2;
    }

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

    if(!ok) return 3;
    qInfo()<<"PASS MEMS search semantic self-test";
    return 0;
}
