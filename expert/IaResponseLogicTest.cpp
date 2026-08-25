#include "IaResponseLogic.h"
#include "i18n.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>

#include <cstdio>

namespace {
QString g_testLanguage = QStringLiteral("fr");

bool require(bool condition, const char *message)
{
    if (!condition) {
        std::fprintf(stdout, "IA RESPONSE SELFTEST FAIL: %s\n", message);
        std::fflush(stdout);
    }
    return condition;
}
}

// The application links the real I18n implementation.  This small self-test
// only needs to control the active language deterministically.
QString I18n::language()
{
    return g_testLanguage;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    bool ok = true;

    g_testLanguage = QStringLiteral("fr");
    ok &= require(IaResponseLogic::classify(QStringLiteral("TENSION DE LA BATTERIE?")) == IaResponseLogic::Intent::Battery,
                  "battery intent fr");
    ok &= require(IaResponseLogic::classify(QStringLiteral("ETAT MOTEUR?")) == IaResponseLogic::Intent::EngineState,
                  "engine state intent fr");
    ok &= require(IaResponseLogic::classify(QStringLiteral("DIAGNOSTIQUE?")) == IaResponseLogic::Intent::Diagnostic,
                  "diagnostique intent fr");
    ok &= require(IaResponseLogic::classify(QStringLiteral("QUE PEUX-TU ME DIRE SUR LES CAPTURES?")) == IaResponseLogic::Intent::Captures,
                  "captures intent fr");

    ok &= require(IaResponseLogic::classify(QStringLiteral("battery voltage?")) == IaResponseLogic::Intent::Battery,
                  "battery intent en");
    ok &= require(IaResponseLogic::classify(QStringLiteral("estado del motor?")) == IaResponseLogic::Intent::EngineState,
                  "engine state intent es");
    ok &= require(IaResponseLogic::classify(QStringLiteral("diagnosi motore?")) == IaResponseLogic::Intent::Diagnostic,
                  "diagnostic intent it");
    ok &= require(IaResponseLogic::classify(QStringLiteral("tensão da bateria?")) == IaResponseLogic::Intent::Battery,
                  "battery intent pt");
    ok &= require(IaResponseLogic::classify(QStringLiteral("Batteriespannung?")) == IaResponseLogic::Intent::Battery,
                  "battery intent de");

    QHash<QString, double> values;
    values.insert(QStringLiteral("rpm"), 1199.0);
    values.insert(QStringLiteral("coolant_c"), 22.0);
    values.insert(QStringLiteral("map_kpa"), 33.0);
    values.insert(QStringLiteral("battery_v"), 13.8);
    values.insert(QStringLiteral("lambda_mv"), 750.0);
    values.insert(QStringLiteral("ignition_advance_deg"), 10.5);
    values.insert(QStringLiteral("coil_time_ms"), 3.252);
    values.insert(QStringLiteral("closed_loop"), 0.0);
    values.insert(QStringLiteral("fault_mask"), 0.0);
    values.insert(QStringLiteral("lambda_fault_active"), 0.0);
    values.insert(QStringLiteral("tps_fault_active"), 0.0);
    values.insert(QStringLiteral("idle_error_hot_corrected"), 7.0);
    values.insert(QStringLiteral("iac_position"), 99.0);
    values.insert(QStringLiteral("idle_error_raw"), 16.0);
    values.insert(QStringLiteral("throttle_pot_raw"), 36.0);

    g_testLanguage = QStringLiteral("fr");
    const QString battery = IaResponseLogic::metricAnswer(IaResponseLogic::Intent::Battery, values, true);
    ok &= require(battery.contains(QStringLiteral("13.8 V")), "battery answer value");
    ok &= require(!battery.contains(QStringLiteral("MAP")) && !battery.contains(QStringLiteral("Régime")),
                  "battery answer stays targeted");

    const QString state = IaResponseLogic::engineStateAnswer(values, true);
    ok &= require(state.contains(QStringLiteral("1199 tr/min")), "engine state rpm");
    ok &= require(state.contains(QStringLiteral("13.8 V")), "engine state battery");
    ok &= require(state.contains(QStringLiteral("33 kPa")), "engine state MAP");

    const QString diagnostic = IaResponseLogic::diagnosticAnswer(values, true);
    ok &= require(diagnostic.contains(QStringLiteral("3.252 ms")), "diagnostic dwell value");
    ok &= require(diagnostic.contains(QStringLiteral("au-dessus")), "diagnostic dwell warning");
    ok &= require(!diagnostic.contains(QStringLiteral("pas assez d'éléments")), "diagnostic is actionable");

    const QDateTime testTime(QDate(2026, 8, 25), QTime(13, 42, 17), Qt::LocalTime);
    IaResponseLogic::noteMeasurementTimestamp(testTime.toMSecsSinceEpoch());
    const QString oldBattery = IaResponseLogic::metricAnswer(IaResponseLogic::Intent::Battery, values, false);
    ok &= require(oldBattery.contains(QStringLiteral("25/08/2026 13:42:17")), "last measurement timestamp");

    const QString captures = IaResponseLogic::capturesAnswer();
    ok &= require(captures.contains(QStringLiteral("n'ouvre ni n'analyse automatiquement")), "captures capability boundary");
    ok &= require(captures.contains(QStringLiteral("CSV/TXT")), "captures points to analysable logs");

    g_testLanguage = QStringLiteral("en");
    const QString batteryEn = IaResponseLogic::metricAnswer(IaResponseLogic::Intent::Battery, values, true);
    ok &= require(batteryEn.contains(QStringLiteral("battery voltage")) && batteryEn.contains(QStringLiteral("13.8 V")),
                  "english targeted answer");

    g_testLanguage = QStringLiteral("es");
    const QString batteryEs = IaResponseLogic::metricAnswer(IaResponseLogic::Intent::Battery, values, true);
    ok &= require(batteryEs.contains(QStringLiteral("batería")) && batteryEs.contains(QStringLiteral("13.8 V")),
                  "spanish targeted answer");

    g_testLanguage = QStringLiteral("de");
    const QString batteryDe = IaResponseLogic::metricAnswer(IaResponseLogic::Intent::Battery, values, true);
    ok &= require(batteryDe.contains(QStringLiteral("Batteriespannung")) && batteryDe.contains(QStringLiteral("13.8 V")),
                  "german targeted answer");

    if (!ok)
        return 1;

    qInfo().noquote() << "PASS IA response intents, multilingual answers and last-measurement timestamp";
    return 0;
}
