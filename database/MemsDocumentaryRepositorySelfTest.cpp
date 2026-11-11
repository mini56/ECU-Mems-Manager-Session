#include "MemsDocumentaryRepository.h"

#include <QCoreApplication>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QVariantMap>

#include <cstdio>

static bool check(bool truth, const char *message)
{
    if (!truth)
        std::fprintf(stderr, "MEMS_DOCUMENTARY_SELFTEST_FAIL: %s\n", message);
    return truth;
}

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    QTemporaryDir temp;
    if (!check(temp.isValid(), "temporary SQLite location"))
        return 1;
    const QString name = QStringLiteral("documentary_test_seed");
    const QString file = QDir(temp.path()).filePath(QStringLiteral("rover_482.sqlite"));
    {
        QSqlDatabase seed = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
        seed.setDatabaseName(file);
        if (!check(seed.open(), "open fixture"))
            return 1;
        const QStringList schema = {
            QStringLiteral("CREATE TABLE source_documents("
                           "document_id TEXT PRIMARY KEY,total_pages INTEGER,"
                           "original_pdf_sha256 TEXT)"),
            QStringLiteral("CREATE TABLE source_page_discovery("
                           "document_id TEXT,physical_page INTEGER,ocr_text TEXT,"
                           "knowledge_state TEXT,original_pdf_sha256 TEXT)"),
            QStringLiteral("CREATE TABLE procedures("
                           "procedure_id TEXT PRIMARY KEY,title TEXT,"
                           "manufacturer_repair_no TEXT,original_pages TEXT,"
                           "transcription_status TEXT,ai_publication_allowed INTEGER,"
                           "technical_approval INTEGER)"),
            QStringLiteral("CREATE TABLE procedure_steps("
                           "procedure_id TEXT,phase TEXT,step_number INTEGER,"
                           "source_text TEXT,source_page INTEGER)"),
            QStringLiteral("CREATE TABLE technical_values("
                           "procedure_id TEXT,literal_number TEXT,literal_unit TEXT)"),
            QStringLiteral("CREATE TABLE illustrations("
                           "procedure_id TEXT,reference_code TEXT,physical_page INTEGER)"),
            QStringLiteral("CREATE TABLE search_aliases("
                           "procedure_id TEXT,alias TEXT)"),
            QStringLiteral("CREATE TABLE extracted_procedure_candidates("
                           "candidate_id TEXT PRIMARY KEY,document_id TEXT,"
                           "original_page INTEGER,source_region TEXT,"
                           "candidate_manufacturer_repair_number TEXT,"
                           "candidate_title TEXT,repair_number_format_status TEXT,"
                           "source_status TEXT,approved_for_ai INTEGER)"),
            QStringLiteral("CREATE VIRTUAL TABLE source_page_fts USING fts5("
                           "physical_page UNINDEXED,ocr_text)")
        };
        for (const QString &statement : schema) {
            QSqlQuery sql(seed);
            if (!check(sql.exec(statement), "create fixture schema"))
                return 1;
        }
        const QString sha(64, QLatin1Char('a'));
        QSqlQuery insertDocument(seed);
        insertDocument.prepare(QStringLiteral(
            "INSERT INTO source_documents VALUES(?,?,?)"));
        insertDocument.addBindValue(QStringLiteral("rover-akm7169eng"));
        insertDocument.addBindValue(482);
        insertDocument.addBindValue(sha);
        if (!check(insertDocument.exec(), "original 482-page provenance"))
            return 1;
        if (!check(seed.transaction(), "start page import"))
            return 1;
        QSqlQuery page(seed), fts(seed);
        page.prepare(QStringLiteral(
            "INSERT INTO source_page_discovery VALUES(?,?,?,?,?)"));
        fts.prepare(QStringLiteral(
            "INSERT INTO source_page_fts VALUES(?,?)"));
        for (int n = 1; n <= 482; ++n) {
            const QString content = n == 92
                ? QStringLiteral("service timing chain gears removal")
                : QStringLiteral("original manual physical page");
            page.bindValue(0, QStringLiteral("rover-akm7169eng"));
            page.bindValue(1, n);
            page.bindValue(2, content);
            page.bindValue(3, QStringLiteral("OCR_DISCOVERY_ONLY"));
            page.bindValue(4, sha);
            fts.bindValue(0, n);
            fts.bindValue(1, content);
            if (!check(page.exec() && fts.exec(), "source page row"))
                return 1;
        }
        if (!check(seed.commit(), "commit 482-page source mapping"))
            return 1;
        const QStringList records = {
            QStringLiteral("INSERT INTO procedures VALUES("
                           "'rover-akm7169eng-12.65.12','Timing Chain and Gears',"
                           "'12.65.12','92-93','TECHNICAL_REVIEW_PENDING',0,0)"),
            QStringLiteral("INSERT INTO procedure_steps VALUES("
                           "'rover-akm7169eng-12.65.12','remove',1,"
                           "'Remove the cover.',92)"),
            QStringLiteral("INSERT INTO procedure_steps VALUES("
                           "'rover-akm7169eng-12.65.12','refit',8,"
                           "'tighten camshaft nut to 88 Nm.',93)"),
            QStringLiteral("INSERT INTO procedure_steps VALUES("
                           "'rover-akm7169eng-12.65.12','refit',3,"
                           "'Shim size = 0.152 mm.',93)"),
            QStringLiteral("INSERT INTO technical_values VALUES("
                           "'rover-akm7169eng-12.65.12','88','Nm')"),
            QStringLiteral("INSERT INTO technical_values VALUES("
                           "'rover-akm7169eng-12.65.12','0.152','mm')"),
            QStringLiteral("INSERT INTO illustrations VALUES("
                           "'rover-akm7169eng-12.65.12','12M0986',93)"),
            QStringLiteral("INSERT INTO search_aliases VALUES("
                           "'rover-akm7169eng-12.65.12','chaîne de distribution')"),
            QStringLiteral("INSERT INTO extracted_procedure_candidates VALUES("
                           "'rover:ocr:93:right:12.65.05','rover-akm7169eng',93,'right',"
                           "'12.65.05','Timing Cover and Oil Seal',"
                           "'NUMBER_PATTERN_MATCH','REVIEW_REQUIRED',0)"),
            QStringLiteral("INSERT INTO extracted_procedure_candidates VALUES("
                           "'rover:ocr:92:right:12.65.12','rover-akm7169eng',92,'right',"
                           "'12.65.12','Timing Chain and Gears',"
                           "'NUMBER_PATTERN_MATCH','REVIEW_REQUIRED',0)")
        };
        for (const QString &statement : records) {
            QSqlQuery sql(seed);
            if (!check(sql.exec(statement), "record fixture"))
                return 1;
        }
        QSqlQuery extra(seed);
        extra.prepare(QStringLiteral(
            "INSERT INTO extracted_procedure_candidates VALUES(?,?,?,?,?,?,?,?,?)"));
        for (int i=1; i<=236; ++i) {
            const QString id=QStringLiteral("rover:ocr:candidate:%1").arg(i);
            const QString number=QStringLiteral("31.%1.%2")
                .arg((i/100)%100,2,10,QLatin1Char('0'))
                .arg(i%100,2,10,QLatin1Char('0'));
            extra.bindValue(0,id);
            extra.bindValue(1,QStringLiteral("rover-akm7169eng"));
            extra.bindValue(2,i);
            extra.bindValue(3,QStringLiteral("left"));
            extra.bindValue(4,number);
            extra.bindValue(5,QStringLiteral("Unreviewed source procedure %1").arg(i));
            extra.bindValue(6,QStringLiteral("NUMBER_PATTERN_MATCH"));
            extra.bindValue(7,QStringLiteral("REVIEW_REQUIRED"));
            extra.bindValue(8,0);
            if (!check(extra.exec(), "populate 238 source candidates for realistic browsing"))
                return 1;
        }
        seed.close();
        seed = QSqlDatabase();
    }
    QSqlDatabase::removeDatabase(name);
    qputenv("MEMS_DOCUMENTARY_DB", file.toUtf8());
    {
        MemsDocumentaryRepository repository;
        if (!check(repository.open(), "strict 482-page reader opens"))
            return 1;
        const QStringList questions = {
            QStringLiteral("12.65.12"),
            QStringLiteral("chaîne de distribution"),
            QStringLiteral("88 Nm"),
            QStringLiteral("0,152 mm"),
            QStringLiteral("service timing"),
            QStringLiteral("chaine distribution")
        };
        for (const QString &question : questions) {
            const QVariantList hits = repository.search(question);
            if (!check(!hits.isEmpty(), "search yields original source"))
                return 1;
            if (!check(!hits.first().toMap().value(
                           QStringLiteral("approved_for_ai")).toBool(),
                       "never approve uncertified text"))
                return 1;
        }
        const QVariantList candidate=repository.search(QStringLiteral("12.65.05"));
        if (!check(!candidate.isEmpty() &&
                candidate.first().toMap().value(QStringLiteral("source_table")).toString()
                    == QStringLiteral("rover_procedure_candidate"),
                "unverified original manufacturer operation can be discovered"))
            return 1;
        const QString candidateContent = candidate.first().toMap().value(
            QStringLiteral("content")).toString();
        if (!check(candidateContent.contains(QStringLiteral("REVIEW_REQUIRED")) &&
                !candidateContent.contains(QStringLiteral("1. Remove")),
                "unverified procedure instructions never promoted to IA facts"))
            return 1;
        const QVariantList curated=repository.search(QStringLiteral("12.65.12"));
        for(const QVariant &item : curated)
            if(!check(item.toMap().value(QStringLiteral("source_table")).toString() !=
                      QStringLiteral("rover_procedure_candidate"),
                      "reviewed procedure replaces duplicate OCR candidate"))
                return 1;
        const QVariantList page = repository.search(QStringLiteral("page 93"));
        if (!check(!page.isEmpty() &&
                page.first().toMap().value(QStringLiteral("source_key")).toString() ==
                    QStringLiteral("rover-akm7169eng:page:93"),
                "direct physical page 93 is returned unapproved"))
            return 1;
        const QVariantList overview=repository.search(QString(),300);
        if (!check(overview.size()==238,
                   "browse all 238 candidates/curated entries without truncation"))
            return 1;
        if (!check(!repository.search(QString()).isEmpty(),
                   "empty query lists available structured procedures"))
            return 1;
        const QVariantList exact = repository.search(QStringLiteral("12.65.12"));
        if (!check(exact.first().toMap().value(
                       QStringLiteral("source_key")).toString() ==
                   QStringLiteral("rover-akm7169eng-12.65.12"),
                   "exact repair reference survives both readers"))
            return 1;
        const QString evidence = MemsDocumentaryRepository::answerFromHits(exact);
        if (!check(evidence.contains(QStringLiteral("page 92")) &&
                   evidence.contains(QStringLiteral("88 Nm")) &&
                   evidence.contains(QStringLiteral("NE SONT PAS")),
                   "answer preserves sources, literal units and review status"))
            return 1;
        if (!check(repository.search(QStringLiteral("unknown-gizmo")).isEmpty(),
                   "unknown query cannot fabricate an answer"))
            return 1;
    }
    // An incomplete schema must fail closed, not fall back to another pack.
    const QString bad = QDir(temp.path()).filePath(QStringLiteral("incomplete.sqlite"));
    QSqlDatabase broken = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
    broken.setDatabaseName(bad);
    if (!check(broken.open(), "corrupt fixture opens"))
        return 1;
    broken.close();
    broken = QSqlDatabase();
    QSqlDatabase::removeDatabase(name);
    qputenv("MEMS_DOCUMENTARY_DB", bad.toUtf8());
    {
        MemsDocumentaryRepository repository;
        if (!check(!repository.open(), "partial schema is rejected"))
            return 1;
    }
    std::puts("MEMS_DOCUMENTARY_SELFTEST_OK");
    return 0;
}
