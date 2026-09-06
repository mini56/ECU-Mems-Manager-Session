#include "IaMemsService.h"

#include "IaMemsConversationRouting.h"
#include "IaMemsLibraryBridge.h"
#include "LocalAiClient.h"

#include <QRegularExpression>
#include <QSet>

namespace {

const QString kLockedDocumentaryPrefix = QStringLiteral("[[MEMS_LOCKED]]");

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

QString mergeGrounding(const QString &legacy, const IaMemsLibraryGrounding &library)
{
    const QString oldGrounding = legacy.trimmed();
    const QString newGrounding = library.text.trimmed();
    if (newGrounding.isEmpty())
        return oldGrounding;
    if (oldGrounding.isEmpty() || genericInsufficientGrounding(oldGrounding))
        return newGrounding;

    return QStringLiteral("%1\n\nContexte expert MEMS Manager complémentaire :\n%2")
        .arg(newGrounding, oldGrounding);
}

} // namespace

void IaMemsService::askWithLibrary(const QString &question)
{
    QString trimmed = question.trimmed();
    if (trimmed.isEmpty())
        return;

    const bool lockedDocumentary = trimmed.startsWith(kLockedDocumentaryPrefix);
    if (lockedDocumentary)
        trimmed = trimmed.mid(kLockedDocumentaryPrefix.size()).trimmed();
    if (trimmed.isEmpty())
        return;

    updateContextFromQuestion(trimmed);

    if (lockedDocumentary) {
        // A numbered documentary choice is already the user's resolved subject.
        // Search only this exact subject: do not generate broad keyword/single-word
        // fallbacks that can jump to another system (for example emissions purge).
        const IaMemsLibraryGrounding libraryGrounding =
            IaMemsLibraryBridge::retrieve(trimmed, QStringList());
        setProperty("iaMemsLastLibraryQuestion", trimmed);
        setProperty("iaMemsLastLibraryEvidence", libraryGrounding.text);
        setProperty("iaMemsLastLibraryLocked", true);

        if (libraryGrounding.text.trimmed().isEmpty()) {
            m_pendingGrounding.clear();
            emit responseReady(QStringLiteral(
                "Je n'ai pas trouvé de donnée documentaire correspondant exactement au sujet sélectionné. Je ne vais pas élargir automatiquement la recherche à un autre sujet."));
            emit statusChanged();
            return;
        }

        // The evidence is embedded in the hidden model request and NOT passed as
        // groundingContext. That deliberately bypasses LocalAiClient's historical
        // non-diagnostic shortcut which displayed the raw RAG text instead of
        // asking Qwen to synthesize it.
        const QString modelQuestion = QStringLiteral(
            "Question documentaire sélectionnée par l'utilisateur : %1\n\n"
            "Documentation technique verrouillée pour cette sélection :\n%2\n\n"
            "Réponds directement à la question à partir de cette documentation uniquement. "
            "Donne une réponse claire, organisée et complète. N'affiche pas le préambule RAG, "
            "les identifiants internes, les clés DOC/REV/SRC, le niveau de preuve ni la provenance, "
            "sauf si l'utilisateur les demande explicitement.")
            .arg(trimmed, libraryGrounding.text.trimmed());

        m_pendingGrounding = QStringLiteral(
            "La documentation sélectionnée a été retrouvée, mais sa synthèse par l'IA locale n'a pas pu être produite.");

        if (m_localAi && m_localAi->isReady()) {
            m_localAi->ask(modelQuestion, QString());
            emit statusChanged();
            return;
        }

        m_pendingGrounding.clear();
        emit responseReady(QStringLiteral(
            "La documentation sélectionnée a bien été retrouvée, mais l'IA locale n'est pas prête pour produire la réponse synthétique."));
        emit statusChanged();
        return;
    }

    const QString legacyGrounding = groundingFor(trimmed);
    const IaMemsLibraryGrounding libraryGrounding =
        IaMemsLibraryBridge::retrieve(trimmed, libraryKeywords(trimmed));
    setProperty("iaMemsLastLibraryQuestion", trimmed);
    setProperty("iaMemsLastLibraryEvidence", libraryGrounding.text);
    setProperty("iaMemsLastLibraryLocked", false);
    m_pendingGrounding = mergeGrounding(legacyGrounding, libraryGrounding);

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
