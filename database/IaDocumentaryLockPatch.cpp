#include "../expert/IaMemsService.h"
#include "../expert/IaMemsConversationRouting.h"
#include "../expert/IaResponseLogic.h"
#include "../i18n.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QHash>
#include <QLineEdit>
#include <QMetaObject>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStringList>
#include <QTextBrowser>
#include <QTimer>
#include <QUuid>
#include <QVariant>
#include <QVector>

#include <algorithm>

namespace {

struct DocumentaryChoice
{
    QString searchKey;
    QString documentKey;
    QString revisionKey;
    QString sourceLanguage;
    QString entityKind;
    QString entityKey;
    QString label;
    QString operationNumber;
    int pageNumber = -1;
    int score = 0;

    bool isValid() const
    {
        return !entityKind.trimmed().isEmpty() && !entityKey.trimmed().isEmpty();
    }

    QString identity() const
    {
        return entityKind + QLatin1Char('|') + entityKey;
    }
};

QString normalizedDocumentary(QString input)
{
    input = input.normalized(QString::NormalizationForm_D).toCaseFolded();
    QString output;
    output.reserve(input.size());
    bool previousSpace = true;
    for (const QChar ch : input) {
        const QChar::Category category = ch.category();
        if (category == QChar::Mark_NonSpacing
            || category == QChar::Mark_SpacingCombining
            || category == QChar::Mark_Enclosing) {
            continue;
        }
        if (ch.isLetterOrNumber() || ch == QLatin1Char('.')) {
            output += ch;
            previousSpace = false;
        } else if (!previousSpace) {
            output += QLatin1Char(' ');
            previousSpace = true;
        }
    }
    return output.simplified();
}

bool exactToken(const QString &normalizedText, const QString &token)
{
    if (normalizedText.isEmpty() || token.isEmpty())
        return false;
    return (QStringLiteral(" ") + normalizedText + QStringLiteral(" "))
        .contains(QStringLiteral(" ") + token + QStringLiteral(" "));
}

QStringList meaningfulDocumentaryTerms(const QString &question)
{
    static const QSet<QString> stop = {
        QStringLiteral("que"), QStringLiteral("quoi"), QStringLiteral("quel"), QStringLiteral("quelle"),
        QStringLiteral("quels"), QStringLiteral("quelles"), QStringLiteral("le"), QStringLiteral("la"),
        QStringLiteral("les"), QStringLiteral("un"), QStringLiteral("une"), QStringLiteral("de"),
        QStringLiteral("du"), QStringLiteral("des"), QStringLiteral("sur"), QStringLiteral("dans"),
        QStringLiteral("pour"), QStringLiteral("avec"), QStringLiteral("est"), QStringLiteral("sont"),
        QStringLiteral("sais"), QStringLiteral("sait"), QStringLiteral("peux"), QStringLiteral("peut"),
        QStringLiteral("dire"), QStringLiteral("donne"), QStringLiteral("donner"), QStringLiteral("cherche"),
        QStringLiteral("recherche"), QStringLiteral("trouve"), QStringLiteral("trouver"), QStringLiteral("moi"),
        QStringLiteral("comment"), QStringLiteral("faire"), QStringLiteral("fait"), QStringLiteral("cette"),
        QStringLiteral("cet"), QStringLiteral("ces"), QStringLiteral("documentation"), QStringLiteral("document"),
        QStringLiteral("procedure"), QStringLiteral("technique")
    };

    QStringList terms;
    for (const QString &word : normalizedDocumentary(question).split(QLatin1Char(' '), Qt::SkipEmptyParts)) {
        if (word.size() < 3 || stop.contains(word))
            continue;
        if (!terms.contains(word))
            terms.append(word);
    }
    return terms;
}

int editDistanceBounded(const QString &a, const QString &b, int limit)
{
    if (qAbs(a.size() - b.size()) > limit)
        return limit + 1;
    QVector<int> previous(b.size() + 1);
    QVector<int> current(b.size() + 1);
    for (int j = 0; j <= b.size(); ++j)
        previous[j] = j;
    for (int i = 1; i <= a.size(); ++i) {
        current[0] = i;
        int rowBest = current[0];
        for (int j = 1; j <= b.size(); ++j) {
            const int cost = a.at(i - 1) == b.at(j - 1) ? 0 : 1;
            current[j] = qMin(qMin(previous[j] + 1, current[j - 1] + 1), previous[j - 1] + cost);
            rowBest = qMin(rowBest, current[j]);
        }
        if (rowBest > limit)
            return limit + 1;
        previous.swap(current);
    }
    return previous[b.size()];
}

bool fuzzyTitleMatch(const QString &title, const QString &term)
{
    if (term.size() < 5)
        return false;
    const int limit = term.size() >= 9 ? 2 : 1;
    for (const QString &word : title.split(QLatin1Char(' '), Qt::SkipEmptyParts)) {
        if (word.size() < 4)
            continue;
        if (editDistanceBounded(word, term, limit) <= limit)
            return true;
    }
    return false;
}

QString cleanDocumentaryLabel(QString label)
{
    label = label.simplified();
    label.replace(QLatin1Char('_'), QLatin1Char(' '));
    if (label.size() > 120)
        label = label.left(117) + QStringLiteral("...");
    if (!label.isEmpty())
        label[0] = label.at(0).toUpper();
    return label;
}

QString preferredLanguage()
{
    const QString active = I18n::language().trimmed().toLower();
    return active.isEmpty() ? QStringLiteral("fr") : active;
}

class DocumentaryPackReader
{
public:
    DocumentaryPackReader()
    {
        m_connectionName = QStringLiteral("ia_documentary_%1")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
        m_database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
        const QString path = QDir(QCoreApplication::applicationDirPath())
                                 .filePath(QStringLiteral("MEMSLibrary_Pack_001/knowledge.sqlite"));
        if (!QFileInfo::exists(path) || !QFileInfo(path).isFile()) {
            m_error = QStringLiteral("Pack001 absent");
            return;
        }
        m_database.setDatabaseName(path);
        m_database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
        if (!m_database.open()) {
            m_error = m_database.lastError().text();
            return;
        }
        QSqlQuery check(m_database);
        if (!check.exec(QStringLiteral("SELECT COUNT(*) FROM memslibrary_search")) || !check.next()) {
            m_error = QStringLiteral("Index documentaire Pack001 indisponible");
            m_database.close();
            return;
        }
        m_ready = true;
    }

