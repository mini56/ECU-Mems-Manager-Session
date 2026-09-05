#pragma once

#include <QString>

namespace IaMemsLibrarySynthesis {

inline bool hasCanonicalLibraryEvidence(const QString &grounding)
{
    return grounding.contains(
        QStringLiteral("Documentation RAVEMEMS retrouvée dans MEMSLibrary_Pack_001."),
        Qt::CaseInsensitive);
}

inline QString promptQuestion(const QString &question, const QString &grounding)
{
    return QStringLiteral(
        "%1\n\n"
        "Extraits documentaires vérifiés fournis par MEMS Manager :\n%2\n\n"
        "Réponds directement à la question dans la langue active, en 2 à 4 phrases. "
        "Donne d'abord les valeurs ou faits demandés, puis la méthode ou le contrôle utile. "
        "Ne recopie pas les identifiants de source, de révision, de langue ou de type, "
        "et ne reproduis pas le bloc documentaire brut. N'ajoute aucune information absente des extraits."
    ).arg(question.trimmed(), grounding.trimmed());
}

} // namespace IaMemsLibrarySynthesis
