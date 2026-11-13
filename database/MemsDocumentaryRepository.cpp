#include "MemsDocumentaryRepository.h"
#include "MemsDocumentaryPackCatalog.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVariantMap>

#include <algorithm>
#include <cstdio>

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

// Question technique libre -> sujet documentaire et opération demandée.
// Ce n'est pas un catalogue de mots-clés : "donne-moi la procédure de dépose
// du joint de culasse" doit retrouver "JOINT DE CULASSE — dépose", même si
// le titre ne contient ni "donne-moi" ni "procédure".
QStringList nativeSubjectTerms(const QString &question)
{
    static const QSet<QString> conversational = {
        QStringLiteral("moi"),QStringLiteral("mon"),QStringLiteral("ma"),
        QStringLiteral("sont"),QStringLiteral("sera"),QStringLiteral("serait"),
        QStringLiteral("souhaite"),QStringLiteral("aimerais"),QStringLiteral("connaitre"),
        QStringLiteral("connait"),QStringLiteral("voudrait"),QStringLiteral("pourrais"),
        QStringLiteral("possible"),QStringLiteral("obtenir"),QStringLiteral("besoin"),
        QStringLiteral("trouver"),QStringLiteral("recherche"),QStringLiteral("demande"),
        QStringLiteral("mes"),QStringLiteral("tu"),QStringLiteral("vous"),
        QStringLiteral("peux"),QStringLiteral("peut"),QStringLiteral("faire"),
        QStringLiteral("fais"),QStringLiteral("faut"),QStringLiteral("veux"),
        QStringLiteral("voudrais"),QStringLiteral("savoir"),
        QStringLiteral("expliquer"),QStringLiteral("explique"),
        QStringLiteral("indiquer"),QStringLiteral("indique"),
        QStringLiteral("montre"),QStringLiteral("montrez"),
        QStringLiteral("etape"),QStringLiteral("etapes"),
        QStringLiteral("operation"),QStringLiteral("operations"),
        QStringLiteral("procedure"),QStringLiteral("procedures"),
        QStringLiteral("procede"),QStringLiteral("technique"),
        QStringLiteral("commentaire"),QStringLiteral("detail"),
        QStringLiteral("details"),QStringLiteral("complete"),
        QStringLiteral("precise"),QStringLiteral("precisement"),
        QStringLiteral("depose"),QStringLiteral("deposer"),
        QStringLiteral("demontage"),QStringLiteral("demonter"),
        QStringLiteral("demonte"),QStringLiteral("repose"),
        QStringLiteral("reposer"),QStringLiteral("remontage"),
        QStringLiteral("remonter"),QStringLiteral("remonte"),
        QStringLiteral("retirer"),QStringLiteral("retire"),
        QStringLiteral("installer"),QStringLiteral("installation")
    };
    QStringList terms;
    for(QString token:meaningfulTerms(question)){
        if(conversational.contains(token) || token.size()<3)
            continue;
        if(token==QStringLiteral("joints"))token=QStringLiteral("joint");
        if(token==QStringLiteral("culasses"))token=QStringLiteral("culasse");
        if(token==QStringLiteral("freins"))token=QStringLiteral("frein");
        if(!terms.contains(token))
            terms<<token;
    }
    return terms.mid(0,6);
}

QString nativeOperation(const QString &question)
{
    const QString plain=normalized(question);
    const auto hasTerm=[&plain](const QStringList &words) {
        for(const QString &word:words)
            if(plain.split(QLatin1Char(' ')).contains(word))
                return true;
        return false;
    };
    const bool remove=hasTerm({QStringLiteral("depose"),QStringLiteral("deposer"),
        QStringLiteral("demontage"),QStringLiteral("demonter"),
        QStringLiteral("retirer"),QStringLiteral("retire")});
    const bool refit=hasTerm({QStringLiteral("repose"),QStringLiteral("reposer"),
        QStringLiteral("remontage"),QStringLiteral("remonter"),
        QStringLiteral("remonte"),QStringLiteral("remettre")});
    return remove==refit?QString():remove?
        QStringLiteral("depose"):QStringLiteral("repose");
}