    ~DocumentaryPackReader()
    {
        if (m_database.isOpen())
            m_database.close();
        m_database = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_connectionName);
    }

    bool isReady() const { return m_ready; }
    QString error() const { return m_error; }

    QList<DocumentaryChoice> discover(const QString &question, int maximum = 10)
    {
        QList<DocumentaryChoice> ranked;
        if (!m_ready)
            return ranked;
        const QStringList terms = meaningfulDocumentaryTerms(question);
        if (terms.isEmpty())
            return ranked;

        const QString normalizedQuestion = normalizedDocumentary(question);
        const QString language = preferredLanguage();
        QSqlQuery query(m_database);
        if (!query.exec(QStringLiteral(
                "SELECT search_key,document_key,revision_key,page_number,entity_kind,entity_key,"
                "source_language,title,body FROM memslibrary_search "
                "WHERE entity_kind IN ('section','operation')"))) {
            return ranked;
        }

        QHash<QString, DocumentaryChoice> bestByLabel;
        while (query.next()) {
            DocumentaryChoice choice;
            choice.searchKey = query.value(0).toString();
            choice.documentKey = query.value(1).toString();
            choice.revisionKey = query.value(2).toString();
            choice.pageNumber = query.value(3).isNull() ? -1 : query.value(3).toInt();
            choice.entityKind = query.value(4).toString();
            choice.entityKey = query.value(5).toString();
            choice.sourceLanguage = query.value(6).toString().toLower();
            choice.label = cleanDocumentaryLabel(query.value(7).toString());
            const QString body = query.value(8).toString();
            if (choice.label.isEmpty() || choice.entityKey.isEmpty())
                continue;

            const QString titleN = normalizedDocumentary(choice.label);
            const QString bodyN = normalizedDocumentary(body);
            int matches = 0;
            int score = 0;
            for (const QString &term : terms) {
                if (exactToken(titleN, term)) {
                    score += 95;
                    ++matches;
                } else if (exactToken(bodyN, term)) {
                    score += 48;
                    ++matches;
                } else if (titleN.contains(term)) {
                    score += 34;
                    ++matches;
                } else if (bodyN.contains(term)) {
                    score += 18;
                    ++matches;
                } else if (fuzzyTitleMatch(titleN, term)) {
                    score += 28;
                    ++matches;
                }
            }
            if (matches == 0)
                continue;
            if (terms.size() >= 2 && matches * 2 < terms.size())
                continue;

            const QString compactTerms = terms.join(QLatin1Char(' '));
            if (titleN == normalizedQuestion || titleN == compactTerms)
                score += 150;
            else if (!compactTerms.isEmpty() && titleN.contains(compactTerms))
                score += 55;

            if (choice.sourceLanguage == language)
                score += 45;
            else if (choice.sourceLanguage == QStringLiteral("fr"))
                score += 18;

            if (choice.entityKind == QStringLiteral("section"))
                score += terms.size() == 1 ? 65 : 12;
            else if (choice.entityKind == QStringLiteral("operation"))
                score += terms.size() >= 2 ? 45 : 25;

            choice.score = score;
            if (choice.entityKind == QStringLiteral("operation")) {
                QSqlQuery op(m_database);
                op.prepare(QStringLiteral(
                    "SELECT manufacturer_operation_no FROM ravemems_operation WHERE operation_key=?"));
                op.addBindValue(choice.entityKey);
                if (op.exec() && op.next())
                    choice.operationNumber = op.value(0).toString().trimmed();
            }

            QString key = normalizedDocumentary(choice.label);
            if (key.isEmpty())
                key = choice.identity();
            const auto existing = bestByLabel.constFind(key);
            if (existing == bestByLabel.constEnd() || choice.score > existing.value().score)
                bestByLabel.insert(key, choice);
        }

        ranked = bestByLabel.values();
        std::sort(ranked.begin(), ranked.end(), [](const DocumentaryChoice &a, const DocumentaryChoice &b) {
            if (a.score != b.score)
                return a.score > b.score;
            if (a.sourceLanguage != b.sourceLanguage)
                return a.sourceLanguage == preferredLanguage();
            if (a.entityKind != b.entityKind)
                return a.entityKind == QStringLiteral("section");
            return a.label < b.label;
        });
        if (ranked.size() > maximum)
            ranked = ranked.mid(0, maximum);
        return ranked;
    }

