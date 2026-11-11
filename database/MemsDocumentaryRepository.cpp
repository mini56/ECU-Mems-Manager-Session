#include "MemsDocumentaryRepository.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVariantMap>

#include <algorithm>

namespace {

QString normalized(QString value)
{
    value = value.normalized(QString::NormalizationForm_D).toCaseFolded();
    QString cleaned;
    for (const QChar ch : value) {
        if (ch.category() == QChar::Mark_NonSpacing ||
            ch.category() == QChar::Mark_SpacingCombining ||
            ch.category() == QChar::Mark_Enclosing)
            continue;
        cleaned += ch.isLetterOrNumber() ? ch : QLatin1Char(' ');
    }
    return cleaned.simplified();
}

QStringList meaningfulTerms(const QString &value)
{
    static const QSet<QString> stop = {
        QStringLiteral("comment"), QStringLiteral("quelle"), QStringLiteral("quelles"),
        QStringLiteral("quels"), QStringLiteral("quel"), QStringLiteral("pourquoi"),
        QStringLiteral("donne"), QStringLiteral("donner"), QStringLiteral("trouve"),
        QStringLiteral("trouver"), QStringLiteral("cherche"), QStringLiteral("dans"),
        QStringLiteral("avec"), QStringLiteral("sans"), QStringLiteral("cette"),
        QStringLiteral("sur"), QStringLiteral("les"), QStringLiteral("des"),
        QStringLiteral("une"), QStringLiteral("est"), QStringLiteral("pour"),
        QStringLiteral("the"), QStringLiteral("and"), QStringLiteral("what"),
        QStringLiteral("how"), QStringLiteral("rover"), QStringLiteral("manuel")
    };
    QStringList output;
    for (const QString &token : normalized(value).split(QLatin1Char(' '), Qt::SkipEmptyParts)) {
        if (token.size() < 3 || stop.contains(token))
            continue;
        if (!output.contains(token))
            output.append(token);
    }
    return output;
}

QString procedureContent(const QSqlDatabase &database, const QString &key,
                         const QString &title, const QString &ref,
                         const QString &pages, const QString &status)
{
    QStringList fields = {
        QStringLiteral("title: %1").arg(title),
        QStringLiteral("référence constructeur: %1").arg(ref),
        QStringLiteral("document: Rover AKM7169ENG"),
        QStringLiteral("pages physiques: %1").arg(pages),
        QStringLiteral("validation: %1").arg(status),
        QStringLiteral("attention: Transcription documentaire. Applicabilité ECU et exactitude technique non validées.")
    };
    QSqlQuery steps(database);
    steps.prepare(QStringLiteral(
        "SELECT phase,step_number,source_text,source_page FROM procedure_steps "
        "WHERE procedure_id=? ORDER BY CASE phase WHEN 'remove' THEN 0 ELSE 1 END, step_number"));
    steps.addBindValue(key);
    if (steps.exec()) {
        while (steps.next()) {
            const QString phase = steps.value(0).toString() == QStringLiteral("remove")
                ? QStringLiteral("Démontage") : QStringLiteral("Remontage");
            fields << QStringLiteral("%1 %2 (page %3): %4")
                .arg(phase).arg(steps.value(1).toInt())
                .arg(steps.value(3).toInt()).arg(steps.value(2).toString());
        }
    }
    QSqlQuery figures(database);
    figures.prepare(QStringLiteral(
        "SELECT reference_code,physical_page FROM illustrations "
        "WHERE procedure_id=? ORDER BY physical_page,reference_code"));
    figures.addBindValue(key);
    if (figures.exec()) {
        while (figures.next())
            fields << QStringLiteral("Figure citée (page %1): %2 — relation graphique non validée")
                .arg(figures.value(1).toInt()).arg(figures.value(0).toString());
    }
    return fields.join(QLatin1Char('\n'));
}

QVariantMap makeRow(const QString &key, const QString &title,
                    const QString &content, const QString &table,
                    const QString &status)
{
    QVariantMap result;
    result.insert(QStringLiteral("category"), QStringLiteral("documentation"));
    result.insert(QStringLiteral("source_table"), table);
    result.insert(QStringLiteral("source_key"), key);
    result.insert(QStringLiteral("generation"), QString()); // Never invent an ECU generation.
    result.insert(QStringLiteral("title"), title);
    result.insert(QStringLiteral("content"), content);
    result.insert(QStringLiteral("source_status"), status);
    result.insert(QStringLiteral("approved_for_ai"), false);
    return result;
}

} // namespace

