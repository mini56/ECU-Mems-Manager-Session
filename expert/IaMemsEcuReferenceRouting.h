#ifndef IAMEMSECUREFERENCEROUTING_H
#define IAMEMSECUREFERENCEROUTING_H

#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QSet>
#include <QVariantList>
#include <QVariantMap>

// A strictly scoped ECU-identity request is NOT a RAVE workshop search.
// Do not infer "an ECU reference" from arbitrary uppercase words, from
// torque operation numbers, or from an existing conversational context.
// Routing is positive and narrow; all other requests stay unchanged.
namespace IaMemsEcuReferenceRouting {

inline QString candidateReference(QString text)
{
    text=text.trimmed().toUpper();
    static const QRegularExpression shape(QStringLiteral(
        "^[A-Z][A-Z0-9-]{4,22}$"));
    if(!shape.match(text).hasMatch())
        return QString();
    int letters=0,digits=0;
    for(QChar ch:text){
        if(ch.isLetter())++letters;
        if(ch.isDigit())++digits;
    }
    return letters>=2 && digits>=2 ? text : QString();
}

inline QString referenceFromQuestion(const QString &question)
{
    const QString trimmed=question.trimmed();
    if(trimmed.isEmpty())
        return QString();

    // A bare part/firmware code is sufficient, including mixed case.
    const QString bare=candidateReference(trimmed);
    if(!bare.isEmpty())
        return bare;

    // Phrases such as "Que sait-on sur AANMP002 ?" are permitted only
    // when *all* remaining words identify the search, not an intervention.
    // This prevents intercepting "dépose ...", "branchement ...",
    // "diagnostic ...", compound component questions, etc.
    // French accents and the SAIT-ON spelling are normal linguistic
    // variation. Only the identifier itself is kept case-insensitive.
    QString phrase;
    for(const QChar ch:trimmed.normalized(
            QString::NormalizationForm_D).toUpper()){
        const QChar::Category cat=ch.category();
        if(cat!=QChar::Mark_NonSpacing &&
           cat!=QChar::Mark_SpacingCombining &&
           cat!=QChar::Mark_Enclosing)
            phrase.append(ch);
    }
    phrase.replace(QRegularExpression(
        QStringLiteral("(?<=[A-Z])-(?=[A-Z])")),QStringLiteral(" "));
    const QStringList words=phrase.split(
        QRegularExpression(QStringLiteral("[^\\p{L}\\p{N}-]+")),
        Qt::SkipEmptyParts);
    if(words.size()<2 || words.size()>12)
        return QString();

    static const QSet<QString> allowed={
        QStringLiteral("QUE"),QStringLiteral("QU"),QStringLiteral("SAIT"),
        QStringLiteral("ON"),QStringLiteral("TU"),QStringLiteral("SAIS"),
        QStringLiteral("CONNAIS"),QStringLiteral("C"),
        QStringLiteral("EST"),QStringLiteral("CE"),QStringLiteral("QUOI"),
        QStringLiteral("QUEL"),QStringLiteral("QUELLE"),
        QStringLiteral("SUR"),QStringLiteral("DE"),QStringLiteral("DU"),
        QStringLiteral("DES"),QStringLiteral("LE"),QStringLiteral("LA"),
        QStringLiteral("LES"),QStringLiteral("UN"),QStringLiteral("UNE"),
        QStringLiteral("A"),QStringLiteral("AU"),QStringLiteral("PROPOS"),
        QStringLiteral("REFERENCES"),QStringLiteral("REFERENCE"),
        QStringLiteral("REF"),QStringLiteral("IDENTIFICATION"),
        QStringLiteral("IDENTIFIER"),QStringLiteral("IDENTIFIE"),
        QStringLiteral("INFORMATIONS"),QStringLiteral("INFORMATION"),
        QStringLiteral("INFOS"),QStringLiteral("INFO"),
        QStringLiteral("ECU"),QStringLiteral("CALCULATEUR"),
        QStringLiteral("FIRMWARE"),QStringLiteral("VERSION"),
        QStringLiteral("CODE"),QStringLiteral("DONNEES")
    };
    QString reference;
    for(const QString &word:words){
        const QString candidate=candidateReference(word);
        if(!candidate.isEmpty()){
            if(!reference.isEmpty())
                return QString(); // never guess which of two codes is meant
            reference=candidate;
        }else if(!allowed.contains(word)){
            return QString();
        }
    }
    return reference;
}

// Known ECU/firmware namespaces do not depend on a particular code.
// A code outside these namespaces can still qualify through an EXACT match
// in ecu_target, never through a fuzzy manual/OCR hit.
inline bool recognizedEcuNamespace(const QString &code)
{
    const QString upper=code.trimmed().toUpper();
    return upper.startsWith(QStringLiteral("MNE")) ||
           upper.startsWith(QStringLiteral("MKC")) ||
           upper.startsWith(QStringLiteral("NNN")) ||
           upper.startsWith(QStringLiteral("AANMP"));
}

inline bool explicitEcuIdentityIntent(const QString &question)
{
    const QString upper=question.toUpper();
    static const QRegularExpression identityWords(
        QStringLiteral("\\b(?:ECU|CALCULATEUR|FIRMWARE)\\b"));
    return identityWords.match(upper).hasMatch();
}

inline bool exactCodeMention(const QString &text,const QString &code)
{
    if(code.isEmpty())return false;
    const QRegularExpression rx(QStringLiteral(
        "(^|[^A-Z0-9])%1([^A-Z0-9]|$)")
        .arg(QRegularExpression::escape(code)),
        QRegularExpression::CaseInsensitiveOption);
    return rx.match(text).hasMatch();
}

inline QString field(const QVariantMap &row,const QString &key)
{
    return row.value(key).toString().trimmed();
}

inline QString exactEcuSummary(const QString &code,
                              const QVariantMap &ecu,
                              const QVariantList &fitments,
                              const QVariantList &files)
{
    if(ecu.isEmpty())
        return QString();
    QStringList lines;
    lines<<QStringLiteral("Référence ECU retrouvée dans la base locale : %1.")
             .arg(code);
    const struct Info{const char *key;const char *label;} infos[]={
        {"brand","Marque"},
        {"system_family","Système"},
        {"injection","Injection"},
        {"makes","Constructeur(s)"},
        {"models","Modèle(s)"}
    };
    for(const Info &info:infos){
        const QString value=field(ecu,QString::fromLatin1(info.key));
        if(!value.isEmpty())
            lines<<QStringLiteral("%1 : %2")
                .arg(QString::fromUtf8(info.label),value);
    }
    const QString from=field(ecu,QStringLiteral("year_from"));
    const QString to=field(ecu,QStringLiteral("year_to"));
    if(!from.isEmpty() || !to.isEmpty())
        lines<<QStringLiteral("Années répertoriées : %1")
                 .arg(from==to?from:from+QStringLiteral("–")+to);
    const QString notes=field(ecu,QStringLiteral("notes"));
    if(!notes.isEmpty())
        lines<<QStringLiteral("Notes de référence : %1").arg(notes);

    if(!fitments.isEmpty()){
        lines<<QStringLiteral("Affectations enregistrées :");
        for(int i=0;i<qMin(5,fitments.size());++i){
            const QVariantMap row=fitments.at(i).toMap();
            QStringList detail;
            for(const QString &key:{QStringLiteral("make"),
                QStringLiteral("model"),QStringLiteral("variant"),
                QStringLiteral("engine"),QStringLiteral("market")}){
                const QString value=field(row,key);
                if(!value.isEmpty())detail<<value;
            }
            if(!detail.isEmpty())
                lines<<QStringLiteral("• %1").arg(detail.join(QStringLiteral(" — ")));
        }
        if(fitments.size()>5)
            lines<<QStringLiteral("%1 autres affectations consultables dans « Base de données ».")
                     .arg(fitments.size()-5);
    }
    if(!files.isEmpty())
        lines<<QStringLiteral("%1 fichier(s) associé(s) dans la base.")
                 .arg(files.size());

    lines<<QStringLiteral("Source : base ECU locale de SESSION, en lecture seule.");
    return lines.join(QLatin1Char('\n'));
}

inline QString notFound(const QString &code,bool databaseAvailable)
{
    if(!databaseAvailable)
        return QStringLiteral(
            "La base des références ECU n'a pas pu être ouverte. "
            "Je ne peux pas vérifier « %1 » actuellement. "
            "Aucune identification n'est inventée.").arg(code);
    return QStringLiteral(
        "La référence « %1 » n'a pas été identifiée dans les "
        "références ECU et les faits experts actuellement disponibles. "
        "Ce résultat ne prouve pas qu'elle n'existe pas ; "
        "il ne s'agit pas d'une recherche de procédure RAVE. "
        "Vérifie le code complet inscrit sur le calculateur "
        "ou consulte la page « Base de données ».").arg(code);
}

} // namespace IaMemsEcuReferenceRouting

#endif // IAMEMSECUREFERENCEROUTING_H
