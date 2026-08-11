#include "diagnosticpanel.h"

#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {
QString stateFor(bool ok, bool warn = false)
{
    return ok ? QStringLiteral("OK") : (warn ? QStringLiteral("SURVEILLER") : QStringLiteral("ANOMALIE"));
}
}

DiagnosticPanel::DiagnosticPanel(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *root = new QVBoxLayout(this);

    QHBoxLayout *top = new QHBoxLayout;
    m_status = new QLabel(QStringLiteral("En attente de données ECU"), this);
    m_score = new QLabel(QStringLiteral("Diagnostic : --"), this);
    QFont scoreFont = m_score->font();
    scoreFont.setBold(true);
    scoreFont.setPointSize(scoreFont.pointSize() + 2);
    m_score->setFont(scoreFont);
    top->addWidget(m_status, 1);
    top->addWidget(m_score);
    root->addLayout(top);

    QHBoxLayout *buttons = new QHBoxLayout;
    m_capture = new QPushButton(QStringLiteral("Capturer comme référence"), this);
    m_clear = new QPushButton(QStringLiteral("Effacer référence"), this);
    m_export = new QPushButton(QStringLiteral("Exporter le rapport"), this);
    buttons->addWidget(m_capture);
    buttons->addWidget(m_clear);
    buttons->addStretch();
    buttons->addWidget(m_export);
    root->addLayout(buttons);

    m_checks = new QTableWidget(0, 4, this);
    m_checks->setHorizontalHeaderLabels(QStringList()
        << QStringLiteral("Contrôle") << QStringLiteral("Valeur")
        << QStringLiteral("État") << QStringLiteral("Interprétation / action"));
    m_checks->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_checks->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_checks->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_checks->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_checks->verticalHeader()->setVisible(false);
    root->addWidget(m_checks, 3);

    QGroupBox *reportBox = new QGroupBox(QStringLiteral("Rapport automatique"), this);
    QVBoxLayout *reportLayout = new QVBoxLayout(reportBox);
    m_report = new QPlainTextEdit(reportBox);
    m_report->setReadOnly(true);
    reportLayout->addWidget(m_report);
    root->addWidget(reportBox, 2);

    connect(m_capture, &QPushButton::clicked, this, &DiagnosticPanel::captureReference);
    connect(m_clear, &QPushButton::clicked, this, &DiagnosticPanel::clearReference);
    connect(m_export, &QPushButton::clicked, this, &DiagnosticPanel::exportReport);
}

QString DiagnosticPanel::hexByte(quint8 v)
{
    return QStringLiteral("0x%1").arg(v, 2, 16, QLatin1Char('0')).toUpper();
}

void DiagnosticPanel::setEcuId(const QByteArray &id)
{
    m_ecuId = id;
    if (m_haveData)
        rebuild(m_last);
}

void DiagnosticPanel::updateData(const mems_data *data)
{
    if (!data)
        return;
    m_last = *data;
    m_haveData = true;
    rebuild(m_last);
}

void DiagnosticPanel::addCheck(const QString &name, const QString &value,
                               const QString &state, const QString &advice)
{
    int row = m_checks->rowCount();
    m_checks->insertRow(row);
    m_checks->setItem(row, 0, new QTableWidgetItem(name));
    m_checks->setItem(row, 1, new QTableWidgetItem(value));
    m_checks->setItem(row, 2, new QTableWidgetItem(state));
    m_checks->setItem(row, 3, new QTableWidgetItem(advice));
}