MemsDocumentaryRepository::MemsDocumentaryRepository()
    : m_connectionName(QStringLiteral("MEMS_DOCUMENTARY_%1")
          .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
{
}

MemsDocumentaryRepository::~MemsDocumentaryRepository()
{
    if (!m_database.isValid())
        return;
    if (m_database.isOpen())
        m_database.close();
    m_database = QSqlDatabase();
    QSqlDatabase::removeDatabase(m_connectionName);
}

QString MemsDocumentaryRepository::databasePath()
{
    // A single packaged SQLite, never the runtime ECU calibration database.
    // Test-only opt-in permits isolated fixtures; normal installation uses
    // the file beside the program, entirely offline.
#ifdef MEMS_DOCUMENTARY_SELFTEST
    const QString overridePath = qEnvironmentVariable("MEMS_DOCUMENTARY_DB").trimmed();
    if (!overridePath.isEmpty())
        return QDir::cleanPath(overridePath);
#endif
    return QDir(QCoreApplication::applicationDirPath()).filePath(
        QStringLiteral("database/documentary/rover_482.sqlite"));
}

QString MemsDocumentaryRepository::originalPdfPath()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(
        QStringLiteral("database/documentary/AKM7169ENG.pdf"));
}

bool MemsDocumentaryRepository::isPackInstalled()
{
    return QFileInfo::exists(databasePath());
}

bool MemsDocumentaryRepository::open()
{
    if (m_database.isOpen())
        return true;
    m_error.clear();
    const QFileInfo file(databasePath());
    if (!file.isFile()) {
        m_error = QStringLiteral("Base documentaire Rover absente.");
        return false;
    }
    m_database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
    m_database.setDatabaseName(file.absoluteFilePath());
    if (!m_database.open()) {
        m_error = QStringLiteral("Ouverture SQLite impossible : %1")
                      .arg(m_database.lastError().text());
        return false;
    }
    QSqlQuery pragma(m_database);
    if (!pragma.exec(QStringLiteral("PRAGMA query_only=ON"))) {
        m_error = QStringLiteral("Lecture seule documentaire non garantie.");
        m_database.close();
        return false;
    }

    // Fail closed on partial/foreign packages. Merely being valid SQLite
    // is not evidence that the document chain is complete.
    const QStringList required = {
        QStringLiteral("source_documents"), QStringLiteral("source_page_discovery"),
        QStringLiteral("procedures"), QStringLiteral("procedure_steps"),
        QStringLiteral("technical_values"), QStringLiteral("illustrations"),
        QStringLiteral("search_aliases"), QStringLiteral("source_page_fts")
    };
    QSqlQuery tables(m_database);
    tables.prepare(QStringLiteral(
        "SELECT name FROM sqlite_master WHERE type IN ('table','view') AND name=?"));
    for (const QString &name : required) {
        tables.bindValue(0, name);
        if (!tables.exec() || !tables.next()) {
            m_error = QStringLiteral("Schéma documentaire incomplet : %1").arg(name);
            m_database.close();
            return false;
        }
    }

    QSqlQuery validation(m_database);
    if (!validation.exec(QStringLiteral(
            "SELECT d.total_pages,COUNT(p.physical_page),MIN(p.physical_page),"
            "MAX(p.physical_page),d.original_pdf_sha256,"
            "COUNT(DISTINCT p.physical_page),"
            "SUM(CASE WHEN p.original_pdf_sha256=d.original_pdf_sha256 "
            "THEN 0 ELSE 1 END) "
            "FROM source_documents d JOIN source_page_discovery p "
            "ON p.document_id=d.document_id "
            "GROUP BY d.document_id")) ||
        !validation.next() ||
        validation.value(0).toInt() != 482 ||
        validation.value(1).toInt() != 482 ||
        validation.value(2).toInt() != 1 ||
        validation.value(3).toInt() != 482 ||
        validation.value(5).toInt() != 482 ||
        validation.value(6).toInt() != 0 ||
        !QRegularExpression(QStringLiteral("^[a-f0-9]{64}$"))
            .match(validation.value(4).toString()).hasMatch() ||
        validation.next()) {
        m_error = QStringLiteral("Corpus Rover incomplet ou provenance invalide (482 pages requises).");
        m_database.close();
        return false;
    }
    QSqlQuery integrity(m_database);
    if (!integrity.exec(QStringLiteral("PRAGMA quick_check")) ||
        !integrity.next() ||
        integrity.value(0).toString() != QStringLiteral("ok")) {
        m_error = QStringLiteral("Intégrité SQLite documentaire non conforme.");
        m_database.close();
        return false;
    }
    QSqlQuery foreignKeys(m_database);
    if (!foreignKeys.exec(QStringLiteral("PRAGMA foreign_key_check")) ||
        foreignKeys.next()) {
        m_error = QStringLiteral("Références de la base documentaire incohérentes.");
        m_database.close();
        return false;
    }
    QSqlQuery approval(m_database);
    if (!approval.exec(QStringLiteral(
            "SELECT COUNT(*) FROM procedures WHERE ai_publication_allowed<>0 "
            "OR technical_approval<>0")) ||
        !approval.next() || approval.value(0).toInt() != 0) {
        // The current pilot schema is review-only. Introducing an approved
        // schema needs its own independently tested publication contract.
        m_error = QStringLiteral("Statut de publication documentaire non conforme.");
        m_database.close();
        return false;
    }
    return true;
}