    QList<DocumentaryChoice> children(const DocumentaryChoice &parent)
    {
        QList<DocumentaryChoice> children;
        if (!m_ready || !parent.isValid())
            return children;

        if (parent.entityKind == QStringLiteral("section")) {
            QSqlQuery query(m_database);
            query.prepare(QStringLiteral(
                "SELECT s.search_key,s.document_key,s.revision_key,s.page_number,s.entity_kind,s.entity_key,"
                "COALESCE(o.source_language,s.source_language),o.title_source,o.manufacturer_operation_no "
                "FROM ravemems_operation o JOIN memslibrary_search s "
                "ON s.entity_kind='operation' AND s.entity_key=o.operation_key "
                "WHERE o.section_key=? ORDER BY o.sequence_no,o.operation_key"));
            query.addBindValue(parent.entityKey);
            if (!query.exec())
                return children;
            while (query.next()) {
                DocumentaryChoice child;
                child.searchKey = query.value(0).toString();
                child.documentKey = query.value(1).toString();
                child.revisionKey = query.value(2).toString();
                child.pageNumber = query.value(3).isNull() ? -1 : query.value(3).toInt();
                child.entityKind = query.value(4).toString();
                child.entityKey = query.value(5).toString();
                child.sourceLanguage = query.value(6).toString();
                child.label = cleanDocumentaryLabel(query.value(7).toString());
                child.operationNumber = query.value(8).toString().trimmed();
                if (child.isValid() && !child.label.isEmpty())
                    children.append(child);
            }
            return children;
        }

        if (parent.entityKind == QStringLiteral("document")) {
            QSqlQuery query(m_database);
            query.prepare(QStringLiteral(
                "SELECT s.search_key,s.document_key,s.revision_key,s.page_number,s.entity_kind,s.entity_key,"
                "s.source_language,s.title FROM ravemems_section sec JOIN memslibrary_search s "
                "ON s.entity_kind='section' AND s.entity_key=sec.section_key "
                "WHERE sec.revision_key=? ORDER BY sec.sequence_no,sec.section_key"));
            query.addBindValue(parent.revisionKey);
            if (!query.exec())
                return children;
            while (query.next()) {
                DocumentaryChoice child;
                child.searchKey = query.value(0).toString();
                child.documentKey = query.value(1).toString();
                child.revisionKey = query.value(2).toString();
                child.pageNumber = query.value(3).isNull() ? -1 : query.value(3).toInt();
                child.entityKind = query.value(4).toString();
                child.entityKey = query.value(5).toString();
                child.sourceLanguage = query.value(6).toString();
                child.label = cleanDocumentaryLabel(query.value(7).toString());
                if (child.isValid() && !child.label.isEmpty())
                    children.append(child);
            }
        }
        return children;
    }

