#include "MemsDocumentaryRepository.h"
#include "MemsDocumentaryPackCatalog.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
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
        QStringLiteral("installer"),QStringLiteral("installation"),
        QStringLiteral("reparation"),QStringLiteral("reparer"),
        QStringLiteral("revision"),QStringLiteral("entretien"),
        QStringLiteral("controle"),QStringLiteral("controler"),
        QStringLiteral("verification"),QStringLiteral("verifier"),
        QStringLiteral("reglage"),QStringLiteral("regler"),
        QStringLiteral("remplacement"),QStringLiteral("remplacer"),
        QStringLiteral("purge"),QStringLiteral("purger"),
        QStringLiteral("diagnostic"),QStringLiteral("inspection"),
        QStringLiteral("serrage"),QStringLiteral("couple"),
        QStringLiteral("schema"),QStringLiteral("schemas"),
        // Vehicle-system variants constrain applicability, not the name of
        // a part. Filtering them as FTS AND tokens hid procedures whose
        // titles described the component but did not repeat SPi/MPi.
        QStringLiteral("spi"),QStringLiteral("mpi"),
        QStringLiteral("monopoint"),QStringLiteral("multipoint")
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
    return published.trimmed().isEmpty() ? QString() : published;
}

QString procedureContent(const QSqlDatabase &database, const QString &key,
                         const QString &title, const QString &ref,
                         const QString &pages, const QString &status,
                         const QHash<QString, QString> &translations,
                         const QString &language)
{
    Q_UNUSED(status);
    QStringList fields = {
        QStringLiteral("title: %1").arg(title),
        QStringLiteral("référence constructeur: %1").arg(ref),
        QStringLiteral("document: Rover AKM7169ENG"),
        QStringLiteral("pages physiques: %1").arg(pages)
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
            fields << QStringLiteral("Figure citée (page %1): %2")
                .arg(figures.value(1).toInt()).arg(figures.value(0).toString());
    }
    return fields.join(QLatin1Char('\n'));
}

QString unverifiedProcedurePreview(const QSqlDatabase &database,
                                   const QString &candidateId,
                                   const QHash<QString, QString> &translations,
                                   const QString &language)
{
    QStringList lines;
    QSqlQuery instructions(database);
    instructions.prepare(QStringLiteral(
        "SELECT phase_kind,manufacturer_step_number,"
        "ocr_instruction_candidate,rowid "
        "FROM extracted_procedure_candidate_steps "
        "WHERE candidate_id=? ORDER BY phase_position,step_position"));
    instructions.addBindValue(candidateId);
    if(!instructions.exec())
        return QString();
    while(instructions.next()){
        const QString kind=instructions.value(0).toString();
        const int number=instructions.value(1).toInt();
        const QString source=documentText(
            QStringLiteral("candidate.step:%1").arg(
                instructions.value(3).toLongLong()),
            instructions.value(2).toString().simplified(),
            translations,language);
        if(!source.trimmed().isEmpty())
            lines << QStringLiteral("%1 %2: %3").arg(kind).arg(number).arg(source);
    }
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

MemsDocumentaryRepository::MemsDocumentaryRepository(const QString &databaseFile)
    : m_explicitPath(databaseFile),
      m_connectionName(QStringLiteral("MEMS_DOCUMENTARY_%1")
          .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
{
}

MemsDocumentaryRepository::~MemsDocumentaryRepository()
{
    qDeleteAll(m_additionalReaders);
    m_additionalReaders.clear();
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
#ifdef MEMS_DOCUMENTARY_SELFTEST
    const QString overridePath=qEnvironmentVariable("MEMS_DOCUMENTARY_DB").trimmed();
    if(!overridePath.isEmpty())
        return QFileInfo(QDir::cleanPath(overridePath)).isFile();
#endif
    return !MemsDocumentaryPackCatalog::installedDatabasePaths(
        MemsDocumentaryPackCatalog::selectedLanguage()).isEmpty();
}

bool MemsDocumentaryRepository::open()
{
    if (m_database.isOpen())
        return true;
    m_error.clear();
    m_frenchComplete = false;
    m_nativeDocumentary = false;
    const QStringList locations=MemsDocumentaryPackCatalog::installedDatabasePaths(
        MemsDocumentaryPackCatalog::selectedLanguage());
    const QFileInfo file(m_explicitPath.isEmpty()
        ? (QFileInfo::exists(databasePath()) ? databasePath()
           : (locations.isEmpty() ? databasePath() : locations.first()))
        : m_explicitPath);
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
        openExtraReaders();
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
    openExtraReaders();
    return true;
}

void MemsDocumentaryRepository::openExtraReaders()
{
    if(!m_explicitPath.isEmpty())
        return; // Children never recursively enumerate installed packs.
    const QStringList paths=MemsDocumentaryPackCatalog::installedDatabasePaths(
        MemsDocumentaryPackCatalog::selectedLanguage());
    for(const QString &path:paths){
        if(QFileInfo(path).absoluteFilePath()==QFileInfo(m_database.databaseName()).absoluteFilePath())
            continue;
        auto *reader=new MemsDocumentaryRepository(path);
        if(reader->open())
            m_additionalReaders.append(reader);
        else
            delete reader; // A damaged extra cannot prevent the historical DB opening.
    }
}

QVariantList MemsDocumentaryRepository::search(
    const QString &question,int maximum,bool allowOcrDiscovery,
    const QString &language) const
{
    maximum=qBound(1,maximum,100);
    QVariantList combined=searchLocal(question,maximum,allowOcrDiscovery,language);
    for(const auto *reader:m_additionalReaders)
        combined.append(reader->searchLocal(question,maximum,allowOcrDiscovery,language));
    // Stable source identity: two packs may include exactly the same document.
    QSet<QString> encountered;
    QVariantList unique;
    for(const QVariant &value:combined){
        const QVariantMap hit=value.toMap();
        const QString identity=hit.value(QStringLiteral("document_id")).toString()
            +QLatin1Char('/')+hit.value(QStringLiteral("entry_id")).toString()
            +QLatin1Char('/')+hit.value(QStringLiteral("source_key")).toString();
        if(encountered.contains(identity))
            continue;
        encountered.insert(identity);
        unique.append(value);
    }
    const QStringList subject=nativeSubjectTerms(question);
    const QString operation=nativeOperation(question);
    if(!subject.isEmpty()){
        std::stable_sort(unique.begin(),unique.end(),
            [&subject,&operation](const QVariant &a,const QVariant &b){
                return nativeRelevance(a.toMap(),subject,operation)>
                       nativeRelevance(b.toMap(),subject,operation);
            });
    }
    return unique.mid(0,maximum);
}

bool MemsDocumentaryRepository::isOpen() const
{
    return m_database.isValid() && m_database.isOpen();
}

QString MemsDocumentaryRepository::lastError() const
{
    return m_error;
}

QVariantList MemsDocumentaryRepository::searchLocal(const QString &question,
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
            tokens << (subject.size()==1 && token.size()>=4
                ? QStringLiteral("\"%1\"*").arg(token)
                : QStringLiteral("\"%1\"").arg(token));
        QSqlQuery query(m_database);
        if (pageMatch.hasMatch()) {
            statement = QStringLiteral(
                "SELECT document_id,entry_id,title,source_text,source_page,"
                "verification_status,entry_type,source_table,source_revision "
                "FROM documentary_entries WHERE source_page=? "
                "ORDER BY CASE entry_type WHEN 'reference' THEN 0 "
                "WHEN 'procedure' THEN 1 ELSE 2 END LIMIT ?");
            query.prepare(statement);
            query.addBindValue(pageMatch.captured(1).toInt());
            query.addBindValue(maximum);
        } else if (!tokens.isEmpty()) {
            statement = QStringLiteral(
                "SELECT e.document_id,e.entry_id,e.title,e.source_text,e.source_page,"
                "e.verification_status,e.entry_type,e.source_table,source_revision "
                "FROM documentary_fts JOIN documentary_entries e "
                "ON e.rowid=documentary_fts.rowid "
                "WHERE documentary_fts MATCH ? "
                "ORDER BY CASE WHEN e.source_table='akm7169fre_page_ocr' "
            "THEN 1 ELSE 0 END, bm25(documentary_fts) LIMIT ?");
            query.prepare(statement);
            // Une question précise doit retrouver ses termes ensemble, et non
            // douze lignes différentes ayant chacune un seul mot en commun.
            const bool multipleSubjects=subject.size()>1 &&
                (normalized(question).contains(QStringLiteral(" et ")) ||
                 normalized(question).contains(QStringLiteral(" and ")));
            // "bielle et piston" asks for two subjects. The FTS must not
            // require both inside the very same page/paragraph.
            query.addBindValue(tokens.join(multipleSubjects
                ? QStringLiteral(" OR ") : QStringLiteral(" AND ")));
#ifdef MEMS_DOCUMENTARY_SELFTEST
            fprintf(stderr,"NATIVE_TERMS=%s MATCH=%s\\n",
                qPrintable(subject.join(QStringLiteral(","))),
                qPrintable(tokens.join(QStringLiteral(" AND "))));
#endif
            // Examiner assez de résultats avant de classer par opération
            // demandée ; ne jamais classer sur seulement les 12 premiers BM25.
            query.addBindValue(qMax(800,maximum));
        } else {
            if(!question.trimmed().isEmpty())
                return result; // Une question vague ne doit pas retourner un annuaire arbitraire.
            statement = QStringLiteral(
                "SELECT document_id,entry_id,title,source_text,source_page,"
                "verification_status,entry_type,source_table,source_revision "
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
            const QString selectedLanguage=MemsDocumentaryPackCatalog::selectedLanguage();
            const QString localeName=selectedLanguage==QStringLiteral("fr")
                ? QStringLiteral("français") : QStringLiteral("anglais");
            const QString content=QStringLiteral(
                "document : %1\nlangue : %2\npage physique : %3\n"
                "type : %4\n\n%5")
                .arg(doc).arg(localeName).arg(page).arg(type)
                .arg(sourceText);
            QVariantMap hit=makeRow(
                QStringLiteral("rave-%1:%2:%3").arg(selectedLanguage,doc,id),
                title,content,
                QStringLiteral("rover_%1_%2").arg(selectedLanguage,table),status);
            hit.insert(QStringLiteral("documentary_database_path"),m_database.databaseName());
            hit.insert(QStringLiteral("source_revision"),query.value(8).toString());
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
    if (locale == QStringLiteral("fr") && !m_frenchComplete)
        return result;
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
                "FROM extracted_procedure_candidates"))) {
            QVector<Candidate> found;
            while (detected.next()) {
                const QString key = detected.value(0).toString();
                const int physicalPage = detected.value(1).toInt();
                const QString region = detected.value(2).toString();
                const QString number = detected.value(3).toString();
                const QString title = detected.value(4).toString();
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
                        "WHERE candidate_id=?"));
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
                    "%5")
                    .arg(number,displayTitle).arg(physicalPage)
                    .arg(displayRegion,
                         unverifiedProcedurePreview(m_database,key,translatedText,locale));
                found.append({score,makeRow(
                    key, QStringLiteral("%1 — %2 (p. %3)")
                        .arg(number,displayTitle).arg(physicalPage),
                    content,QStringLiteral("rover_procedure_candidate"),
                    detected.value(6).toString())});
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
                               
                               "extrait OCR: %2").arg(pageNumber).arg(body),
                QStringLiteral("rover_ocr_discovery"), status));
        }
    }
    return result;
}

