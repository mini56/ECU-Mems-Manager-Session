#include "../database/MemsDocumentaryRepository.h"

#include <QCoreApplication>
#include <QString>
#include <QVariantMap>
#include <cstdio>

static bool check(bool ok,const char* msg)
{
    if (!ok) std::fprintf(stderr,"RAVE_EN_RUNTIME_FAIL: %s\n",msg);
    return ok;
}

int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv);
    if (!check(argc==2,"one source database argument"))
        return 1;
    qputenv("MEMS_DOCUMENTARY_DB",QByteArray(argv[1]));
    MemsDocumentaryRepository repository;
    if (!check(repository.open(),"English original SQLite reader can open")) {
        std::fprintf(stderr,"%s\n",qPrintable(repository.lastError()));
        return 1;
    }
    const auto exact=repository.search(QStringLiteral("12.65.12"),12,true,
                                        QStringLiteral("en"));
    if (!check(!exact.isEmpty(),"Original English constructor repair operation"))
        return 1;
    if (!check(exact.first().toMap().value(QStringLiteral("source_key"))
                   .toString()==QStringLiteral("rover-akm7169eng-12.65.12"),
               "correct manufacturer procedure identity"))
        return 1;
    const auto evidence=MemsDocumentaryRepository::answerFromHits(exact);
    if (!check(evidence.contains(QStringLiteral("AKM7169ENG")),
               "AI evidence source citation"))
        return 1;
    for (const QVariant &item: exact)
        if (!check(!item.toMap().value(QStringLiteral("approved_for_ai")).toBool(),
                   "Technical OCR cannot be approved automatically"))
            return 1;
    const auto french=repository.search(QStringLiteral("Cylinder Head"),12,true,
                                        QStringLiteral("fr"));
    if (!check(!french.isEmpty() &&
               french.first().toMap().value(QStringLiteral("source_table"))
                   .toString()==QStringLiteral("rover_translation_status"),
               "No silent English substitution for missing French manuscript"))
        return 1;
    std::puts("RAVE_EN_RUNTIME_READER_PASS");
    return 0;
}
