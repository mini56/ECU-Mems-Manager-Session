#include "ExpertKnowledgeReader.h"

#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTextStream>
#include <QUuid>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir temporary;
    if (!temporary.isValid())
        return 1;

    const QString databasePath = temporary.filePath(QStringLiteral("fixture.sqlite"));
    const QString connection = QStringLiteral("VISUAL_REF_FIXTURE_%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
    database.setDatabaseName(databasePath);
    if (!database.open())
        return 2;
    {
        QSqlQuery query(database);
        if (!query.exec(QStringLiteral(
                "CREATE TABLE mems_knowledge_item (knowledge_key TEXT PRIMARY KEY, image_ref TEXT)")))
            return 3;
        query.prepare(QStringLiteral(
            "INSERT INTO mems_knowledge_item(knowledge_key,image_ref) VALUES(:key,:image)"));
        query.bindValue(QStringLiteral(":key"), QStringLiteral("TEST-PROCEDURE-FACT"));
        query.bindValue(QStringLiteral(":image"),
                        QStringLiteral("images/rave/AKM7169ENG_PDF_133.png"));
        if (!query.exec())
            return 4;
    }
    database.close();
    database = QSqlDatabase();
    QSqlDatabase::removeDatabase(connection);

    ExpertKnowledgeReader reader;
    if (!reader.openReadOnly(databasePath))
        return 5;
    if (reader.visualReferenceForFact(QStringLiteral("TEST-PROCEDURE-FACT"))
        != QStringLiteral("images/rave/AKM7169ENG_PDF_133.png"))
        return 6;
    if (reader.visualReferenceForFact(
            QStringLiteral("asset:images/rover_rosco_3pin_black.svg"))
        != QStringLiteral("images/rover_rosco_3pin_black.svg"))
        return 7;
    if (!reader.visualReferenceForFact(QStringLiteral("UNKNOWN-FACT")).isEmpty())
        return 8;

    QTextStream(stdout) << "EXPERT_VISUAL_REFERENCE_PASS" << Qt::endl;
    return 0;
}