// Title formatting is presentation only. Source identifiers and numerical
// values remain immutable inside the selected documentary rows.
static QString readableDocumentaryTitle(const QVariantMap &row)
{
    QString title=row.value(QStringLiteral("title")).toString().simplified();
    const QRegularExpression identifier(
        QStringLiteral("^[a-z0-9]+(?:_[a-z0-9]+){2,}(?:\\s+[a-z]+)?$"),
        QRegularExpression::CaseInsensitiveOption);
    if(!identifier.match(title).hasMatch())
        return title;
    const bool torque=title.startsWith(QStringLiteral("torque_"),
                                      Qt::CaseInsensitive);
    if(torque)
        title=title.mid(QStringLiteral("torque_").size());
    title.replace(QLatin1Char('_'),QLatin1Char(' '));
    if(torque){
        title.remove(QRegularExpression(QStringLiteral("\\s+torque$"),
                         QRegularExpression::CaseInsensitiveOption));
        title=QStringLiteral("Couple de serrage — %1").arg(title);
    }else if(row.value(QStringLiteral("entry_type")).toString()
              ==QStringLiteral("technical_value")){
        title=QStringLiteral("Valeur technique — %1").arg(title);
    }
    return title;
}

QVariantList MemsDocumentaryRepository::groupedChoices(const QVariantList &hits)
{
    QVariantList groups;
    QHash<QString,int> positions;
    for(const QVariant &entry:hits){
        const QVariantMap row=entry.toMap();
        const QString key=normalized(readableDocumentaryTitle(row))
            +QLatin1Char('|')+row.value(QStringLiteral("entry_type")).toString()
            +QLatin1Char('|')+(row.value(QStringLiteral("iaMemsMentionOnly")).toBool()
                 ? QStringLiteral("mention"):QStringLiteral("rubrique"));
        if(!positions.contains(key)){
            positions.insert(key,groups.size());
            groups.append(row);
            continue;
        }
        const int index=positions.value(key);
        QVariantMap current=groups.at(index).toMap();
        QVariantList originals=current.value(QStringLiteral("iaMemsGroupRows")).toList();
        if(originals.isEmpty())
            originals.append(current);
        originals.append(row);
        current.insert(QStringLiteral("iaMemsGroupRows"),originals);
        groups[index]=current;
    }
    return groups;
}

