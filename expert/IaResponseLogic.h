#ifndef IA_RESPONSE_LOGIC_H
#define IA_RESPONSE_LOGIC_H

#include "i18n.h"

#include <QDateTime>
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
    return result;
}

inline bool hasAny(const QString &text, std::initializer_list<const char*> words)
{
    for (const char *word : words) {
        if (text.contains(QString::fromUtf8(word)))
            return true;
    }
    return false;
}

inline QString languageCode()
{
    QString code = I18n::language().trimmed().toLower();
    if (code != QStringLiteral("fr") && code != QStringLiteral("en")
        && code != QStringLiteral("es") && code != QStringLiteral("it")
        && code != QStringLiteral("pt") && code != QStringLiteral("de"))
        code = QStringLiteral("fr");
    return code;
}

inline QString text6(const char *fr, const char *en, const char *es,
                     const char *it, const char *pt, const char *de)
{
    const QString lang = languageCode();
    if (lang == QStringLiteral("en")) return QString::fromUtf8(en);
    if (lang == QStringLiteral("es")) return QString::fromUtf8(es);
    if (lang == QStringLiteral("it")) return QString::fromUtf8(it);
    if (lang == QStringLiteral("pt")) return QString::fromUtf8(pt);
    if (lang == QStringLiteral("de")) return QString::fromUtf8(de);
    return QString::fromUtf8(fr);
}

inline qint64 &lastMeasurementTimestampStorage()
{
    static qint64 timestampMs = 0;
    return timestampMs;
}

inline void noteMeasurementTimestamp(qint64 timestampMs)
{
    if (timestampMs > 0)
        lastMeasurementTimestampStorage() = timestampMs;
}

inline QString lastMeasurementTimestampText()
{
    const qint64 timestampMs = lastMeasurementTimestampStorage();
    if (timestampMs <= 0)
        return QString();
    return QDateTime::fromMSecsSinceEpoch(timestampMs).toString(QStringLiteral("dd/MM/yyyy HH:mm:ss"));
}

inline Intent classify(const QString &question)
{
    const QString text = normalize(question);

    if (hasAny(text, {"capture", "captures", "screenshot", "screenshots", "captura", "capturas",
                      "schermata", "schermate", "captura de tela", "capturas de tela", "bildschirmfoto", "screenshots"}))
        return Intent::Captures;

    if (hasAny(text, {"etat moteur", "etat du moteur", "comment est le moteur", "moteur tourne",
                      "engine state", "engine status", "how is the engine", "motor state",
                      "estado motor", "estado del motor", "estado do motor", "stato motore", "stato del motore",
                      "motorzustand", "zustand des motors"}))
        return Intent::EngineState;

    if (hasAny(text, {"diagnostic", "diagnostique", "anormal", "anomalie", "panne", "probleme moteur", "probleme ecu",
                      "diagnosis", "diagnose", "fault", "engine problem", "ecu problem",
                      "diagnostico", "averia", "problema motor", "problema ecu",
                      "diagnosi", "guasto", "problema motore",
                      "diagnose", "fehler", "motorproblem", "steuergeraet problem"}))
        return Intent::Diagnostic;

    if (hasAny(text, {"batterie", "tension batterie", "voltage batterie", "battery", "battery voltage",
                      "bateria", "tension bateria", "tensao bateria", "voltagem bateria",
                      "batteria", "tensione batteria", "batteriespannung"}))
        return Intent::Battery;
    if (hasAny(text, {"regime", "rpm", "tr/min", "engine speed", "revoluciones", "revolucoes", "giri motore", "motordrehzahl"}))
        return Intent::Rpm;
    if (hasAny(text, {"temperature liquide", "temperature moteur", "liquide refroidissement", "ldr", "coolant", "coolant temperature",
                      "temperatura refrigerante", "temperatura liquido", "temperatura do refrigerante", "temperatura acqua", "kuehlmittel", "kuehlmitteltemperatur"}))
        return Intent::Coolant;
    if (hasAny(text, {"map", "pression collecteur", "pression admission", "manifold pressure", "presion colector", "pressao coletor", "pressione collettore", "saugrohrdruck"}))
        return Intent::Map;
    if (hasAny(text, {"lambda", "sonde o2", "sonde oxygene", "oxygen sensor", "o2 sensor", "sonda oxigeno", "sonda lambda", "lambdasonde"}))
        return Intent::Lambda;
    if (hasAny(text, {"avance", "allumage", "ignition advance", "timing advance", "avance encendido", "avanco ignicao", "anticipo accensione", "zuendwinkel"}))
        return Intent::Advance;
    if (hasAny(text, {"dwell", "temps bobine", "charge bobine", "coil time", "coil charge", "tiempo bobina", "tempo bobina", "tempo carica bobina", "spulenladezeit"}))
        return Intent::Dwell;
    if (hasAny(text, {"ralenti", "idle", "ralenti moteur", "ralenti chaud", "ralenti en caliente", "marcha lenta", "minimo", "leerlauf"}))
        return Intent::Idle;
    if (hasAny(text, {"papillon", "tps", "throttle", "mariposa", "borboleta", "farfalla", "drosselklappe"}))
        return Intent::Throttle;

    return Intent::None;
}