    QString evidenceForOperation(const DocumentaryChoice &choice)
    {
        if (!m_ready || choice.entityKind != QStringLiteral("operation") || choice.entityKey.isEmpty())
            return QString();

        QSqlQuery operation(m_database);
        operation.prepare(QStringLiteral(
            "SELECT title_source,manufacturer_operation_no,operation_kind,source_language,section_key "
            "FROM ravemems_operation WHERE operation_key=?"));
        operation.addBindValue(choice.entityKey);
        if (!operation.exec() || !operation.next())
            return QString();

        QStringList output;
        const QString title = operation.value(0).toString().trimmed();
        const QString opNo = operation.value(1).toString().trimmed();
        const QString opKind = operation.value(2).toString().trimmed();
        output << QStringLiteral("Sujet documentaire : %1").arg(title.isEmpty() ? choice.label : title);
        if (!opNo.isEmpty())
            output << QStringLiteral("Opération constructeur : %1").arg(opNo);
        if (!opKind.isEmpty())
            output << QStringLiteral("Type d'opération : %1").arg(opKind);

        QSqlQuery pages(m_database);
        pages.prepare(QStringLiteral(
            "SELECT MIN(p.physical_page),MAX(p.physical_page) FROM ravemems_provenance pr "
            "JOIN ravemems_page p ON p.page_key=pr.page_key WHERE "
            "(pr.entity_kind='operation' AND pr.entity_key=?) OR "
            "(pr.entity_kind='phase' AND pr.entity_key IN (SELECT phase_key FROM ravemems_phase WHERE operation_key=?)) OR "
            "(pr.entity_kind='step' AND pr.entity_key IN (SELECT st.step_key FROM ravemems_step st JOIN ravemems_phase ph ON ph.phase_key=st.phase_key WHERE ph.operation_key=?)) OR "
            "(pr.entity_kind='notice' AND pr.entity_key IN (SELECT notice_key FROM ravemems_notice WHERE operation_key=?)) OR "
            "(pr.entity_kind='requirement' AND pr.entity_key IN (SELECT requirement_key FROM ravemems_requirement WHERE operation_key=?))"));
        for (int i = 0; i < 5; ++i)
            pages.addBindValue(choice.entityKey);
        if (pages.exec() && pages.next() && !pages.value(0).isNull()) {
            const int first = pages.value(0).toInt();
            const int last = pages.value(1).toInt();
            output << (first == last
                       ? QStringLiteral("Étendue documentaire : page %1").arg(first)
                       : QStringLiteral("Étendue documentaire : pages %1 à %2").arg(first).arg(last));
        }

        QSqlQuery phases(m_database);
        phases.prepare(QStringLiteral(
            "SELECT phase_key,sequence_no,phase_kind_source,normalized_phase_kind,title_source "
            "FROM ravemems_phase WHERE operation_key=? ORDER BY sequence_no,phase_key"));
        phases.addBindValue(choice.entityKey);
        if (phases.exec()) {
            while (phases.next()) {
                const QString phaseKey = phases.value(0).toString();
                const QString phaseKind = phases.value(2).toString().trimmed();
                const QString normalizedKind = phases.value(3).toString().trimmed();
                const QString phaseTitle = phases.value(4).toString().trimmed();
                QString heading = phaseTitle;
                if (heading.isEmpty())
                    heading = !phaseKind.isEmpty() ? phaseKind : normalizedKind;
                if (!heading.isEmpty())
                    output << QStringLiteral("\n%1").arg(heading);

                QSqlQuery steps(m_database);
                steps.prepare(QStringLiteral(
                    "SELECT sequence_no,manufacturer_step_no,instruction_source,condition_text,source_page_start,source_page_end "
                    "FROM ravemems_step WHERE phase_key=? ORDER BY sequence_no,step_key"));
                steps.addBindValue(phaseKey);
                if (steps.exec()) {
                    while (steps.next()) {
                        const QString manufacturerStep = steps.value(1).toString().trimmed();
                        const QString instruction = steps.value(2).toString().trimmed();
                        const QString condition = steps.value(3).toString().trimmed();
                        if (instruction.isEmpty())
                            continue;
                        QString line = manufacturerStep.isEmpty()
                            ? QStringLiteral("• %1").arg(instruction)
                            : QStringLiteral("• Étape %1 : %2").arg(manufacturerStep, instruction);
                        if (!condition.isEmpty())
                            line += QStringLiteral(" — condition : %1").arg(condition);
                        output << line;
                    }
                }
            }
        }

        QSqlQuery notices(m_database);
        notices.prepare(QStringLiteral(
            "SELECT notice_kind,source_text,scope_kind FROM ravemems_notice "
            "WHERE operation_key=? ORDER BY sequence_no,notice_key"));
        notices.addBindValue(choice.entityKey);
        if (notices.exec()) {
            QStringList rows;
            while (notices.next()) {
                const QString text = notices.value(1).toString().trimmed();
                if (text.isEmpty())
                    continue;
                const QString kind = notices.value(0).toString().trimmed();
                rows << (kind.isEmpty() ? QStringLiteral("• %1").arg(text)
                                        : QStringLiteral("• %1 : %2").arg(kind, text));
            }
            if (!rows.isEmpty()) {
                output << QStringLiteral("\nAvertissements / remarques :");
                output.append(rows);
            }
        }

        QSqlQuery requirements(m_database);
        requirements.prepare(QStringLiteral(
            "SELECT requirement_type,requirement_source,part_number,quantity,unit,before_start,figure_ref "
            "FROM ravemems_requirement WHERE operation_key=? ORDER BY sequence_no,requirement_key"));
        requirements.addBindValue(choice.entityKey);
        if (requirements.exec()) {
            QStringList rows;
            while (requirements.next()) {
                const QString type = requirements.value(0).toString().trimmed();
                const QString text = requirements.value(1).toString().trimmed();
                const QString part = requirements.value(2).toString().trimmed();
                const QString quantity = requirements.value(3).toString().trimmed();
                const QString unit = requirements.value(4).toString().trimmed();
                if (text.isEmpty() && part.isEmpty())
                    continue;
                QStringList fields;
                if (!type.isEmpty()) fields << type;
                if (!text.isEmpty()) fields << text;
                if (!part.isEmpty()) fields << QStringLiteral("réf. %1").arg(part);
                if (!quantity.isEmpty()) fields << QStringLiteral("%1 %2").arg(quantity, unit).trimmed();
                rows << QStringLiteral("• %1").arg(fields.join(QStringLiteral(" — ")));
            }
            if (!rows.isEmpty()) {
                output << QStringLiteral("\nExigences / spécifications :");
                output.append(rows);
            }
        }

        return output.join(QLatin1Char('\n')).trimmed();
    }

private:
    QString m_connectionName;
    QSqlDatabase m_database;
    QString m_error;
    bool m_ready = false;
};

