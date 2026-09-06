#include "IaMemsService.h"

#include "IaMemsConversationRouting.h"
#include "IaMemsLibraryBridge.h"
#include "LocalAiClient.h"

#include <QRegularExpression>
#include <QSet>

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
                                  const QString &subject,
                                  const QString &evidence)
{
    return QStringLiteral(
        "[[MEMS_GROUNDED]]Analyse documentaire technique.\n"
        "Question de l'utilisateur : %1\n"
        "Rubrique sélectionnée : %2\n\n"
        "Documentation structurée à utiliser exclusivement :\n%3\n\n"
        "Réponds directement en français à la question. Organise la réponse dans l'ordre utile. "
        "N'invente rien et n'ajoute aucune information extérieure à la documentation fournie. "
        "Si la documentation contient plusieurs étapes, conserve leur ordre et les informations nécessaires. "
        "N'affiche pas les identifiants internes, le préambule de recherche, la provenance, les clés techniques "
        "ou le niveau de preuve, sauf demande explicite de l'utilisateur.")
        .arg(question.trimmed(), subject.trimmed().isEmpty() ? question.trimmed() : subject.trimmed(), evidence.trimmed());
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

        m_pendingGrounding = QStringLiteral(
            "La documentation sélectionnée a été retrouvée, mais la synthèse par l'IA locale n'a pas pu être produite.");

        if (m_localAi && m_localAi->isReady()) {
            m_localAi->ask(documentaryModelQuestion(trimmed, subject, evidence), QString());
            emit statusChanged();
            return;
        }

        m_pendingGrounding.clear();
        emit responseReady(QStringLiteral(
            "La documentation sélectionnée a bien été retrouvée, mais l'IA locale n'est pas prête pour produire la réponse."));
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
        m_pendingGrounding = QStringLiteral(
            "La documentation a été retrouvée, mais la synthèse par l'IA locale n'a pas pu être produite.");
        if (m_localAi && m_localAi->isReady()) {
            m_localAi->ask(documentaryModelQuestion(trimmed, trimmed, libraryGrounding.text), QString());
            emit statusChanged();
            return;
        }
        m_pendingGrounding.clear();
        emit responseReady(QStringLiteral(
            "La documentation correspondante a été retrouvée, mais l'IA locale n'est pas prête pour produire la réponse."));
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
