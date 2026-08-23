#!/usr/bin/env python3
from pathlib import Path

path = Path("database/MemsReferenceDatabase.cpp")
text = path.read_text(encoding="utf-8")

anchor = '''QString cacheReferenceRoot()\n{\n    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)\n        + QStringLiteral("/reference");\n}\n'''
addition = anchor + '''\nint manifestDatabaseRevision(const QString &root)\n{\n    QFile file(QDir(root).filePath(QStringLiteral("manifest.json")));\n    if(!file.open(QIODevice::ReadOnly|QIODevice::Text)) return 0;\n    const QJsonDocument document=QJsonDocument::fromJson(file.readAll());\n    if(!document.isObject()) return 0;\n    return document.object().value(QStringLiteral("database_revision")).toInt(0);\n}\n\nint cachedDatabaseRevision(const QString &databasePath)\n{\n    if(!QFileInfo::exists(databasePath)) return 0;\n    const QString connection=QStringLiteral("MEMS_REFERENCE_REV_%1").arg(QUuid::createUuid().toString());\n    QSqlDatabase database=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),connection);\n    database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));\n    database.setDatabaseName(databasePath);\n    int revision=0;\n    if(database.open()){\n        QSqlQuery query(database);\n        if(query.exec(QStringLiteral("PRAGMA user_version")) && query.next())\n            revision=query.value(0).toInt();\n        database.close();\n    }\n    database=QSqlDatabase();\n    QSqlDatabase::removeDatabase(connection);\n    return revision;\n}\n'''
if "int manifestDatabaseRevision" not in text:
    if anchor not in text:
        raise SystemExit("cacheReferenceRoot anchor not found")
    text = text.replace(anchor, addition, 1)

old = '''    const QString cacheRoot=cacheReferenceRoot();\n    QDir().mkpath(cacheRoot);\n    m_databasePath=cacheRoot+QStringLiteral("/ecu_mems_reference_r5.sqlite");\n\n    if(!QFileInfo::exists(m_databasePath)){\n'''
new = '''    const QString cacheRoot=cacheReferenceRoot();\n    QDir().mkpath(cacheRoot);\n    m_databasePath=cacheRoot+QStringLiteral("/ecu_mems_reference_r5.sqlite");\n\n    const int expectedRevision=manifestDatabaseRevision(referenceRoot());\n    if(QFileInfo::exists(m_databasePath) && expectedRevision>0\n       && cachedDatabaseRevision(m_databasePath)!=expectedRevision){\n        if(!QFile::remove(m_databasePath))\n            return false;\n    }\n\n    if(!QFileInfo::exists(m_databasePath)){\n'''
if "const int expectedRevision=manifestDatabaseRevision" not in text:
    if old not in text:
        raise SystemExit("open() cache anchor not found")
    text = text.replace(old, new, 1)

old2 = '''        if(ok) ok=registerReferenceAssets(buildDb,referenceRoot());\n\n        buildDb.close();\n'''
new2 = '''        if(ok) ok=registerReferenceAssets(buildDb,referenceRoot());\n        if(ok && expectedRevision>0)\n            ok=query.exec(QStringLiteral("PRAGMA user_version=%1").arg(expectedRevision));\n\n        buildDb.close();\n'''
if "PRAGMA user_version=%1" not in text:
    if old2 not in text:
        raise SystemExit("build revision anchor not found")
    text = text.replace(old2, new2, 1)

path.write_text(text, encoding="utf-8")
print("MemsReferenceDatabase cache revision patch applied")