QString menuFingerprint(const QList<DocumentaryChoice> &choices)
{
    QStringList ids;
    for (const DocumentaryChoice &choice : choices)
        ids << choice.identity();
    return ids.join(QStringLiteral("||"));
}

QString displayChoiceLabel(const DocumentaryChoice &choice)
{
    QString label = cleanDocumentaryLabel(choice.label);
    if (choice.entityKind == QStringLiteral("operation")
        && !choice.operationNumber.isEmpty()
        && !normalizedDocumentary(label).contains(normalizedDocumentary(choice.operationNumber))) {
        label += QStringLiteral(" — opération %1").arg(choice.operationNumber);
    }
    return label;
}

void appendDocumentaryTranscript(QTextBrowser *browser, const QString &speaker, const QString &text)
{
    if (!browser)
        return;
    QString safe = text.toHtmlEscaped();
    safe.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
    browser->append(QStringLiteral("<p><b>%1</b><br>%2</p>")
                    .arg(speaker.toHtmlEscaped(), safe));
}

class DocumentaryPipelineController final : public QObject
{
public:
    explicit DocumentaryPipelineController(QWidget *tab)
        : QObject(tab), m_tab(tab)
    {
        if (!m_tab)
            return;
        m_question = m_tab->findChild<QLineEdit*>(QStringLiteral("iaMemsQuestion"));
        m_send = m_tab->findChild<QPushButton*>(QStringLiteral("iaMemsSend"));
        m_transcript = m_tab->findChild<QTextBrowser*>(QStringLiteral("iaMemsTranscript"));
        if (!m_question || !m_send)
            return;

        QObject::disconnect(m_send, nullptr, nullptr, nullptr);
        QObject::disconnect(m_question, nullptr, nullptr, nullptr);
        QObject::connect(m_send, &QPushButton::clicked, this, [this]() { handleSend(); });
        QObject::connect(m_question, &QLineEdit::returnPressed, this, [this]() { handleSend(); });
        m_tab->setProperty("iaDocumentaryClarificationTest", true);
        m_tab->setProperty("iaDocumentaryIdentityPipelineV2", true);
    }

private:
    void invokeOriginalSend(const QString &question)
    {
        clearPending();
        if (!m_tab || !m_question)
            return;
        m_question->setText(question);
        QMetaObject::invokeMethod(m_tab.data(), "sendQuestion", Qt::DirectConnection);
    }

