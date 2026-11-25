#include "IaMemsService.h"

#include "IaMemsConversationRouting.h"
#include "IaMemsEcuReferenceRouting.h"
#include "IaMemsLibraryBridge.h"
#include "IaResponseLogic.h"
#include "LocalAiClient.h"
#include "../database/MemsReferenceDatabase.h"
#include "../database/MemsDocumentaryRepository.h"
#include "../i18n.h"

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

QString requestedWorkshopOperation(const QString &question)
{
    const QString plain = IaMemsConversationRouting::normalize(question);
    const QStringList words = plain.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    const auto has = [&words](const QStringList &candidates) {
        for (const QString &candidate : candidates)
            if (words.contains(candidate))
                return true;
        return false;
    };
    if (has({QStringLiteral("depose"), QStringLiteral("deposer"),
             QStringLiteral("demontage"), QStringLiteral("demonter")}))
        return QStringLiteral("depose");
    if (has({QStringLiteral("repose"), QStringLiteral("reposer"),
             QStringLiteral("remontage"), QStringLiteral("remonter")}))
        return QStringLiteral("repose");
    if (has({QStringLiteral("reparation"), QStringLiteral("reparer")}))
        return QStringLiteral("reparation");
    if (has({QStringLiteral("controle"), QStringLiteral("controler"),
             QStringLiteral("verification"), QStringLiteral("verifier"),
             QStringLiteral("inspection"), QStringLiteral("inspecter")}))
        return QStringLiteral("controle");
    if (has({QStringLiteral("reglage"), QStringLiteral("regler"),
             QStringLiteral("ajustement")}))
        return QStringLiteral("reglage");
    if (has({QStringLiteral("revision"), QStringLiteral("entretien")}))
        return QStringLiteral("revision");
    if (has({QStringLiteral("remplacement"), QStringLiteral("remplacer"),
             QStringLiteral("changer")}))
        return QStringLiteral("remplacement");
    if (has({QStringLiteral("purge"), QStringLiteral("purger")}))
        return QStringLiteral("purge");
    if (has({QStringLiteral("serrage"), QStringLiteral("couple")}))
        return QStringLiteral("serrage");
    if (has({QStringLiteral("schema"), QStringLiteral("schemas")}))
        return QStringLiteral("schema");
    if (has({QStringLiteral("diagnostic")}))
        return QStringLiteral("diagnostic");
    return QString();
}

// Preserve the last explicitly named technical subject, including its
// engine variant, even when the current installed corpus has no matching
// heading. A later bare "depose" must NEVER revive an older SPi request.
QString documentaryComponentForContext(const QString &question)
{
    QStringList words=MemsDocumentaryRepository::subjectTerms(question);
    if(words.isEmpty())return QString();
    const QStringList tokens=IaMemsConversationRouting::normalize(question)
        .split(QRegularExpression(QStringLiteral("[^\\p{L}\\p{N}]+")),
               Qt::SkipEmptyParts);
    for(const QString &qualifier:{QStringLiteral("spi"),QStringLiteral("mpi"),
                                 QStringLiteral("monopoint"),QStringLiteral("multipoint")}){
        if(tokens.contains(qualifier) && !words.contains(qualifier))
            words.append(qualifier);
    }
    return words.join(QLatin1Char(' '));
}