int nativeRelevance(const QVariantMap &hit,const QStringList &subject,
                    const QString &operation)
{
    const QString title=normalized(hit.value(QStringLiteral("title")).toString());
    const QString type=hit.value(QStringLiteral("entry_type")).toString();
    int score=0;
    for(const QString &term:subject){
        if(title.split(QLatin1Char(' ')).contains(term))
            score+=25;
    }
    if(!operation.isEmpty()){
        if(title.contains(operation))
            score+=160;
        const QString opposite=operation==QStringLiteral("depose")?
            QStringLiteral("repose"):QStringLiteral("depose");
        if(title.contains(opposite))
            score-=200;
    }
    if(type==QStringLiteral("procedure"))score+=20;
    if(type==QStringLiteral("step"))score+=10;
    if(hit.value(QStringLiteral("source_table")).toString()
          .contains(QStringLiteral("page_ocr")))score-=50;
    return score;
}

// The one documentary text accessor: no per-word substitutions, no live
// model inference, and never silently display English inside a French page.
QString documentText(const QString &unitId, const QString &original,
                     const QHash<QString, QString> &translations,
                     const QString &language)
{
    if (language != QStringLiteral("fr"))
        return original;
    const QString published = translations.value(unitId);
    return published.trimmed().isEmpty()
        ? QStringLiteral("Texte français indisponible : ce document ne peut pas être présenté.")
        : published;
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
            const QString content = documentText(
                QStringLiteral("procedure.step:%1").arg(steps.value(5).toLongLong()),
                sourceStep, translations, language);
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
                                   const QString &candidateId,
                                   const QHash<QString, QString> &translations,
                                   const QString &language)
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
        "ocr_instruction_candidate,source_status,rowid "
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
        const QString text = documentText(
            QStringLiteral("candidate.step:%1").arg(
                instructions.value(4).toLongLong()),
            instructions.value(2).toString().simplified(),
            translations, language);
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
    // Only the selected documentary language is optional/downloadable.
    // ECU user data, MEMS reference and expert runtime paths never change.
#ifdef MEMS_DOCUMENTARY_SELFTEST
    const QString overridePath = qEnvironmentVariable("MEMS_DOCUMENTARY_DB").trimmed();
    if (!overridePath.isEmpty())
        return QDir::cleanPath(overridePath);
#endif
    return MemsDocumentaryPackCatalog::installedDatabasePath(
        MemsDocumentaryPackCatalog::selectedLanguage());
}

