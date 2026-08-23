#!/usr/bin/env python3
from pathlib import Path

path = Path("database/MemsReferenceDatabase.cpp")
text = path.read_text(encoding="utf-8")

old = r'''bool executeQz64Sql(QSqlDatabase &database,const QString &path)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) return false;
    const QByteArray sqlBytes=uncompressReferenceQz64Payload(file.readAll());
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
'''

new = r'''QString cleanedReferenceSqlText(const QByteArray &sqlBytes)
{
    QString cleaned;
    const QList<QByteArray> lines=sqlBytes.split('\n');
    for(const QByteArray &rawLine:lines){
        const QString line=QString::fromUtf8(rawLine);
        if(line.trimmed().startsWith(QStringLiteral("--"))) continue;
        cleaned+=line;
        cleaned+=QLatin1Char('\n');
    }
    return cleaned;
}

bool referenceSqlHasTerminator(const QString &sql)
{
    bool inSingleQuote=false;
    bool inDoubleQuote=false;
    for(int i=0;i<sql.size();++i){
        const QChar ch=sql.at(i);
        if(ch==QLatin1Char('\'') && !inDoubleQuote){
            if(inSingleQuote && i+1<sql.size() && sql.at(i+1)==QLatin1Char('\'')){
                ++i;
                continue;
            }
            inSingleQuote=!inSingleQuote;
            continue;
        }
        if(ch==QLatin1Char('"') && !inSingleQuote){
            if(inDoubleQuote && i+1<sql.size() && sql.at(i+1)==QLatin1Char('"')){
                ++i;
                continue;
            }
            inDoubleQuote=!inDoubleQuote;
            continue;
        }
        if(ch==QLatin1Char(';') && !inSingleQuote && !inDoubleQuote) return true;
    }
    return false;
}

bool executeLegacyReferenceSql(QSqlDatabase &database,const QString &sql)
{
    QSqlQuery query(database);
    const QStringList lines=sql.split(QLatin1Char('\n'));
    for(const QString &rawLine:lines){
        const QString statement=rawLine.trimmed();
        if(statement.isEmpty()) continue;
        if(!query.exec(statement)) return false;
    }
    return true;
}

bool executeTerminatedReferenceSql(QSqlDatabase &database,const QString &sql)
{
    QSqlQuery query(database);
    QString statement;
    statement.reserve(sql.size());
    bool inSingleQuote=false;
    bool inDoubleQuote=false;

    auto executeStatement=[&](const QString &value)->bool{
        const QString trimmed=value.trimmed();
        if(trimmed.isEmpty()) return true;
        return query.exec(trimmed);
    };

    for(int i=0;i<sql.size();++i){
        const QChar ch=sql.at(i);
        if(ch==QLatin1Char('\'') && !inDoubleQuote){
            if(inSingleQuote && i+1<sql.size() && sql.at(i+1)==QLatin1Char('\'')){
                statement+=ch;
                statement+=sql.at(++i);
                continue;
            }
            inSingleQuote=!inSingleQuote;
            statement+=ch;
            continue;
        }
        if(ch==QLatin1Char('"') && !inSingleQuote){
            if(inDoubleQuote && i+1<sql.size() && sql.at(i+1)==QLatin1Char('"')){
                statement+=ch;
                statement+=sql.at(++i);
                continue;
            }
            inDoubleQuote=!inDoubleQuote;
            statement+=ch;
            continue;
        }
        if(ch==QLatin1Char(';') && !inSingleQuote && !inDoubleQuote){
            if(!executeStatement(statement)) return false;
            statement.clear();
            continue;
        }
        statement+=ch;
    }

    if(inSingleQuote || inDoubleQuote) return false;
    return executeStatement(statement);
}

bool executeQz64Sql(QSqlDatabase &database,const QString &path)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) return false;
    const QByteArray sqlBytes=uncompressReferenceQz64Payload(file.readAll());
    if(sqlBytes.isEmpty()) return false;

    const QString sql=cleanedReferenceSqlText(sqlBytes);
    if(!referenceSqlHasTerminator(sql))
        return executeLegacyReferenceSql(database,sql);
    return executeTerminatedReferenceSql(database,sql);
}
'''

if new in text:
    print("Main reference SQL parser already supports both formats")
    raise SystemExit(0)
if old not in text:
    raise SystemExit("executeQz64Sql anchor not found")

path.write_text(text.replace(old,new,1), encoding="utf-8")
print("Main reference SQL parser now supports legacy line SQL and terminated multiline SQL")