bool MemsDocumentaryRepository::isOpen() const
{
    return m_database.isValid() && m_database.isOpen();
}

QString MemsDocumentaryRepository::lastError() const
{
    return m_error;
}

QVariantList MemsDocumentaryRepository::search(const QString &question,
                                                int maximum, bool allowOcrDiscovery) const
{
    QVariantList result;
    if (!isOpen())
        return result;
    const bool browse = question.trimmed().isEmpty();
    const QString input = normalized(question);
    const QStringList terms = meaningfulTerms(question);
    maximum = qBound(1, maximum, 30);
    const QRegularExpression refRx(QStringLiteral("\\b\\d{2}\\.\\d{2}\\.\\d{2}\\b"));
    const QRegularExpressionMatch repair = refRx.match(question);
    const QString repairNo = repair.hasMatch() ? repair.captured(0) : QString();
    const QRegularExpression valueRx(
        QStringLiteral("\\b(\\d+(?:[.,]\\d+)?)\\s*(Nm|mm|kPa|bar|mV|V|A)\\b"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch measured = valueRx.match(question);
    QString numeric = measured.hasMatch() ? measured.captured(1) : QString();
    numeric.replace(QLatin1Char(','), QLatin1Char('.'));
    const QString unit = measured.hasMatch() ? measured.captured(2) : QString();

    struct Candidate {
        int score;
        QVariantMap row;
    };
    QVector<Candidate> candidates;
    QSet<QString> seen;

    // An explicit physical-page question must locate that precise PDF page,
    // including pages that have no usable OCR. It must not turn the page
    // itself into an approved repair instruction.
    const QRegularExpression pageRx(
        QStringLiteral("\\bpages?\\s+(\\d{1,3})\\b"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch pageMatch = pageRx.match(question);
    if (pageMatch.hasMatch()) {
        const int requestedPage = pageMatch.captured(1).toInt();
        QSqlQuery sourcePage(m_database);
        sourcePage.prepare(QStringLiteral(
            "SELECT physical_page,ocr_text,knowledge_state "
            "FROM source_page_discovery WHERE physical_page=?"));
        sourcePage.addBindValue(requestedPage);
        if (sourcePage.exec() && sourcePage.next()) {
            const QString key = QStringLiteral("rover-akm7169eng:page:%1")
                                    .arg(requestedPage);
            QString original = sourcePage.value(1).toString().simplified();
            if (original.isEmpty())
                original = QStringLiteral("[Aucun texte OCR exploitable sur cette page]");
            result.append(makeRow(
                key, QStringLiteral("Rover AKM7169ENG — page %1").arg(requestedPage),
                QStringLiteral("document: Rover AKM7169ENG\npage physique: %1\n"
                               "validation: OCR_DISCOVERY_ONLY — texte non vérifié\n"
                               "extrait OCR: %2").arg(requestedPage).arg(original.left(1300)),
                QStringLiteral("rover_ocr_discovery"),
                sourcePage.value(2).toString()));
            seen.insert(key);
        }
    }
    QSqlQuery procedures(m_database);
    if (procedures.exec(QStringLiteral(
            "SELECT procedure_id,title,manufacturer_repair_no,original_pages,"
            "transcription_status FROM procedures ORDER BY manufacturer_repair_no"))) {
        while (procedures.next()) {
            const QString key = procedures.value(0).toString();
            const QString title = procedures.value(1).toString();
            const QString reference = procedures.value(2).toString();
            const QString pages = procedures.value(3).toString();
            const QString status = procedures.value(4).toString();
            int score = browse ? 10 :
                (repairNo == reference && !repairNo.isEmpty() ? 1000 : 0);
            const QString titleN = normalized(title);
            QStringList aliases;
            QSqlQuery alias(m_database);
            alias.prepare(QStringLiteral("SELECT alias FROM search_aliases WHERE procedure_id=?"));
            alias.addBindValue(key);
            if (alias.exec())
                while (alias.next()) aliases << normalized(alias.value(0).toString());
            for (const QString &name : aliases) {
                if (name.size() >= 5 && input.contains(name))
                    score = qMax(score, 400);
                if (terms.isEmpty())
                    continue;
                const QStringList aliasWords =
                    name.split(QLatin1Char(' '), Qt::SkipEmptyParts);
                int matched = 0;
                for (const QString &term : terms)
                    if (aliasWords.contains(term))
                        ++matched;
                if (matched >= 2 || (matched == 1 && terms.size() == 1 &&
                                     terms.first().size() >= 6))
                    score = qMax(score, matched * 110);
            }
            if (score == 0) {
                for (const QString &term : terms) {
                    if (titleN.split(QLatin1Char(' ')).contains(term))
                        score += 60;
                    else if (term.size() > 4 && titleN.contains(term))
                        score += 20;
                }
            }
            // Real questions often refer to a step rather than the
            // manufacturer title; search the recorded step text but require
            // more than a generic single keyword before ranking it highly.
            if (!browse && !terms.isEmpty()) {
                QSqlQuery stepsSearch(m_database);
                stepsSearch.prepare(QStringLiteral(
                    "SELECT source_text FROM procedure_steps WHERE procedure_id=?"));
                stepsSearch.addBindValue(key);
                if (stepsSearch.exec()) {
                    while (stepsSearch.next()) {
                        const QStringList words =
                            normalized(stepsSearch.value(0).toString())
                                .split(QLatin1Char(' '), Qt::SkipEmptyParts);
                        int matched = 0;
                        for (const QString &term : terms)
                            if (words.contains(term))
                                ++matched;
                        if (matched >= 2 || (matched == 1 && terms.size() == 1 &&
                                             terms.first().size() >= 7))
                            score = qMax(score, matched * 75);
                    }
                }
            }
            if (measured.hasMatch()) {
                QSqlQuery values(m_database);
                values.prepare(QStringLiteral(
                    "SELECT COUNT(*) FROM technical_values WHERE procedure_id=? "
                    "AND literal_number=? AND lower(literal_unit)=lower(?)"));
                values.addBindValue(key);
                values.addBindValue(numeric);
                values.addBindValue(unit);
                if (values.exec() && values.next() && values.value(0).toInt() > 0)
                    score += 650;
            }
            if (score <= 0)
                continue;
            const QString content = procedureContent(
                m_database, key, title, reference, pages, status);
            candidates.append({score, makeRow(
                key, QStringLiteral("%1 — %2").arg(reference, title),
                content, QStringLiteral("rover_documentary"),
                QStringLiteral("TECHNICAL_REVIEW_PENDING"))});
            seen.insert(key);
        }
    }
    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate &a, const Candidate &b) {
                  if (a.score != b.score) return a.score > b.score;
                  return a.row.value(QStringLiteral("source_key")).toString() <
                         b.row.value(QStringLiteral("source_key")).toString();
              });
    for (const Candidate &entry : candidates) {
        if (result.size() >= maximum)
            break;
        result.append(entry.row);
    }
    if (!allowOcrDiscovery || browse || result.size() >= maximum || terms.isEmpty())
        return result;

    // OCR discovery never becomes a technical fact. FTS terms are escaped,
    // and no raw user input is interpolated into the SQL statement.
    QStringList matchTokens;
    for (const QString &token : terms) {
        if (token.size() >= 3 && token.size() <= 40)
            matchTokens << QStringLiteral("\"%1\"").arg(token);
        if (matchTokens.size() >= 4)
            break;
    }
    if (matchTokens.isEmpty())
        return result;
    QSqlQuery pages(m_database);
    pages.prepare(QStringLiteral(
        "SELECT f.physical_page,substr(p.ocr_text,1,1300),p.knowledge_state "
        "FROM source_page_fts f JOIN source_page_discovery p "
        "ON p.physical_page=CAST(f.physical_page AS INTEGER) "
        "WHERE source_page_fts MATCH ? ORDER BY bm25(source_page_fts) LIMIT ?"));
    pages.addBindValue(matchTokens.join(QStringLiteral(" AND ")));
    pages.addBindValue(maximum - result.size());
    if (pages.exec()) {
        while (pages.next()) {
            const int pageNumber = pages.value(0).toInt();
            const QString body = pages.value(1).toString().simplified();
            const QString status = pages.value(2).toString();
            const QString title = QStringLiteral("Rover AKM7169ENG — page %1")
                                      .arg(pageNumber);
            const QString key = QStringLiteral("rover-akm7169eng:page:%1")
                                    .arg(pageNumber);
            if (seen.contains(key))
                continue;
            seen.insert(key);
            result.append(makeRow(
                key, title,
                QStringLiteral("document: Rover AKM7169ENG\npage physique: %1\n"
                               "validation: OCR_DISCOVERY_ONLY — texte non vérifié\n"
                               "extrait OCR: %2").arg(pageNumber).arg(body),
                QStringLiteral("rover_ocr_discovery"), status));
        }
    }
    return result;
}

QString MemsDocumentaryRepository::answerFromHits(const QVariantList &hits)
{
    if (hits.isEmpty())
        return QString();
    QStringList paragraphs;
    paragraphs << QStringLiteral(
        "Documentation Rover AKM7169ENG — recherche dans la SQLite commune "
        "au lecteur et à l'IA. Les transcriptions ci-dessous NE SONT PAS "
        "des procédures techniquement approuvées. Vérifier le PDF constructeur "
        "et l'applicabilité au véhicule avant une intervention.");
    const int count = qMin(3, hits.size());
    for (int i = 0; i < count; ++i) {
        const QVariantMap item = hits.at(i).toMap();
        QString content = item.value(QStringLiteral("content")).toString();
        if (content.size() > 3500)
            content = content.left(3500) + QStringLiteral("\n[Extrait abrégé dans l'IA]");
        paragraphs << QStringLiteral("%1\n%2")
            .arg(item.value(QStringLiteral("title")).toString(), content);
    }
    return paragraphs.join(QStringLiteral("\n\n"));
}
