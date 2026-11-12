#include "MemsDocumentaryRepository.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QHash>
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
                         const QString &pages, const QString &status,
                         const QHash<QString, QString> &translations,
                         const QString &language)
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
        "SELECT phase,step_number,source_text,source_page,original_phase_label,rowid FROM procedure_steps "
        "WHERE procedure_id=? ORDER BY CASE phase WHEN 'remove' THEN 0 ELSE 1 END, step_number"));
    steps.addBindValue(key);
    if (steps.exec()) {
        while (steps.next()) {
            const QString phaseCode = steps.value(0).toString();
            const QString printedPhase = steps.value(4).toString().trimmed();
            QString phase;
            if (phaseCode == QStringLiteral("remove"))
                phase = QStringLiteral("Démontage");
            else if (phaseCode == QStringLiteral("refit"))
                phase = QStringLiteral("Remontage");
            else if (phaseCode == QStringLiteral("check"))
                phase = QStringLiteral("Contrôle");
            else if (phaseCode == QStringLiteral("adjust"))
                phase = QStringLiteral("Réglage");
            else if (phaseCode == QStringLiteral("inspection"))
                phase = QStringLiteral("Inspection");
            else if (phaseCode == QStringLiteral("other"))
                phase = QStringLiteral("Phase non précisée");
            else
                phase = printedPhase.isEmpty() ? phaseCode : printedPhase;
            const QString sourceStep = steps.value(2).toString().simplified();
            const QString translatedStep = translations.value(
                QStringLiteral("procedure.step:%1").arg(steps.value(5).toLongLong()));
            const QString content = language == QStringLiteral("en") ||
                language.isEmpty() ? sourceStep :
                (translatedStep.isEmpty()
                    ? QStringLiteral("[EN original — traduction %1 absente] %2")
                          .arg(language.toUpper(), sourceStep)
                    : translatedStep);
            fields << QStringLiteral("%1 %2 (page %3): %4")
                .arg(phase).arg(steps.value(1).toInt())
                .arg(steps.value(3).toInt()).arg(content);
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

QString unverifiedProcedurePreview(const QSqlDatabase &database,
                                   const QString &candidateId)
{
    // Present source OCR in the database browser as raw evidence only.
    // This is never approved as an instruction or passed to Qwen as facts.
    QStringList lines;
    QSqlQuery countQuery(database);
    countQuery.prepare(QStringLiteral(
        "SELECT COUNT(*) FROM extracted_procedure_candidate_steps "
        "WHERE candidate_id=?"));
    countQuery.addBindValue(candidateId);
    int total = 0;
    if (countQuery.exec() && countQuery.next())
        total = countQuery.value(0).toInt();

    QSqlQuery instructions(database);
    instructions.prepare(QStringLiteral(
        "SELECT phase_kind,manufacturer_step_number,"
        "ocr_instruction_candidate,source_status "
        "FROM extracted_procedure_candidate_steps "
        "WHERE candidate_id=? "
        "ORDER BY phase_position,step_position"));
    instructions.addBindValue(candidateId);
    if (!instructions.exec())
        return QStringLiteral(
            "source: extraction des étapes momentanément indisponible");
    int visible = 0;
    while (instructions.next()) {
        const QString kind = instructions.value(0).toString();
        const int number = instructions.value(1).toInt();
        const QString text = instructions.value(2).toString().simplified();
        const QString state = instructions.value(3).toString();
        if (text.isEmpty() || state != QStringLiteral("OCR_UNVERIFIED")) {
            lines << QStringLiteral(
                "OCR à examiner (étape %1 / %2): ligne incomplète ou incertaine")
                .arg(kind).arg(number);
        } else {
            lines << QStringLiteral(
                "OCR brut, NON VALIDÉ — %1 %2: %3")
                .arg(kind).arg(number).arg(text);
        }
        ++visible;
    }
    if (total > visible)
        lines << QStringLiteral(
            "Attention: %1 étapes OCR supplémentaires ne sont pas affichées. "
            "Lire la page originale du PDF avant toute utilisation.")
            .arg(total - visible);
    if (lines.isEmpty())
        return QStringLiteral(
            "Aucune étape OCR rattachée localement — rechercher les fragments "
            "sur les pages voisines et examiner le PDF.");
    return lines.join(QLatin1Char('\n'));
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
        QStringLiteral("documentary_language_meta"),
        QStringLiteral("documentary_text_units"),
        QStringLiteral("documentary_translations"),
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
    // The multilingual layer is only accepted when bound to this exact
    // immutable original corpus.  Its SHA is recorded during verified build.
    QSqlQuery localeMeta(m_database);
    if (!localeMeta.exec(QStringLiteral(
            "SELECT key,value FROM documentary_language_meta "
            "WHERE key IN ('schema_version','original_sqlite_sha256')")) ||
        !localeMeta.isActive()) {
        m_error = QStringLiteral("Métadonnées multilingues Rover absentes.");
        m_database.close();
        return false;
    }
    QHash<QString,QString> localeIdentity;
    while (localeMeta.next())
        localeIdentity.insert(localeMeta.value(0).toString(),
                              localeMeta.value(1).toString());
    if (localeIdentity.value(QStringLiteral("schema_version")) != QStringLiteral("1") ||
        !QRegularExpression(QStringLiteral("^[a-f0-9]{64}$"))
            .match(localeIdentity.value(QStringLiteral("original_sqlite_sha256"))).hasMatch()) {
        m_error = QStringLiteral("Version ou provenance multilingue incorrecte.");
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
                                                int maximum, bool allowOcrDiscovery,
                                                const QString &language) const
{
    QVariantList result;
    if (!isOpen())
        return result;
    const bool browse = question.trimmed().isEmpty();
    const QString locale = language.left(2).toLower();
    // A shared, read-only lookup of translations imported into the
    // SAME SQLite as the immutable Rover manufacturer data.
    QHash<QString, QString> translatedText;
    if (!locale.isEmpty() && locale != QStringLiteral("en")) {
        QSqlQuery translated(m_database);
        translated.prepare(QStringLiteral(
            "SELECT t.unit_id,t.translated_text FROM documentary_translations t "
            "JOIN documentary_text_units u ON u.unit_id=t.unit_id "
            "WHERE t.language=? AND t.source_sha256=u.source_sha256 "
            "AND t.review_status IN ('machine_draft','reviewed')"));
        translated.addBindValue(locale);
        if (translated.exec()) {
            while (translated.next())
                translatedText.insert(translated.value(0).toString(),
                                      translated.value(1).toString());
        }
    }
    const QString input = normalized(question);
    QStringList terms = meaningfulTerms(question);
    maximum = qBound(1, maximum, 300);
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
    QSet<QString> curatedRepairNumbers;

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
                               "extrait OCR: %2").arg(requestedPage).arg(original),
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
            curatedRepairNumbers.insert(reference);
            const QString pages = procedures.value(3).toString();
            const QString status = procedures.value(4).toString();
            int score = browse ? 10 :
                (repairNo == reference && !repairNo.isEmpty() ? 1000 : 0);
            const QString translatedTitle = translatedText.value(
                QStringLiteral("procedure.title:%1").arg(key));
            const QString titleN = normalized(title + QLatin1Char(' ') + translatedTitle);
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
                    "SELECT rowid,source_text FROM procedure_steps WHERE procedure_id=?"));
                stepsSearch.addBindValue(key);
                if (stepsSearch.exec()) {
                    while (stepsSearch.next()) {
                        const QStringList words =
                            normalized(stepsSearch.value(1).toString() + QLatin1Char(' ') +
                                translatedText.value(QStringLiteral("procedure.step:%1")
                                    .arg(stepsSearch.value(0).toLongLong())))
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
                m_database, key, translatedTitle.isEmpty()? title : translatedTitle,
                reference, pages, status, translatedText, locale);
            const QString uiTitle = locale == QStringLiteral("en") || locale.isEmpty()
                ? title : (translatedTitle.isEmpty()
                    ? QStringLiteral("[EN / %1 indisponible] %2")
                          .arg(locale.toUpper(), title)
                    : translatedTitle);
            QVariantMap row = makeRow(
                key, QStringLiteral("%1 — %2").arg(reference, uiTitle),
                content, QStringLiteral("rover_documentary"),
                QStringLiteral("TECHNICAL_REVIEW_PENDING"));
            row.insert(QStringLiteral("original_title"),
                       QStringLiteral("%1 — %2").arg(reference, title));
            row.insert(QStringLiteral("translation_language"), locale);
            row.insert(QStringLiteral("translation_available"), !translatedTitle.isEmpty());
            candidates.append({score, row});
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
    // The 482-page source audit contains automatically detected repairs.
    // They are discoverable by number/title ONLY, not executable technical
    // instructions. A candidate must never replace a separately reviewed
    // procedure sharing the same manufacturer number.
    QSqlQuery tableCheck(m_database);
    if (tableCheck.exec(QStringLiteral(
            "SELECT 1 FROM sqlite_master WHERE type='table' "
            "AND name='extracted_procedure_candidates'")) && tableCheck.next()) {
        QSqlQuery detected(m_database);
        if (detected.exec(QStringLiteral(
                "SELECT candidate_id,original_page,source_region,"
                "candidate_manufacturer_repair_number,candidate_title,"
                "repair_number_format_status,source_status "
                "FROM extracted_procedure_candidates "
                "WHERE source_status='REVIEW_REQUIRED' AND approved_for_ai=0"))) {
            QVector<Candidate> found;
            while (detected.next()) {
                const QString key = detected.value(0).toString();
                const int physicalPage = detected.value(1).toInt();
                const QString region = detected.value(2).toString();
                const QString number = detected.value(3).toString();
                const QString title = detected.value(4).toString();
                const QString numberState = detected.value(5).toString();
                if (key.isEmpty() || title.isEmpty() ||
                    curatedRepairNumbers.contains(number))
                    continue;
                const QString translatedTitle = translatedText.value(
                    QStringLiteral("candidate.title:%1").arg(key));
                const QString titleN = normalized(title + QLatin1Char(' ') + translatedTitle);
                int score = browse ? 9 : 0;
                if (!repairNo.isEmpty() && repairNo == number)
                    score = 850;
                for (const QString &term : terms) {
                    if (titleN.split(QLatin1Char(' ')).contains(term))
                        score += 55;
                    else if (term.size() >= 5 && titleN.contains(term))
                        score += 15;
                }
                // Search OCR step vocabulary as source discovery only. A
                // single generic term must never create a spurious operation.
                if (!browse && !terms.isEmpty()) {
                    QSqlQuery sourceSteps(m_database);
                    sourceSteps.prepare(QStringLiteral(
                        "SELECT rowid,ocr_instruction_candidate "
                        "FROM extracted_procedure_candidate_steps "
                        "WHERE candidate_id=? AND source_status='OCR_UNVERIFIED'"));
                    sourceSteps.addBindValue(key);
                    if (sourceSteps.exec()) {
                        while (sourceSteps.next()) {
                            const QStringList words =
                                normalized(sourceSteps.value(1).toString() +
                                    QLatin1Char(' ') + translatedText.value(
                                        QStringLiteral("candidate.step:%1")
                                        .arg(sourceSteps.value(0).toLongLong())))
                                    .split(QLatin1Char(' '),Qt::SkipEmptyParts);
                            int matched=0;
                            for (const QString &term:terms)
                                if (words.contains(term))
                                    ++matched;
                            if (matched>=2 || (matched==1 && terms.size()==1
                                    && terms.first().size()>=8))
                                score=qMax(score,matched*48);
                        }
                    }
                }
                if (score <= 0)
                    continue;
                const QString validity = numberState == QStringLiteral("NUMBER_PATTERN_MATCH")
                    ? QStringLiteral("format du numéro repéré par OCR")
                    : QStringLiteral("numéro constructeur illisible ou incomplet — vérification obligatoire");
                const QString content = QStringLiteral(
                    "document: Rover AKM7169ENG\n"
                    "référence candidate: %1\n"
                    "titre OCR candidat: %2\n"
                    "page physique: %3\n"
                    "colonne source: %4\n"
                    "numéro: %5\n"
                    "validation: REVIEW_REQUIRED — extraction OCR non vérifiée\n"
                    "attention: Les étapes suivantes sont des extraits OCR à "
                    "contrôler sur le PDF, PAS des instructions techniquement approuvées.\n"
                    "%6")
                    .arg(number,title).arg(physicalPage)
                    .arg(region,validity,unverifiedProcedurePreview(m_database,key));
                found.append({score,makeRow(
                    key, QStringLiteral("%1 — %2 (p. %3, à vérifier)")
                        .arg(number,title).arg(physicalPage),
                    content,QStringLiteral("rover_procedure_candidate"),
                    QStringLiteral("REVIEW_REQUIRED"))});
            }
            std::sort(found.begin(),found.end(),
                [](const Candidate &left,const Candidate &right){
                    if (left.score!=right.score) return left.score>right.score;
                    return left.row.value(QStringLiteral("title")).toString() <
                           right.row.value(QStringLiteral("title")).toString();
                });
            for (const Candidate &item : found) {
                if (result.size()>=maximum) break;
                result.append(item.row);
            }
        }
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
        "SELECT f.physical_page,p.ocr_text,p.knowledge_state "
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

QString MemsDocumentaryRepository::numberedMenu(
    const QString &question, const QVariantList &choices, bool french)
{
    QStringList lines;
    lines << QStringLiteral("Rubriques retrouvées pour « %1 » :").arg(question);
    for (int index = 0; index < choices.size(); ++index) {
        const QString title =
            choices.at(index).toMap().value(QStringLiteral("title")).toString();
        lines << QStringLiteral("%1. %2").arg(index + 1)
            .arg(title);
    }
    lines << QStringLiteral(
        "Réponds par le numéro de la rubrique à consulter, "
        "ou écris une autre recherche.");
    return lines.join(QLatin1Char('\n'));
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
        if (item.value(QStringLiteral("source_table")).toString() ==
            QStringLiteral("rover_procedure_candidate")) {
            // Review-only OCR is visible in the database screen, but the IA
            // must not output its unverified steps as repair instructions.
            const int firstStep = content.indexOf(
                QStringLiteral("OCR brut, NON VALIDÉ"));
            if (firstStep >= 0)
                content = content.left(firstStep);
            content += QStringLiteral(
                "\nIA: procédure candidate retrouvée, mais ses instructions "
                "et schémas ne sont pas encore vérifiés sur les pages "
                "constructeur. Aucun conseil de réparation validé disponible.");
        }
        if (content.size() > 3500)
            content = content.left(3500) + QStringLiteral("\n[Extrait abrégé dans l'IA]");
        paragraphs << QStringLiteral("%1\n%2")
            .arg(item.value(QStringLiteral("title")).toString(), content);
    }
    return paragraphs.join(QStringLiteral("\n\n"));
}