    void clearPending()
    {
        m_pendingChoices.clear();
        m_pendingSubject.clear();
        m_originalQuestion.clear();
        m_lastMenuFingerprint.clear();
    }

    void showChoices(const QString &subject,
                     const QList<DocumentaryChoice> &choices,
                     const QString &originalQuestion)
    {
        if (choices.isEmpty())
            return;
        const QString fingerprint = menuFingerprint(choices);
        if (!m_lastMenuFingerprint.isEmpty() && fingerprint == m_lastMenuFingerprint) {
            for (const DocumentaryChoice &choice : choices) {
                if (choice.entityKind == QStringLiteral("operation")) {
                    sendResolved(choice, originalQuestion);
                    return;
                }
            }
            invokeOriginalSend(originalQuestion);
            return;
        }

        m_lastMenuFingerprint = fingerprint;
        m_pendingChoices = choices;
        m_pendingSubject = subject;
        m_originalQuestion = originalQuestion;
        m_question->clear();

        QString prompt = QStringLiteral("J'ai trouvé plusieurs rubriques correspondant à « %1 ». Laquelle recherchez-vous ?")
                             .arg(subject);
        for (int i = 0; i < choices.size(); ++i)
            prompt += QStringLiteral("\n%1. %2").arg(i + 1).arg(displayChoiceLabel(choices.at(i)));
        prompt += QStringLiteral("\nRépondez par le numéro, ou écrivez un autre sujet pour relancer une recherche.");
        appendDocumentaryTranscript(m_transcript, QStringLiteral("IA MEMS"), prompt);
        m_question->setFocus();
    }

