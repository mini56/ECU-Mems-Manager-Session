#include "IaMemsService.h"

#include "IaMemsConversationRouting.h"
#include "IaMemsLibraryBridge.h"
#include "IaResponseLogic.h"
#include "LocalAiClient.h"
#include "../database/MemsReferenceDatabase.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>
#include <QUuid>
#include <QVariant>

#include <algorithm>

namespace {

const QString kResolvedDocumentaryPrefix = QStringLiteral("[[MEMS_RESOLVED]]");

void appendUnique(QStringList &terms, const QString &term)
{
    const QString clean = term.trimmed().toLower();
    if (!clean.isEmpty() && !terms.contains(clean) && terms.size() < 12)
        terms.append(clean);
}

QStringList libraryKeywords(const QString &question)
{
    QString text = IaMemsConversationRouting::normalize(question);
    text.replace(QRegularExpression(QStringLiteral("[^a-z0-9_.-]+")), QStringLiteral(" "));

    const QSet<QString> stop = {
        QStringLiteral("que"), QStringLiteral("quoi"), QStringLiteral("quel"), QStringLiteral("quelle"),
        QStringLiteral("les"), QStringLiteral("des"), QStringLiteral("une"), QStringLiteral("dans"),
        QStringLiteral("pour"), QStringLiteral("avec"), QStringLiteral("est"), QStringLiteral("sais"),
        QStringLiteral("sait"), QStringLiteral("peux"), QStringLiteral("dire"), QStringLiteral("cette"),
        QStringLiteral("sur"), QStringLiteral("mems"), QStringLiteral("source"), QStringLiteral("sources"),
        QStringLiteral("document"), QStringLiteral("documentation"), QStringLiteral("procedure")
    };

    QStringList terms;
    for (const QString &word : text.split(QLatin1Char(' '), Qt::SkipEmptyParts)) {
        if (word.size() >= 3 && !stop.contains(word))
            appendUnique(terms, word);
        if (terms.size() >= 7)
            break;
    }

    const QString normalized = text.simplified();
    if (normalized.contains(QStringLiteral("batterie"))) appendUnique(terms, QStringLiteral("battery"));
    if (normalized.contains(QStringLiteral("restauration"))) appendUnique(terms, QStringLiteral("restoration"));
    if (normalized.contains(QStringLiteral("engrenage"))) appendUnique(terms, QStringLiteral("gear"));
    if (normalized.contains(QStringLiteral("primaire"))) appendUnique(terms, QStringLiteral("primary"));
    if (normalized.contains(QStringLiteral("jeu axial"))) {
        appendUnique(terms, QStringLiteral("end"));
        appendUnique(terms, QStringLiteral("float"));
    }
    if (normalized.contains(QStringLiteral("bobine"))) {
        appendUnique(terms, QStringLiteral("coil"));
        appendUnique(terms, QStringLiteral("dwell"));
    }
    if (normalized.contains(QStringLiteral("papillon"))) appendUnique(terms, QStringLiteral("throttle"));
    if (normalized.contains(QStringLiteral("ralenti"))) appendUnique(terms, QStringLiteral("idle"));
    if (normalized.contains(QStringLiteral("refroidissement"))) appendUnique(terms, QStringLiteral("coolant"));
    if (normalized.contains(QStringLiteral("admission"))) appendUnique(terms, QStringLiteral("intake"));
    if (normalized.contains(QStringLiteral("broche")) || normalized.contains(QStringLiteral("brochage"))) {
        appendUnique(terms, QStringLiteral("pin"));
        appendUnique(terms, QStringLiteral("wiring"));
    }
    if (normalized.contains(QStringLiteral("couleur")) && normalized.contains(QStringLiteral("fil"))) {
        appendUnique(terms, QStringLiteral("wire"));
        appendUnique(terms, QStringLiteral("colour"));
    }
    return terms;
}

bool genericInsufficientGrounding(const QString &text)
{
    const QString lower = text.toLower();
    return lower.contains(QStringLiteral("je n'ai pas trouvé de donnée documentaire"))
        || lower.contains(QStringLiteral("je n'ai pas assez d'éléments"))
        || lower.contains(QStringLiteral("je n'ai pas assez d elements"));
}

QString documentaryModelQuestion(const QString &question,
                                  const QString &subject)
{
    return QStringLiteral(
        "[[MEMS_GROUNDED]]Analyse documentaire technique.\n"
        "Question de l'utilisateur : %1\n"
        "Rubrique sélectionnée : %2\n\n"
        "Réponds directement en français à la question à partir des faits structurés fournis séparément par MEMS Manager. "
        "Organise la réponse dans l'ordre utile. N'invente rien et n'ajoute aucune information extérieure. "
        "Si plusieurs étapes sont fournies, conserve leur ordre et les avertissements nécessaires. "
        "N'affiche pas les identifiants internes, la provenance, les clés techniques ni le niveau de preuve, "
        "sauf demande explicite de l'utilisateur.")
        .arg(question.trimmed(), subject.trimmed().isEmpty() ? question.trimmed() : subject.trimmed());
}

QString normalizedReference(const QString &text)
{
    return IaMemsConversationRouting::normalize(text);
}

bool looseTermMatch(const QString &normalizedText, const QString &term)
{
    if (normalizedText.isEmpty() || term.isEmpty())
        return false;
    const QStringList words = normalizedText.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (const QString &word : words) {
        if (word == term)
            return true;
        if (term.size() >= 4 && word.size() >= 4
            && (word.startsWith(term) || term.startsWith(word)))
            return true;
    }
    return normalizedText.contains(term);
}

int pageFromSection(const QString &section)
{
    static const QRegularExpression rx(
        QStringLiteral("(?:PDF\\s*)?p\\.\\s*(\\d+)"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = rx.match(section);
    return match.hasMatch() ? match.captured(1).toInt() : -1;
}

QString cleanReferenceLabel(QString label)
{
    label = label.simplified();
    static const QRegularExpression opSuffix(
        QStringLiteral("\\s*[-—]\\s*op[ée]ration\\s+[0-9][0-9.]*\\s*$"),
        QRegularExpression::CaseInsensitiveOption);
    label.remove(opSuffix);
    label = label.simplified();
    if (!label.isEmpty() && label.size() > 1 && label == label.toUpper()) {
        QString lower = label.toLower();
        lower[0] = lower.at(0).toUpper();
        label = lower;
    }
    return label;
}

struct ReferenceChoice
{
    QString knowledgeKey;
    QString label;
    QString document;
    QString sourceSection;
    QString sourceText;
    int page = -1;
    int score = 0;

    bool isValid() const
    {
        return !knowledgeKey.trimmed().isEmpty() && !label.trimmed().isEmpty();
    }
};

class StructuredReferenceReader
{
public:
    StructuredReferenceReader()
    {
        MemsReferenceDatabase bootstrap;
        if (!bootstrap.open()) {
            m_error = QStringLiteral("Base documentaire de référence indisponible");
            return;
        }
        const QString databasePath = bootstrap.databasePath();
        if (databasePath.isEmpty() || !QFileInfo::exists(databasePath)) {
            m_error = QStringLiteral("Cache documentaire de référence absent");
            return;
        }

        m_connectionName = QStringLiteral("IA_STRUCTURED_REFERENCE_%1")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
        m_database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
        m_database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
        m_database.setDatabaseName(databasePath);
        if (!m_database.open()) {
            m_error = QStringLiteral("Ouverture de la base documentaire impossible");
            return;
        }

        QSqlQuery check(m_database);
        if (!check.exec(QStringLiteral(
                "SELECT 1 FROM sqlite_master WHERE type='table' AND name='mems_knowledge_item'"))
            || !check.next()) {
            m_error = QStringLiteral("Tables documentaires structurées absentes");
            m_database.close();
            return;
        }
        m_ready = true;
    }

    ~StructuredReferenceReader()
    {
        if (m_database.isOpen())
            m_database.close();
        m_database = QSqlDatabase();
        if (!m_connectionName.isEmpty())
            QSqlDatabase::removeDatabase(m_connectionName);
    }

    bool isReady() const { return m_ready; }

    QList<ReferenceChoice> choicesForQuestion(const QString &question, int maximum = 24)
    {
        if (!m_ready)
            return QList<ReferenceChoice>();

        const QStringList terms = libraryKeywords(question);
        if (terms.isEmpty())
            return QList<ReferenceChoice>();

        const ReferenceChoice toc = bestTableOfContents(terms);
        if (toc.isValid()) {
            const QList<ReferenceChoice> children = sectionChildren(toc, maximum);
            if (!children.isEmpty())
                return children;
        }

        return directStructuredMatches(terms, maximum);
    }

    ReferenceChoice choiceByKey(const QString &knowledgeKey)
    {
        ReferenceChoice choice;
        if (!m_ready || knowledgeKey.trimmed().isEmpty())
            return choice;

        QSqlQuery query(m_database);
        query.prepare(QStringLiteral(
            "SELECT knowledge_key,topic,document,source_section,source_text "
            "FROM mems_knowledge_item WHERE knowledge_key=?"));
        query.addBindValue(knowledgeKey);
        if (!query.exec() || !query.next())
            return choice;
        return choiceFromQuery(query);
    }

    QString evidenceForChoice(const ReferenceChoice &choice)
    {
        if (!m_ready || !choice.isValid())
            return QString();

        QStringList output;
        output << QStringLiteral("Sujet : %1").arg(choice.label);

        bool haveStructured = false;

        QSqlQuery procedures(m_database);
        procedures.prepare(QStringLiteral(
            "SELECT procedure_key,operation,title_source "
            "FROM mems_procedure WHERE knowledge_key=? ORDER BY rowid"));
        procedures.addBindValue(choice.knowledgeKey);
        if (procedures.exec()) {
            while (procedures.next()) {
                haveStructured = true;
                const QString procedureKey = procedures.value(0).toString();
                const QString operation = procedures.value(1).toString().trimmed();
                const QString title = procedures.value(2).toString().trimmed();
                QString heading = cleanReferenceLabel(title);
                if (heading.isEmpty())
                    heading = operation;
                if (!heading.isEmpty())
                    output << QStringLiteral("\n%1").arg(heading);

                QSqlQuery steps(m_database);
                steps.prepare(QStringLiteral(
                    "SELECT step_no,instruction_source,condition_text "
                    "FROM mems_procedure_step WHERE procedure_key=? ORDER BY step_no"));
                steps.addBindValue(procedureKey);
                if (steps.exec()) {
                    while (steps.next()) {
                        const int stepNo = steps.value(0).toInt();
                        const QString instruction = steps.value(1).toString().trimmed();
                        const QString condition = steps.value(2).toString().trimmed();
                        if (instruction.isEmpty())
                            continue;
                        QString line = QStringLiteral("%1. %2").arg(stepNo).arg(instruction);
                        if (!condition.isEmpty())
                            line += QStringLiteral(" — condition : %1").arg(condition);
                        output << line;
                    }
                }

                QSqlQuery requirements(m_database);
                requirements.prepare(QStringLiteral(
                    "SELECT step_no,requirement_type,requirement_source,part_number,quantity,unit "
                    "FROM mems_procedure_requirement WHERE procedure_key=? "
                    "ORDER BY CASE WHEN step_no IS NULL THEN 999999 ELSE step_no END,requirement_no"));
                requirements.addBindValue(procedureKey);
                QStringList requirementLines;
                if (requirements.exec()) {
                    while (requirements.next()) {
                        const QString type = requirements.value(1).toString().trimmed();
                        const QString text = requirements.value(2).toString().trimmed();
                        const QString part = requirements.value(3).toString().trimmed();
                        const QString quantity = requirements.value(4).toString().trimmed();
                        const QString unit = requirements.value(5).toString().trimmed();
                        QStringList fields;
                        if (!type.isEmpty()) fields << type;
                        if (!text.isEmpty()) fields << text;
                        if (!part.isEmpty()) fields << QStringLiteral("réf. %1").arg(part);
                        if (!quantity.isEmpty()) fields << QStringLiteral("%1 %2").arg(quantity, unit).trimmed();
                        if (!fields.isEmpty())
                            requirementLines << QStringLiteral("• %1").arg(fields.join(QStringLiteral(" — ")));
                    }
                }
                if (!requirementLines.isEmpty()) {
                    output << QStringLiteral("Avertissements / exigences :");
                    output.append(requirementLines);
                }
            }
        }

        QSqlQuery specifications(m_database);
        specifications.prepare(QStringLiteral(
            "SELECT spec_key,operation,parameter,default_unit,condition_text "
            "FROM mems_specification WHERE knowledge_key=? ORDER BY rowid"));
        specifications.addBindValue(choice.knowledgeKey);
        if (specifications.exec()) {
            while (specifications.next()) {
                haveStructured = true;
                const QString specKey = specifications.value(0).toString();
                const QString operation = specifications.value(1).toString().trimmed();
                const QString parameter = specifications.value(2).toString().trimmed();
                const QString defaultUnit = specifications.value(3).toString().trimmed();
                const QString condition = specifications.value(4).toString().trimmed();

                QString heading = parameter;
                if (!operation.isEmpty())
                    heading = heading.isEmpty() ? operation : QStringLiteral("%1 — %2").arg(operation, heading);
                if (!heading.isEmpty())
                    output << QStringLiteral("\nSpécification : %1").arg(heading);
                if (!condition.isEmpty())
                    output << QStringLiteral("Condition : %1").arg(condition);

                QSqlQuery values(m_database);
                values.prepare(QStringLiteral(
                    "SELECT sequence_no,value_numeric,value_min,value_max,value_text,unit,instruction_text "
                    "FROM mems_specification_value WHERE spec_key=? ORDER BY sequence_no"));
                values.addBindValue(specKey);
                if (values.exec()) {
                    while (values.next()) {
                        const int sequence = values.value(0).toInt();
                        const QVariant numeric = values.value(1);
                        const QVariant minValue = values.value(2);
                        const QVariant maxValue = values.value(3);
                        const QString valueText = values.value(4).toString().trimmed();
                        QString unit = values.value(5).toString().trimmed();
                        if (unit.isEmpty())
                            unit = defaultUnit;
                        const QString instruction = values.value(6).toString().trimmed();

                        QString value;
                        if (!valueText.isEmpty()) {
                            value = valueText;
                        } else if (!minValue.isNull() && !maxValue.isNull()) {
                            value = QStringLiteral("%1 à %2 %3")
                                .arg(minValue.toString(), maxValue.toString(), unit).trimmed();
                        } else if (!numeric.isNull()) {
                            value = QStringLiteral("%1 %2").arg(numeric.toString(), unit).trimmed();
                        }

                        QString line = QStringLiteral("• Séquence %1").arg(sequence);
                        if (!value.isEmpty())
                            line += QStringLiteral(" : %1").arg(value);
                        if (!instruction.isEmpty())
                            line += QStringLiteral(" — %1").arg(instruction);
                        output << line;
                    }
                }
            }
        }

        if (!haveStructured && !choice.sourceText.trimmed().isEmpty())
            output << choice.sourceText.trimmed();

        return output.join(QLatin1Char('\n')).trimmed();
    }

private:
    ReferenceChoice choiceFromQuery(const QSqlQuery &query) const
    {
        ReferenceChoice choice;
        choice.knowledgeKey = query.value(0).toString();
        choice.label = cleanReferenceLabel(query.value(1).toString());
        choice.document = query.value(2).toString();
        choice.sourceSection = query.value(3).toString();
        choice.sourceText = query.value(4).toString();
        choice.page = pageFromSection(choice.sourceSection);
        return choice;
    }

    ReferenceChoice bestTableOfContents(const QStringList &terms)
    {
        ReferenceChoice best;
        QSqlQuery query(m_database);
        if (!query.exec(QStringLiteral(
                "SELECT knowledge_key,topic,document,source_section,source_text "
                "FROM mems_knowledge_item")))
            return best;

        while (query.next()) {
            ReferenceChoice choice = choiceFromQuery(query);
            const QString topic = normalizedReference(choice.label);
            if (!topic.contains(QStringLiteral("table des matieres"))
                && !topic.contains(QStringLiteral("table of contents")))
                continue;

            int score = 0;
            bool all = true;
            for (const QString &term : terms) {
                if (!looseTermMatch(topic, term)) {
                    all = false;
                    break;
                }
                score += 100;
            }
            if (!all)
                continue;
            if (choice.page >= 0)
                score += 5;
            choice.score = score;
            if (!best.isValid() || choice.score > best.score)
                best = choice;
        }
        return best;
    }

    QList<ReferenceChoice> sectionChildren(const ReferenceChoice &toc, int maximum)
    {
        QList<ReferenceChoice> result;
        if (!toc.isValid() || toc.document.isEmpty() || toc.page < 0)
            return result;

        int nextTocPage = 1000000;
        QSqlQuery boundaries(m_database);
        boundaries.prepare(QStringLiteral(
            "SELECT topic,source_section FROM mems_knowledge_item WHERE document=?"));
        boundaries.addBindValue(toc.document);
        if (boundaries.exec()) {
            while (boundaries.next()) {
                const QString topic = normalizedReference(boundaries.value(0).toString());
                if (!topic.contains(QStringLiteral("table des matieres"))
                    && !topic.contains(QStringLiteral("table of contents")))
                    continue;
                const int page = pageFromSection(boundaries.value(1).toString());
                if (page > toc.page && page < nextTocPage)
                    nextTocPage = page;
            }
        }

        QSqlQuery query(m_database);
        query.prepare(QStringLiteral(
            "SELECT k.knowledge_key,k.topic,k.document,k.source_section,k.source_text "
            "FROM mems_knowledge_item k "
            "WHERE k.document=? AND EXISTS "
            "(SELECT 1 FROM mems_procedure p WHERE p.knowledge_key=k.knowledge_key)"));
        query.addBindValue(toc.document);
        if (!query.exec())
            return result;

        QSet<QString> seen;
        while (query.next()) {
            ReferenceChoice choice = choiceFromQuery(query);
            if (choice.page <= toc.page || choice.page >= nextTocPage)
                continue;
            const QString signature = normalizedReference(choice.label);
            if (signature.isEmpty() || seen.contains(signature))
                continue;
            seen.insert(signature);
            result.append(choice);
        }

        std::sort(result.begin(), result.end(), [](const ReferenceChoice &a, const ReferenceChoice &b) {
            if (a.page != b.page)
                return a.page < b.page;
            return a.label < b.label;
        });
        if (result.size() > maximum)
            result = result.mid(0, maximum);
        return result;
    }

    QList<ReferenceChoice> directStructuredMatches(const QStringList &terms, int maximum)
    {
        QList<ReferenceChoice> result;
        QSqlQuery query(m_database);
        if (!query.exec(QStringLiteral(
            "SELECT DISTINCT k.knowledge_key,k.topic,k.document,k.source_section,k.source_text "
            "FROM mems_knowledge_item k WHERE "
            "EXISTS (SELECT 1 FROM mems_procedure p WHERE p.knowledge_key=k.knowledge_key) "
            "OR EXISTS (SELECT 1 FROM mems_specification s WHERE s.knowledge_key=k.knowledge_key)")))
            return result;

        while (query.next()) {
            ReferenceChoice choice = choiceFromQuery(query);
            const QString topic = normalizedReference(choice.label);
            const QString source = normalizedReference(choice.sourceText);
            int score = 0;
            bool all = true;
            for (const QString &term : terms) {
                if (looseTermMatch(topic, term)) {
                    score += 100;
                } else if (looseTermMatch(source, term)) {
                    score += 30;
                } else {
                    all = false;
                    break;
                }
            }
            if (!all)
                continue;
            if (terms.size() == 1 && looseTermMatch(topic, terms.first()))
                score += 40;
            choice.score = score;
            result.append(choice);
        }

        std::sort(result.begin(), result.end(), [](const ReferenceChoice &a, const ReferenceChoice &b) {
            if (a.score != b.score)
                return a.score > b.score;
            if (a.page != b.page)
                return a.page < b.page;
            return a.label < b.label;
        });

        QSet<QString> seen;
        QList<ReferenceChoice> unique;
        for (const ReferenceChoice &choice : result) {
            const QString signature = normalizedReference(choice.label);
            if (signature.isEmpty() || seen.contains(signature))
                continue;
            seen.insert(signature);
            unique.append(choice);
            if (unique.size() >= maximum)
                break;
        }
        return unique;
    }

private:
    QString m_connectionName;
    QSqlDatabase m_database;
    QString m_error;
    bool m_ready = false;
};

void clearStructuredMenuState(IaMemsService *service)
{
    if (!service)
        return;
    service->setProperty("iaStructuredMenuKeys", QVariant());
    service->setProperty("iaStructuredMenuLabels", QVariant());
    service->setProperty("iaStructuredMenuOriginal", QVariant());
}

QString structuredMenuText(const QList<ReferenceChoice> &choices)
{
    QString text = QStringLiteral("J'ai trouvé plusieurs rubriques documentaires correspondantes. Laquelle recherchez-vous ?");
    for (int i = 0; i < choices.size(); ++i)
        text += QStringLiteral("\n%1. %2").arg(i + 1).arg(choices.at(i).label);
    text += QStringLiteral("\nRépondez par le numéro.");
    return text;
}

bool isStructuredReferenceEligible(const QString &question)
{
    if (IaMemsConversationRouting::isDocumentationQuestion(question))
        return true;
    return IaResponseLogic::classify(question) == IaResponseLogic::Intent::None;
}

} // namespace

void IaMemsService::askWithLibrary(const QString &question)
{
    QString trimmed = question.trimmed();
    if (trimmed.isEmpty())
        return;

    const bool resolvedDocumentary = trimmed.startsWith(kResolvedDocumentaryPrefix);
    if (resolvedDocumentary)
        trimmed = trimmed.mid(kResolvedDocumentaryPrefix.size()).trimmed();
    if (trimmed.isEmpty())
        return;

    updateContextFromQuestion(trimmed);

    if (resolvedDocumentary) {
        const QString evidence = property("iaMemsResolvedEvidence").toString().trimmed();
        const QString subject = property("iaMemsResolvedSubject").toString().trimmed();
        setProperty("iaMemsLastLibraryQuestion", trimmed);
        setProperty("iaMemsLastLibraryEvidence", evidence);
        setProperty("iaMemsLastLibraryLocked", true);

        if (evidence.isEmpty()) {
            m_pendingGrounding.clear();
            emit responseReady(QStringLiteral(
                "La rubrique documentaire a été sélectionnée, mais son contenu structuré est vide. Je ne vais pas élargir automatiquement la recherche à un autre sujet."));
            emit statusChanged();
            return;
        }

        m_pendingGrounding = evidence;

        if (m_localAi && m_localAi->isReady()) {
            m_localAi->ask(documentaryModelQuestion(trimmed, subject), evidence);
            emit statusChanged();
            return;
        }

        m_pendingGrounding.clear();
        emit responseReady(evidence);
        emit statusChanged();
        return;
    }

    const QStringList pendingKeys = property("iaStructuredMenuKeys").toStringList();
    const QStringList pendingLabels = property("iaStructuredMenuLabels").toStringList();
    const QString pendingOriginal = property("iaStructuredMenuOriginal").toString().trimmed();
    if (!pendingKeys.isEmpty()) {
        bool ok = false;
        const int selected = trimmed.toInt(&ok);
        if (ok && selected >= 1 && selected <= pendingKeys.size()) {
            StructuredReferenceReader reader;
            const ReferenceChoice choice = reader.choiceByKey(pendingKeys.at(selected - 1));
            clearStructuredMenuState(this);
            if (choice.isValid()) {
                const QString evidence = reader.evidenceForChoice(choice);
                if (!evidence.isEmpty()) {
                    const QString subject = selected - 1 < pendingLabels.size()
                        ? pendingLabels.at(selected - 1) : choice.label;
                    const QString effectiveQuestion = pendingOriginal.isEmpty()
                        ? subject : QStringLiteral("%1 — %2").arg(pendingOriginal, subject);
                    setProperty("iaMemsLastLibraryQuestion", effectiveQuestion);
                    setProperty("iaMemsLastLibraryEvidence", evidence);
                    setProperty("iaMemsLastLibraryLocked", true);
                    m_pendingGrounding = evidence;
                    if (m_localAi && m_localAi->isReady()) {
                        m_localAi->ask(documentaryModelQuestion(effectiveQuestion, subject), evidence);
                        emit statusChanged();
                        return;
                    }
                    m_pendingGrounding.clear();
                    emit responseReady(evidence);
                    emit statusChanged();
                    return;
                }
            }
        } else {
            clearStructuredMenuState(this);
        }
    }

    if (isStructuredReferenceEligible(trimmed)) {
        StructuredReferenceReader reader;
        if (reader.isReady()) {
            const QList<ReferenceChoice> choices = reader.choicesForQuestion(trimmed, 24);
            if (choices.size() >= 2) {
                QStringList keys;
                QStringList labels;
                for (const ReferenceChoice &choice : choices) {
                    keys << choice.knowledgeKey;
                    labels << choice.label;
                }
                setProperty("iaStructuredMenuKeys", keys);
                setProperty("iaStructuredMenuLabels", labels);
                setProperty("iaStructuredMenuOriginal", trimmed);
                m_pendingGrounding.clear();
                emit responseReady(structuredMenuText(choices));
                emit statusChanged();
                return;
            }
            if (choices.size() == 1) {
                const ReferenceChoice choice = choices.first();
                const QString evidence = reader.evidenceForChoice(choice);
                if (!evidence.isEmpty()) {
                    setProperty("iaMemsLastLibraryQuestion", trimmed);
                    setProperty("iaMemsLastLibraryEvidence", evidence);
                    setProperty("iaMemsLastLibraryLocked", true);
                    m_pendingGrounding = evidence;
                    if (m_localAi && m_localAi->isReady()) {
                        m_localAi->ask(documentaryModelQuestion(trimmed, choice.label), evidence);
                        emit statusChanged();
                        return;
                    }
                    m_pendingGrounding.clear();
                    emit responseReady(evidence);
                    emit statusChanged();
                    return;
                }
            }
        }
    }

    const IaMemsLibraryGrounding libraryGrounding =
        IaMemsLibraryBridge::retrieve(trimmed, libraryKeywords(trimmed));
    setProperty("iaMemsLastLibraryQuestion", trimmed);
    setProperty("iaMemsLastLibraryEvidence", libraryGrounding.text);
    setProperty("iaMemsLastLibraryLocked", false);

    // When Pack001 really found documentary evidence, do not concatenate a second
    // legacy answer that may concern another topic. Ask the local model to
    // synthesize the Pack001 evidence instead of displaying the raw RAG block.
    if (!libraryGrounding.text.trimmed().isEmpty()) {
        m_pendingGrounding = libraryGrounding.text;
        if (m_localAi && m_localAi->isReady()) {
            m_localAi->ask(documentaryModelQuestion(trimmed, trimmed), libraryGrounding.text);
            emit statusChanged();
            return;
        }
        m_pendingGrounding.clear();
        emit responseReady(libraryGrounding.text);
        emit statusChanged();
        return;
    }

    // No Pack001 evidence: preserve the existing deterministic/expert path.
    const QString legacyGrounding = groundingFor(trimmed);
    m_pendingGrounding = legacyGrounding;

    if (m_localAi && m_localAi->isReady()) {
        m_localAi->ask(trimmed, m_pendingGrounding);
        emit statusChanged();
        return;
    }

    const QString fallback = m_pendingGrounding.trimmed().isEmpty()
        ? QStringLiteral("L'IA locale n'est pas encore prête.")
        : m_pendingGrounding;
    m_pendingGrounding.clear();
    emit responseReady(fallback);
    emit statusChanged();
}
