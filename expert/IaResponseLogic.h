#ifndef IA_RESPONSE_LOGIC_H
#define IA_RESPONSE_LOGIC_H

#include <QHash>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include <cmath>

namespace IaResponseLogic {

enum class Intent {
    None,
    Battery,
    Rpm,
    Coolant,
    Map,
    Lambda,
    Advance,
    Dwell,
    Idle,
    Throttle,
    EngineState,
    Diagnostic,
    Captures
};

inline QString normalize(QString text)
{
    text = text.toLower().simplified();
    const QPair<QChar, QChar> replacements[] = {
        {QChar(0x00E0), QLatin1Char('a')}, {QChar(0x00E2), QLatin1Char('a')},
        {QChar(0x00E4), QLatin1Char('a')}, {QChar(0x00E7), QLatin1Char('c')},
        {QChar(0x00E8), QLatin1Char('e')}, {QChar(0x00E9), QLatin1Char('e')},
        {QChar(0x00EA), QLatin1Char('e')}, {QChar(0x00EB), QLatin1Char('e')},
        {QChar(0x00EE), QLatin1Char('i')}, {QChar(0x00EF), QLatin1Char('i')},
        {QChar(0x00F4), QLatin1Char('o')}, {QChar(0x00F6), QLatin1Char('o')},
        {QChar(0x00F9), QLatin1Char('u')}, {QChar(0x00FB), QLatin1Char('u')},
        {QChar(0x00FC), QLatin1Char('u')}
    };
    for (const auto &replacement : replacements)
        text.replace(replacement.first, replacement.second);
    return text;
}

inline bool hasAny(const QString &text, std::initializer_list<const char*> words)
{
    for (const char *word : words) {
        if (text.contains(QString::fromLatin1(word)))
            return true;
    }
    return false;
}

inline Intent classify(const QString &question)
{
    const QString text = normalize(question);

    if (hasAny(text, {"capture", "captures", "capture ecran", "captures ecran"}))
        return Intent::Captures;
    if (hasAny(text, {"etat moteur", "etat du moteur", "comment est le moteur", "moteur tourne"}))
        return Intent::EngineState;
    if (hasAny(text, {"diagnostic", "diagnostique", "anormal", "anomalie", "panne", "probleme moteur", "probleme ecu"}))
        return Intent::Diagnostic;
    if (hasAny(text, {"batterie", "tension batterie", "voltage batterie"}))
        return Intent::Battery;
    if (hasAny(text, {"regime", "rpm", "tr/min"}))
        return Intent::Rpm;
    if (hasAny(text, {"temperature liquide", "temperature moteur", "liquide refroidissement", "ldr"}))
        return Intent::Coolant;
    if (hasAny(text, {"map", "pression collecteur", "pression admission"}))
        return Intent::Map;
    if (hasAny(text, {"lambda", "sonde o2", "sonde oxygene"}))
        return Intent::Lambda;
    if (hasAny(text, {"avance", "allumage"}))
        return Intent::Advance;
    if (hasAny(text, {"dwell", "temps bobine", "charge bobine"}))
        return Intent::Dwell;
    if (hasAny(text, {"ralenti", "idle"}))
        return Intent::Idle;
    if (hasAny(text, {"papillon", "tps"}))
        return Intent::Throttle;
    return Intent::None;
}

inline QString number(double value, int decimals)
{
    if (!std::isfinite(value))
        return QStringLiteral("—");
    return QString::number(value, 'f', decimals);
}

inline QString availabilityPrefix(bool connected, const QString &timestampText)
{
    if (connected)
        return QStringLiteral("Mesure ECU actuelle");
    if (!timestampText.trimmed().isEmpty())
        return QStringLiteral("Dernière mesure ECU disponible le %1").arg(timestampText.trimmed());
    return QStringLiteral("Dernière mesure ECU disponible");
}

inline QString metricAnswer(Intent intent,
                            const QHash<QString, double> &values,
                            bool connected,
                            const QString &timestampText = QString())
{
    if (values.isEmpty())
        return QStringLiteral("Je n'ai encore aucune mesure ECU disponible.");

    const QString prefix = availabilityPrefix(connected, timestampText);
    switch (intent) {
    case Intent::Battery:
        return QStringLiteral("%1 : tension batterie %2 V.")
            .arg(prefix, number(values.value(QStringLiteral("battery_v")), 1));
    case Intent::Rpm:
        return QStringLiteral("%1 : régime moteur %2 tr/min.")
            .arg(prefix, number(values.value(QStringLiteral("rpm")), 0));
    case Intent::Coolant:
        return QStringLiteral("%1 : température liquide de refroidissement %2 °C.")
            .arg(prefix, number(values.value(QStringLiteral("coolant_c")), 0));
    case Intent::Map:
        return QStringLiteral("%1 : pression collecteur MAP %2 kPa.")
            .arg(prefix, number(values.value(QStringLiteral("map_kpa")), 0));
    case Intent::Lambda:
        return QStringLiteral("%1 : tension sonde lambda %2 mV.")
            .arg(prefix, number(values.value(QStringLiteral("lambda_mv")), 0));
    case Intent::Advance:
        return QStringLiteral("%1 : avance à l'allumage %2°.")
            .arg(prefix, number(values.value(QStringLiteral("ignition_advance_deg")), 1));
    case Intent::Dwell:
        return QStringLiteral("%1 : temps de charge bobine (dwell) %2 ms.")
            .arg(prefix, number(values.value(QStringLiteral("coil_time_ms")), 3));
    case Intent::Idle:
        return QStringLiteral("%1 : position IAC %2 ; erreur de ralenti %3 ; erreur de ralenti chaud corrigée %4 ECU.")
            .arg(prefix,
                 number(values.value(QStringLiteral("iac_position")), 0),
                 number(values.value(QStringLiteral("idle_error_raw")), 0),
                 number(values.value(QStringLiteral("idle_error_hot_corrected")), 0));
    case Intent::Throttle:
        return QStringLiteral("%1 : position papillon brute %2 / 255.")
            .arg(prefix, number(values.value(QStringLiteral("throttle_pot_raw")), 0));
    default:
        return QString();
    }
}

inline QString engineStateAnswer(const QHash<QString, double> &values,
                                 bool connected,
                                 const QString &timestampText = QString())
{
    if (values.isEmpty())
        return QStringLiteral("Je n'ai encore aucune mesure ECU disponible pour décrire l'état du moteur.");

    const double rpm = values.value(QStringLiteral("rpm"));
    QStringList lines;
    lines << QStringLiteral("État moteur d'après %1 :")
                 .arg(availabilityPrefix(connected, timestampText).toLower());
    lines << (rpm > 0.5
        ? QStringLiteral("• Moteur en marche à %1 tr/min.").arg(number(rpm, 0))
        : QStringLiteral("• Moteur à l'arrêt selon le régime reçu : 0 tr/min."));
    lines << QStringLiteral("• Batterie : %1 V ; MAP : %2 kPa ; liquide : %3 °C.")
                 .arg(number(values.value(QStringLiteral("battery_v")), 1),
                      number(values.value(QStringLiteral("map_kpa")), 0),
                      number(values.value(QStringLiteral("coolant_c")), 0));
    lines << QStringLiteral("• Avance : %1° ; dwell bobine : %2 ms ; boucle fermée : %3.")
                 .arg(number(values.value(QStringLiteral("ignition_advance_deg")), 1),
                      number(values.value(QStringLiteral("coil_time_ms")), 3),
                      values.value(QStringLiteral("closed_loop")) != 0.0 ? QStringLiteral("oui") : QStringLiteral("non"));
    lines << (values.value(QStringLiteral("fault_mask")) == 0.0
        ? QStringLiteral("• Aucun bit défaut actif dans les champs DTC 7D/80 surveillés.")
        : QStringLiteral("• Des bits défaut sont actifs dans les champs DTC 7D/80 surveillés."));
    return lines.join(QLatin1Char('\n'));
}

inline QString diagnosticAnswer(const QHash<QString, double> &values,
                                bool connected,
                                const QString &timestampText = QString())
{
    if (values.isEmpty())
        return QStringLiteral("Je ne peux pas établir de diagnostic sans mesure ECU disponible.");

    QStringList findings;
    const double faultMask = values.value(QStringLiteral("fault_mask"));
    const double battery = values.value(QStringLiteral("battery_v"));
    const double dwell = values.value(QStringLiteral("coil_time_ms"));
    const double hotIdle = values.value(QStringLiteral("idle_error_hot_corrected"));

    findings << (faultMask != 0.0
        ? QStringLiteral("• Des bits défaut sont actifs dans les DTC 7D/80 surveillés.")
        : QStringLiteral("• Aucun bit défaut actif dans les DTC 7D/80 surveillés."));

    if (battery < 11.5)
        findings << QStringLiteral("• Batterie basse : %1 V.").arg(number(battery, 1));
    else if (battery > 15.2)
        findings << QStringLiteral("• Batterie élevée : %1 V.").arg(number(battery, 1));
    else
        findings << QStringLiteral("• Tension batterie cohérente : %1 V.").arg(number(battery, 1));

    if (battery >= 13.5 && battery <= 14.5) {
        if (dwell > 3.1)
            findings << QStringLiteral("• Dwell à surveiller : %1 ms, au-dessus de la plage 1,9–3,1 ms utilisée par MEMS Manager vers 14 V.")
                            .arg(number(dwell, 3));
        else if (dwell < 1.9)
            findings << QStringLiteral("• Dwell à surveiller : %1 ms, au-dessous de la plage 1,9–3,1 ms utilisée par MEMS Manager vers 14 V.")
                            .arg(number(dwell, 3));
        else
            findings << QStringLiteral("• Dwell dans la plage surveillée vers 14 V : %1 ms.").arg(number(dwell, 3));
    } else {
        findings << QStringLiteral("• Dwell mesuré : %1 ms ; contrôle automatique 1,9–3,1 ms non appliqué car la batterie n'est pas proche de 14 V.")
                        .arg(number(dwell, 3));
    }

    if (values.value(QStringLiteral("lambda_fault_active")) != 0.0)
        findings << QStringLiteral("• Défaut lambda actif détecté dans les bits surveillés.");
    if (values.value(QStringLiteral("tps_fault_active")) != 0.0)
        findings << QStringLiteral("• Défaut TPS actif détecté dans les bits surveillés.");
    if (std::fabs(hotIdle) > 15.0)
        findings << QStringLiteral("• Erreur de ralenti chaud corrigée à surveiller : %1 ECU.").arg(number(hotIdle, 0));

    QStringList lines;
    lines << QStringLiteral("Diagnostic direct à partir de %1 :")
                 .arg(availabilityPrefix(connected, timestampText).toLower());
    lines.append(findings);
    lines << QStringLiteral("Ces contrôles sont des vérifications de cohérence de MEMS Manager ; ils ne remplacent pas une spécification constructeur.");
    return lines.join(QLatin1Char('\n'));
}

inline QString capturesAnswer()
{
    return QStringLiteral(
        "Les captures de MEMS Manager sont des images de l'écran enregistrées par la fonction « Vue instantanée ». "
        "IA MEMS n'ouvre ni n'analyse automatiquement le contenu des fichiers image enregistrés. "
        "Je peux en revanche analyser les mesures ECU et leur historique lorsqu'ils sont disponibles dans la session, "
        "et l'onglet Analyse peut exploiter les journaux CSV/TXT pour étudier leur évolution.");
}

} // namespace IaResponseLogic

#endif // IA_RESPONSE_LOGIC_H