    void sendResolved(const DocumentaryChoice &choice, const QString &originalQuestion)
    {
        DocumentaryPackReader reader;
        if (!reader.isReady()) {
            invokeOriginalSend(originalQuestion);
            return;
        }

        if (choice.entityKind == QStringLiteral("section") || choice.entityKind == QStringLiteral("document")) {
            const QList<DocumentaryChoice> children = reader.children(choice);
            if (children.size() == 1) {
                sendResolved(children.first(), originalQuestion);
                return;
            }
            if (children.size() >= 2) {
                showChoices(displayChoiceLabel(choice), children, originalQuestion);
                return;
            }
            invokeOriginalSend(originalQuestion);
            return;
        }

        if (choice.entityKind != QStringLiteral("operation")) {
            invokeOriginalSend(originalQuestion);
            return;
        }

        const QString evidence = reader.evidenceForOperation(choice);
        if (evidence.trimmed().isEmpty()) {
            appendDocumentaryTranscript(m_transcript, QStringLiteral("IA MEMS"),
                QStringLiteral("La rubrique sélectionnée existe, mais son contenu structuré n'a pas pu être lu. Je ne vais pas remplacer cette sélection par une autre recherche approximative."));
            clearPending();
            return;
        }

        IaMemsService *service = IaMemsService::instance();
        if (!service)
            return;
        clearPending();
        m_question->clear();
        m_send->setEnabled(false);
        m_question->setEnabled(false);
        service->setProperty("iaMemsResolvedEvidence", evidence);
        service->setProperty("iaMemsResolvedSubject", displayChoiceLabel(choice));
        service->setProperty("iaMemsResolvedEntityKind", choice.entityKind);
        service->setProperty("iaMemsResolvedEntityKey", choice.entityKey);
        service->askWithLibrary(QStringLiteral("[[MEMS_RESOLVED]]%1").arg(originalQuestion.trimmed()));
    }

    void resolveInitial(const QString &raw)
    {
        DocumentaryPackReader reader;
        if (!reader.isReady()) {
            invokeOriginalSend(raw);
            return;
        }

        QList<DocumentaryChoice> choices = reader.discover(raw, 10);
        if (choices.isEmpty()) {
            invokeOriginalSend(raw);
            return;
        }

        appendDocumentaryTranscript(m_transcript, QStringLiteral("Vous"), raw);

        const QStringList terms = meaningfulDocumentaryTerms(raw);
        if (terms.size() == 1 && choices.first().entityKind == QStringLiteral("section")) {
            const QList<DocumentaryChoice> children = reader.children(choices.first());
            if (children.size() == 1) {
                sendResolved(children.first(), raw);
                return;
            }
            if (children.size() >= 2) {
                showChoices(displayChoiceLabel(choices.first()), children, raw);
                return;
            }
        }

        if (choices.first().entityKind == QStringLiteral("operation")) {
            const bool clearWinner = choices.size() == 1
                || choices.first().score >= choices.at(1).score + 70;
            if (clearWinner) {
                sendResolved(choices.first(), raw);
                return;
            }
        }

        showChoices(raw, choices, raw);
    }

