#include "IaResponseLogic.h"
#include "../database/MemsReferenceSheetRenderer.h"
#include "i18n.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QTemporaryFile>

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

    // A definition must not be mistaken for a request for a live ECU value.
    ok &= require(IaResponseLogic::classify(QStringLiteral("C'EST QUOI LA LAMBDA ?")) == IaResponseLogic::Intent::None,
                  "lambda definition is not a live measurement");
    ok &= require(IaResponseLogic::classify(QStringLiteral("A QUOI SERT LE TPS ?")) == IaResponseLogic::Intent::None,
                  "TPS definition is not a live measurement");
    ok &= require(IaResponseLogic::classify(QStringLiteral("C'EST QUOI LE DWELL ?")) == IaResponseLogic::Intent::None,
                  "dwell definition is not a live measurement");
    ok &= require(IaResponseLogic::classify(QStringLiteral("QUELLE EST LA VALEUR LAMBDA ?")) == IaResponseLogic::Intent::Lambda,
                  "lambda value request stays a live measurement");
    ok &= require(IaResponseLogic::classify(QStringLiteral("MESURE TPS ACTUELLE ?")) == IaResponseLogic::Intent::Throttle,
                  "TPS current measurement stays a live measurement");

    ok &= require(IaResponseLogic::classify(QStringLiteral("Broche MAP Mini MPi 1997")) == IaResponseLogic::Intent::None,
                  "MAP pinout is documentation, not a live MAP reading");
    ok &= require(IaResponseLogic::classify(QStringLiteral("Couleur des fils sonde lambda")) == IaResponseLogic::Intent::None,
                  "lambda wire colours are documentation, not a live lambda reading");
    ok &= require(IaResponseLogic::classify(QStringLiteral("Couple de serrage sonde température ECT")) == IaResponseLogic::Intent::None,
                  "ECT torque is documentation, not a live coolant reading");
    ok &= require(IaResponseLogic::classify(QStringLiteral("Valeur MAP ?")) == IaResponseLogic::Intent::Map,
                  "explicit MAP value request stays live");
    ok &= require(IaMemsConversationRouting::needsInductionClarification(QStringLiteral("Broche MAP Mini")),
                  "ambiguous Mini pinout requests SPi/MPi clarification");
    ok &= require(!IaMemsConversationRouting::needsInductionClarification(QStringLiteral("Broche MAP Mini MPi 1997")),
                  "explicit MPi does not ask again");
    ok &= require(IaMemsConversationRouting::isSearchDirective(QStringLiteral("cherche")),
                  "search directive is recognised");
    ok &= require(IaMemsConversationRouting::requestedGeneration(QStringLiteral("Je cherche la documentation MEMS 1.9")) == QStringLiteral("1.9"),
                  "documentation generation extraction");
    ok &= require(!IaMemsConversationRouting::shouldUseDiagnosticGeneration(
                      QStringLiteral("Couple de serrage sonde température ECT"),
                      QStringLiteral("Couple ECT : 15 Nm. Niveau de preuve : constructeur.")),
                  "documentary proof metadata does not trigger diagnostic generation");
    ok &= require(!IaMemsConversationRouting::shouldUseDiagnosticGeneration(
                      QStringLiteral("Couleur des fils sonde lambda"),
                      QStringLiteral("Fils lambda gris — preuve : constructeur.")),
                  "lambda documentary proof does not trigger diagnostic generation");
    ok &= require(IaMemsConversationRouting::shouldUseDiagnosticGeneration(
                      QStringLiteral("diagnostic moteur instable"),
                      QStringLiteral("Hypothèses actuelles : alimentation.")),
                  "real diagnostic still uses diagnostic generation");

    ok &= require(!IaMemsConversationRouting::explicitVariantLabelsCompatible(
                      QStringLiteral("Broche MAP Mini MPi 1997"),
                      QStringLiteral("RCL0194ENG gestion moteur MEMS SPi Japon 97MY")),
                  "explicit MPi rejects primary SPi Japan fact");
    ok &= require(IaMemsConversationRouting::explicitVariantLabelsCompatible(
                      QStringLiteral("Broche MAP Mini MPi 1997"),
                      QStringLiteral("RCL0194ENG capteur MAP MPi C159-8")),
                  "explicit MPi accepts primary MPi MAP fact");
    ok &= require(!IaMemsConversationRouting::explicitVariantLabelsCompatible(
                      QStringLiteral("Broche MAP Mini SPi Japan 1997"),
                      QStringLiteral("RCL0194ENG gestion moteur MEMS MPi Mini 97MY")),
                  "explicit SPi Japan rejects primary MPi fact");
    ok &= require(IaMemsConversationRouting::explicitVariantLabelsCompatible(
                      QStringLiteral("Broche MAP Mini SPi Japan 1997"),
                      QStringLiteral("RCL0194ENG gestion moteur MEMS SPi Japon 97MY")),
                  "explicit SPi Japan accepts matching primary fact");
    ok &= require(IaMemsConversationRouting::isKnowledgeContextQualifier(QStringLiteral("mini")),
                  "Mini is context and cannot make CKP/bobine relevant to MAP");
    ok &= require(!IaMemsConversationRouting::isKnowledgeContextQualifier(QStringLiteral("map")),
                  "MAP remains a required subject term");
    ok &= require(!IaMemsConversationRouting::hasDirectWireColourEvidence(
                      QStringLiteral("La sonde HO2S mesure l'oxygène et son chauffage est commandé par le relais HO2S.")),
                  "generic HO2S description is not wire-colour evidence");
    ok &= require(IaMemsConversationRouting::hasDirectWireColourEvidence(
                      QStringLiteral("Couleur de fil SPi Japon : fil BG noir/vert vers C159-36.")),
                  "actual relay wire colour is direct wire evidence");
    ok &= require(IaMemsConversationRouting::hasTorqueEvidence(
                      QStringLiteral("Serrer la sonde ECT à 15 Nm.")),
                  "ECT 15 Nm procedure is torque evidence");
    ok &= require(!IaMemsConversationRouting::hasTorqueEvidence(
                      QStringLiteral("L'ECT est une thermistance surveillée par l'ECM.")),
                  "generic ECT description is not torque evidence");
    ok &= require(IaMemsConversationRouting::knowledgeStatementSignature(QStringLiteral("  Même fait ECT. "))
                      == IaMemsConversationRouting::knowledgeStatementSignature(QStringLiteral("Même fait ECT.")),
                  "identical statements dedupe across family/source mirrors");
    ok &= require(IaMemsConversationRouting::knowledgeStatementSignature(
                      QStringLiteral("Capteur MAP vérifié.\nIllustration locale: RCL0194ENG:20.2"))
                      == IaMemsConversationRouting::knowledgeStatementSignature(QStringLiteral("Capteur MAP vérifié.")),
                  "illustration suffix does not defeat mirror deduplication");
    ok &= require(IaMemsConversationRouting::isMiniSpiMapPinoutQuestion(
                      QStringLiteral("Broche MAP Mini SPi Japan 1997")),
                  "Mini SPi MAP pinout gets the architecture fallback");
    ok &= require(!IaMemsConversationRouting::isMiniSpiMapPinoutQuestion(
                      QStringLiteral("Broche MAP Mini MPi 1997")),
                  "Mini MPi MAP pinout keeps the external MPi database path");
    const QString miniSpiMapAnswer = IaMemsConversationRouting::miniSpiMapPinoutAnswer();
    ok &= require(miniSpiMapAnswer.contains(QStringLiteral("intégré au calculateur"))
                      && miniSpiMapAnswer.contains(QStringLiteral("durite de dépression"))
                      && miniSpiMapAnswer.contains(QStringLiteral("pas de broche de signal MAP externe")),
                  "Mini SPi MAP fallback explains internal sensor without inventing a pin");
    ok &= require(IaMemsConversationRouting::wireColourRoleLabel(
                      QStringLiteral("Commande du relais de sonde oxygène par fil BG noir/vert"))
                      == QStringLiteral("Commande relais/chauffage"),
                  "lambda relay colour is labelled as relay/heater wiring");
    ok &= require(IaMemsConversationRouting::wireColourRoleLabel(
                      QStringLiteral("SCREEN GROUND de la sonde par fil B noir"))
                      == QStringLiteral("Blindage / masse écran"),
                  "lambda screen colour is labelled as shielding");

    ok &= require(IaMemsConversationRouting::inductionEvidenceProbes(
                      QStringLiteral("Broche MAP Mini"), false, QString()).isEmpty(),
                  "generic Mini while disconnected is not SPi/MPi evidence");
    const QStringList connectedProbes = IaMemsConversationRouting::inductionEvidenceProbes(
        QStringLiteral("Broche MAP Mini"), true, QStringLiteral("AANMP002"));
    ok &= require(connectedProbes.size() == 1
                      && connectedProbes.at(0) == QStringLiteral("AANMP002"),
                  "connected firmware is strong SPi/MPi evidence");
    ok &= require(IaMemsConversationRouting::inductionEvidenceProbes(
                      QStringLiteral("Broche MAP Mini MKC104341"), false, QString()).contains(QStringLiteral("MKC104341")),
                  "explicit ECU reference in the question is strong evidence");

    QTemporaryFile customSheet;
    ok &= require(customSheet.open(), "open temporary custom XML sheet");
    if (customSheet.isOpen()) {
        const QByteArray xml = QByteArrayLiteral(
            "<fiche><table><ligne><broche>BROCHE</broche><fonction>FONCTION</fonction><couleur>COULEUR</couleur></ligne>"
            "<ligne><broche>8</broche><fonction>MAP</fonction><couleur><svg><rect fill='#f2a5bc'/><path fill='#050505'/></svg><texte>Rose / Noir</texte></couleur></ligne></table></fiche>");
        customSheet.write(xml);
        customSheet.flush();
        const QString rendered = MemsReferenceSheetRenderer::renderFile(customSheet.fileName(), QStringLiteral("erreur"));
        ok &= require(rendered.contains(QStringLiteral("MAP")) && rendered.contains(QStringLiteral("Rose / Noir"))
                      && rendered.contains(QStringLiteral("#f2a5bc")) && rendered.contains(QStringLiteral("#050505"))
                      && rendered.contains(QStringLiteral("class='wirebox'"))
                      && rendered.contains(QStringLiteral("background:#555d64")),
                      "custom XML keeps colours inside grey wire enclosure");
    }

    QTemporaryFile genericSheet;
    ok &= require(genericSheet.open(), "open temporary generic XML sheet");
    if (genericSheet.isOpen()) {
        const QByteArray xml = QByteArrayLiteral(
            "<fiche><table><ligne><cellule>BROCHE</cellule><cellule>COULEUR</cellule></ligne>"
            "<ligne><cellule>7</cellule><cellule><svg><rect fill='#1769e8'/><path fill='#ed2224'/></svg><texte>Bleu / Rouge</texte></cellule></ligne></table></fiche>");
        genericSheet.write(xml);
        genericSheet.flush();
        const QString rendered = MemsReferenceSheetRenderer::renderFile(genericSheet.fileName(), QStringLiteral("erreur"));
        ok &= require(rendered.contains(QStringLiteral("Bleu / Rouge"))
                      && rendered.contains(QStringLiteral("#1769e8")) && rendered.contains(QStringLiteral("#ed2224")),
                      "generic cellule XML keeps embedded visual colours");
    }

    ok &= require(IaResponseLogic::classify(QStringLiteral("battery voltage?")) == IaResponseLogic::Intent::Battery,
                  "battery intent en");
    ok &= require(IaResponseLogic::classify(QStringLiteral("what is the battery voltage?")) == IaResponseLogic::Intent::None,
                  "english battery definition is not a live measurement");
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

    qInfo().noquote() << "PASS IA response intents, definition routing, multilingual answers and last-measurement timestamp";
    return 0;
}
