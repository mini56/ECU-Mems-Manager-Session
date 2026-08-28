#ifndef IAMEMSCONVERSATIONROUTING_H
#define IAMEMSCONVERSATIONROUTING_H

#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QSet>

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

inline bool shouldUseDiagnosticGeneration(const QString &question, const QString &grounding)
{
    const QString text = normalize(question);
    const QString facts = normalize(grounding);
    if (containsAny(text, {
            QStringLiteral("diagnostic"), QStringLiteral("diagnostique"),
            QStringLiteral("analyse"), QStringLiteral("analyser"),
            QStringLiteral("anormal"), QStringLiteral("panne"),
            QStringLiteral("probleme"), QStringLiteral("hypothese"),
            QStringLiteral("oscill"), QStringLiteral("instable")
        }))
        return true;

    // A documentary answer often contains "preuve : constructeur".  That is
    // provenance, not a request to ask Qwen for diagnostic reasoning.
    return facts.contains(QStringLiteral("hypotheses actuelles"))
        || facts.contains(QStringLiteral("confiance"));
}

inline bool isUnknownDirective(const QString &answer)
{
    const QString text = normalize(answer);
    return text == QStringLiteral("je ne sais pas")
        || text == QStringLiteral("j en sais rien")
        || text == QStringLiteral("aucune idee")
        || text == QStringLiteral("inconnu");
}

inline bool explicitVariantLabelsCompatible(const QString &question, const QString &factScopeText)
{
    const QString requestedInduction = explicitInduction(question);
    const QString fact = normalize(factScopeText);
    const bool factSpi = containsWord(fact, QStringLiteral("spi"));
    const bool factMpi = containsWord(fact, QStringLiteral("mpi"));
    if (requestedInduction == QStringLiteral("SPi") && factMpi && !factSpi)
        return false;
    if (requestedInduction == QStringLiteral("MPi") && factSpi && !factMpi)
        return false;

    const QString requested = normalize(question);
    const bool wantsJapan = requested.contains(QStringLiteral("japan"))
        || requested.contains(QStringLiteral("japon"));
    const bool wantsEurope = requested.contains(QStringLiteral("europe"));
    const bool wantsUk = containsWord(requested, QStringLiteral("uk"))
        || requested.contains(QStringLiteral("royaume uni"))
        || requested.contains(QStringLiteral("britannique"));
    const bool factJapan = fact.contains(QStringLiteral("japan"))
        || fact.contains(QStringLiteral("japon"));
    const bool factEurope = fact.contains(QStringLiteral("europe"));
    const bool factUk = containsWord(fact, QStringLiteral("uk"))
        || fact.contains(QStringLiteral("royaume uni"));
    if (wantsJapan && (factEurope || factUk) && !factJapan)
        return false;
    if ((wantsEurope || wantsUk) && factJapan && !factEurope && !factUk)
        return false;
    return true;
}

inline bool isKnowledgeContextQualifier(const QString &term)
{
    const QString value = normalize(term);
    static const QSet<QString> qualifiers = {
        QStringLiteral("spi"), QStringLiteral("mpi"), QStringLiteral("monopoint"),
        QStringLiteral("multipoint"), QStringLiteral("japan"), QStringLiteral("japon"),
        QStringLiteral("europe"), QStringLiteral("europeen"), QStringLiteral("uk"),
        QStringLiteral("automatic"), QStringLiteral("automatique"), QStringLiteral("manual"),
        QStringLiteral("manuelle"), QStringLiteral("mini"), QStringLiteral("97my"),
        QStringLiteral("1993"), QStringLiteral("1994"), QStringLiteral("1995"),
        QStringLiteral("1996"), QStringLiteral("1997"), QStringLiteral("1998"),
        QStringLiteral("1999"), QStringLiteral("2000")
    };
    return qualifiers.contains(value);
}

inline bool hasDirectWireColourEvidence(const QString &factText)
{
    const QString text = normalize(factText);
    if (containsAny(text, {
            QStringLiteral("wire_color"), QStringLiteral("wire color"),
            QStringLiteral("wire colour"), QStringLiteral("couleur de fil"),
            QStringLiteral("couleur du fil"), QStringLiteral("couleur des fils"),
            QStringLiteral("couleur fil"), QStringLiteral("code couleur")
        }))
        return true;
    const bool mentionsWire = containsWord(text, QStringLiteral("fil"))
        || containsWord(text, QStringLiteral("fils"))
        || containsWord(text, QStringLiteral("wire"));
    const bool mentionsColour = containsAny(text, {
        QStringLiteral("noir"), QStringLiteral("blanc"), QStringLiteral("vert"),
        QStringLiteral("bleu"), QStringLiteral("rouge"), QStringLiteral("jaune"),
        QStringLiteral("rose"), QStringLiteral("violet"), QStringLiteral("gris"),
        QStringLiteral("orange"), QStringLiteral("marron"), QStringLiteral("black"),
        QStringLiteral("white"), QStringLiteral("green"), QStringLiteral("blue"),
        QStringLiteral("red"), QStringLiteral("yellow"), QStringLiteral("pink"),
        QStringLiteral("purple"), QStringLiteral("grey"), QStringLiteral("gray"),
        QStringLiteral("brown")
    });
    return mentionsWire && mentionsColour;
}

inline bool hasTorqueEvidence(const QString &factText)
{
    const QString text = normalize(factText);
    return text.contains(QStringLiteral(" nm"))
        || text.contains(QStringLiteral("serrer"))
        || text.contains(QStringLiteral("serrage"))
        || text.contains(QStringLiteral("torque"));
}

inline QString knowledgeStatementSignature(const QString &statement)
{
    QString signature = normalize(statement);
    const int illustration = signature.indexOf(QStringLiteral("illustration locale:"));
    if (illustration >= 0)
        signature = signature.left(illustration).trimmed();
    return signature;
}

inline bool isMiniSpiMapPinoutQuestion(const QString &question)
{
    const QString text = normalize(question);
    return mentionsMini(question)
        && explicitInduction(question) == QStringLiteral("SPi")
        && containsWord(text, QStringLiteral("map"))
        && (containsWord(text, QStringLiteral("broche"))
            || text.contains(QStringLiteral("brochage"))
            || text.contains(QStringLiteral("pinout"))
            || text.contains(QStringLiteral("connecteur"))
            || text.contains(QStringLiteral("cablage"))
            || text.contains(QStringLiteral("wiring")));
}

inline QString miniSpiMapPinoutAnswer()
{
    return QStringLiteral(
        "Sur les Mini SPi documentées, le capteur MAP est intégré au calculateur MEMS et relié au collecteur d'admission par une durite de dépression. "
        "Il n'existe donc pas de broche de signal MAP externe comparable au montage MPi. Je ne vais pas inventer une broche.");
}

inline QString wireColourRoleLabel(const QString &statement)
{
    const QString text = normalize(statement);
    if (containsAny(text, {QStringLiteral("screen ground"), QStringLiteral("blindage"),
                           QStringLiteral("masse ecran"), QStringLiteral("ecran")}))
        return QStringLiteral("Blindage / masse écran");
    if (text.contains(QStringLiteral("relais")))
        return QStringLiteral("Commande relais/chauffage");
    if (containsAny(text, {QStringLiteral("+ve"), QStringLiteral("-ve"),
                           QStringLiteral("signal sonde"), QStringLiteral("signal lambda")}))
        return QStringLiteral("Signal sonde");
    return QString();
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