QVariantMap MemsDocumentaryRepository::choiceForNumber(
    const QVariantList &choices, const QString &response)
{
    const QString number=response.trimmed();
    static const QRegularExpression integer(QStringLiteral("^[1-9][0-9]*$"));
    if(!integer.match(number).hasMatch())
        return {};
    bool valid=false;
    const int selected=number.toInt(&valid);
    if(!valid || selected<1 || selected>choices.size())
        return {};
    return choices.at(selected-1).toMap();
}

QString MemsDocumentaryRepository::numberedMenu(
    const QString &question, const QVariantList &choices, bool french)
{
    Q_UNUSED(french);
    QStringList lines;
    lines << QStringLiteral("Rubriques retrouvées pour « %1 » :").arg(question);
    for (int index = 0; index < choices.size(); ++index) {
        const QVariantMap row=choices.at(index).toMap();
        QString title=readableDocumentaryTitle(row);
        if(row.value(QStringLiteral("iaMemsMentionOnly")).toBool())
            title+=QStringLiteral(" [mention dans le document]");
        const QVariantList group=row.value(QStringLiteral("iaMemsGroupRows")).toList();
        if(!group.isEmpty())
            title+=QStringLiteral(" — %1 références").arg(group.size());
        if(row.value(QStringLiteral("iaMemsGroupChild")).toBool()){
            const int page=row.value(QStringLiteral("source_page")).toInt();
            if(page>0)
                title+=QStringLiteral(" — page %1").arg(page);
            const QString type=row.value(QStringLiteral("entry_type")).toString();
            if(!type.isEmpty() && type!=QStringLiteral("reference"))
                title+=QStringLiteral(" (%1)").arg(type);
        }
        lines<<QStringLiteral("%1. %2").arg(index+1).arg(title);
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
    const int headerEnd=raw.indexOf(QStringLiteral("\n\n"));
    if(headerEnd>=0)
        raw=raw.mid(headerEnd+2);
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
    // Cataloguing notes are not manufacturer repair instructions. Keep them
    // only when no source text was indexed for this entry.
    if (facts.isEmpty())
        facts.append(supplementary);
    QStringList unique;
    for(const QString &fact : facts){
        if(!unique.contains(fact))
            unique.append(fact);
    }
    return unique.join(QStringLiteral("\n")).trimmed();
}


// Manufacturer PDFs sometimes carry running headers, printed page numbers,
// and the next chapter heading in the same indexed source excerpt. Do not
// change the original document or its provenance: remove only recognisable
// page-layout furniture from the IA display. Numbered repair instructions,
// phase headings and safety warnings must remain byte-for-byte unchanged.
static QString readableWorkshopExcerpt(const QString &source)
{
    const QStringList lines=source.split(QLatin1Char('\n'),Qt::KeepEmptyParts);
    static const QRegularExpression printedPage(QStringLiteral("^\\d{1,3}$"));
    static const QRegularExpression numberedStep(QStringLiteral("^\\d{1,3}[.)]\\s+\\S"));
    const auto isPage=[](const QString &line){
        return printedPage.match(line.trimmed()).hasMatch();
    };
    const auto isStep=[](const QString &line){
        return numberedStep.match(line.trimmed()).hasMatch();
    };
    const auto isHeader=[](const QString &line){
        const QString text=line.trimmed();
        if(text.size()<4 || text.size()>80 || text!=text.toUpper())
            return false;
        bool hasLetter=false;
        for(const QChar ch:text){
            if(ch.isLetter())hasLetter=true;
            else if(!ch.isSpace() && !ch.isDigit() &&
                    ch!=QLatin1Char('-') && ch!=QLatin1Char('/'))
                return false;
        }
        return hasLetter;
    };
    const auto isWorkshopPhase=[](const QString &line){
        static const QSet<QString> phases={
            QStringLiteral("depose"),QStringLiteral("repose"),
            QStringLiteral("inspection"),QStringLiteral("controle"),
            QStringLiteral("reglage"),QStringLiteral("reparation"),
            QStringLiteral("purge"),QStringLiteral("essai"),
            QStringLiteral("remplacement"),QStringLiteral("demontage"),
            QStringLiteral("remontage")
        };
        return phases.contains(normalized(line));
    };
    QStringList output;
    bool hasInstruction=false;
    for(int i=0;i<lines.size();++i){
        const QString line=lines.at(i).trimmed();
        // A running PDF header is two capitalized headings plus a printed
        // page number, in any order. Page layout can be "MOTEUR / 10 /
        // REPARATIONS" or "MOTEUR / REPARATIONS / 10". Remove the entire
        // triple, but ONLY when it starts a page or precedes a new step.
        if(i+2<lines.size()){
            int headers=0,pages=0;
            for(int k=i;k<i+3;++k){
                headers+=isHeader(lines.at(k)) ? 1 : 0;
                pages+=isPage(lines.at(k)) ? 1 : 0;
            }
            const bool nextStep=i+3<lines.size() && isStep(lines.at(i+3));
            if(headers==2 && pages==1 && (!hasInstruction || nextStep)){
                i+=2;
                continue;
            }
        }
        // Two running headings at the end are layout, not a next operation.
        if(hasInstruction && i+2==lines.size() &&
           isHeader(lines.at(i)) && isHeader(lines.at(i+1)) &&
           !isWorkshopPhase(lines.at(i)))
            break;
        // A new chapter title after a page number belongs to the next
        // section; never incorporate it into the current procedure.
        if(hasInstruction && i+1<lines.size() &&
           isPage(lines.at(i)) && isHeader(lines.at(i+1)))
            break;
        // The final single uppercase chapter heading (e.g. "EMBRAYAGE")
        // is not part of the preceding numbered operation.
        if(hasInstruction && i+1==lines.size() && isHeader(line) &&
           !isWorkshopPhase(line))
            break;
        output.append(lines.at(i));
        if(isStep(line))
            hasInstruction=true;
    }
    return output.join(QLatin1Char('\n')).trimmed();
}

// Only an IA display transform. Original source_text, operation steps,
// pagination and PDF images remain unchanged in the installed SQLite.
// Hard line breaks from PDF column extraction are joined within a paragraph
// or numbered instruction; real structure starts a new line.
static QString readableWorkshopParagraphs(const QString &excerpt)
{
    const QStringList lines=excerpt.split(QLatin1Char('\n'),Qt::KeepEmptyParts);
    static const QRegularExpression stepStart(
        QStringLiteral("^\\d{1,3}[.)]\\s+\\S"));
    static const QRegularExpression safetyStart(
        QStringLiteral("^(AVERTISSEMENT|ATTENTION|REMARQUE|IMPORTANT|NOTE)\\s*[:：]"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression operationNumber(
        QStringLiteral("^(?:Opération de réparation|Service Repair No\\.)\\b"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression numericValue(
        QStringLiteral("^[+-]?\\d+(?:[.,]\\d+)?$"));
    static const QSet<QString> phaseTitles={
        QStringLiteral("depose"),QStringLiteral("repose"),
        QStringLiteral("inspection"),QStringLiteral("controle"),
        QStringLiteral("reglage"),QStringLiteral("reparation"),
        QStringLiteral("purge"),QStringLiteral("essai"),
        QStringLiteral("remplacement"),QStringLiteral("demontage"),
        QStringLiteral("remontage")
    };
    QStringList paragraphs;
    QString current;
    const auto flush=[&paragraphs,&current](){
        if(!current.isEmpty()){
            paragraphs.append(current);
            current.clear();
        }
    };
    for(const QString &rawLine:lines){
        const QString line=rawLine.trimmed();
        if(line.isEmpty()){
            flush();
            continue;
        }
        const bool startsNew=stepStart.match(line).hasMatch() ||
            safetyStart.match(line).hasMatch() ||
            operationNumber.match(line).hasMatch() ||
            phaseTitles.contains(normalized(line)) ||
            numericValue.match(line).hasMatch();
        if(startsNew)
            flush();
        if(current.isEmpty())
            current=line;
        else
            current+=QLatin1Char(' ')+line;
        // A standalone number may be a technical value, not a page header:
        // preserve its boundaries until the next source row.
        if(numericValue.match(line).hasMatch())
            flush();
    }
    flush();
    return paragraphs.join(QLatin1Char('\n'));
}

// Conserve uniquement les faits textuels lisibles. Les marqueurs techniques
// "SPEC-... valeur 1" ne constituent ni un couple de serrage ni une consigne.

QStringList MemsDocumentaryRepository::subjectTerms(const QString &question)
{
    return nativeSubjectTerms(question);
}

static bool documentaryTokenMatches(const QString &candidate,
                                    const QString &requested)
{
    if(candidate==requested)
        return true;
    // Only grammatical endings, never arbitrary substring matches (e.g.
    // "injecteur" cannot match the unrelated word "injection").
    return requested.size()>=4 && candidate.startsWith(requested) &&
           candidate.size()<=requested.size()+2;
}

static int documentaryTitleHits(const QString &title,
                                const QStringList &terms)
{
    const QStringList words=normalized(title).split(
        QLatin1Char(' '),Qt::SkipEmptyParts);
    int matches=0;
    for(const QString &term:terms){
        for(const QString &word:words){
            if(documentaryTokenMatches(word,term)){
                ++matches;
                break;
            }
        }
    }
    return matches;
}

// A printed technical table uses dot leaders between its parameter and
// value. Dot leaders are layout, never technical data. Only the copy shown
// to IA MEMS is cleaned; SQLite and PDF page remain untouched.
static QString readableReferenceLayout(QString text)
{
    // In the manufacturer's PDF the dotted leaders delimit individual
    // specification cells. Replacing them by an em dash alone used to
    // concatenate the entire engine settings table into one unreadable
    // sentence. Rebuild display rows without altering the stored source
    // text, values, units or the linked PDF.
    static const QRegularExpression leaders(
        QStringLiteral("(?:[ \\t]*\\.[ \\t]*){4,}"));
    static const QRegularExpression spaces(QStringLiteral("[ \\t]{2,}"));
    static const QRegularExpression valueOnly(
        QStringLiteral("^(?:[+\\-]?\\d|[A-Z]{2,7}[ \\-]?\\d)"));
    static const QRegularExpression numberedStep(
        QStringLiteral("^\\d{1,3}[.)]\\s+\\S"));
    text.remove(QLatin1Char('\r'));
    QStringList rows;
    for(const QString &rawLine:text.split(QLatin1Char('\n'),Qt::KeepEmptyParts)){
        const QString line=rawLine.trimmed();
        if(line.isEmpty()){
            if(!rows.isEmpty() && !rows.last().isEmpty())
                rows.append(QString());
            continue;
        }
        // Each sequence of dotted leaders is a potential column or row
        // boundary. When the next cell is an unambiguous numeric value or
        // reference code, keep it with its parameter; otherwise begin a
        // new readable row. This is vocabulary-independent.
        for(const QString &field:line.split(leaders,Qt::SkipEmptyParts)){
            QString cell=field.trimmed();
            cell.replace(spaces,QStringLiteral(" "));
            if(cell.isEmpty())
                continue;
            const bool looksLikeValue=valueOnly.match(cell).hasMatch() &&
                !numberedStep.match(cell).hasMatch();
            if(looksLikeValue && !rows.isEmpty() && !rows.last().isEmpty()){
                rows.last()+=QStringLiteral(" : ")+cell;
            }else{
                rows.append(cell);
            }
        }
    }
    while(!rows.isEmpty() && rows.last().isEmpty())
        rows.removeLast();
    return rows.join(QLatin1Char('\n')).trimmed();
}

static QString documentaryTopicExcerpt(const QVariantMap &hit,
                                       const QStringList &terms,
                                       bool separateSubjects=false)
{
    const QString plainSource=readableWorkshopExcerpt(workshopText(hit));
    const QString source=hit.value(QStringLiteral("entry_type")).toString()
        ==QStringLiteral("reference")
        ? readableReferenceLayout(plainSource)
        : readableWorkshopParagraphs(plainSource);
    const QStringList lines=source.split(QLatin1Char('\n'));
    QStringList excerpts;
    for(int i=0;i<lines.size();++i){
        const QString line=normalized(lines.at(i));
        bool potential=false;
        for(const QString &term:terms){
            for(const QString &word:line.split(QLatin1Char(' '),Qt::SkipEmptyParts))
                if(documentaryTokenMatches(word,term)){
                    potential=true;
                    break;
                }
            if(potential)
                break;
        }
        if(!potential)
            continue;
        const int start=qMax(0,i-1);
        const int finish=qMin(lines.size(),i+4);
        QStringList context;
        for(int k=start;k<finish;++k)
            context.append(lines.at(k));
        const QString joined=normalized(context.join(QLatin1Char(' ')));
        bool complete=true;
        for(const QString &term:terms){
            bool found=false;
            for(const QString &word:joined.split(QLatin1Char(' '),Qt::SkipEmptyParts))
                if(documentaryTokenMatches(word,term)){
                    found=true;
                    break;
                }
            if(!found){
                complete=false;
                break;
            }
        }
        if(!complete && !separateSubjects)
            continue;
        QString excerpt=readableReferenceLayout(
            context.join(QLatin1Char('\n')).trimmed());
        // Some PDF extracts flatten a whole technical table to ONE line.
        // A documentary 'mention' should show the matching passage, not
        // dump an entire table with unrelated parameters. Do not invent a
        // value or discard the page link: the original remains consultable.
        if(excerpt.size()>650 && !terms.isEmpty()){
            int at=-1;
            for(const QString &term:terms){
                at=excerpt.indexOf(term,0,Qt::CaseInsensitive);
                if(at>=0)break;
            }
            if(at<0)at=0;
            int begin=qMax(0,at-140);
            int end=qMin(excerpt.size(),at+380);
            if(begin>0){
                const int space=excerpt.indexOf(QLatin1Char(' '),begin);
                if(space>0 && space<at)begin=space+1;
            }
            if(end<excerpt.size()){
                const int space=excerpt.lastIndexOf(QLatin1Char(' '),end);
                if(space>at)end=space;
            }
            excerpt=(begin>0?QStringLiteral("… "):QString())+
                excerpt.mid(begin,end-begin).trimmed()+
                (end<source.size()?QStringLiteral(" …"):QString());
        }
        if(!excerpt.isEmpty() && !excerpts.contains(excerpt))
            excerpts.append(excerpt);
        if(excerpts.size()==2)
            break;
    }
    return excerpts.join(QStringLiteral("\n[…]\n"));
}

QVariantList MemsDocumentaryRepository::relevantChoices(
    const QString &question,const QVariantList &hits)
{
    const QStringList terms=nativeSubjectTerms(question);
    if(terms.isEmpty() || hits.isEmpty())
        return {};
    const QString requestNorm=normalized(question);
    const bool separateSubjects=terms.size()>1 &&
        (requestNorm.contains(QStringLiteral(" et ")) ||
         requestNorm.contains(QStringLiteral(" and ")));
    const int required=separateSubjects ? 1 :
        (terms.size()>=3?terms.size()-1:terms.size());
    const QStringList requestWords=requestNorm.split(QLatin1Char(' '),Qt::SkipEmptyParts);
    const bool wantsSpi=requestWords.contains(QStringLiteral("spi")) ||
                        requestWords.contains(QStringLiteral("monopoint"));
    const bool wantsMpi=requestWords.contains(QStringLiteral("mpi")) ||
                        requestWords.contains(QStringLiteral("multipoint"));
    QVariantList direct;
    QVariantList mentions;
    bool hasConciseTopicHeading=false;
    for(const QVariant &value:hits){
        const QVariantMap row=value.toMap();
        const QString title=row.value(QStringLiteral("title")).toString();
        const QStringList words=normalized(title).split(
            QLatin1Char(' '),Qt::SkipEmptyParts);
        // An explicitly incompatible variant in the heading is evidence
        // against this result. Unlabelled generic headings may still apply,
        // but they do not prove MPi/SPi compatibility.
        const bool titleSpi=words.contains(QStringLiteral("spi")) ||
                            words.contains(QStringLiteral("monopoint"));
        const bool titleMpi=words.contains(QStringLiteral("mpi")) ||
                            words.contains(QStringLiteral("multipoint"));
        if((wantsSpi && titleMpi && !titleSpi) ||
           (wantsMpi && titleSpi && !titleMpi))
            continue;
        const int matched=documentaryTitleHits(title,terms);
        if(matched>=required){
            // A precise heading such as "Injecteur" or "Emplacement :
            // Injecteur" takes precedence over a longer unrelated IAT
            // heading that merely mentions injecteurs as another concept.
            bool concise=false;
            if(terms.size()==1 && words.size()<=2){
                for(const QString &word:words)
                    if(documentaryTokenMatches(word,terms.first()))
                        concise=true;
            }
            if(concise)
                hasConciseTopicHeading=true;
            QVariantMap named=row;
            named.insert(QStringLiteral("iaMemsConciseHeading"),concise);
            if((wantsSpi && !titleSpi) || (wantsMpi && !titleMpi))
                named.insert(QStringLiteral("iaMemsVariantUnverified"),true);
            direct.append(named);
            continue;
        }
        // When there is no relevant heading, expose real source mentions as
        // excerpts (never pretend the unrelated operation is a piston repair).
        const QString entryType=row.value(QStringLiteral("entry_type")).toString();
        // A component might be mentioned only in a linked workshop step.
        // Present that as a source mention, not as a complete repair.
        if(row.value(QStringLiteral("source_table")).toString().contains(
               QStringLiteral("ocr_discovery")))
            continue;
        const QString excerpt=documentaryTopicExcerpt(row,terms,separateSubjects);
        if(excerpt.isEmpty())
            continue;
        QVariantMap mentioned=row;
        mentioned.insert(QStringLiteral("iaMemsMentionOnly"),true);
        mentioned.insert(QStringLiteral("iaMemsMentionSubject"),terms.join(QLatin1Char(' ')));
        mentioned.insert(QStringLiteral("iaMemsMentionExcerpt"),excerpt);
        if((wantsSpi && !titleSpi) || (wantsMpi && !titleMpi))
            mentioned.insert(QStringLiteral("iaMemsVariantUnverified"),true);
        mentions.append(mentioned);
    }
    if(!direct.isEmpty()){
        // Only narrow down when an unambiguous short heading exists.
        // A broad topic such as "frein" must still list its full workshop
        // catalogue, including pedals, calipers, cylinders and linings.
        if(hasConciseTopicHeading && terms.size()==1){
            QVariantList focused;
            const QString term=terms.first();
            for(const QVariant &entry:direct){
                const QVariantMap row=entry.toMap();
                const QStringList words=normalized(row.value(QStringLiteral("title"))
                    .toString()).split(QLatin1Char(' '),Qt::SkipEmptyParts);
                bool startsWithTopic=false;
                if(!words.isEmpty())
                    startsWithTopic=documentaryTokenMatches(words.first(),term);
                if(row.value(QStringLiteral("iaMemsConciseHeading")).toBool() ||
                   startsWithTopic)
                    focused.append(row);
            }
            if(!focused.isEmpty())
                direct=focused;
        }
        std::stable_sort(direct.begin(),direct.end(),
            [](const QVariant &left,const QVariant &right){
                return left.toMap().value(QStringLiteral("iaMemsConciseHeading")).toBool() &&
                      !right.toMap().value(QStringLiteral("iaMemsConciseHeading")).toBool();
            });
        // If a vehicle variant is part of the request, retain any verified
        // mention in a specifically tagged technical section alongside the
        // generic component heading, with its 'mention' label.
        if(wantsSpi || wantsMpi){
            for(const QVariant &entry:mentions){
                const QVariantMap mention=entry.toMap();
                const QStringList mentionWords=normalized(mention.value(
                    QStringLiteral("title")).toString()).split(
                        QLatin1Char(' '),Qt::SkipEmptyParts);
                if((wantsSpi && mentionWords.contains(QStringLiteral("spi"))) ||
                   (wantsMpi && mentionWords.contains(QStringLiteral("mpi"))))
                    direct.append(entry);
            }
        }
        return direct;
    }
    // A word absent from all titles is not "unknown": it may occur inside
    // a checked reference. Return a small, explicitly labelled source index.
    std::stable_sort(mentions.begin(),mentions.end(),
        [](const QVariant &left,const QVariant &right){
            const QVariantMap a=left.toMap(),b=right.toMap();
            const bool aChecked=a.value(QStringLiteral("source_status")).toString()
                ==QStringLiteral("source_checked");
            const bool bChecked=b.value(QStringLiteral("source_status")).toString()
                ==QStringLiteral("source_checked");
            return aChecked && !bChecked;
        });
    return mentions.mid(0,30);
}

QString MemsDocumentaryRepository::imagePathForHit(const QVariantMap &hit)
{
    const QString key=hit.value(QStringLiteral("source_key")).toString();
    static const QRegularExpression source(
        QStringLiteral("^rave-(?:fr|en|pt|es|it|de):([A-Z0-9_-]{1,100}):"),
        QRegularExpression::CaseInsensitiveOption);
    const auto match=source.match(key);
    if(!match.hasMatch())
        return QString();
    const QString document=hit.value(QStringLiteral("document_id"),
                                     match.captured(1)).toString();
    if(document.compare(match.captured(1),Qt::CaseInsensitive)!=0)
        return QString();
    int page=hit.value(QStringLiteral("source_page")).toInt();
    if(page<=0){
        static const QRegularExpression pageExpression(
            QStringLiteral("page physique\\s*:\\s*(\\d{1,5})"));
        const auto pageMatch=pageExpression.match(
            hit.value(QStringLiteral("content")).toString());
        if(pageMatch.hasMatch())
            page=pageMatch.captured(1).toInt();
    }
    if(page<=0 || page>50000)
        return QString();

    const QString requested=hit.value(
        QStringLiteral("documentary_database_path")).toString();
    QString verified=databasePath();
    if(!requested.isEmpty()){
        const QStringList allowed=MemsDocumentaryPackCatalog::installedDatabasePaths(
            MemsDocumentaryPackCatalog::selectedLanguage());
        if(!allowed.contains(requested))
            return QString();
        verified=requested;
    }

    const QString root=QFileInfo(verified).absolutePath();
    const QString imageRoot=QFileInfo(
        QDir(root).filePath(QStringLiteral("images"))).canonicalFilePath();
    if(imageRoot.isEmpty())
        return QString();

    const QString revision=hit.value(QStringLiteral("source_revision")).toString();
    if(!revision.isEmpty()){
        const QString connection=QStringLiteral("DOCUMENTARY_PAGE_IMAGE_%1").arg(
            QUuid::createUuid().toString(QUuid::WithoutBraces));
        QString relative;
        QString sha256;
        qint64 bytes=0;
        {
            QSqlDatabase db=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),connection);
            db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
            db.setDatabaseName(verified);
            if(db.open()){
                QSqlQuery images(db);
                images.prepare(QStringLiteral(
                    "SELECT relative_path,asset_sha256,asset_bytes "
                    "FROM documentary_images WHERE document_id=? "
                    "AND source_revision=? AND physical_page=?"));
                images.addBindValue(document);
                images.addBindValue(revision);
                images.addBindValue(page);
                if(images.exec() && images.next()){
                    relative=images.value(0).toString();
                    sha256=images.value(1).toString();
                    bytes=images.value(2).toLongLong();
                    if(images.next())
                        relative.clear();
                }
                db.close();
            }
        }
        QSqlDatabase::removeDatabase(connection);
        if(!relative.startsWith(QStringLiteral("images/")) ||
           QDir::cleanPath(relative)!=relative ||
           relative.split(QLatin1Char('/')).contains(QStringLiteral("..")))
            return QString();
        const QFileInfo file(QDir(root).filePath(relative));
        const QString absolute=file.canonicalFilePath();
        if(!file.isFile() || file.isSymLink() ||
           !absolute.startsWith(imageRoot+QLatin1Char('/'),Qt::CaseInsensitive) ||
           bytes<=0 || file.size()!=bytes)
            return QString();
        QFile stream(absolute);
        if(!stream.open(QIODevice::ReadOnly))
            return QString();
        QCryptographicHash hash(QCryptographicHash::Sha256);
        while(!stream.atEnd())
            hash.addData(stream.read(1024*1024));
        if(QString::fromLatin1(hash.result().toHex())!=sha256)
            return QString();
        return absolute;
    }

    // Historic unversioned image lookup remains available for old callers.
    const QString filename=QStringLiteral("%1_PDF_%2.png")
        .arg(match.captured(1))
        .arg(page,3,10,QLatin1Char('0'));
    const QFileInfo file(QDir(imageRoot).filePath(filename));
    const QString absolute=file.canonicalFilePath();
    if(!file.isFile() || file.isSymLink() ||
       !absolute.startsWith(imageRoot+QLatin1Char('/'),Qt::CaseInsensitive))
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
    const QString chosen=hit.value(QStringLiteral("documentary_database_path")).toString();
    for(const auto *reader:m_additionalReaders){
        if(!chosen.isEmpty() && chosen==reader->m_database.databaseName())
            return reader->procedureAnswer(hit,imagePaths);
    }
    if(!isOpen() || !m_nativeDocumentary ||
       hit.value(QStringLiteral("entry_type")).toString()!=QStringLiteral("procedure"))
        return QString();

    const QString document=hit.value(QStringLiteral("document_id")).toString();
    const QString entry=hit.value(QStringLiteral("entry_id")).toString();
    if(document.isEmpty() || entry.isEmpty())
        return QString();

    QSqlQuery procedureQuery(m_database);
    procedureQuery.prepare(QStringLiteral(
        "SELECT source_revision,source_page,raw_json,title "
        "FROM documentary_entries "
        "WHERE document_id=? AND entry_id=? AND entry_type='procedure'"));
    procedureQuery.addBindValue(document);
    procedureQuery.addBindValue(entry);
    if(!procedureQuery.exec() || !procedureQuery.next())
        return QString();

    const QString revision=procedureQuery.value(0).toString();
    const int openingPage=procedureQuery.value(1).toInt();
    const QJsonObject procedure=QJsonDocument::fromJson(
        procedureQuery.value(2).toByteArray()).object();
    // All documentary procedures follow one logical key contract. Existing
    // snapshots use either "procedure_key" (FR) or "procedure_id" (EN AKM).
    // Read both from original provenance without altering their source rows.
    QString procedureKey=procedure.value(QStringLiteral("procedure_key"))
        .toString().trimmed();
    if(procedureKey.isEmpty())
        procedureKey=procedure.value(QStringLiteral("procedure_id"))
            .toString().trimmed();
    const QString title=procedureQuery.value(3).toString().trimmed();

    // New FR/EN/other packs expose ONE normalized relation. It is the
    // authoritative source for ordered operations regardless of manufacturer
    // or source-PDF format. The historical raw-json reader below is used only
    // for already installed legacy packs which lack these common tables.
    QSqlQuery structure(m_database);
    const bool normalizedPack=structure.exec(QStringLiteral(
        "SELECT 1 FROM sqlite_master WHERE type='table' "
        "AND name='documentary_operations'")) && structure.next();
    if(normalizedPack){
        QSqlQuery operation(m_database);
        operation.prepare(QStringLiteral(
            "SELECT operation_id,opening_page FROM documentary_operations "
            "WHERE document_id=? AND source_revision=? AND source_entry_id=?"));
        operation.addBindValue(document);
        operation.addBindValue(revision);
        operation.addBindValue(entry);
        if(!operation.exec() || !operation.next())
            return QString(); // Source inventory; no invented workshop operation.
        const QString operationId=operation.value(0).toString();
        QSet<int> pages;
        const int firstPage=operation.value(1).toInt();
        if(firstPage>0) pages.insert(firstPage);

        QSqlQuery steps(m_database);
        steps.prepare(QStringLiteral(
            "SELECT step_number,phase,source_text,source_page,verification_status "
            "FROM documentary_operation_steps "
            "WHERE document_id=? AND source_revision=? AND operation_id=? "
            "ORDER BY CASE phase WHEN 'remove' THEN 0 WHEN 'depose' THEN 0 "
            "WHEN 'refit' THEN 1 WHEN 'repose' THEN 1 "
            "WHEN 'check' THEN 2 WHEN 'inspection' THEN 2 "
            "WHEN 'adjust' THEN 3 ELSE 4 END,step_number,source_entry_id"));
        steps.addBindValue(document);
        steps.addBindValue(revision);
        steps.addBindValue(operationId);
        if(!steps.exec()) return QString();

        struct LinkedStep {
            int number=0;
            int page=0;
            QString phase;
            QString text;
            bool verified=false;
        };
        QList<LinkedStep> linked;
        while(steps.next()){
            LinkedStep item;
            item.number=steps.value(0).toInt();
            item.phase=steps.value(1).toString();
            item.text=steps.value(2).toString().trimmed();
            item.page=steps.value(3).toInt();
            const QString status=steps.value(4).toString();
            item.verified=status==QStringLiteral("source_checked") ||
                          status==QStringLiteral("technical_reviewed");
            if(item.text.isEmpty())continue;
            if(item.page>0)pages.insert(item.page);
            linked.append(item);
        }
        if(linked.isEmpty())
            return QString(); // PDF indexed but no validated step structure.

        QStringList answer;
        answer << title;
        answer << QStringLiteral(
            "Manuel constructeur %1 — %2 étapes disponibles dans l'ordre.")
            .arg(document).arg(linked.size());

        QSqlQuery precautions(m_database);
        precautions.prepare(QStringLiteral(
            "SELECT source_text,source_page FROM documentary_operation_notices "
            "WHERE document_id=? AND source_revision=? AND operation_id=? "
            "ORDER BY source_page,source_entry_id"));
        precautions.addBindValue(document);
        precautions.addBindValue(revision);
        precautions.addBindValue(operationId);
        if(!precautions.exec())return QString();
        QStringList notices;
        while(precautions.next()){
            const QString text=precautions.value(0).toString().trimmed();
            const int page=precautions.value(1).toInt();
            if(!text.isEmpty())notices.append(QStringLiteral("• %1").arg(text));
            if(page>0)pages.insert(page);
        }
        if(!notices.isEmpty()){
            answer << QStringLiteral("Précautions constructeur :");
            answer << notices;
        }
        answer << QStringLiteral("Étapes de l'opération :");
        QString lastPhase;
        for(int i=0;i<linked.size();++i){
            const LinkedStep &item=linked.at(i);
            if(!item.phase.isEmpty() && item.phase!=lastPhase){
                QString label=item.phase;
                if(item.phase==QStringLiteral("remove") ||
                   item.phase==QStringLiteral("depose"))
                    label=QStringLiteral("Dépose");
                else if(item.phase==QStringLiteral("refit") ||
                        item.phase==QStringLiteral("repose"))
                    label=QStringLiteral("Repose");
                else if(item.phase==QStringLiteral("check"))
                    label=QStringLiteral("Contrôle");
                else if(item.phase==QStringLiteral("adjust"))
                    label=QStringLiteral("Réglage");
                answer << label+QLatin1Char(':');
                lastPhase=item.phase;
            }
            // Some source transcriptions already include the printed step number.
            // Display it only once without changing the original stored text.
            QString instruction=readableWorkshopParagraphs(item.text);
            const QRegularExpression printedNumber(QStringLiteral("^\\s*\\d+\\s*[.)]\\s*"));
            instruction.remove(printedNumber);
            QString line=QStringLiteral("%1. %2")
                .arg(item.number>0 ? item.number : i+1).arg(instruction);
            if(item.page>0)
                line+=QStringLiteral(" (%1, p. %2)").arg(document).arg(item.page);
            answer << line;
        }
        if(imagePaths){
            QList<int> sorted=pages.values();
            std::sort(sorted.begin(),sorted.end());
            for(const int page:sorted){
                QVariantMap imageHit=hit;
                imageHit.insert(QStringLiteral("source_page"),page);
                const QString file=imagePathForHit(imageHit);
                if(!file.isEmpty() && !imagePaths->contains(file))
                    imagePaths->append(file);
            }
        }
        return answer.join(QLatin1Char('\n'));
    }

    if(procedureKey.isEmpty())
        return QString();

    struct WorkshopStep {
        int order=0;
        int page=0;
        QString text;
        QString phase;
        int phaseOrder=0;
        bool checked=false;
    };
    QList<WorkshopStep> steps;
    QList<WorkshopStep> requirements;
    QSet<int> pageSet;
    if(openingPage>0)
        pageSet.insert(openingPage);

    // A procedure's children are identified by their *type and source key*,
    // never by the importer-specific source_table. The same reader works
    // for FR, EN and later manufacturer documents.
    QSqlQuery related(m_database);
    related.prepare(QStringLiteral(
        "SELECT entry_type,source_page,verification_status,raw_json,source_text "
        "FROM documentary_entries "
        "WHERE document_id=? AND source_revision=? "
        "AND entry_type IN ('step','notice')"));
    related.addBindValue(document);
    related.addBindValue(revision);
    if(!related.exec())
        return QString();

    while(related.next()){
        const QString kind=related.value(0).toString();
        const int page=related.value(1).toInt();
        const QString status=related.value(2).toString();
        const QJsonObject record=QJsonDocument::fromJson(
            related.value(3).toByteArray()).object();
        QString childKey=record.value(QStringLiteral("procedure_key"))
            .toString().trimmed();
        if(childKey.isEmpty())
            childKey=record.value(QStringLiteral("procedure_id"))
                .toString().trimmed();
        if(childKey!=procedureKey)
            continue;

        QString source;
        const QStringList fields=kind==QStringLiteral("step")
            ? QStringList{
                QStringLiteral("instruction_source"),
                QStringLiteral("instruction_text"),
                QStringLiteral("step_text"),
                QStringLiteral("action_text"),
                QStringLiteral("instruction"),
                QStringLiteral("statement"),
                QStringLiteral("source_text")
              }
            : QStringList{
                QStringLiteral("requirement_source"),
                QStringLiteral("requirement_text"),
                QStringLiteral("condition_text"),
                QStringLiteral("description"),
                QStringLiteral("notes"),
                QStringLiteral("source_text")
              };
        for(const QString &field:fields){
            source=record.value(field).toString().trimmed();
            if(!source.isEmpty())
                break;
        }
        // Raw schema fields vary by edition; the common indexed source_text
        // is authoritative and must not be lost when a PDF importer changes.
        if(source.isEmpty())
            source=related.value(4).toString().trimmed();
        if(source.isEmpty())
            continue;

        WorkshopStep item;
        item.order=record.value(QStringLiteral("step_no")).toInt();
        if(item.order<=0){
            const QJsonValue number=record.value(QStringLiteral("step_number"));
            item.order=number.isDouble()?number.toInt():number.toString().toInt();
        }
        item.page=page;
        item.text=source;
        item.checked=status==QStringLiteral("source_checked") ||
                     status==QStringLiteral("technical_reviewed");
        item.phase=record.value(QStringLiteral("phase")).toString().trimmed();
        const QString normalizedPhase=item.phase.toLower();
        if(normalizedPhase==QStringLiteral("refit") ||
           normalizedPhase==QStringLiteral("repose"))
            item.phaseOrder=1;
        else if(normalizedPhase==QStringLiteral("check") ||
                normalizedPhase==QStringLiteral("inspection"))
            item.phaseOrder=2;
        else if(normalizedPhase==QStringLiteral("adjust"))
            item.phaseOrder=3;
        else if(!normalizedPhase.isEmpty() &&
                normalizedPhase!=QStringLiteral("remove"))
            item.phaseOrder=4;
        if(page>0)
            pageSet.insert(page);
        if(kind==QStringLiteral("step"))
            steps.append(item);
        else
            requirements.append(item);
    }
    std::stable_sort(steps.begin(),steps.end(),
        [](const WorkshopStep &a,const WorkshopStep &b){
            if(a.phaseOrder!=b.phaseOrder)
                return a.phaseOrder<b.phaseOrder;
            if(a.phase!=b.phase && a.order==b.order)
                return a.phase<b.phase;
            return a.order<b.order;
        });
    if(steps.isEmpty())
        return QString(); // Never claim a full procedure from a page-only index.

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
    answer << title;
    answer << QStringLiteral(
        "Manuel constructeur %1 — %2 étapes conservées dans l'ordre d'origine.")
        .arg(document).arg(steps.size());
    if(!requirements.isEmpty()){
        answer << QStringLiteral("Précautions et conditions documentaires :");
        for(const WorkshopStep &requirement:requirements)
            answer << QStringLiteral("• %1").arg(requirement.text);
    }
    answer << QStringLiteral("Étapes de l'opération :");
    QString lastPhase;
    for(int i=0;i<steps.size();++i){
        const WorkshopStep &step=steps.at(i);
        if(!step.phase.isEmpty() && step.phase!=lastPhase){
            QString name=step.phase;
            if(step.phase==QStringLiteral("remove"))name=QStringLiteral("Dépose");
            else if(step.phase==QStringLiteral("refit"))name=QStringLiteral("Repose");
            else if(step.phase==QStringLiteral("check"))name=QStringLiteral("Contrôle");
            else if(step.phase==QStringLiteral("adjust"))name=QStringLiteral("Réglage");
            answer << name+QLatin1Char(':');
            lastPhase=step.phase;
        }
        QString instruction=readableWorkshopParagraphs(step.text);
        instruction.remove(QRegularExpression(QStringLiteral("^\\s*\\d+\\s*[.)]\\s*")));
        QString line=QStringLiteral("%1. %2")
            .arg(step.order>0 ? step.order : i+1).arg(instruction);
        if(step.page>0)
            line+=QStringLiteral(" (%1, p. %2)").arg(document).arg(step.page);
        answer << line;
    }
    if(!pageSet.isEmpty()){
        QList<int> pages=pageSet.values();
        std::sort(pages.begin(),pages.end());
        QStringList pageNumbers;
        for(int page:pages)
            pageNumbers.append(QString::number(page));
        answer << QStringLiteral("Pages constructeur associées : %1.")
            .arg(pageNumbers.join(QStringLiteral(", ")));
    }
    return answer.join(QLatin1Char('\n'));
}

QString MemsDocumentaryRepository::answerFromHits(const QVariantList &hits)
{
    if (hits.isEmpty())
        return QString();
    QStringList paragraphs;
    const QVariantMap selected=hits.first().toMap();
    if(selected.value(QStringLiteral("iaMemsMentionOnly")).toBool()){
        const QString document=selected.value(QStringLiteral("document_id")).toString();
        const int page=selected.value(QStringLiteral("source_page")).toInt();
        const QString subject=selected.value(QStringLiteral("iaMemsMentionSubject")).toString();
        QStringList answer{
            QStringLiteral("Mention de « %1 » dans la documentation :").arg(subject),
            selected.value(QStringLiteral("title")).toString()
        };
        answer<<readableReferenceLayout(
            selected.value(QStringLiteral("iaMemsMentionExcerpt")).toString());
        if(!document.isEmpty())
            answer<<QStringLiteral("Source constructeur : %1%2").arg(document,
                page>0?QStringLiteral(", page %1").arg(page):QString());
        return answer.join(QLatin1Char('\n'));
    }
    const QString nativeKey=hits.first().toMap()
        .value(QStringLiteral("source_key")).toString();
    const bool nativeDocument=nativeKey.startsWith(QStringLiteral("rave-fr:"))
        || nativeKey.startsWith(QStringLiteral("rave-en:"));
    if (nativeDocument) {
        QStringList sections;
        QSet<QString> seen;
        const int maximum=qMin(3,hits.size());
        for(const QVariant &value : hits){
            if(sections.size()>=maximum)
                break;
            const QVariantMap item=value.toMap();
            const QString key=item.value(QStringLiteral("source_key")).toString();
            const QStringList sourceFields=key.split(QLatin1Char(':'));
            const QString document=sourceFields.size()>=3 ?
                sourceFields.at(1) : QStringLiteral("Rover");
            const int page=item.value(QStringLiteral("source_page")).toInt();
            const QString title=item.value(QStringLiteral("title")).toString().simplified();
            const QString source=readableWorkshopExcerpt(workshopText(item));
            const bool technicalReference=
                item.value(QStringLiteral("entry_type")).toString()
                ==QStringLiteral("reference");
            QString body=technicalReference
                ?readableReferenceLayout(source)
                :readableWorkshopParagraphs(source);
            if(body.isEmpty() && title.isEmpty())
                continue;
            const QString signature=QStringLiteral("%1|%2|%3|%4")
                .arg(document,title.toCaseFolded()).arg(page)
                .arg(body.left(250).toCaseFolded());
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
                "Aucun texte disponible pour cette rubrique.");
        }
        return sections.join(QStringLiteral("\n\n"));
    }
    const int count = qMin(3, hits.size());
    for (int i = 0; i < count; ++i) {
        const QVariantMap item = hits.at(i).toMap();
        QString content = item.value(QStringLiteral("content")).toString();
        paragraphs << QStringLiteral("%1\n%2")
            .arg(item.value(QStringLiteral("title")).toString(), content);
    }
    return paragraphs.join(QStringLiteral("\n\n"));
}
