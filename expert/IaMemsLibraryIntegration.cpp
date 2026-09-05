#include "IaMemsService.h"

#include "IaMemsConversationRouting.h"
#include "IaMemsLibraryBridge.h"
#include "LocalAiClient.h"

#include <QRegularExpression>
#include <QSet>

namespace {

void appendUnique(QStringList &terms, const QString &term)
{
    const QString clean = term.trimmed().toLower();
    if (!clean.isEmpty() && !terms.contains(clean) && terms.size() < 12)
        terms.append(clean);
}

QStringList manufacturerReferenceAliases(const QString &question)
{
    const QString normalized = IaMemsConversationRouting::normalize(question);
    static const QRegularExpression referenceRx(
        QStringLiteral("\\b(\\d{1,3}(?:\\.\\d{1,3}){2,3})\\b"));

    QStringList aliases;
    QRegularExpressionMatchIterator matches = referenceRx.globalMatch(normalized);
    while (matches.hasNext()) {
        QString alias = matches.next().captured(1);
        alias.replace(QLatin1Char('.'), QLatin1Char('_'));
        appendUnique(aliases, alias);
    }
    return aliases;
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

    // RAVE operation references such as 12.21.28 are encoded in entity keys
    // with underscores. Add that generic alias so the library can resolve any
    // manufacturer operation number without hard-coding a particular one.
    const QStringList referenceAliases = manufacturerReferenceAliases(question);
    for (const QString &alias : referenceAliases)
        appendUnique(terms, alias);

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

    // Once MEMSLibrary has re-queried and verified the winning physical page,
    // that documentary proof is authoritative. Re-adding legacy grounding can
    // reintroduce an unrelated procedure even though the library answer is good.
    if (library.provenanceFiltered)
        return newGrounding;

    if (oldGrounding.isEmpty() || genericInsufficientGrounding(oldGrounding))
        return newGrounding;

    return QStringLiteral("%1\n\nContexte expert MEMS Manager complémentaire :\n%2")
        .arg(newGrounding, oldGrounding);
}

} // namespace

void IaMemsService::askWithLibrary(const QString &question)
{
    const QString trimmed = question.trimmed();
    if (trimmed.isEmpty())
        return;

    updateContextFromQuestion(trimmed);
    const QString legacyGrounding = groundingFor(trimmed);
    const IaMemsLibraryGrounding libraryGrounding =
        IaMemsLibraryBridge::retrieve(trimmed, libraryKeywords(trimmed));
    setProperty("iaMemsLastLibraryQuestion", trimmed);
    setProperty("iaMemsLastLibraryEvidence", libraryGrounding.text);
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
