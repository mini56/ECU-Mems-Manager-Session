#include "MemsDocumentaryRepository.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QVariantMap>
#include <QStringList>
#include <cstdio>

static bool verify(bool value, const char *name)
{
    if (!value)
        std::fprintf(stderr,"ROVER_482_REAL_CORPUS_FAIL %s\n",name);
    return value;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (!verify(argc == 3,"required_args_database_pdf"))
        return 1;
    const QString database = QString::fromLocal8Bit(argv[1]);
    const QString pdf = QString::fromLocal8Bit(argv[2]);
    if (!verify(QFileInfo(database).isFile() && QFileInfo(pdf).isFile(),"installed_files_missing"))
        return 1;
    qputenv("MEMS_DOCUMENTARY_DB",QFile::encodeName(database));
    MemsDocumentaryRepository corpus;
    if (!verify(corpus.open(),"real_sqlite_reader_open")) {
        std::fprintf(stderr,"%s\n",qPrintable(corpus.lastError()));
        return 1;
    }
    const QStringList queries = {
        QStringLiteral("12.65.12"), QStringLiteral("Timing Chain and Gears"),
        QStringLiteral("88 Nm"), QStringLiteral("page 93")
    };
    for (const QString &question : queries) {
        const QVariantList rows = corpus.search(question, 8, true);
        if (!verify(!rows.isEmpty(),"shared_documentary_search"))
            return 1;
        if (!verify(!rows.first().toMap().value(QStringLiteral("approved_for_ai")).toBool(),
                    "unapproved_manufacturer_ocr_must_stay_unapproved"))
            return 1;
        const QString answer = MemsDocumentaryRepository::answerFromHits(rows);
        if (!verify(!answer.trimmed().isEmpty() && answer.contains(QStringLiteral("AKM7169ENG")),
                    "documentary_ai_must_use_shared_sqlite_and_cite_original"))
            return 1;
    }
    // The English source is always searchable. Before importing the French
    // pack, the reader must not claim that a translated procedure exists.
    const QVariantList originalHead = corpus.search(
        QStringLiteral("Cylinder Head"),12,true,QStringLiteral("en"));
    if (!verify(!originalHead.isEmpty(),"english_constructor_lookup"))
        return 1;
    const QVariantList frenchHead = corpus.search(
        QStringLiteral("Cylinder Head"),12,true,QStringLiteral("fr"));
    if (!verify(!frenchHead.isEmpty(),"french_reader_returns_publication_status_or_reviewed_source"))
        return 1;
    const QVariantMap french = frenchHead.first().toMap();
    const QString sourceTable = french.value(QStringLiteral("source_table")).toString();
    if (sourceTable == QStringLiteral("rover_translation_status")) {
        // The real English corpus has no reviewed French translation pack:
        // the user sees ONE French-only status, not hybrid instructions.
        if (!verify(french.value(QStringLiteral("title")).toString().contains(
                        QStringLiteral("traduction française")) &&
                    !french.value(QStringLiteral("content")).toString().contains(
                        QStringLiteral("Cylinder Head")) &&
                    french.value(QStringLiteral("source_status")).toString()
                        == QStringLiteral("FR_NOT_PUBLISHED"),
                    "incomplete_french_must_be_hidden_not_mixed"))
            return 1;
    } else {
        // Once the French pack is complete, all displayed Rover entries must
        // originate from reviewed translations, never partial English.
        if (!verify(french.value(QStringLiteral("translation_available")).toBool(),
                    "published_french_requires_real_translated_source"))
            return 1;
    }
    const QVariantList exact = corpus.search(QStringLiteral("12.65.12"));
    if (!verify(exact.first().toMap().value(QStringLiteral("source_key")).toString()
                == QStringLiteral("rover-akm7169eng-12.65.12"),
                "source_id_shared_ui_and_ai"))
        return 1;
    std::puts("ROVER_482_REAL_CORPUS_READER_AND_AI_CONTRACT_OK");
    return 0;
}
