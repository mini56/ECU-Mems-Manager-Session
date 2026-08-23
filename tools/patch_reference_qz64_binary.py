#!/usr/bin/env python3
from pathlib import Path

path = Path("database/MemsReferenceDatabase.cpp")
text = path.read_text(encoding="utf-8")

if "uncompressReferenceQz64Payload" in text:
    print("MemsReferenceDatabase binary qz64 compatibility already applied")
    raise SystemExit(0)

old = '''bool executeQz64Sql(QSqlDatabase &database,const QString &path)\n{\n    QFile file(path);\n    if(!file.open(QIODevice::ReadOnly|QIODevice::Text)) return false;\n    const QByteArray sqlBytes=qUncompress(QByteArray::fromBase64(file.readAll().trimmed()));\n    if(sqlBytes.isEmpty()) return false;\n\n    QSqlQuery query(database);\n    const QList<QByteArray> statements=sqlBytes.split('\\n');\n    for(const QByteArray &line:statements){\n        const QString statement=QString::fromUtf8(line).trimmed();\n        if(statement.isEmpty() || statement.startsWith(QStringLiteral("--"))) continue;\n        if(!query.exec(statement)) return false;\n    }\n    return true;\n}\n'''

new = '''bool looksLikeReferenceBase64Text(const QByteArray &data)\n{\n    const QByteArray trimmed=data.trimmed();\n    if(trimmed.isEmpty()) return false;\n    for(char value:trimmed){\n        const unsigned char c=static_cast<unsigned char>(value);\n        const bool base64=(c>='A'&&c<='Z') || (c>='a'&&c<='z')\n            || (c>='0'&&c<='9') || c=='+' || c=='/' || c=='='\n            || c=='\\r' || c=='\\n' || c=='\\t' || c==' ';\n        if(!base64) return false;\n    }\n    return true;\n}\n\nQByteArray uncompressReferenceQz64Payload(const QByteArray &fileBytes)\n{\n    if(looksLikeReferenceBase64Text(fileBytes))\n        return qUncompress(QByteArray::fromBase64(fileBytes.trimmed()));\n\n    // Historical Rover lot 1620 is stored directly as a binary qCompress\n    // stream. Keep its original bytes unchanged and decode that packaging.\n    return qUncompress(fileBytes);\n}\n\nbool executeQz64Sql(QSqlDatabase &database,const QString &path)\n{\n    QFile file(path);\n    if(!file.open(QIODevice::ReadOnly)) return false;\n    const QByteArray sqlBytes=uncompressReferenceQz64Payload(file.readAll());\n    if(sqlBytes.isEmpty()) return false;\n\n    QSqlQuery query(database);\n    const QList<QByteArray> statements=sqlBytes.split('\\n');\n    for(const QByteArray &line:statements){\n        const QString statement=QString::fromUtf8(line).trimmed();\n        if(statement.isEmpty() || statement.startsWith(QStringLiteral("--"))) continue;\n        if(!query.exec(statement)) return false;\n    }\n    return true;\n}\n'''

if old not in text:
    raise SystemExit("executeQz64Sql anchor not found")

path.write_text(text.replace(old, new, 1), encoding="utf-8", newline="\n")
print("MemsReferenceDatabase binary qz64 compatibility applied")