void DiagnosticPanel::rebuild(const mems_data &d)
{
    m_checks->setRowCount(0);
    int issues = 0;
    int warnings = 0;

    const bool hasDtc = (d.dtc0 != 0 || d.dtc1 != 0 || d.dtc2 != 0);
    addCheck(QStringLiteral("Défauts ECU"),
             QStringLiteral("DTC0=%1  DTC1=%2  DTC2=%3").arg(hexByte(d.dtc0), hexByte(d.dtc1), hexByte(d.dtc2)),
             hasDtc ? QStringLiteral("ANOMALIE") : QStringLiteral("OK"),
             hasDtc ? QStringLiteral("Lire les bits concernés avant tout effacement.")
                    : QStringLiteral("Aucun bit défaut actif dans les trames 0x7D/0x80."));
    if (hasDtc) ++issues;

    const double battery = d.battery_voltage / 10.0;
    const bool batteryOk = battery >= 11.5 && battery <= 15.2;
    addCheck(QStringLiteral("Tension batterie"), QStringLiteral("%1 V").arg(battery, 0, 'f', 1),
             stateFor(batteryOk, true),
             battery < 11.5 ? QStringLiteral("Tension basse : contrôler batterie, masses et alimentation ECU.")
                            : (battery > 15.2 ? QStringLiteral("Tension élevée : contrôler charge/régulateur.")
                                              : QStringLiteral("Plage cohérente pour un contrôle en cours de fonctionnement.")));
    if (!batteryOk) ++warnings;

    const double coolant = d.coolant_temp;
    const bool coolantPlausible = coolant > 0 && coolant < 250;
    addCheck(QStringLiteral("Température LDR brute"), QString::number(coolant),
             stateFor(coolantPlausible, true),
             coolantPlausible ? QStringLiteral("Valeur exploitable ; interpréter selon l'échelle MEMS du calculateur.")
                              : QStringLiteral("Valeur hors plage plausible : contrôler la sonde et son circuit."));
    if (!coolantPlausible) ++issues;

    const bool rpmPlausible = d.engine_rpm <= 8000;
    addCheck(QStringLiteral("Régime moteur"), QStringLiteral("%1 tr/min").arg(d.engine_rpm),
             stateFor(rpmPlausible, true),
             rpmPlausible ? QStringLiteral("Trame régime cohérente.")
                          : QStringLiteral("Valeur régime anormale : contrôler signal régime / câblage."));
    if (!rpmPlausible) ++issues;

    const bool mapPlausible = d.map_kpa <= 160;
    addCheck(QStringLiteral("MAP"), QStringLiteral("%1 kPa").arg(d.map_kpa),
             stateFor(mapPlausible, true),
             mapPlausible ? QStringLiteral("Valeur dans la plage décodée par MEMS.")
                          : QStringLiteral("Valeur hors plage décodée : contrôler mesure MAP."));
    if (!mapPlausible) ++issues;

    const bool tpsPlausible = d.throttle_pot <= 255;
    addCheck(QStringLiteral("TPS"), QStringLiteral("%1 / 255").arg(d.throttle_pot),
             stateFor(tpsPlausible),
             (d.dtc1 & 0x80) || (d.dtc2 & 0x01)
                 ? QStringLiteral("Un défaut TPS/alimentation TPS est signalé : contrôler alimentation, masse et progression du capteur.")
                 : QStringLiteral("Aucun défaut TPS connu actif dans les bits surveillés."));
    if ((d.dtc1 & 0x80) || (d.dtc2 & 0x01)) ++issues;

    const bool lambdaFault = (d.dtc2 & 0x04) || (d.dtc2 & 0x08);
    addCheck(QStringLiteral("Lambda"),
             QStringLiteral("U=%1  freq=%2  duty=%3  status=%4")
                 .arg(d.lambda_voltage).arg(d.lambda_sensor_frequency)
                 .arg(d.lambda_sensor_dutycycle).arg(d.lambda_sensor_status),
             lambdaFault ? QStringLiteral("ANOMALIE") : QStringLiteral("INFORMATION"),
             lambdaFault ? QStringLiteral("Défaut circuit/alimentation lambda : contrôler chauffage, alimentation et câblage.")
                         : (d.closed_loop ? QStringLiteral("Boucle fermée active : les corrections lambda sont en cours d'utilisation.")
                                          : QStringLiteral("Boucle fermée inactive à cet instant ; interpréter avec température et conditions moteur.")));
    if (lambdaFault) ++issues;

    const int fuelTrim = int(d.short_term_fuel_trim) - 100;
    const bool trimWatch = qAbs(fuelTrim) > 20 || qAbs(int(d.long_term_fuel_trim) - 100) > 20;
    addCheck(QStringLiteral("Corrections carburant"),
             QStringLiteral("court terme=%1  long terme=%2")
                 .arg(fuelTrim).arg(int(d.long_term_fuel_trim) - 100),
             stateFor(!trimWatch, true),
             trimWatch ? QStringLiteral("Correction importante : rechercher prise d'air, pression/carburant, injection ou mesure lambda avant de modifier les réglages.")
                       : QStringLiteral("Corrections sans écart important selon ce critère indicatif."));
    if (trimWatch) ++warnings;

    // Temps de charge de la bobine : contrôle spécifique demandé pour
    // une tension batterie proche de 14 V. Hors de cette tension, le
    // logiciel affiche la mesure mais ne porte pas de jugement automatique.
    const double coilTime = d.coil_time;
    const bool batteryNear14 = battery >= 13.5 && battery <= 14.5;
    const bool coilOk = coilTime >= 1.9 && coilTime <= 3.1;
    QString coilState;
    QString coilAdvice;
    if (!batteryNear14) {
        coilState = QStringLiteral("NON ÉVALUÉ");
        coilAdvice = QStringLiteral("Mesure affichée. Le contrôle 1,9–3,1 ms est appliqué automatiquement uniquement avec une tension batterie proche de 14 V.");
    } else if (coilOk) {
        coilState = QStringLiteral("OK");
        coilAdvice = QStringLiteral("Temps de charge dans la plage 1,9–3,1 ms à environ 14 V.");
    } else if (coilTime > 3.1) {
        coilState = QStringLiteral("ANOMALIE");
        coilAdvice = QStringLiteral("Temps de charge trop élevé à environ 14 V : contrôler en priorité le circuit primaire de la bobine, la bobine et son câblage.");
        ++issues;
    } else {
        coilState = QStringLiteral("SURVEILLER");
        coilAdvice = QStringLiteral("Temps de charge inférieur à 1,9 ms à environ 14 V : contrôler la mesure, l'alimentation et le circuit de commande avant conclusion.");
        ++warnings;
    }
    addCheck(QStringLiteral("Temps bobine"), QStringLiteral("%1 ms  | batterie %2 V").arg(coilTime, 0, 'f', 2).arg(battery, 0, 'f', 1),
             coilState, coilAdvice);

    // Décodage du champ 7D14-15. La correction de -3 n'est pas une constante :
    // elle correspond au réglage actuel « Position ralenti chaud » (idle_hot - 35).
    const int raw7d1415 = (static_cast<int>(d.idle_error2) << 8) | static_cast<int>(d.uk10);
    const int hotIdleCorrection = static_cast<int>(d.idle_hot) - 35;
    const int hotIdleErrorCorrected = (raw7d1415 - 32768) + hotIdleCorrection;
    const bool hotIdleErrorOk = qAbs(hotIdleErrorCorrected) <= 15;
    addCheck(QStringLiteral("Erreur ralenti à chaud"),
             QStringLiteral("%1 ECU | brut 7D14-15=%2 | correction=%3")
                 .arg(hotIdleErrorCorrected).arg(raw7d1415).arg(hotIdleCorrection),
             stateFor(hotIdleErrorOk, true),
             QStringLiteral("Décodage : (7D14-15 brut - 32768) + correction Position ralenti chaud. "
                            "La correction suit le réglage de l'onglet Réglages ; elle n'est pas codée en dur. "
                            "Valeur indicative cohérente si elle reste proche de zéro."));
    if (!hotIdleErrorOk) ++warnings;

    const bool iacSuspicious = (d.iac_position == 0 && d.idle_error >= 50 && d.idle_switch == 0 && d.uk3 != 0);
    addCheck(QStringLiteral("Commande de ralenti IAC"),
             QStringLiteral("position=%1  erreur=%2").arg(d.iac_position).arg(d.idle_error),
             iacSuspicious ? QStringLiteral("ANOMALIE") : QStringLiteral("OK"),
             iacSuspicious ? QStringLiteral("IAC en butée avec erreur de ralenti : contrôler moteur pas-à-pas, butée, prise d'air et réglage mécanique.")
                           : QStringLiteral("Aucune combinaison critique IAC/erreur détectée."));
    if (iacSuspicious) ++issues;

    if (m_reference.valid) {
        const int rpmDelta = int(d.engine_rpm) - int(m_reference.data.engine_rpm);
        const int mapDelta = int(d.map_kpa) - int(m_reference.data.map_kpa);
        const int tpsDelta = int(d.throttle_pot) - int(m_reference.data.throttle_pot);
        addCheck(QStringLiteral("Comparaison référence"),
                 QStringLiteral("ΔRPM=%1  ΔMAP=%2 kPa  ΔTPS=%3").arg(rpmDelta).arg(mapDelta).arg(tpsDelta),
                 QStringLiteral("INFO"),
                 QStringLiteral("Référence capturée le %1. Cette comparaison sert à repérer une dérive ; elle ne constitue pas une spécification constructeur.").arg(m_reference.timestamp));
    }

    const int total = issues * 2 + warnings;
    const QString level = issues == 0 ? (warnings == 0 ? QStringLiteral("NORMAL") : QStringLiteral("SURVEILLER"))
                                      : QStringLiteral("ANOMALIE À INVESTIGUER");
    m_score->setText(QStringLiteral("Diagnostic : %1  |  %2 anomalie(s), %3 avertissement(s)").arg(level).arg(issues).arg(warnings));
    m_status->setText(QStringLiteral("Dernière analyse : %1  |  score interne=%2")
                      .arg(QDateTime::currentDateTime().toString(QStringLiteral("dd/MM/yyyy hh:mm:ss"))).arg(total));
    m_report->setPlainText(buildReport());
}

