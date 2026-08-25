#include "IaResponseLogic.h"

#include <QCoreApplication>
#include <QDebug>

namespace {

bool require(bool condition, const char *message)
{
    if (!condition)
        qCritical().noquote() << "IA RESPONSE SELFTEST FAIL:" << message;
    return condition;
}

}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    bool ok = true;

    ok &= require(IaResponseLogic::classify(QStringLiteral("TENSION DE LA BATTERIE?")) == IaResponseLogic::Intent::Battery,
                  "battery intent");
    ok &= require(IaResponseLogic::classify(QStringLiteral("ETAT MOTEUR?")) == IaResponseLogic::Intent::EngineState,
                  "engine state intent");
    ok &= require(IaResponseLogic::classify(QStringLiteral("DIAGNOSTIQUE?")) == IaResponseLogic::Intent::Diagnostic,
                  "diagnostique intent");
    ok &= require(IaResponseLogic::classify(QStringLiteral("QUE PEUX-TU ME DIRE SUR LES CAPTURES?")) == IaResponseLogic::Intent::Captures,
                  "captures intent");

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

    const QString captures = IaResponseLogic::capturesAnswer();
    ok &= require(captures.contains(QStringLiteral("n'ouvre ni n'analyse automatiquement")), "captures capability boundary");
    ok &= require(captures.contains(QStringLiteral("CSV/TXT")), "captures points to analysable logs");

    if (!ok)
        return 1;

    qInfo().noquote() << "PASS IA response intents and targeted answers";
    return 0;
}