inline QString number(double value, int decimals)
{
    if (!std::isfinite(value))
        return QStringLiteral("—");
    return QString::number(value, 'f', decimals);
}

inline QString availabilityPrefix(bool connected, const QString &timestampText = QString())
{
    if (connected)
        return text6("Mesure ECU actuelle", "Current ECU measurement", "Medición ECU actual",
                     "Misura ECU attuale", "Medição ECU atual", "Aktueller ECU-Messwert");

    QString stamp = timestampText.trimmed();
    if (stamp.isEmpty())
        stamp = lastMeasurementTimestampText();
    if (!stamp.isEmpty()) {
        return text6("Dernière mesure ECU disponible le %1", "Last ECU measurement available on %1",
                     "Última medición ECU disponible el %1", "Ultima misura ECU disponibile il %1",
                     "Última medição ECU disponível em %1", "Letzter verfügbarer ECU-Messwert vom %1").arg(stamp);
    }
    return text6("Dernière mesure ECU disponible", "Last ECU measurement available", "Última medición ECU disponible",
                 "Ultima misura ECU disponibile", "Última medição ECU disponível", "Letzter verfügbarer ECU-Messwert");
}

inline QString metricAnswer(Intent intent,
                            const QHash<QString, double> &values,
                            bool connected,
                            const QString &timestampText = QString())
{
    if (values.isEmpty())
        return text6("Je n'ai encore aucune mesure ECU disponible.", "I do not have any ECU measurement available yet.",
                     "Todavía no tengo ninguna medición ECU disponible.", "Non ho ancora alcuna misura ECU disponibile.",
                     "Ainda não tenho nenhuma medição ECU disponível.", "Es ist noch kein ECU-Messwert verfügbar.");

    const QString prefix = availabilityPrefix(connected, timestampText);
    switch (intent) {
    case Intent::Battery:
        return text6("%1 : tension batterie %2 V.", "%1: battery voltage %2 V.", "%1: tensión de batería %2 V.",
                     "%1: tensione batteria %2 V.", "%1: tensão da bateria %2 V.", "%1: Batteriespannung %2 V.")
            .arg(prefix, number(values.value(QStringLiteral("battery_v")), 1));
    case Intent::Rpm:
        return text6("%1 : régime moteur %2 tr/min.", "%1: engine speed %2 rpm.", "%1: régimen motor %2 rpm.",
                     "%1: regime motore %2 giri/min.", "%1: rotação do motor %2 rpm.", "%1: Motordrehzahl %2 U/min.")
            .arg(prefix, number(values.value(QStringLiteral("rpm")), 0));
    case Intent::Coolant:
        return text6("%1 : température liquide de refroidissement %2 °C.", "%1: coolant temperature %2 °C.",
                     "%1: temperatura del refrigerante %2 °C.", "%1: temperatura liquido di raffreddamento %2 °C.",
                     "%1: temperatura do líquido de arrefecimento %2 °C.", "%1: Kühlmitteltemperatur %2 °C.")
            .arg(prefix, number(values.value(QStringLiteral("coolant_c")), 0));
    case Intent::Map:
        return text6("%1 : pression collecteur MAP %2 kPa.", "%1: MAP manifold pressure %2 kPa.",
                     "%1: presión MAP del colector %2 kPa.", "%1: pressione collettore MAP %2 kPa.",
                     "%1: pressão MAP do coletor %2 kPa.", "%1: MAP-Saugrohrdruck %2 kPa.")
            .arg(prefix, number(values.value(QStringLiteral("map_kpa")), 0));
    case Intent::Lambda:
        return text6("%1 : tension sonde lambda %2 mV.", "%1: lambda sensor voltage %2 mV.",
                     "%1: tensión de la sonda lambda %2 mV.", "%1: tensione sonda lambda %2 mV.",
                     "%1: tensão da sonda lambda %2 mV.", "%1: Lambdasondenspannung %2 mV.")
            .arg(prefix, number(values.value(QStringLiteral("lambda_mv")), 0));
    case Intent::Advance:
        return text6("%1 : avance à l'allumage %2°.", "%1: ignition advance %2°.", "%1: avance de encendido %2°.",
                     "%1: anticipo accensione %2°.", "%1: avanço de ignição %2°.", "%1: Zündwinkel %2°.")
            .arg(prefix, number(values.value(QStringLiteral("ignition_advance_deg")), 1));
    case Intent::Dwell:
        return text6("%1 : temps de charge bobine (dwell) %2 ms.", "%1: coil dwell time %2 ms.",
                     "%1: tiempo de carga de bobina (dwell) %2 ms.", "%1: tempo di carica bobina (dwell) %2 ms.",
                     "%1: tempo de carga da bobina (dwell) %2 ms.", "%1: Spulenladezeit (Dwell) %2 ms.")
            .arg(prefix, number(values.value(QStringLiteral("coil_time_ms")), 3));
    case Intent::Idle:
        return text6("%1 : position IAC %2 ; erreur de ralenti %3 ; erreur de ralenti chaud corrigée %4 ECU.",
                     "%1: IAC position %2; idle error %3; corrected hot-idle error %4 ECU.",
                     "%1: posición IAC %2; error de ralentí %3; error de ralentí caliente corregido %4 ECU.",
                     "%1: posizione IAC %2; errore minimo %3; errore minimo a caldo corretto %4 ECU.",
                     "%1: posição IAC %2; erro de marcha lenta %3; erro de marcha lenta quente corrigido %4 ECU.",
                     "%1: IAC-Position %2; Leerlauffehler %3; korrigierter Warmlauf-Leerlauffehler %4 ECU.")
            .arg(prefix,
                 number(values.value(QStringLiteral("iac_position")), 0),
                 number(values.value(QStringLiteral("idle_error_raw")), 0),
                 number(values.value(QStringLiteral("idle_error_hot_corrected")), 0));
    case Intent::Throttle:
        return text6("%1 : position papillon brute %2 / 255.", "%1: raw throttle position %2 / 255.",
                     "%1: posición bruta de mariposa %2 / 255.", "%1: posizione farfalla grezza %2 / 255.",
                     "%1: posição bruta da borboleta %2 / 255.", "%1: rohe Drosselklappenposition %2 / 255.")
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
        return text6("Je n'ai encore aucune mesure ECU disponible pour décrire l'état du moteur.",
                     "I do not have any ECU measurement available yet to describe the engine state.",
                     "Todavía no tengo ninguna medición ECU disponible para describir el estado del motor.",
                     "Non ho ancora misure ECU disponibili per descrivere lo stato del motore.",
                     "Ainda não tenho medições ECU disponíveis para descrever o estado do motor.",
                     "Es sind noch keine ECU-Messwerte verfügbar, um den Motorzustand zu beschreiben.");

    const double rpm = values.value(QStringLiteral("rpm"));
    QStringList lines;
    lines << text6("État moteur d'après %1 :", "Engine state from %1:", "Estado del motor según %1:",
                   "Stato motore da %1:", "Estado do motor com base em %1:", "Motorzustand anhand von %1:")
                 .arg(availabilityPrefix(connected, timestampText).toLower());
    lines << (rpm > 0.5
        ? text6("• Moteur en marche à %1 tr/min.", "• Engine running at %1 rpm.", "• Motor en marcha a %1 rpm.",
                "• Motore in funzione a %1 giri/min.", "• Motor em funcionamento a %1 rpm.", "• Motor läuft mit %1 U/min.").arg(number(rpm, 0))
        : text6("• Moteur à l'arrêt selon le régime reçu : 0 tr/min.", "• Engine stopped according to received speed: 0 rpm.",
                "• Motor parado según el régimen recibido: 0 rpm.", "• Motore fermo secondo il regime ricevuto: 0 giri/min.",
                "• Motor parado segundo a rotação recebida: 0 rpm.", "• Motor steht laut empfangener Drehzahl: 0 U/min."));
    lines << text6("• Batterie : %1 V ; MAP : %2 kPa ; liquide : %3 °C.", "• Battery: %1 V; MAP: %2 kPa; coolant: %3 °C.",
                   "• Batería: %1 V; MAP: %2 kPa; refrigerante: %3 °C.", "• Batteria: %1 V; MAP: %2 kPa; liquido: %3 °C.",
                   "• Bateria: %1 V; MAP: %2 kPa; líquido: %3 °C.", "• Batterie: %1 V; MAP: %2 kPa; Kühlmittel: %3 °C.")
                 .arg(number(values.value(QStringLiteral("battery_v")), 1),
                      number(values.value(QStringLiteral("map_kpa")), 0),
                      number(values.value(QStringLiteral("coolant_c")), 0));
    lines << text6("• Avance : %1° ; dwell bobine : %2 ms ; boucle fermée : %3.",
                   "• Ignition advance: %1°; coil dwell: %2 ms; closed loop: %3.",
                   "• Avance: %1°; dwell bobina: %2 ms; bucle cerrado: %3.",
                   "• Anticipo: %1°; dwell bobina: %2 ms; anello chiuso: %3.",
                   "• Avanço: %1°; dwell bobina: %2 ms; malha fechada: %3.",
                   "• Zündwinkel: %1°; Spulen-Dwell: %2 ms; geschlossener Regelkreis: %3.")
                 .arg(number(values.value(QStringLiteral("ignition_advance_deg")), 1),
                      number(values.value(QStringLiteral("coil_time_ms")), 3),
                      values.value(QStringLiteral("closed_loop")) != 0.0
                          ? text6("oui", "yes", "sí", "sì", "sim", "ja")
                          : text6("non", "no", "no", "no", "não", "nein"));
    lines << (values.value(QStringLiteral("fault_mask")) == 0.0
        ? text6("• Aucun bit défaut actif dans les champs DTC 7D/80 surveillés.",
                "• No active fault bit in the monitored 7D/80 DTC fields.",
                "• Ningún bit de fallo activo en los campos DTC 7D/80 supervisados.",
                "• Nessun bit guasto attivo nei campi DTC 7D/80 monitorati.",
                "• Nenhum bit de falha ativo nos campos DTC 7D/80 monitorados.",
                "• Kein aktives Fehlerbit in den überwachten 7D/80-DTC-Feldern.")
        : text6("• Des bits défaut sont actifs dans les champs DTC 7D/80 surveillés.",
                "• Fault bits are active in the monitored 7D/80 DTC fields.",
                "• Hay bits de fallo activos en los campos DTC 7D/80 supervisados.",
                "• Sono attivi bit guasto nei campi DTC 7D/80 monitorati.",
                "• Existem bits de falha ativos nos campos DTC 7D/80 monitorados.",
                "• In den überwachten 7D/80-DTC-Feldern sind Fehlerbits aktiv."));
    return lines.join(QLatin1Char('\n'));
}

inline QString diagnosticAnswer(const QHash<QString, double> &values,
                                bool connected,
                                const QString &timestampText = QString())
{
    if (values.isEmpty())
        return text6("Je ne peux pas établir de diagnostic sans mesure ECU disponible.",
                     "I cannot establish a diagnostic without an available ECU measurement.",
                     "No puedo establecer un diagnóstico sin una medición ECU disponible.",
                     "Non posso formulare una diagnosi senza una misura ECU disponibile.",
                     "Não posso estabelecer um diagnóstico sem uma medição ECU disponível.",
                     "Ohne verfügbaren ECU-Messwert kann ich keine Diagnose erstellen.");

    QStringList findings;
    const double faultMask = values.value(QStringLiteral("fault_mask"));
    const double battery = values.value(QStringLiteral("battery_v"));
    const double dwell = values.value(QStringLiteral("coil_time_ms"));
    const double hotIdle = values.value(QStringLiteral("idle_error_hot_corrected"));

    findings << (faultMask != 0.0
        ? text6("• Des bits défaut sont actifs dans les DTC 7D/80 surveillés.", "• Fault bits are active in the monitored 7D/80 DTC fields.",
                "• Hay bits de fallo activos en los DTC 7D/80 supervisados.", "• Sono attivi bit guasto nei DTC 7D/80 monitorati.",
                "• Existem bits de falha ativos nos DTC 7D/80 monitorados.", "• In den überwachten 7D/80-DTC-Feldern sind Fehlerbits aktiv.")
        : text6("• Aucun bit défaut actif dans les DTC 7D/80 surveillés.", "• No active fault bit in the monitored 7D/80 DTC fields.",
                "• Ningún bit de fallo activo en los DTC 7D/80 supervisados.", "• Nessun bit guasto attivo nei DTC 7D/80 monitorati.",
                "• Nenhum bit de falha ativo nos DTC 7D/80 monitorados.", "• Kein aktives Fehlerbit in den überwachten 7D/80-DTC-Feldern."));

    if (battery < 11.5)
        findings << text6("• Batterie basse : %1 V.", "• Low battery: %1 V.", "• Batería baja: %1 V.",
                          "• Batteria bassa: %1 V.", "• Bateria baixa: %1 V.", "• Batteriespannung niedrig: %1 V.").arg(number(battery, 1));
    else if (battery > 15.2)
        findings << text6("• Batterie élevée : %1 V.", "• High battery voltage: %1 V.", "• Tensión de batería alta: %1 V.",
                          "• Tensione batteria alta: %1 V.", "• Tensão da bateria alta: %1 V.", "• Batteriespannung hoch: %1 V.").arg(number(battery, 1));
    else
        findings << text6("• Tension batterie cohérente : %1 V.", "• Battery voltage coherent: %1 V.", "• Tensión de batería coherente: %1 V.",
                          "• Tensione batteria coerente: %1 V.", "• Tensão da bateria coerente: %1 V.", "• Batteriespannung plausibel: %1 V.").arg(number(battery, 1));

    if (battery >= 13.5 && battery <= 14.5) {
        if (dwell > 3.1)
            findings << text6("• Dwell à surveiller : %1 ms, au-dessus de la plage 1,9–3,1 ms utilisée par MEMS Manager vers 14 V.",
                              "• Dwell to watch: %1 ms, above the 1.9–3.1 ms range used by MEMS Manager around 14 V.",
                              "• Dwell a vigilar: %1 ms, por encima del rango 1,9–3,1 ms usado por MEMS Manager cerca de 14 V.",
                              "• Dwell da controllare: %1 ms, sopra l'intervallo 1,9–3,1 ms usato da MEMS Manager intorno a 14 V.",
                              "• Dwell a observar: %1 ms, acima da faixa 1,9–3,1 ms usada pelo MEMS Manager perto de 14 V.",
                              "• Dwell beobachten: %1 ms, über dem von MEMS Manager bei etwa 14 V verwendeten Bereich 1,9–3,1 ms.").arg(number(dwell, 3));
        else if (dwell < 1.9)
            findings << text6("• Dwell à surveiller : %1 ms, au-dessous de la plage 1,9–3,1 ms utilisée par MEMS Manager vers 14 V.",
                              "• Dwell to watch: %1 ms, below the 1.9–3.1 ms range used by MEMS Manager around 14 V.",
                              "• Dwell a vigilar: %1 ms, por debajo del rango 1,9–3,1 ms usado por MEMS Manager cerca de 14 V.",
                              "• Dwell da controllare: %1 ms, sotto l'intervallo 1,9–3,1 ms usato da MEMS Manager intorno a 14 V.",
                              "• Dwell a observar: %1 ms, abaixo da faixa 1,9–3,1 ms usada pelo MEMS Manager perto de 14 V.",
                              "• Dwell beobachten: %1 ms, unter dem von MEMS Manager bei etwa 14 V verwendeten Bereich 1,9–3,1 ms.").arg(number(dwell, 3));
        else
            findings << text6("• Dwell dans la plage surveillée vers 14 V : %1 ms.", "• Dwell within the monitored range around 14 V: %1 ms.",
                              "• Dwell dentro del rango supervisado cerca de 14 V: %1 ms.", "• Dwell nell'intervallo monitorato intorno a 14 V: %1 ms.",
                              "• Dwell dentro da faixa monitorada perto de 14 V: %1 ms.", "• Dwell im überwachten Bereich bei etwa 14 V: %1 ms.").arg(number(dwell, 3));
    } else {
        findings << text6("• Dwell mesuré : %1 ms ; contrôle automatique 1,9–3,1 ms non appliqué car la batterie n'est pas proche de 14 V.",
                          "• Measured dwell: %1 ms; the automatic 1.9–3.1 ms check is not applied because battery voltage is not close to 14 V.",
                          "• Dwell medido: %1 ms; no se aplica el control automático 1,9–3,1 ms porque la batería no está cerca de 14 V.",
                          "• Dwell misurato: %1 ms; il controllo automatico 1,9–3,1 ms non viene applicato perché la batteria non è vicina a 14 V.",
                          "• Dwell medido: %1 ms; o controle automático 1,9–3,1 ms não é aplicado porque a bateria não está próxima de 14 V.",
                          "• Gemessener Dwell: %1 ms; die automatische Prüfung 1,9–3,1 ms wird nicht angewendet, da die Batteriespannung nicht nahe 14 V liegt.").arg(number(dwell, 3));
    }

    if (values.value(QStringLiteral("lambda_fault_active")) != 0.0)
        findings << text6("• Défaut lambda actif détecté dans les bits surveillés.", "• Active lambda fault detected in the monitored bits.",
                          "• Fallo lambda activo detectado en los bits supervisados.", "• Guasto lambda attivo rilevato nei bit monitorati.",
                          "• Falha lambda ativa detectada nos bits monitorados.", "• Aktiver Lambdafehler in den überwachten Bits erkannt.");
    if (values.value(QStringLiteral("tps_fault_active")) != 0.0)
        findings << text6("• Défaut TPS actif détecté dans les bits surveillés.", "• Active TPS fault detected in the monitored bits.",
                          "• Fallo TPS activo detectado en los bits supervisados.", "• Guasto TPS attivo rilevato nei bit monitorati.",
                          "• Falha TPS ativa detectada nos bits monitorados.", "• Aktiver TPS-Fehler in den überwachten Bits erkannt.");
    if (std::fabs(hotIdle) > 15.0)
        findings << text6("• Erreur de ralenti chaud corrigée à surveiller : %1 ECU.", "• Corrected hot-idle error to watch: %1 ECU.",
                          "• Error de ralentí caliente corregido a vigilar: %1 ECU.", "• Errore minimo a caldo corretto da controllare: %1 ECU.",
                          "• Erro de marcha lenta quente corrigido a observar: %1 ECU.", "• Korrigierter Warmlauf-Leerlauffehler beobachten: %1 ECU.").arg(number(hotIdle, 0));

    QStringList lines;
    lines << text6("Diagnostic direct à partir de %1 :", "Direct diagnostic from %1:", "Diagnóstico directo a partir de %1:",
                   "Diagnosi diretta da %1:", "Diagnóstico direto a partir de %1:", "Direkte Diagnose anhand von %1:")
                 .arg(availabilityPrefix(connected, timestampText).toLower());
    lines.append(findings);
    lines << text6("Ces contrôles sont des vérifications de cohérence de MEMS Manager ; ils ne remplacent pas une spécification constructeur.",
                   "These checks are MEMS Manager consistency checks; they do not replace a manufacturer specification.",
                   "Estos controles son verificaciones de coherencia de MEMS Manager; no sustituyen una especificación del fabricante.",
                   "Questi controlli sono verifiche di coerenza di MEMS Manager; non sostituiscono una specifica del costruttore.",
                   "Estes controles são verificações de coerência do MEMS Manager; não substituem uma especificação do fabricante.",
                   "Diese Prüfungen sind Plausibilitätsprüfungen von MEMS Manager; sie ersetzen keine Herstellerspezifikation.");
    return lines.join(QLatin1Char('\n'));
}

inline QString capturesAnswer()
{
    return text6(
        "Les captures de MEMS Manager sont des images de l'écran enregistrées par la fonction « Vue instantanée ». IA MEMS n'ouvre ni n'analyse automatiquement le contenu des fichiers image enregistrés. Je peux en revanche analyser les mesures ECU et leur historique lorsqu'ils sont disponibles dans la session, et l'onglet Analyse peut exploiter les journaux CSV/TXT pour étudier leur évolution.",
        "MEMS Manager captures are screen images saved by the snapshot function. IA MEMS does not automatically open or analyse the contents of saved image files. I can analyse ECU measurements and their history when they are available in the session, and the Analysis tab can use CSV/TXT logs to study their evolution.",
        "Las capturas de MEMS Manager son imágenes de pantalla guardadas por la función de instantánea. IA MEMS no abre ni analiza automáticamente el contenido de los archivos de imagen guardados. Puedo analizar las mediciones ECU y su historial cuando están disponibles en la sesión, y la pestaña Análisis puede utilizar registros CSV/TXT para estudiar su evolución.",
        "Le catture di MEMS Manager sono immagini dello schermo salvate dalla funzione istantanea. IA MEMS non apre né analizza automaticamente il contenuto dei file immagine salvati. Posso analizzare le misure ECU e il loro storico quando sono disponibili nella sessione, e la scheda Analisi può utilizzare i log CSV/TXT per studiarne l'evoluzione.",
        "As capturas do MEMS Manager são imagens do ecrã guardadas pela função de instantâneo. A IA MEMS não abre nem analisa automaticamente o conteúdo dos ficheiros de imagem guardados. Posso analisar as medições ECU e o respetivo histórico quando estão disponíveis na sessão, e o separador Análise pode utilizar registos CSV/TXT para estudar a sua evolução.",
        "MEMS-Manager-Aufnahmen sind Bildschirmbilder, die mit der Schnappschussfunktion gespeichert werden. IA MEMS öffnet oder analysiert gespeicherte Bilddateien nicht automatisch. Ich kann ECU-Messwerte und deren Verlauf analysieren, wenn sie in der Sitzung verfügbar sind, und die Registerkarte Analyse kann CSV/TXT-Protokolle zur Verlaufsauswertung verwenden.");
}

} // namespace IaResponseLogic

#endif // IA_RESPONSE_LOGIC_H