QString MemsDocumentaryRepository::originalPdfPath()
{
    // A source PDF is only exposed when explicitly included in an installed
    // and licensed language pack. No English PDF is assumed in French mode.
    const QString location = databasePath();
    if (location.isEmpty())
        return QString();
    return QDir(QFileInfo(location).absolutePath()).filePath(
        QStringLiteral("original-source.pdf"));
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
    m_frenchComplete = false;
    m_nativeDocumentary = false;
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

    // Schéma documentaire multimanuel par langue (sans dépendance aux bases ECU).
    QSqlQuery nativeSchema(m_database);
    if (nativeSchema.exec(QStringLiteral(
            "SELECT name FROM sqlite_master WHERE type='table' "
            "AND name='documentary_database'")) && nativeSchema.next()) {
        auto rejectNative = [this](const QString &message) {
            m_error = message;
            m_database.close();
            return false;
        };
        const QStringList requiredNative = {
            QStringLiteral("documentary_database"),
            QStringLiteral("documentary_documents"),
            QStringLiteral("documentary_pages"),
            QStringLiteral("documentary_entries"),
            QStringLiteral("documentary_images"),
            QStringLiteral("documentary_fts")
        };
        QSqlQuery check(m_database);
        check.prepare(QStringLiteral(
            "SELECT COUNT(*) FROM sqlite_master WHERE type IN ('table','view') "
            "AND name=?"));
        for (const QString &name : requiredNative) {
            check.bindValue(0,name);
            if (!check.exec() || !check.next() || check.value(0).toInt()!=1)
                return rejectNative(QStringLiteral(
                    "Schéma documentaire natif incomplet : %1").arg(name));
        }
        QSqlQuery meta(m_database);
        if (!meta.exec(QStringLiteral(
              "SELECT schema_version,language,publication_status "
              "FROM documentary_database WHERE singleton=1")) ||
            !meta.next() || meta.value(0).toInt()!=1 ||
            meta.value(1).toString()!=MemsDocumentaryPackCatalog::selectedLanguage() ||
            !QStringList{QStringLiteral("source_inventory"),
                         QStringLiteral("validated")}
                .contains(meta.value(2).toString()) || meta.next())
            return rejectNative(QStringLiteral(
                "Métadonnées documentaires invalides."));
        QSqlQuery document(m_database);
        if (!document.exec(QStringLiteral(
              "SELECT COUNT(*),MIN(language),MAX(language) "
              "FROM documentary_documents")) ||
            !document.next() || document.value(0).toInt()<1 ||
            document.value(1).toString()!=MemsDocumentaryPackCatalog::selectedLanguage() ||
            document.value(2).toString()!=MemsDocumentaryPackCatalog::selectedLanguage())
            return rejectNative(QStringLiteral("Langues des sources incohérentes."));
        QSqlQuery counts(m_database);
        if (!counts.exec(QStringLiteral(
              "SELECT (SELECT COUNT(*) FROM documentary_entries), "
              "(SELECT COUNT(*) FROM documentary_fts), "
              "(SELECT COUNT(*) FROM documentary_pages)")) ||
            !counts.next() || counts.value(0).toInt()<=0 ||
            counts.value(0).toInt()!=counts.value(1).toInt() ||
            counts.value(2).toInt()<=0)
            return rejectNative(QStringLiteral(
                "Index documentaire natif incomplet."));
        QSqlQuery integrity(m_database);
        if (!integrity.exec(QStringLiteral("PRAGMA quick_check")) ||
            !integrity.next() || integrity.value(0).toString()!=QStringLiteral("ok"))
            return rejectNative(QStringLiteral(
                "Intégrité SQLite documentaire non conforme."));
        QSqlQuery foreignKeys(m_database);
        if (!foreignKeys.exec(QStringLiteral("PRAGMA foreign_key_check")) ||
            foreignKeys.next())
            return rejectNative(QStringLiteral(
                "Références documentaires incohérentes."));
        m_nativeDocumentary = true;
        m_frenchComplete = MemsDocumentaryPackCatalog::selectedLanguage()==QStringLiteral("fr");
        return true;
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
    // Publish French atomically. Even one absent, unreviewed, or
    // source-mismatched unit makes the complete French corpus unavailable.
    QSqlQuery frenchPublication(m_database);
    if (!frenchPublication.exec(QStringLiteral(
            "SELECT COUNT(*),"
            "COALESCE(SUM(CASE WHEN t.review_status='reviewed' "
            "AND LENGTH(TRIM(t.reviewer))>0 "
            "AND t.source_sha256=u.source_sha256 "
            "THEN 1 ELSE 0 END),0) "
            "FROM documentary_text_units u "
            "LEFT JOIN documentary_translations t "
            "ON t.unit_id=u.unit_id AND t.language='fr'")) ||
        !frenchPublication.next()) {
        m_error = QStringLiteral("Vérification de la couverture française impossible.");
        m_database.close();
        return false;
    }
    const qlonglong expected = frenchPublication.value(0).toLongLong();
    const qlonglong approved = frenchPublication.value(1).toLongLong();
    m_frenchComplete = expected > 0 && expected == approved;
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
    if (m_nativeDocumentary) {
        const QString requested = language.left(2).toLower();
        if (!requested.isEmpty() && requested != MemsDocumentaryPackCatalog::selectedLanguage())
            return result; // Pas de substitution silencieuse entre langues.
        maximum = qBound(1,maximum,100);
        QString statement;
        const QRegularExpression pageRx(
            QStringLiteral("\\bpages?\\s+(\\d{1,3})\\b"),
            QRegularExpression::CaseInsensitiveOption);
        const auto pageMatch = pageRx.match(question);
        const QStringList subject=nativeSubjectTerms(question);
        const QString operation=nativeOperation(question);
        QStringList tokens;
        for(const QString &token:subject)
            tokens << QStringLiteral("\"%1\"").arg(token);
        QSqlQuery query(m_database);
        if (pageMatch.hasMatch()) {
            statement = QStringLiteral(
                "SELECT document_id,entry_id,title,source_text,source_page,"
                "verification_status,entry_type,source_table "
                "FROM documentary_entries WHERE source_page=? "
                "ORDER BY CASE entry_type WHEN 'reference' THEN 0 "
                "WHEN 'procedure' THEN 1 ELSE 2 END LIMIT ?");
            query.prepare(statement);
            query.addBindValue(pageMatch.captured(1).toInt());
            query.addBindValue(maximum);
        } else if (!tokens.isEmpty()) {
            statement = QStringLiteral(
                "SELECT e.document_id,e.entry_id,e.title,e.source_text,e.source_page,"
                "e.verification_status,e.entry_type,e.source_table "
                "FROM documentary_fts JOIN documentary_entries e "
                "ON e.rowid=documentary_fts.rowid "
                "WHERE documentary_fts MATCH ? "
                "ORDER BY CASE WHEN e.source_table='akm7169fre_page_ocr' "
            "THEN 1 ELSE 0 END, bm25(documentary_fts) LIMIT ?");
            query.prepare(statement);
            // Une question précise doit retrouver ses termes ensemble, et non
            // douze lignes différentes ayant chacune un seul mot en commun.
            query.addBindValue(tokens.join(QStringLiteral(" AND ")));
            // Examiner assez de résultats avant de classer par opération
            // demandée ; ne jamais classer sur seulement les 12 premiers BM25.
            query.addBindValue(qMax(60,maximum));
        } else {
            if(!question.trimmed().isEmpty())
                return result; // Une question vague ne doit pas retourner un annuaire arbitraire.
            statement = QStringLiteral(
                "SELECT document_id,entry_id,title,source_text,source_page,"
                "verification_status,entry_type,source_table "
                "FROM documentary_entries "
                "ORDER BY CASE entry_type WHEN 'reference' THEN 0 "
                "WHEN 'procedure' THEN 1 ELSE 2 END,source_page LIMIT ?");
            query.prepare(statement);
            query.addBindValue(maximum);
        }
        if (!query.exec()) {
#ifdef MEMS_DOCUMENTARY_SELFTEST
            fprintf(stderr,"NATIVE_SQL_SEARCH_FAILED: %s\\n",
                    qPrintable(query.lastError().text()));
#endif
            return result;
        }
        while (query.next()) {
            const QString doc = query.value(0).toString();
            const QString id = query.value(1).toString();
            const QString title = query.value(2).toString();
            const QString sourceText = query.value(3).toString();
            const QString page = query.value(4).isNull()
                ? QStringLiteral("non précisée")
                : query.value(4).toString();
            const QString status = query.value(5).toString();
            const QString type = query.value(6).toString();
            const QString table = query.value(7).toString();
            const QString content = QStringLiteral(
                "document : %1\nlangue : français\npage physique : %2\n"
                "type : %3\nstatut de vérification : %4\n\n%5")
                .arg(doc,page,type,status,sourceText.left(10000));
            QVariantMap hit=makeRow(
                QStringLiteral("rave-fr:%1:%2").arg(doc,id),
                title,content,QStringLiteral("rover_fr_") + table,status);
            hit.insert(QStringLiteral("document_id"),doc);
            hit.insert(QStringLiteral("entry_id"),id);
            hit.insert(QStringLiteral("source_page"),
                       query.value(4).isNull() ? 0 : query.value(4).toInt());
            hit.insert(QStringLiteral("entry_type"),type);
            result.append(hit);
        }
        // Classement par sens de la question : dépose et repose ne sont pas
        // interchangeables, même si "joint de culasse" est commun aux deux.
        if(!subject.isEmpty() && !pageMatch.hasMatch()){
            std::stable_sort(result.begin(),result.end(),
                [&subject,&operation](const QVariant &a,const QVariant &b){
                    return nativeRelevance(a.toMap(),subject,operation)>
                           nativeRelevance(b.toMap(),subject,operation);
                });
            if(!operation.isEmpty()){
                const QString opposite=operation==QStringLiteral("depose")?
                    QStringLiteral("repose"):QStringLiteral("depose");
                QVariantList relevant;
                bool hasMatchingOperation=false;
                for(const QVariant &v:result){
                    if(normalized(v.toMap().value(QStringLiteral("title")).toString())
                       .contains(operation)){
                        hasMatchingOperation=true;
                        break;
                    }
                }
                for(const QVariant &v:result){
                    const QString title=normalized(v.toMap()
                        .value(QStringLiteral("title")).toString());
                    if(hasMatchingOperation && title.contains(opposite))
                        continue;
                    relevant.append(v);
                }
                result=relevant;
            }
        }
        return result.mid(0,maximum);
    }
    const bool browse = question.trimmed().isEmpty();
    // Until the French corpus is complete, all documentary display is kept
    // in French and exposes no partial English manufacturer instructions.
    // Other languages temporarily use the original English corpus, never a
    // mixture of partial translated paragraphs.
    const QString requestedLanguage = language.left(2).toLower();
    const QString locale = requestedLanguage == QStringLiteral("fr")
        ? QStringLiteral("fr") : QStringLiteral("en");
    if (locale == QStringLiteral("fr") && !m_frenchComplete) {
        result.append(makeRow(
            QStringLiteral("rover-fr-translation-not-published"),
            QStringLiteral("Documentation Rover — traduction française en préparation"),
            QStringLiteral(
                "La traduction française intégrale de ce manuel n'est pas encore "
                "terminée et vérifiée. Aucune procédure anglaise n'est mélangée "
                "à cette consultation française. Le PDF constructeur original "
                "reste conservé séparément."),
            QStringLiteral("rover_translation_status"),
            QStringLiteral("FR_NOT_PUBLISHED")));
        return result;
    }
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
            const QString localizedPage = documentText(
                QStringLiteral("ocr.page:%1").arg(requestedPage),
                original, translatedText, locale);
            result.append(makeRow(
                key, QStringLiteral("Rover AKM7169ENG — page %1").arg(requestedPage),
                QStringLiteral("document: Rover AKM7169ENG\npage physique: %1\n"
                               "validation: OCR_DISCOVERY_ONLY — texte non vérifié\n"
                               "extrait OCR: %2").arg(requestedPage).arg(localizedPage),
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
                const QString displayTitle = documentText(
                    QStringLiteral("candidate.title:%1").arg(key),
                    title, translatedText, locale);
                const QString displayRegion = locale == QStringLiteral("fr")
                    ? (region == QStringLiteral("left") ? QStringLiteral("gauche")
                      : region == QStringLiteral("right") ? QStringLiteral("droite")
                      : region)
                    : region;
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
                    .arg(number,displayTitle).arg(physicalPage)
                    .arg(displayRegion,validity,
                         unverifiedProcedurePreview(m_database,key,translatedText,locale));
                found.append({score,makeRow(
                    key, QStringLiteral("%1 — %2 (p. %3, à vérifier)")
                        .arg(number,displayTitle).arg(physicalPage),
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
            const QString body = documentText(
                QStringLiteral("ocr.page:%1").arg(pages.value(0).toInt()),
                pages.value(1).toString().simplified(),
                translatedText, locale);
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

// Les valeurs affichées par IA MEMS doivent être des textes techniques et non
// des champs SQLite ("topic:", "source_text:", "source_section:", etc.).
// Aucune reformulation automatique de couple/valeur ne doit modifier la source.
static QString workshopText(const QVariantMap &row)
{
    QString raw=row.value(QStringLiteral("content")).toString();
    if(raw.contains(QStringLiteral("\n\n")))
        raw=raw.section(QStringLiteral("\n\n"),1);
    QStringList facts;
    QStringList supplementary;
    for(const QString &record : raw.split(QLatin1Char('\n'))){
        const QString line=record.trimmed();
        if(line.isEmpty())
            continue;
        const QString lower=line.toLower();
        if(lower.startsWith(QStringLiteral("source_text:"))){
            const QString value=line.mid(line.indexOf(QLatin1Char(':'))+1).trimmed();
            if(!value.isEmpty()) facts.append(value);
        }else if(lower.startsWith(QStringLiteral("notes:"))){
            const QString value=line.mid(line.indexOf(QLatin1Char(':'))+1).trimmed();
            if(!value.isEmpty()) supplementary.append(value);
        }else if(lower.startsWith(QStringLiteral("topic:")) ||
                 lower.startsWith(QStringLiteral("source_section:")) ||
                 lower.startsWith(QStringLiteral("document:")) ||
                 lower.startsWith(QStringLiteral("document :")) ||
                 lower.startsWith(QStringLiteral("langue :")) ||
                 lower.startsWith(QStringLiteral("page physique :")) ||
                 lower.startsWith(QStringLiteral("type :")) ||
                 lower.startsWith(QStringLiteral("statut de vérification :"))){
            continue;
        }else{
            facts.append(line);
        }
    }
    facts.append(supplementary);
    QStringList unique;
    for(const QString &fact : facts){
        if(!unique.contains(fact))
            unique.append(fact);
    }
    return unique.join(QStringLiteral("\n")).trimmed();
}

// Conserve uniquement les faits textuels lisibles. Les marqueurs techniques
// "SPEC-... valeur 1" ne constituent ni un couple de serrage ni une consigne.
static bool informativeWorkshopText(const QString &text)
{
    if(text.trimmed().size()<12)
        return false;
    static const QRegularExpression placeholder(
        QStringLiteral("^(?:valeur|value)\\s*\\d+$"),
        QRegularExpression::CaseInsensitiveOption);
    return !placeholder.match(text.trimmed()).hasMatch();
}

QString MemsDocumentaryRepository::imagePathForHit(const QVariantMap &hit)
{
    const QString key=hit.value(QStringLiteral("source_key")).toString();
    static const QRegularExpression source(
        QStringLiteral("^rave-fr:([A-Z0-9_-]{1,60}):"),
        QRegularExpression::CaseInsensitiveOption);
    const auto match=source.match(key);
    if(!match.hasMatch())
        return QString();
    int page=hit.value(QStringLiteral("source_page")).toInt();
    if(page<=0){
        static const QRegularExpression pageExpression(
            QStringLiteral("page physique\\s*:\\s*(\\d{1,3})"));
        const auto pageMatch=pageExpression.match(
            hit.value(QStringLiteral("content")).toString());
        if(pageMatch.hasMatch())
            page=pageMatch.captured(1).toInt();
    }
    if(page<=0 || page>999)
        return QString();
    const QString root=QFileInfo(databasePath()).absolutePath();
    const QString imageRoot=QFileInfo(
        QDir(root).filePath(QStringLiteral("images"))).canonicalFilePath();
    if(imageRoot.isEmpty())
        return QString();
    const QString filename=QStringLiteral("%1_PDF_%2.png")
        .arg(match.captured(1))
        .arg(page,3,10,QLatin1Char('0'));
    const QFileInfo file(QDir(imageRoot).filePath(filename));
    const QString absolute=file.canonicalFilePath();
    if(!file.isFile() || file.isSymLink() ||
       !absolute.startsWith(imageRoot+QLatin1Char('/'),
                           Qt::CaseInsensitive))
        return QString();
    return absolute;
}

// Les procédures françaises du constructeur sont conservées dans une SQLite
// structurée : la ligne "mems_procedure" contient la clé d'opération, les
// lignes "mems_procedure_step" et "mems_procedure_requirement" portent la même
// procédure dans leur raw_json. La recherche FTS seule ne sait pas faire cette
// jointure ; auparavant SESSION n'affichait que le titre de l'opération.
QString MemsDocumentaryRepository::procedureAnswer(
    const QVariantMap &hit, QStringList *imagePaths) const
{
    if(imagePaths)
        imagePaths->clear();
    if(!isOpen() || !m_nativeDocumentary ||
       hit.value(QStringLiteral("entry_type")).toString()!=QStringLiteral("procedure"))
        return QString();

    const QString document=hit.value(QStringLiteral("document_id")).toString();
    const QString entry=hit.value(QStringLiteral("entry_id")).toString();
    if(document.isEmpty() || entry.isEmpty())
        return QString();

    QSqlQuery main(m_database);
    main.prepare(QStringLiteral(
        "SELECT source_revision,source_page,raw_json,title "
        "FROM documentary_entries "
        "WHERE document_id=? AND entry_id=? AND entry_type='procedure'"));
    main.addBindValue(document);
    main.addBindValue(entry);
    if(!main.exec() || !main.next())
        return QString();

    const QString revision=main.value(0).toString();
    const int openingPage=main.value(1).toInt();
    const QJsonObject procedure=QJsonDocument::fromJson(
        main.value(2).toByteArray()).object();
    const QString procedureKey=procedure.value(
        QStringLiteral("procedure_key")).toString().trimmed();
    const QString title=main.value(3).toString().trimmed();
    if(procedureKey.isEmpty())
        return QString();

    struct WorkshopStep {
        int order=0;
        int page=0;
        QString text;
        bool checked=false;
    };
    QList<WorkshopStep> steps;
    QList<WorkshopStep> requirements;
    QSet<int> pageSet;
    if(openingPage>0)
        pageSet.insert(openingPage);

    QSqlQuery related(m_database);
    related.prepare(QStringLiteral(
        "SELECT source_table,source_page,verification_status,raw_json "
        "FROM documentary_entries "
        "WHERE document_id=? AND source_revision=? "
        "AND source_table IN ('mems_procedure_step',"
        "                     'mems_procedure_requirement')"));
    related.addBindValue(document);
    related.addBindValue(revision);
    if(!related.exec())
        return QString();

    while(related.next()){
        const QString kind=related.value(0).toString();
        const int page=related.value(1).toInt();
        const bool checked=related.value(2).toString() ==
            QStringLiteral("source_checked") ||
            related.value(2).toString()==QStringLiteral("technical_reviewed");
        const QJsonObject record=QJsonDocument::fromJson(
            related.value(3).toByteArray()).object();
        if(record.value(QStringLiteral("procedure_key")).toString()!=procedureKey)
            continue;

        QString source;
        if(kind==QStringLiteral("mems_procedure_step")){
            const QStringList fields={
                QStringLiteral("instruction_source"),
                QStringLiteral("instruction_text"),
                QStringLiteral("step_text"),
                QStringLiteral("action_text"),
                QStringLiteral("instruction"),
                QStringLiteral("statement")
            };
            for(const QString &field:fields){
                source=record.value(field).toString().trimmed();
                if(!source.isEmpty())break;
            }
        } else {
            const QStringList fields={
                QStringLiteral("requirement_source"),
                QStringLiteral("requirement_text"),
                QStringLiteral("condition_text"),
                QStringLiteral("description"),
                QStringLiteral("notes")
            };
            for(const QString &field:fields){
                source=record.value(field).toString().trimmed();
                if(!source.isEmpty())break;
            }
        }
        if(source.isEmpty())
            continue;
        WorkshopStep item;
        item.order=record.value(QStringLiteral("step_no")).toInt();
        item.page=page;
        item.text=source;
        item.checked=checked;
        if(page>0)
            pageSet.insert(page);
        if(kind==QStringLiteral("mems_procedure_step"))
            steps.append(item);
        else
            requirements.append(item);
    }
    std::stable_sort(steps.begin(),steps.end(),
        [](const WorkshopStep &a,const WorkshopStep &b){
            return a.order<b.order;
        });
    if(steps.isEmpty())
        return QString(); // Ne jamais annoncer une procédure complète sans étapes.

    if(imagePaths){
        QList<int> pages=pageSet.values();
        std::sort(pages.begin(),pages.end());
        for(const int page:pages){
            QVariantMap imageHit=hit;
            imageHit.insert(QStringLiteral("source_page"),page);
            const QString path=imagePathForHit(imageHit);
            if(!path.isEmpty() && !imagePaths->contains(path))
                imagePaths->append(path);
        }
    }

    QStringList answer;
    answer << QStringLiteral("%1").arg(title);
    answer << QStringLiteral(
        "Manuel constructeur %1 — %2 étapes conservées dans l'ordre d'origine.")
        .arg(document).arg(steps.size());
    answer << QStringLiteral(
        "Transcription issue de la documentation : les étapes non validées "
        "techniquement doivent être vérifiées sur les pages constructeur "
        "affichées ci-dessous, ainsi que l'applicabilité au véhicule.");
    if(!requirements.isEmpty()){
        answer << QStringLiteral("Précautions et conditions documentaires :");
        for(const WorkshopStep &requirement:requirements)
            answer << QStringLiteral("• %1").arg(requirement.text);
    }
    answer << QStringLiteral("Étapes de l'opération :");
    for(int i=0;i<steps.size();++i){
        const WorkshopStep &step=steps.at(i);
        QString line=QStringLiteral("%1. %2")
            .arg(step.order>0 ? step.order : i+1).arg(step.text);
        if(step.page>0)
            line+=QStringLiteral(" (RCL0193FRE, p. %1)").arg(step.page);
        if(!step.checked)
            line+=QStringLiteral(" [transcription à vérifier]");
        answer << line;
    }
    if(!pageSet.isEmpty()){
        QList<int> pages=pageSet.values();
        std::sort(pages.begin(),pages.end());
        QStringList pageNumbers;
        for(int page:pages)
            pageNumbers.append(QString::number(page));
        answer << QStringLiteral("Fac-similés constructeur associés : pages %1.")
            .arg(pageNumbers.join(QStringLiteral(", ")));
    }
    return answer.join(QLatin1Char('\n'));
}

QString MemsDocumentaryRepository::answerFromHits(const QVariantList &hits)
{
    if (hits.isEmpty())
        return QString();
    QStringList paragraphs;
    const bool originalFrench = hits.first().toMap()
        .value(QStringLiteral("source_key")).toString()
        .startsWith(QStringLiteral("rave-fr:"));
    if (originalFrench) {
        QStringList sections;
        QSet<QString> seen;
        const int maximum=qMin(3,hits.size());
        for(const QVariant &value : hits){
            if(sections.size()>=maximum)
                break;
            const QVariantMap item=value.toMap();
            const QString sourceTable=item.value(QStringLiteral("source_table")).toString();
            const QString key=item.value(QStringLiteral("source_key")).toString();
            const QStringList sourceFields=key.split(QLatin1Char(':'));
            const QString document=sourceFields.size()>=3 ?
                sourceFields.at(1) : QStringLiteral("Rover");
            const int page=item.value(QStringLiteral("source_page")).toInt();
            const QString title=item.value(QStringLiteral("title")).toString().simplified();
            QString body=workshopText(item);
            if(sourceTable==QStringLiteral("rover_fr_akm7169fre_page_ocr")){
                // Le scan n'est pas une procédure vérifiée. Proposer sa page,
                // mais jamais reproduire ses instructions non relues.
                body=QStringLiteral(
                    "OCR de repérage non validé : consulter l'image du constructeur. Aucune instruction non relue n'est transmise à l'IA.");
            }
            if(!informativeWorkshopText(body))
                continue;
            const QString signature=body.left(250).toCaseFolded();
            if(seen.contains(signature))
                continue;
            seen.insert(signature);
            QString section;
            static const QRegularExpression internalId(
                QStringLiteral("^(?:SPEC|PROC)-[A-Z0-9_-]+$"),
                QRegularExpression::CaseInsensitiveOption);
            if(!title.isEmpty() && !internalId.match(title).hasMatch())
                section=title+QStringLiteral("\n");
            section+=body;
            section+=QStringLiteral("\nSource constructeur : %1").arg(document);
            if(page>0)
                section+=QStringLiteral(", page %1").arg(page);
            sections.append(section);
        }
        if(sections.isEmpty()){
            return QStringLiteral(
                "Des rubriques sont indexées, mais aucun extrait technique "
                "suffisamment explicite ne répond à la demande. "
                "Consulte la page constructeur correspondante, sans déduire de valeur absente.");
        }
        QString answer=sections.join(QStringLiteral("\n\n"));
        answer+=QStringLiteral(
            "\n\nVérifier la variante du moteur et la page constructeur "
            "avant toute intervention.");
        return answer;
    }
    {
        paragraphs << QStringLiteral(
            "Documentation Rover AKM7169ENG — recherche dans la SQLite commune "
            "au lecteur et à l'IA. Les transcriptions ci-dessous NE SONT PAS "
            "des procédures techniquement approuvées. Vérifier le PDF constructeur "
            "et l'applicabilité au véhicule avant une intervention.");
    }
    const int count = qMin(3, hits.size());
    for (int i = 0; i < count; ++i) {
        const QVariantMap item = hits.at(i).toMap();
        QString content = item.value(QStringLiteral("content")).toString();
        const QString sourceTable=item.value(
            QStringLiteral("source_table")).toString();
        if (sourceTable==QStringLiteral("rover_fr_akm7169fre_page_ocr")) {
            // Le texte d'un scan n'a pas été relu ligne par ligne. Il permet
            // de retrouver une page dans la base, jamais de générer une
            // consigne de réparation prétendument vérifiée dans l'IA.
            content=content.section(QStringLiteral("\n\n"),0,0)
                +QStringLiteral(
                    "\nOCR de repérage non validé : consulter l'image "
                    "du constructeur dans Base de données. "
                    "Aucune étape OCR n'est approuvée pour l'IA.");
        }
        if (sourceTable == QStringLiteral("rover_procedure_candidate")) {
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