void DiagnosticPanel::captureReference()
{
    if (!m_haveData) {
        QMessageBox::information(this, QStringLiteral("Diagnostic"), QStringLiteral("Aucune donnée ECU disponible."));
        return;
    }
    m_reference.valid = true;
    m_reference.data = m_last;
    m_reference.timestamp = QDateTime::currentDateTime().toString(QStringLiteral("dd/MM/yyyy hh:mm:ss"));
    rebuild(m_last);
}

void DiagnosticPanel::clearReference()
{
    m_reference = Snapshot();
    if (m_haveData) rebuild(m_last);
}

QString DiagnosticPanel::buildReport() const
{
    if (!m_haveData)
        return QStringLiteral("Aucune donnée ECU disponible.");

    const mems_data &d = m_last;
    QString text;
    text += QStringLiteral("ECU MEMS MANAGER - RAPPORT DE DIAGNOSTIC\n");
    text += QStringLiteral("Date : %1\n").arg(QDateTime::currentDateTime().toString(QStringLiteral("dd/MM/yyyy hh:mm:ss")));
    if (!m_ecuId.isEmpty()) text += QStringLiteral("Identification : %1\n").arg(QString::fromLatin1(m_ecuId.toHex(' ').toUpper()));
    text += QStringLiteral("DTC : %1 %2 %3\n").arg(hexByte(d.dtc0), hexByte(d.dtc1), hexByte(d.dtc2));
    const int raw7d1415 = (static_cast<int>(d.idle_error2) << 8) | static_cast<int>(d.uk10);
    const int hotIdleCorrection = static_cast<int>(d.idle_hot) - 35;
    const int hotIdleErrorCorrected = (raw7d1415 - 32768) + hotIdleCorrection;
    text += QStringLiteral("Temps bobine=%1 ms\n").arg(d.coil_time, 0, 'f', 2);
    text += QStringLiteral("7D14-15 brut=%1 | correction ralenti chaud=%2 | erreur ralenti chaud corrigée=%3 ECU\n")
        .arg(raw7d1415).arg(hotIdleCorrection).arg(hotIdleErrorCorrected);
    text += QStringLiteral("RPM=%1 | MAP=%2 kPa | batterie=%3 V | TPS=%4 | IAC=%5 | erreur ralenti=%6\n")
        .arg(d.engine_rpm).arg(d.map_kpa).arg(d.battery_voltage / 10.0, 0, 'f', 1)
        .arg(d.throttle_pot).arg(d.iac_position).arg(d.idle_error);
    text += QStringLiteral("LDR brut=%1 | air admission brut=%2 | lambda=%3 | boucle fermee=%4\n")
        .arg(d.coolant_temp).arg(d.intake_air_temp).arg(d.lambda_voltage).arg(d.closed_loop ? QStringLiteral("oui") : QStringLiteral("non"));
    if (m_reference.valid)
        text += QStringLiteral("Référence : %1\n").arg(m_reference.timestamp);
    text += QStringLiteral("\nLes états et conseils de ce rapport sont des contrôles de cohérence et ne remplacent pas les spécifications constructeur.\n");
    return text;
}

void DiagnosticPanel::exportReport()
{
    if (!m_haveData) {
        QMessageBox::information(this, QStringLiteral("Diagnostic"), QStringLiteral("Aucune donnée à exporter."));
        return;
    }
    const QString fileName = QFileDialog::getSaveFileName(this, QStringLiteral("Exporter le rapport"),
        QStringLiteral("diagnostic_mems_%1.txt").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss"))),
        QStringLiteral("Rapport texte (*.txt)"));
    if (fileName.isEmpty()) return;
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("Diagnostic"), QStringLiteral("Impossible d'écrire le rapport."));
        return;
    }
    file.write(m_report->toPlainText().toUtf8());
    file.close();
}