bool workshopOperationMatches(const QString &title, const QString &operation)
{
    if (operation.isEmpty())
        return true;
    const QString normalized = IaMemsConversationRouting::normalize(title);
    const auto containsAny = [&normalized](const QStringList &variants) {
        for (const QString &variant : variants)
            if (normalized.contains(variant))
                return true;
        return false;
    };
    if (operation == QStringLiteral("depose"))
        return containsAny({QStringLiteral("depose"), QStringLiteral("demontage")});
    if (operation == QStringLiteral("repose"))
        return containsAny({QStringLiteral("repose"), QStringLiteral("remontage")});
    if (operation == QStringLiteral("reparation"))
        return containsAny({QStringLiteral("repar"), QStringLiteral("remise en etat")});
    if (operation == QStringLiteral("controle"))
        return containsAny({QStringLiteral("controle"), QStringLiteral("verif"),
                            QStringLiteral("inspect"), QStringLiteral("essai")});
    if (operation == QStringLiteral("reglage"))
        return containsAny({QStringLiteral("reglage"), QStringLiteral("ajust")});
    if (operation == QStringLiteral("revision"))
        return containsAny({QStringLiteral("revision"), QStringLiteral("entretien")});
    if (operation == QStringLiteral("remplacement"))
        return containsAny({QStringLiteral("remplac"), QStringLiteral("echange")});
    if (operation == QStringLiteral("purge"))
        return normalized.contains(QStringLiteral("purge"));
    if (operation == QStringLiteral("serrage"))
        return containsAny({QStringLiteral("serrage"), QStringLiteral("couple")});
    if (operation == QStringLiteral("schema"))
        return containsAny({QStringLiteral("schema"), QStringLiteral("cablage")});
    if (operation == QStringLiteral("diagnostic"))
        return normalized.contains(QStringLiteral("diagnostic"));
    return false;
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
        "Si plusieurs étapes sont fournies, conserve leur ordre. "
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
        // Tolère un petit suffixe (pluriel : eau/eaux, feu/feux, roue/roues)
        // sans jamais matcher au milieu d'un mot sans rapport.
        if (word.size() >= term.size() && word.startsWith(term)
            && word.size() - term.size() <= 2)
            return true;
        if (term.size() >= 4 && word.size() >= 4
            && (word.startsWith(term) || term.startsWith(word)))
            return true;
    }
    return false;
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

        if (output.size() <= 1)
            return QString();

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

    // The documentary repository has already selected the installed
    // language from the same SQLite used by the Database page.  Answers and
    // numbered selections are immediate and cannot stall on model inference.
    const auto emitLocalizedDocument = [this](const QString &sourceText) {
        emit responseReady(sourceText);
    };

    const bool resolvedDocumentary = trimmed.startsWith(kResolvedDocumentaryPrefix);
    if (resolvedDocumentary) {
        trimmed = trimmed.mid(kResolvedDocumentaryPrefix.size()).trimmed();
        // An externally resolved reference replaces any older numbered
        // menu; otherwise the next digit could select a stale source.
        setProperty("iaMemsRoverMenuRows", QVariant());
        setProperty("iaMemsRoverMenuQuestion", QVariant());
        clearStructuredMenuState(this);
    }
    if (trimmed.isEmpty())
        return;

    // Les pages appartiennent à la réponse et à l'opération en cours.
    setProperty("iaMemsLastDocumentaryImagePath", QVariant());
    setProperty("iaMemsLastDocumentaryImagePaths", QVariant());

    // Only a single operation word can refine the previous technical subject.
    // New named searches always replace it, even when previous documents
    // happen to mention the new word.
    const QString currentComponent=property("iaMemsActiveDocumentaryComponent").toString();
    const bool isNumericSelection=
        QRegularExpression(QStringLiteral("^[0-9]+$")).match(trimmed).hasMatch();
    const bool followUpOperation=IaMemsConversationRouting::isBareWorkshopOperation(trimmed);
    if (!isNumericSelection && !resolvedDocumentary) {
        trimmed=IaMemsConversationRouting::contextualWorkshopQuestion(
            currentComponent,trimmed);
        if (!followUpOperation)
            setProperty("iaMemsActiveDocumentaryComponent",
                        documentaryComponentForContext(trimmed));
    }

    // When the Rover catalogue is mounted, a numbered reply selects the
    // exact stored source row rather than starting a new, unrelated search.
    // New text starts a new query; selecting an item never changes the corpus.
    const QVariantList roverChoices = property("iaMemsRoverMenuRows").toList();
    if (!roverChoices.isEmpty() && !resolvedDocumentary) {
        bool numeric = false;
        const int selected = trimmed.toInt(&numeric);
        if (numeric) {
            if (selected < 1 || selected > roverChoices.size()) {
                m_pendingGrounding.clear();
                emit responseReady(QStringLiteral(
                    "Choisis un numéro entre 1 et %1, ou saisis une nouvelle recherche.")
                    .arg(roverChoices.size()));
                emit statusChanged();
                return;
            }
            const QVariantMap chosen =
                MemsDocumentaryRepository::choiceForNumber(roverChoices,trimmed);
            if(chosen.isEmpty()){
                emit responseReady(QStringLiteral(
                    "Ce numéro ne correspond à aucune source affichée. "
                    "Relance une recherche documentaire."));
                emit statusChanged();
                return;
            }
            clearStructuredMenuState(this);
            const QString originalQuestion =
                property("iaMemsRoverMenuQuestion").toString().trimmed();
            // Identical headings may designate several real source rows.
            // Open a second menu rather than silently choosing the first or
            // dropping the other manufacturer references.
            const QVariantList members=chosen.value(
                QStringLiteral("iaMemsGroupRows")).toList();
            if (!members.isEmpty()) {
                QVariantList subChoices;
                for(const QVariant &member:members){
                    QVariantMap source=member.toMap();
                    source.remove(QStringLiteral("iaMemsGroupRows"));
                    source.insert(QStringLiteral("iaMemsGroupChild"),true);
                    subChoices.append(source);
                }
                setProperty("iaMemsRoverMenuRows",subChoices);
                setProperty("iaMemsRoverMenuQuestion",originalQuestion);
                m_pendingGrounding.clear();
                const QString subMenuSubject=QStringLiteral("%1 — %2")
                    .arg(originalQuestion,chosen.value(QStringLiteral("title")).toString());
                emit responseReady(MemsDocumentaryRepository::numberedMenu(
                    subMenuSubject,subChoices,
                    I18n::language().startsWith(QStringLiteral("fr"),
                                                Qt::CaseInsensitive)));
                emit statusChanged();
                return;
            }
            setProperty("iaMemsRoverMenuRows", QVariant());
            setProperty("iaMemsRoverMenuQuestion", QVariant());
            MemsDocumentaryRepository documentary;
            if (!documentary.open()) {
                m_pendingGrounding.clear();
                emit responseReady(QStringLiteral(
                    "La base documentaire Rover est indisponible : %1.")
                    .arg(documentary.lastError()));
                emit statusChanged();
                return;
            }
            QStringList linkedPages;
            QString evidence;
            if (!chosen.value(QStringLiteral("iaMemsMentionOnly")).toBool())
                evidence=documentary.procedureAnswer(chosen,&linkedPages);
            if(evidence.isEmpty())
                evidence=MemsDocumentaryRepository::answerFromHits(
                    QVariantList{chosen});
            if(!linkedPages.isEmpty())
                setProperty("iaMemsLastDocumentaryImagePaths",linkedPages);
            setProperty("iaMemsLastDocumentaryImagePath",
                        MemsDocumentaryRepository::imagePathForHit(chosen));
            setProperty("iaMemsLastLibraryQuestion",
                        originalQuestion.isEmpty() ? trimmed : originalQuestion);
            setProperty("iaMemsLastLibraryEvidence", evidence);
            setProperty("iaMemsLastLibraryLocked", true);
            m_pendingGrounding.clear();
            emitLocalizedDocument(evidence);
            emit statusChanged();
            return;
        }
        setProperty("iaMemsRoverMenuRows", QVariant());
        setProperty("iaMemsRoverMenuQuestion", QVariant());
    }

    if(isNumericSelection && !resolvedDocumentary &&
       !currentComponent.trimmed().isEmpty()){
        m_pendingGrounding.clear();
        emit responseReady(QStringLiteral(
            "Ce numéro ne correspond à aucune source affichée. "
            "Relance une recherche documentaire."));
        emit statusChanged();
        return;
    }

    // A qualified NA diagnosis must bypass obsolete rules and model-generated
    // conversions. Explicit workshop documentation keeps its original route.
    const QString na=(!resolvedDocumentary &&
        !IaMemsConversationRouting::isDocumentationQuestion(trimmed))?naAnswer(trimmed):QString();
    if(!na.isEmpty()){
        clearStructuredMenuState(this);
        m_pendingGrounding.clear();
        emit responseReady(na);
        emit statusChanged();
        return;
    }
    updateContextFromQuestion(trimmed);

    // Live ECU diagnosis remains on the measurement/expert route unless
    // the user explicitly asks for a technical document. Otherwise a vague
    // word in an OCR page could hijack an actual vehicle diagnosis.
    const bool explicitDocumentaryRequest =
        resolvedDocumentary ||
        IaMemsConversationRouting::isDocumentationQuestion(trimmed);
    const bool diagnosticRequest =
        IaMemsConversationRouting::shouldUseDiagnosticGeneration(
            trimmed, QString());
    const IaResponseLogic::Intent liveIntent = IaResponseLogic::classify(trimmed);
    const bool requestsLiveEcuData = liveIntent != IaResponseLogic::Intent::None;
    const bool softwareHelpRequest = !softwareAnswer(trimmed).trimmed().isEmpty();
    // Positively identified ECU references are looked up in the ECU
    // reference catalogue (and, if present, in verified expert facts).
    // Critically this is NOT a broad keyword classifier: ordinary workshop
    // parts, their numbered menus, RAVE citations, wiring, diagnostics,
    // telemetry and software help remain on their existing paths.
    const QString ecuCode=IaMemsEcuReferenceRouting::referenceFromQuestion(trimmed);
    if(!resolvedDocumentary && !ecuCode.isEmpty() &&
       !explicitDocumentaryRequest && !diagnosticRequest &&
       !requestsLiveEcuData && !softwareHelpRequest){
        MemsReferenceDatabase references;
        const bool available=references.open();
        QVariantMap exact;
        if(available){
            exact=references.ecu(ecuCode);
            if(exact.isEmpty()){
                // Search may find aliases and mentions in fitment data,
                // but only an EXACT part_number is an identified ECU.
                // Never turn a fuzzy match into a proved identification.
                for(const QVariant &item:references.searchEcus(ecuCode,QString(),50)){
                    const QVariantMap row=item.toMap();
                    if(row.value(QStringLiteral("part_number")).toString()
                          .compare(ecuCode,Qt::CaseInsensitive)==0){
                        exact=references.ecu(row.value(
                            QStringLiteral("part_number")).toString());
                        break;
                    }
                }
            }
        }
        const bool isEcuIdentity=
            !exact.isEmpty() ||
            IaMemsEcuReferenceRouting::recognizedEcuNamespace(ecuCode) ||
            IaMemsEcuReferenceRouting::explicitEcuIdentityIntent(trimmed);
        if(isEcuIdentity){
        clearStructuredMenuState(this);
        setProperty("iaMemsRoverMenuRows",QVariant());
        setProperty("iaMemsRoverMenuQuestion",QVariant());
        QString answer;
        if(!exact.isEmpty()){
            const QString part=exact.value(QStringLiteral("part_number")).toString();
            answer=IaMemsEcuReferenceRouting::exactEcuSummary(
                ecuCode,exact,references.fitments(part),references.files(part));
        }

        // The existing read-only expert knowledge may additionally contain
        // verified statements about an ECU firmware/part code even when the
        // target ECU catalogue does not hold that exact reference.
        if(m_knowledgeReady && m_reader.isOpen()){
            QStringList facts;
            // ExpertKnowledgeReader intentionally filters by family/firmware.
            // An empty context hides scoped facts. Check all supported MEMS
            // families, but accept ONLY statements containing this exact code
            // (or an identical firmware key) and show each source/proof.
            QStringList families;
            if(!m_context.family.trimmed().isEmpty())
                families<<m_context.family.trimmed();
            for(const QString &family:{QStringLiteral("1.2"),
                QStringLiteral("1.3"),QStringLiteral("1.6"),
                QStringLiteral("1.9")}){
                if(!families.contains(family))families<<family;
            }
            for(const QString &family:families){
                ExpertContext searchContext;
                searchContext.family=family;
                searchContext.firmware=ecuCode;
                const QList<ExpertFact> known=m_reader.facts(searchContext);
                for(const ExpertFact &fact:known){
                    const bool sameFirmware=fact.firmware.trimmed()
                        .compare(ecuCode,Qt::CaseInsensitive)==0;
                    const bool sameKey=IaMemsEcuReferenceRouting::exactCodeMention(
                        fact.factKey,ecuCode);
                    const bool directStatement=IaMemsEcuReferenceRouting::exactCodeMention(
                        fact.statement,ecuCode);
                    const bool directTopic=IaMemsEcuReferenceRouting::exactCodeMention(
                        fact.topic,ecuCode);
                    if(!sameFirmware && !sameKey && !directStatement && !directTopic)
                        continue;
                    if(fact.statement.trimmed().isEmpty())continue;
                    QString item=QStringLiteral("• %1").arg(fact.statement.trimmed());
                    if(!fact.family.trimmed().isEmpty() &&
                       fact.family.trimmed()!=QStringLiteral("*"))
                        item+=QStringLiteral(" [famille : %1]").arg(fact.family.trimmed());
                    if(!fact.sourceKey.trimmed().isEmpty())
                        item+=QStringLiteral(" (source : %1)").arg(fact.sourceKey.trimmed());
                    if(!fact.verificationLevel.trimmed().isEmpty())
                        item+=QStringLiteral(" [preuve : %1]").arg(
                            fact.verificationLevel.trimmed());
                    if(!facts.contains(item))facts.append(item);
                    if(facts.size()>=5)break;
                }
                if(facts.size()>=5)break;
            }
            if(!facts.isEmpty()){
                if(!answer.isEmpty())answer+=QStringLiteral("\n\n");
                answer+=QStringLiteral(
                    "Informations supplémentaires de la base experte :\n")+
                    facts.join(QLatin1Char('\n'));
            }
        }
        if(answer.isEmpty())
            answer=IaMemsEcuReferenceRouting::notFound(ecuCode,available);
        setProperty("iaMemsLastLibraryQuestion",trimmed);
        setProperty("iaMemsLastLibraryEvidence",answer);
        setProperty("iaMemsLastLibraryLocked",true);
        m_pendingGrounding.clear();
        emit responseReady(answer);
        emit statusChanged();
        return;
        }
        // A manual identifier (e.g. a RAVE publication number) does not
        // become an ECU part number just because it contains letters and
        // digits; preserve the existing documentary answer path.
    }

    if (MemsDocumentaryRepository::isPackInstalled() &&
        (explicitDocumentaryRequest ||
         (!diagnosticRequest && !requestsLiveEcuData && !softwareHelpRequest))) {
        // One documentary SQLite and one retrieval policy for UI and AI.
        // All new-source evidence is exposed directly, with page and review
        // status. Never send unapproved OCR steps to Qwen for unsupervised
        // reformulation and never silently substitute older Pack001 facts.
        MemsDocumentaryRepository documentary;
        if (!documentary.open()) {
            m_pendingGrounding.clear();
            emit responseReady(QStringLiteral(
                "Base documentaire indisponible : %1.")
                .arg(documentary.lastError()));
            emit statusChanged();
            return;
        }
        const QVariantList documents = documentary.search(
            trimmed, 100, true, I18n::language());
        if (!documents.isEmpty()) {
            // A new documentary search owns the only active numbered menu.
            // The older structured-reference menu must not accept its numbers.
            clearStructuredMenuState(this);
            // Prefer identifiable workshop operations to loose page OCR hits.
            // A menu of precise sources restores the numbered navigation the
            // documentary UI offered before installing the Rover corpus.
            QVariantList choices;
            QSet<QString> seenKeys;
            for (const QVariant &entry : documents) {
                const QVariantMap row = entry.toMap();
                const QString key = row.value(QStringLiteral("source_key")).toString();
                if (key.isEmpty() || seenKeys.contains(key)
                    || row.value(QStringLiteral("source_table")).toString()
                       == QStringLiteral("rover_ocr_discovery"))
                    continue;
                seenKeys.insert(key);
                choices.append(row);
            }
            if (choices.isEmpty())
                choices = documents;
            // One relevance policy for both direct manufacturer headings
            // and explicitly labelled source mentions (e.g. "PISTON").
            // A mention cannot become an unrelated workshop procedure.
            choices = MemsDocumentaryRepository::relevantChoices(trimmed, choices);
            // IA MEMS presents all REAL operations found for the component.
            // The choices come from the installed documentary pack, never
            // from a hardcoded list of hypothetical repairs.
            const QString operation = requestedWorkshopOperation(trimmed);
            const bool french = I18n::language().startsWith(
                QStringLiteral("fr"), Qt::CaseInsensitive);
            if (!choices.isEmpty()) {
                // Keep the subject, so a later "reparation" or "reglage"
                // refines this component instead of starting a new topic.
                const QString activeSubject=documentaryComponentForContext(trimmed);
                if (!activeSubject.trimmed().isEmpty())
                    setProperty("iaMemsActiveDocumentaryComponent", activeSubject);
            }

            // If the user has named an intervention, respond directly when a
            // unique matching structured procedure exists; otherwise offer
            // the available operations without silently substituting one.
            QVariantList matchingOperationChoices;
            for (const QVariant &candidate : choices) {
                const QVariantMap row = candidate.toMap();
                if (workshopOperationMatches(
                        row.value(QStringLiteral("title")).toString(), operation))
                    matchingOperationChoices.append(candidate);
            }
            if (!operation.isEmpty() && matchingOperationChoices.size() == 1) {
                const QVariantMap row = matchingOperationChoices.first().toMap();
                if (!row.value(QStringLiteral("iaMemsMentionOnly")).toBool() &&
                    !row.value(QStringLiteral("iaMemsGroupRows")).isValid() &&
                    row.value(QStringLiteral("entry_type")).toString()
                    == QStringLiteral("procedure")) {
                    QStringList pages;
                    const QString detail = documentary.procedureAnswer(row, &pages);
                    if (!detail.isEmpty()) {
                        setProperty("iaMemsLastDocumentaryImagePaths", pages);
                        setProperty("iaMemsLastLibraryQuestion", trimmed);
                        setProperty("iaMemsLastLibraryEvidence", detail);
                        setProperty("iaMemsLastLibraryLocked", true);
                        m_pendingGrounding.clear();
                        emitLocalizedDocument(detail);
                        emit statusChanged();
                        return;
                    }
                }
            }
            if (choices.isEmpty()) {
                m_pendingGrounding.clear();
                emit responseReady(QStringLiteral(
                    "Aucune rubrique correspondante pour « %1 ».").arg(trimmed));
                emit statusChanged();
                return;
            }

            // A specific request that lacks an exact title still receives
            // a truthful list of everything available on the subject.
            const QVariantList offers = MemsDocumentaryRepository::groupedChoices(
                (!operation.isEmpty() && !matchingOperationChoices.isEmpty())
                    ? matchingOperationChoices : choices);
            QString menu = MemsDocumentaryRepository::numberedMenu(
                trimmed, offers, french);
            setProperty("iaMemsRoverMenuRows", offers);
            setProperty("iaMemsRoverMenuQuestion", trimmed);
            setProperty("iaMemsLastLibraryQuestion", trimmed);
            setProperty("iaMemsLastLibraryEvidence", QVariant());
            setProperty("iaMemsLastLibraryLocked", true);
            m_pendingGrounding.clear();
            emit responseReady(menu);
            emit statusChanged();
            return;
        }
        const bool referenceNumber=QRegularExpression(
            QStringLiteral("\\b\\d{2}\\.\\d{2}\\.\\d{2}\\b"))
            .match(trimmed).hasMatch();
        const bool documentaryIntent=explicitDocumentaryRequest || referenceNumber;
        if (documentaryIntent) {
            m_pendingGrounding.clear();
            emit responseReady(QStringLiteral(
                "Aucune information correspondante dans la base documentaire installée."));
            emit statusChanged();
            return;
        }
        // Live diagnostic and software questions keep their own domain;
        // the Rover workshop manual must not replace ECU telemetry.
    }

    if (resolvedDocumentary) {
        const QString evidence = property("iaMemsResolvedEvidence").toString().trimmed();
        const QString subject = property("iaMemsResolvedSubject").toString().trimmed();
        setProperty("iaMemsLastLibraryQuestion", trimmed);
        setProperty("iaMemsLastLibraryEvidence", evidence);
        setProperty("iaMemsLastLibraryLocked", true);

        if (evidence.isEmpty()) {
            m_pendingGrounding.clear();
            emit responseReady(QStringLiteral(
                "Aucun texte disponible pour cette rubrique."));
            emit statusChanged();
            return;
        }

        // The selected structured operation is already the manufacturer answer.
        // A generative rewrite can omit steps, change values or produce a
        // different operation. Return the complete ordered evidence directly.
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
                const QString emptySubject = selected - 1 < pendingLabels.size()
                    ? pendingLabels.at(selected - 1) : choice.label;
                m_pendingGrounding.clear();
                emit responseReady(QStringLiteral(
                    "Aucun texte disponible pour cette rubrique."));
                emit statusChanged();
                return;
            }
        } else {
            clearStructuredMenuState(this);
        }
    }

    // A bare digit is a menu selection only while one of the two documented
    // menus is active. Never run it as a fresh free-text or ECU query.
    if (isNumericSelection && property("iaStructuredMenuKeys").toStringList().isEmpty()){
        m_pendingGrounding.clear();
        emit responseReady(QStringLiteral(
            "Aucun menu numéroté n'est actif. Saisis un sujet pour afficher ses rubriques."));
        emit statusChanged();
        return;
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

    // The installed French workshop corpus is authoritative for component
    // questions. If no FR source matched, do not silently substitute an old
    // English step from the legacy Pack001 library.
    if (MemsDocumentaryRepository::isPackInstalled() &&
        !diagnosticRequest && !requestsLiveEcuData && !softwareHelpRequest &&
        !MemsDocumentaryRepository::subjectTerms(trimmed).isEmpty()) {
        m_pendingGrounding.clear();
        emit responseReady(QStringLiteral(
            "Aucune rubrique trouvée pour « %1 ».").arg(trimmed));
        emit statusChanged();
        return;
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
