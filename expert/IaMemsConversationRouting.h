#ifndef IAMEMSCONVERSATIONROUTING_H
#define IAMEMSCONVERSATIONROUTING_H

#include <QString>
#include <QStringList>
#include <QRegularExpression>

namespace IaMemsConversationRouting {

inline QString normalize(QString text)
{
    const QString decomposed = text.toLower().simplified().normalized(QString::NormalizationForm_D);
    QString result;
    result.reserve(decomposed.size());
    for (const QChar ch : decomposed) {
        const QChar::Category category = ch.category();
        if (category == QChar::Mark_NonSpacing
            || category == QChar::Mark_SpacingCombining
            || category == QChar::Mark_Enclosing)
            continue;
        result.append(ch);
    }
    return result.simplified();
}

inline bool containsAny(const QString &text, const QStringList &terms)
{
    for (const QString &term : terms) {
        if (text.contains(term))
            return true;
    }
    return false;
}

inline bool containsWord(const QString &text, const QString &word)
{
    const int size = word.size();
    int from = 0;
    while ((from = text.indexOf(word, from, Qt::CaseInsensitive)) >= 0) {
        const bool left = from == 0 || !text.at(from - 1).isLetterOrNumber();
        const int end = from + size;
        const bool right = end >= text.size() || !text.at(end).isLetterOrNumber();
        if (left && right)
            return true;
        from = end;
    }
    return false;
}

inline bool isDocumentationQuestion(const QString &question)
{
    const QString text = normalize(question);
    return containsAny(text, {
        QStringLiteral("broche"), QStringLiteral("brochage"), QStringLiteral("pinout"),
        QStringLiteral("connecteur"), QStringLiteral("cablage"), QStringLiteral("wiring"),
        QStringLiteral("couleur de fil"), QStringLiteral("couleur du fil"),
        QStringLiteral("couleur des fils"), QStringLiteral("couleur fil"),
        QStringLiteral("wire color"), QStringLiteral("wire colour"),
        QStringLiteral("schema"), QStringLiteral("diagramme"),
        QStringLiteral("documentation"), QStringLiteral("document technique"),
        QStringLiteral("fiche xml"), QStringLiteral(".xml"), QStringLiteral(" xml"),
        QStringLiteral("procedure"), QStringLiteral("couple de serrage"),
        QStringLiteral("couple serrage"), QStringLiteral("torque")
    });
}

inline bool isVariantSensitiveQuestion(const QString &question)
{
    const QString text = normalize(question);
    return containsAny(text, {
        QStringLiteral("broche"), QStringLiteral("brochage"), QStringLiteral("pinout"),
        QStringLiteral("connecteur"), QStringLiteral("cablage"), QStringLiteral("wiring"),
        QStringLiteral("couleur de fil"), QStringLiteral("couleur du fil"),
        QStringLiteral("couleur des fils"), QStringLiteral("wire color"),
        QStringLiteral("wire colour"), QStringLiteral("schema"), QStringLiteral("diagramme")
    });
}

inline QString requestedGeneration(const QString &question)
{
    const QString text = normalize(question);
    for (const QString &generation : {QStringLiteral("1.2"), QStringLiteral("1.3"),
                                      QStringLiteral("1.6"), QStringLiteral("1.9")}) {
        QString compact = generation;
        compact.remove(QLatin1Char('.'));
        if (text.contains(generation)
            || text.contains(QStringLiteral("mems%1").arg(compact)))
            return generation;
    }
    return QString();
}

inline QString explicitInduction(const QString &question)
{
    const QString text = normalize(question);
    if (containsWord(text, QStringLiteral("spi")) || text.contains(QStringLiteral("monopoint")))
        return QStringLiteral("SPi");
    if (containsWord(text, QStringLiteral("mpi")) || text.contains(QStringLiteral("multipoint")))
        return QStringLiteral("MPi");
    return QString();
}

inline bool mentionsMini(const QString &question)
{
    return containsWord(normalize(question), QStringLiteral("mini"));
}

inline bool isSearchDirective(const QString &answer)
{
    const QString text = normalize(answer);
    return text == QStringLiteral("cherche")
        || text == QStringLiteral("recherche")
        || text == QStringLiteral("cherche toi meme")
        || text == QStringLiteral("cherche toi-meme")
        || text == QStringLiteral("trouve")
        || text == QStringLiteral("a toi de chercher");
}

inline QStringList inductionEvidenceProbes(const QString &question,
                                           bool ecuConnected,
                                           const QString &firmware)
{
    QStringList probes;
    const auto appendUnique = [&probes](const QString &value) {
        const QString trimmed = value.trimmed().toUpper();
        if (!trimmed.isEmpty() && !probes.contains(trimmed, Qt::CaseInsensitive))
            probes.append(trimmed);
    };

    if (ecuConnected)
        appendUnique(firmware);

    static const QRegularExpression ecuReferenceRx(
        QStringLiteral("\\b(?:MNE|MKC|NNN)[A-Z0-9-]{3,}\\b|\\bAANMP[0-9]{3,}\\b"),
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator iterator = ecuReferenceRx.globalMatch(question);
    while (iterator.hasNext())
        appendUnique(iterator.next().captured(0));

    return probes;
}

inline bool isUnknownDirective(const QString &answer)
{
    const QString text = normalize(answer);
    return text == QStringLiteral("je ne sais pas")
        || text == QStringLiteral("j en sais rien")
        || text == QStringLiteral("aucune idee")
        || text == QStringLiteral("inconnu");
}

inline bool needsInductionClarification(const QString &question)
{
    return isVariantSensitiveQuestion(question)
        && mentionsMini(question)
        && explicitInduction(question).isEmpty();
}

inline bool needsGenerationClarification(const QString &question, const QString &knownGeneration)
{
    if (!isDocumentationQuestion(question))
        return false;
    if (!requestedGeneration(question).isEmpty() || !knownGeneration.trimmed().isEmpty())
        return false;
    const QString text = normalize(question);
    return containsAny(text, {
        QStringLiteral("schema"), QStringLiteral("diagramme"), QStringLiteral("documentation"),
        QStringLiteral("fiche xml"), QStringLiteral(".xml"), QStringLiteral(" xml"),
        QStringLiteral("connecteur ecu"), QStringLiteral("brochage ecu")
    });
}

inline QString enrichWithKnownGeneration(const QString &question, const QString &knownGeneration)
{
    if (!isDocumentationQuestion(question)
        || !requestedGeneration(question).isEmpty()
        || knownGeneration.trimmed().isEmpty())
        return question;
    return QStringLiteral("%1 MEMS %2").arg(question.trimmed(), knownGeneration.trimmed());
}

inline QString focusedQuestion(QString question)
{
    const QString text = normalize(question);
    if (containsWord(text, QStringLiteral("ect"))) {
        question.replace(QStringLiteral("température"), QString(), Qt::CaseInsensitive);
        question.replace(QStringLiteral("temperature"), QString(), Qt::CaseInsensitive);
    } else if (containsWord(text, QStringLiteral("iat"))) {
        question.replace(QStringLiteral("température"), QString(), Qt::CaseInsensitive);
        question.replace(QStringLiteral("temperature"), QString(), Qt::CaseInsensitive);
    }
    return question.simplified();
}

} // namespace IaMemsConversationRouting

#endif // IAMEMSCONVERSATIONROUTING_H