    void handlePendingSelection(const QString &raw)
    {
        appendDocumentaryTranscript(m_transcript, QStringLiteral("Vous"), raw);
        bool ok = false;
        const int selected = raw.toInt(&ok);
        if (ok && selected >= 1 && selected <= m_pendingChoices.size()) {
            const DocumentaryChoice choice = m_pendingChoices.at(selected - 1);
            const QString original = m_originalQuestion;
            m_pendingChoices.clear();
            m_pendingSubject.clear();
            sendResolved(choice, original);
            return;
        }

        const QString original = m_originalQuestion;
        clearPending();
        DocumentaryPackReader reader;
        if (!reader.isReady()) {
            invokeOriginalSend(raw);
            return;
        }
        const QString refined = QStringLiteral("%1 %2").arg(original, raw).simplified();
        QList<DocumentaryChoice> choices = reader.discover(refined, 10);
        if (choices.isEmpty()) {
            invokeOriginalSend(refined);
            return;
        }
        if (choices.first().entityKind == QStringLiteral("operation")
            && (choices.size() == 1 || choices.first().score >= choices.at(1).score + 70)) {
            sendResolved(choices.first(), original);
            return;
        }
        showChoices(raw, choices, original);
    }

    void handleSend()
    {
        if (!m_question || !m_tab)
            return;
        const QString raw = m_question->text().trimmed();
        if (raw.isEmpty())
            return;

        if (!m_pendingChoices.isEmpty()) {
            handlePendingSelection(raw);
            return;
        }

        const IaResponseLogic::Intent intent = IaResponseLogic::classify(raw);
        const bool explicitDocumentation = IaMemsConversationRouting::isDocumentationQuestion(raw);
        if (intent != IaResponseLogic::Intent::None && !explicitDocumentation) {
            invokeOriginalSend(raw);
            return;
        }

        resolveInitial(raw);
    }

    QPointer<QWidget> m_tab;
    QPointer<QLineEdit> m_question;
    QPointer<QPushButton> m_send;
    QPointer<QTextBrowser> m_transcript;
    QList<DocumentaryChoice> m_pendingChoices;
    QString m_pendingSubject;
    QString m_originalQuestion;
    QString m_lastMenuFingerprint;
};

void patchDocumentaryPipeline(QWidget *tab)
{
    if (!tab || tab->property("iaDocumentaryIdentityPipelineV2").toBool())
        return;
    new DocumentaryPipelineController(tab);
}

class DocumentaryPipelineInstaller final : public QObject
{
public:
    explicit DocumentaryPipelineInstaller(QObject *parent = nullptr) : QObject(parent) {}

    bool eventFilter(QObject *watched, QEvent *event) override
    {
        QWidget *widget = qobject_cast<QWidget*>(watched);
        if (widget && event
            && (event->type() == QEvent::Show || event->type() == QEvent::Polish)
            && widget->objectName() == QStringLiteral("ia_mems_tab")) {
            QPointer<QWidget> guarded(widget);
            QTimer::singleShot(350, this, [guarded]() {
                if (guarded)
                    patchDocumentaryPipeline(guarded);
            });
        }
        return QObject::eventFilter(watched, event);
    }
};

void installDocumentaryIdentityPipeline()
{
    QCoreApplication *core = QCoreApplication::instance();
    if (!core)
        return;
    DocumentaryPipelineInstaller *installer = new DocumentaryPipelineInstaller(core);
    core->installEventFilter(installer);
    QTimer::singleShot(900, installer, [installer]() {
        Q_UNUSED(installer)
        for (QWidget *widget : QApplication::allWidgets()) {
            if (widget && widget->objectName() == QStringLiteral("ia_mems_tab"))
                patchDocumentaryPipeline(widget);
        }
    });
}

} // namespace

Q_COREAPP_STARTUP_FUNCTION(installDocumentaryIdentityPipeline)
