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
#include <QObject>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {
QString stateFor(bool ok, bool warn = false)
{
    return ok ? QObject::tr("OK") : (warn ? QObject::tr("SURVEILLER") : QObject::tr("ANOMALIE"));
}
}

DiagnosticPanel::DiagnosticPanel(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *root = new QVBoxLayout(this);

    QHBoxLayout *top = new QHBoxLayout;
    m_status = new QLabel(tr("En attente de données ECU"), this);
    m_score = new QLabel(tr("Diagnostic : --"), this);
    QFont scoreFont = m_score->font();
    scoreFont.setBold(true);
    scoreFont.setPointSize(scoreFont.pointSize() + 2);
    m_score->setFont(scoreFont);
    top->addWidget(m_status, 1);
    top->addWidget(m_score);
    root->addLayout(top);

    QHBoxLayout *buttons = new QHBoxLayout;
    m_capture = new QPushButton(tr("Capturer comme référence"), this);
    m_clear = new QPushButton(tr("Effacer référence"), this);
    m_export = new QPushButton(tr("Exporter le rapport"), this);
    buttons->addWidget(m_capture);
    buttons->addWidget(m_clear);
    buttons->addStretch();
    buttons->addWidget(m_export);
    root->addLayout(buttons);

    m_checks = new QTableWidget(0, 4, this);
    m_checks->setHorizontalHeaderLabels(QStringList()
        << tr("Contrôle") << tr("Valeur")
        << tr("État") << tr("Interprétation / action"));
    m_checks->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_checks->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_checks->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_checks->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_checks->verticalHeader()->setVisible(false);
    root->addWidget(m_checks, 3);

    QGroupBox *reportBox = new QGroupBox(tr("Rapport automatique"), this);
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
    addCheck(tr("Défauts ECU"),
             tr("DTC0=%1  DTC1=%2  DTC2=%3").arg(hexByte(d.dtc0), hexByte(d.dtc1), hexByte(d.dtc2)),
             hasDtc ? tr("ANOMALIE") : tr("OK"),
             hasDtc ? tr("Lire les bits concernés avant tout effacement.")
                    : tr("Aucun bit défaut actif dans les trames 0x7D/0x80."));
    if (hasDtc) ++issues;

    const double battery = d.battery_voltage / 10.0;
    const bool batteryOk = battery >= 11.5 && battery <= 15.2;
    addCheck(tr("Tension batterie"), tr("%1 V").arg(battery, 0, 'f', 1),
             stateFor(batteryOk, true),
             battery < 11.5 ? tr("Tension basse : contrôler batterie, masses et alimentation ECU.")
                            : (battery > 15.2 ? tr("Tension élevée : contrôler charge/régulateur.")
                                              : tr("Plage cohérente pour un contrôle en cours de fonctionnement.")));
    if (!batteryOk) ++warnings;

    const double coolant = d.coolant_temp;
    const bool coolantPlausible = coolant > 0 && coolant < 250;
    addCheck(tr("Température LDR brute"), QString::number(coolant),
             stateFor(coolantPlausible, true),
             coolantPlausible ? tr("Valeur exploitable ; interpréter selon l'échelle MEMS du calculateur.")
                              : tr("Valeur hors plage plausible : contrôler la sonde et son circuit."));
    if (!coolantPlausible) ++issues;

    const bool rpmPlausible = d.engine_rpm <= 8000;
    addCheck(tr("Régime moteur"), tr("%1 tr/min").arg(d.engine_rpm),
             stateFor(rpmPlausible, true),
             rpmPlausible ? tr("Trame régime cohérente.")
                          : tr("Valeur régime anormale : contrôler signal régime / câblage."));
    if (!rpmPlausible) ++issues;

    const bool mapPlausible = d.map_kpa <= 160;
    addCheck(tr("MAP"), tr("%1 kPa").arg(d.map_kpa),
             stateFor(mapPlausible, true),
             mapPlausible ? tr("Valeur dans la plage décodée par MEMS.")
                          : tr("Valeur hors plage décodée : contrôler mesure MAP."));
    if (!mapPlausible) ++issues;

    const bool tpsPlausible = d.throttle_pot <= 255;
    addCheck(tr("TPS"), QStringLiteral("%1 / 255").arg(d.throttle_pot),
             stateFor(tpsPlausible),
             (d.dtc1 & 0x80) || (d.dtc2 & 0x01)
                 ? tr("Un défaut TPS/alimentation TPS est signalé : contrôler alimentation, masse et progression du capteur.")
                 : tr("Aucun défaut TPS connu actif dans les bits surveillés."));
    if ((d.dtc1 & 0x80) || (d.dtc2 & 0x01)) ++issues;

    const bool lambdaFault = (d.dtc2 & 0x04) || (d.dtc2 & 0x08);
    addCheck(tr("Lambda"),
             tr("U=%1  freq=%2  duty=%3  status=%4")
                 .arg(d.lambda_voltage).arg(d.lambda_sensor_frequency)
                 .arg(d.lambda_sensor_dutycycle).arg(d.lambda_sensor_status),
             lambdaFault ? tr("ANOMALIE") : tr("INFORMATION"),
             lambdaFault ? tr("Défaut circuit/alimentation lambda : contrôler chauffage, alimentation et câblage.")
                         : (d.closed_loop ? tr("Boucle fermée active : les corrections lambda sont en cours d'utilisation.")
                                          : tr("Boucle fermée inactive à cet instant ; interpréter avec température et conditions moteur.")));
    if (lambdaFault) ++issues;

    const int fuelTrim = int(d.short_term_fuel_trim) - 100;
    const bool trimWatch = qAbs(fuelTrim) > 20 || qAbs(int(d.long_term_fuel_trim) - 100) > 20;
    addCheck(tr("Corrections carburant"),
             tr("court terme=%1  long terme=%2")
                 .arg(fuelTrim).arg(int(d.long_term_fuel_trim) - 100),
             stateFor(!trimWatch, true),
             trimWatch ? tr("Correction importante : rechercher prise d'air, pression/carburant, injection ou mesure lambda avant de modifier les réglages.")
                       : tr("Corrections sans écart important selon ce critère indicatif."));
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
        coilState = tr("NON ÉVALUÉ");
        coilAdvice = tr("Mesure affichée. Le contrôle 1,9–3,1 ms est appliqué automatiquement uniquement avec une tension batterie proche de 14 V.");
    } else if (coilOk) {
        coilState = tr("OK");
        coilAdvice = tr("Temps de charge dans la plage 1,9–3,1 ms à environ 14 V.");
    } else if (coilTime > 3.1) {
        coilState = tr("ANOMALIE");
        coilAdvice = tr("Temps de charge trop élevé à environ 14 V : contrôler en priorité le circuit primaire de la bobine, la bobine et son câblage.");
        ++issues;
    } else {
        coilState = tr("SURVEILLER");
        coilAdvice = tr("Temps de charge inférieur à 1,9 ms à environ 14 V : contrôler la mesure, l'alimentation et le circuit de commande avant conclusion.");
        ++warnings;
    }
    addCheck(tr("Temps bobine"), tr("%1 ms  | batterie %2 V").arg(coilTime, 0, 'f', 2).arg(battery, 0, 'f', 1),
             coilState, coilAdvice);

    // Décodage du champ 7D14-15. La correction de -3 n'est pas une constante :
    // elle correspond au réglage actuel « Position ralenti chaud » (idle_hot - 35).
    const int raw7d1415 = (static_cast<int>(d.idle_error2) << 8) | static_cast<int>(d.uk10);
    const int hotIdleCorrection = static_cast<int>(d.idle_hot) - 35;
    const int hotIdleErrorCorrected = (raw7d1415 - 32768) + hotIdleCorrection;
    const bool hotIdleErrorOk = qAbs(hotIdleErrorCorrected) <= 15;
    addCheck(tr("Erreur ralenti à chaud"),
             tr("%1 ECU | brut 7D14-15=%2 | correction=%3")
                 .arg(hotIdleErrorCorrected).arg(raw7d1415).arg(hotIdleCorrection),
             stateFor(hotIdleErrorOk, true),
             QStringLiteral("Décodage : (7D14-15 brut - 32768) + correction Position ralenti chaud. "
                            "La correction suit le réglage de l'onglet Réglages ; elle n'est pas codée en dur. "
                            "Valeur indicative cohérente si elle reste proche de zéro."));
    if (!hotIdleErrorOk) ++warnings;

    const bool iacSuspicious = (d.iac_position == 0 && d.idle_error >= 50 && d.idle_switch == 0 && d.uk3 != 0);
    addCheck(tr("Commande de ralenti IAC"),
             tr("position=%1  erreur=%2").arg(d.iac_position).arg(d.idle_error),
             iacSuspicious ? tr("ANOMALIE") : tr("OK"),
             iacSuspicious ? tr("IAC en butée avec erreur de ralenti : contrôler moteur pas-à-pas, butée, prise d'air et réglage mécanique.")
                           : tr("Aucune combinaison critique IAC/erreur détectée."));
    if (iacSuspicious) ++issues;

    if (m_reference.valid) {
        const int rpmDelta = int(d.engine_rpm) - int(m_reference.data.engine_rpm);
        const int mapDelta = int(d.map_kpa) - int(m_reference.data.map_kpa);
        const int tpsDelta = int(d.throttle_pot) - int(m_reference.data.throttle_pot);
        addCheck(tr("Comparaison référence"),
                 tr("ΔRPM=%1  ΔMAP=%2 kPa  ΔTPS=%3").arg(rpmDelta).arg(mapDelta).arg(tpsDelta),
                 tr("INFO"),
                 tr("Référence capturée le %1. Cette comparaison sert à repérer une dérive ; elle ne constitue pas une spécification constructeur.").arg(m_reference.timestamp));
    }

    const int total = issues * 2 + warnings;
    const QString level = issues == 0 ? (warnings == 0 ? tr("NORMAL") : tr("SURVEILLER"))
                                      : tr("ANOMALIE À INVESTIGUER");
    m_score->setText(tr("Diagnostic : %1  |  %2 anomalie(s), %3 avertissement(s)").arg(level).arg(issues).arg(warnings));
    m_status->setText(tr("Dernière analyse : %1  |  score interne=%2")
                      .arg(QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss")).arg(total));
    m_report->setPlainText(buildReport());
}

void DiagnosticPanel::captureReference()
{
    if (!m_haveData) {
        QMessageBox::information(this, tr("Diagnostic"), tr("Aucune donnée ECU disponible."));
        return;
    }
    m_reference.valid = true;
    m_reference.data = m_last;
    m_reference.timestamp = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
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
        return tr("Aucune donnée ECU disponible.");

    const mems_data &d = m_last;
    QString text;
    text += tr("ECU MEMS MANAGER - RAPPORT DE DIAGNOSTIC\n");
    text += tr("Date : %1\n").arg(QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss"));
    if (!m_ecuId.isEmpty()) text += tr("Identification : %1\n").arg(QString::fromLatin1(m_ecuId.toHex(' ').toUpper()));
    text += tr("DTC : %1 %2 %3\n").arg(hexByte(d.dtc0), hexByte(d.dtc1), hexByte(d.dtc2));
    const int raw7d1415 = (static_cast<int>(d.idle_error2) << 8) | static_cast<int>(d.uk10);
    const int hotIdleCorrection = static_cast<int>(d.idle_hot) - 35;
    const int hotIdleErrorCorrected = (raw7d1415 - 32768) + hotIdleCorrection;
    text += tr("Temps bobine=%1 ms\n").arg(static_cast<double>(d.coil_time), 0, 'f', 2);
    text += tr("7D14-15 brut=%1 | correction ralenti chaud=%2 | erreur ralenti chaud corrigée=%3 ECU\n")
        .arg(raw7d1415).arg(hotIdleCorrection).arg(hotIdleErrorCorrected);
    text += tr("RPM=%1 | MAP=%2 kPa | batterie=%3 V | TPS=%4 | IAC=%5 | erreur ralenti=%6\n")
        .arg(d.engine_rpm).arg(d.map_kpa).arg(d.battery_voltage / 10.0, 0, 'f', 1)
        .arg(d.throttle_pot).arg(d.iac_position).arg(d.idle_error);
    text += tr("LDR brut=%1 | air admission brut=%2 | lambda=%3 | boucle fermee=%4\n")
        .arg(d.coolant_temp).arg(d.intake_air_temp).arg(d.lambda_voltage).arg(d.closed_loop ? tr("oui") : tr("non"));
    if (m_reference.valid)
        text += tr("Référence : %1\n").arg(m_reference.timestamp);
    text += tr("\nLes états et conseils de ce rapport sont des contrôles de cohérence et ne remplacent pas les spécifications constructeur.\n");
    return text;
}

void DiagnosticPanel::exportReport()
{
    if (!m_haveData) {
        QMessageBox::information(this, tr("Diagnostic"), tr("Aucune donnée à exporter."));
        return;
    }
    const QString fileName = QFileDialog::getSaveFileName(this, tr("Exporter le rapport"),
        QStringLiteral("diagnostic_mems_%1.txt").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss"))),
        tr("Rapport texte (*.txt)"));
    if (fileName.isEmpty()) return;
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Diagnostic"), tr("Impossible d'écrire le rapport."));
        return;
    }
    file.write(m_report->toPlainText().toUtf8());
    file.close();
}
