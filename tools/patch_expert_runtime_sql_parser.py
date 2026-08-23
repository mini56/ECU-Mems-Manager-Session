#!/usr/bin/env python3
from pathlib import Path

path = Path('expert/ExpertRuntimeDatabase.cpp')
text = path.read_text(encoding='utf-8')
old = '''bool executeSqlLines(QSqlDatabase &database, const QByteArray &sqlBytes, QString *error)
{
    QSqlQuery query(database);
    const QList<QByteArray> statements = sqlBytes.split('\\n');
    for (const QByteArray &line : statements) {
        const QString statement = QString::fromUtf8(line).trimmed();
        if (statement.isEmpty() || statement.startsWith(QStringLiteral("--")))
            continue;
        if (!query.exec(statement)) {
            if (error)
                *error = query.lastError().text() + QStringLiteral(" | ") + statement.left(180);
            return false;
        }
    }
    return true;
}
'''
new = '''bool executeSqlLines(QSqlDatabase &database, const QByteArray &sqlBytes, QString *error)
{
    // Enrichment lots are SQL scripts, not necessarily one statement per line.
    // Split on semicolons only when they are outside quoted strings/comments.
    QSqlQuery query(database);
    QByteArray statement;
    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    bool inLineComment = false;
    bool inBlockComment = false;

    auto executeStatement = [&]() -> bool {
        const QString sql = QString::fromUtf8(statement).trimmed();
        statement.clear();
        if (sql.isEmpty())
            return true;
        if (!query.exec(sql)) {
            if (error)
                *error = query.lastError().text() + QStringLiteral(" | ") + sql.left(240);
            return false;
        }
        return true;
    };

    for (int i = 0; i < sqlBytes.size(); ++i) {
        const char c = sqlBytes.at(i);
        const char next = (i + 1 < sqlBytes.size()) ? sqlBytes.at(i + 1) : '\\0';

        if (inLineComment) {
            if (c == '\\n' || c == '\\r') {
                inLineComment = false;
                statement.append(' ');
            }
            continue;
        }
        if (inBlockComment) {
            if (c == '*' && next == '/') {
                inBlockComment = false;
                ++i;
                statement.append(' ');
            }
            continue;
        }

        if (!inSingleQuote && !inDoubleQuote && c == '-' && next == '-') {
            inLineComment = true;
            ++i;
            continue;
        }
        if (!inSingleQuote && !inDoubleQuote && c == '/' && next == '*') {
            inBlockComment = true;
            ++i;
            continue;
        }

        if (!inDoubleQuote && c == '\\'') {
            statement.append(c);
            if (inSingleQuote && next == '\\'') {
                statement.append(next);
                ++i;
                continue;
            }
            inSingleQuote = !inSingleQuote;
            continue;
        }
        if (!inSingleQuote && c == '"') {
            statement.append(c);
            if (inDoubleQuote && next == '"') {
                statement.append(next);
                ++i;
                continue;
            }
            inDoubleQuote = !inDoubleQuote;
            continue;
        }

        if (!inSingleQuote && !inDoubleQuote && c == ';') {
            if (!executeStatement())
                return false;
            continue;
        }

        if ((c == '\\n' || c == '\\r' || c == '\\t') && !inSingleQuote && !inDoubleQuote)
            statement.append(' ');
        else
            statement.append(c);
    }

    if (inSingleQuote || inDoubleQuote || inBlockComment) {
        if (error)
            *error = QStringLiteral("Script SQL incomplet: guillemet ou commentaire non fermé");
        return false;
    }
    return executeStatement();
}
'''
if old not in text:
    if 'Split on semicolons only when they are outside quoted strings/comments.' in text:
        print('Expert runtime SQL parser already patched')
        raise SystemExit(0)
    raise SystemExit('executeSqlLines anchor not found')
path.write_text(text.replace(old, new, 1), encoding='utf-8', newline='\n')
print('Expert runtime SQL parser patched')
