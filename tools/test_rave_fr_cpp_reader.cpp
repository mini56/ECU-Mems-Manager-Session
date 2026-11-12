#include "../database/MemsDocumentaryRepository.h"

#include <QCoreApplication>
#include <QString>
#include <QVariantMap>
#include <cstdio>

static bool check(bool condition, const char *message)
{
    if (!condition)
        std::fprintf(stderr,"RAVE_FR_CPP_READER_FAIL: %s\n",message);
    return condition;
}

int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv);
    if (argc!=2 || !check(qputenv("MEMS_DOCUMENTARY_DB",QByteArray(argv[1])),
                          "environment override"))
        return 1;
    MemsDocumentaryRepository reader;
    if (!check(reader.open(),"open French source SQLite")) {
        std::fprintf(stderr,"Error: %s\n",qPrintable(reader.lastError()));
        return 1;
    }
    const QVariantList french=reader.search(QStringLiteral("bobine d'allumage"),12,true,
                                             QStringLiteral("fr"));
    if (!check(!french.isEmpty(),"French technical lookup"))
        return 1;
    if (!check(french.first().toMap().value(QStringLiteral("source_key"))
                   .toString().startsWith(QStringLiteral("rave-fr:")),
               "French source identity"))
        return 1;
    const QVariantList page=reader.search(QStringLiteral("page 111"),12,true,
                                           QStringLiteral("fr"));
    if (!check(!page.isEmpty(),"French physical page lookup"))
        return 1;
    const QVariantList english=reader.search(QStringLiteral("bobine"),12,true,
                                              QStringLiteral("en"));
    if (!check(english.isEmpty(),"No silent French-English mixing"))
        return 1;
    const QString response=MemsDocumentaryRepository::answerFromHits(french);
    if (!check(response.contains(QStringLiteral("RCL0193FRE")) &&
               !response.contains(QStringLiteral("AKM7169ENG")),
               "French IA response cites French constructor document"))
        return 1;
    for (const QVariant &entry : french) {
        if (!check(!entry.toMap().value(QStringLiteral("approved_for_ai")).toBool(),
                    "Source-only entries never auto-approved"))
            return 1;
    }
    std::puts("RAVE_FR_CPP_READER_PASS");
    return 0;
}
