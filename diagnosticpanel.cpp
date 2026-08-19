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
#include "i18n.h"
#define tr I18n::text

namespace {
QString stateFor(bool ok, bool warn = false)
{
    return ok ? I18n::text(6800) /* EN: OK */ : (warn ? I18n::text(6801) /* EN: SURVEILLER */ : I18n::text(6802) /* EN: anomaly */);
}
}

DiagnosticPanel::DiagnosticPanel(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *root = new QVBoxLayout(this);

    QHBoxLayout *top = new QHBoxLayout;
    m_status = new QLabel(I18n::text(6803) /* EN: Waiting for ECU data */, this);
    m_score = new QLabel(I18n::text(6804) /* EN: Diagnostics: -- */, this);
    QFont scoreFont = m_score->font();
    scoreFont.setBold(true);
    scoreFont.setPointSize(scoreFont.pointSize() + 2);
    m_score->setFont(scoreFont);
    top->addWidget(m_status, 1);
    top->addWidget(m_score);
    root->addLayout(top);

    QHBoxLayout *buttons = new QHBoxLayout;
    m_capture = new QPushButton(I18n::text(6805) /* EN: Capture as reference */, this);
    m_clear = new QPushButton(I18n::text(6806) /* EN: Clear reference */, this);
    m_export = new QPushButton(I18n::text(6807) /* EN: Export report */, this);
    buttons->addWidget(m_capture);
    buttons->addWidget(m_clear);
    buttons->addStretch();
    buttons->addWidget(m_export);
    root->addLayout(buttons);

    m_checks = new QTableWidget(0, 4, this);
    m_checks->setHorizontalHeaderLabels(QStringList()
        << I18n::text(6808) /* EN: Check */ << I18n::text(6809) /* EN: Value */
        << I18n::text(6810) /* EN: Status */ << I18n::text(6811) /* EN: Interpretation / action */);
    m_checks->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_checks->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_checks->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_checks->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_checks->verticalHeader()->setVisible(false);
    root->addWidget(m_checks, 3);

    QGroupBox *reportBox = new QGroupBox(I18n::text(6812) /* EN: Automatic report */, this);
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
    addCheck(I18n::text(6813) /* EN: ECU faults */,
             I18n::text(6814) /* EN: DTC0=%1  DTC1=%2  DTC2=%3 */.arg(hexByte(d.dtc0), hexByte(d.dtc1), hexByte(d.dtc2)),
             hasDtc ? I18n::text(6815) /* EN: anomaly */ : I18n::text(6816) /* EN: OK */,
             hasDtc ? I18n::text(6817) /* EN: Read the relevant bits before clearing anything. */
                    : I18n::text(6818) /* EN: No active fault bit in 0x7D/0x80 frames. */);
    if (hasDtc) ++issues;

    const double battery = d.battery_voltage / 10.0;
    const bool batteryOk = battery >= 11.5 && battery <= 15.2;
    addCheck(I18n::text(6819) /* EN: Battery voltage */, I18n::text(6820) /* EN: %1 V */.arg(battery, 0, 'f', 1),
             stateFor(batteryOk, true),
             battery < 11.5 ? I18n::text(6821) /* EN: Low voltage: check battery, grounds and ECU supply. */
                            : (battery > 15.2 ? I18n::text(6822) /* EN: High voltage: check charging system/regulator. */
                                              : I18n::text(6823) /* EN: Range is consistent for a check while the engine is running. */));
    if (!batteryOk) ++warnings;

    const double coolant = d.coolant_temp;
    const bool coolantPlausible = coolant > 0 && coolant < 250;
    addCheck(I18n::text(6824) /* EN: Temperature LDR raw */, QString::number(coolant),
             stateFor(coolantPlausible, true),
             coolantPlausible ? I18n::text(6825) /* EN: Usable value; interpret it according to the ECU's MEMS scale. */
                              : I18n::text(6826) /* EN: Value outside plausible range: check the sensor and its circuit. */);
    if (!coolantPlausible) ++issues;

    const bool rpmPlausible = d.engine_rpm <= 8000;
    addCheck(I18n::text(6827) /* EN: Engine speed */, I18n::text(6828) /* EN: %1 tr/min */.arg(d.engine_rpm),
             stateFor(rpmPlausible, true),
             rpmPlausible ? I18n::text(6829) /* EN: Engine-speed frame is consistent. */
                          : I18n::text(6830) /* EN: Abnormal engine-speed value: check engine-speed signal and wiring. */);
    if (!rpmPlausible) ++issues;

    const bool mapPlausible = d.map_kpa <= 160;
    addCheck(I18n::text(6831) /* EN: MAP */, I18n::text(6832) /* EN: %1 kPa */.arg(d.map_kpa),
             stateFor(mapPlausible, true),
             mapPlausible ? I18n::text(6833) /* EN: Value is within the range decoded by MEMS. */
                          : I18n::text(6834) /* EN: Decoded value out of range: check MAP measurement. */);
    if (!mapPlausible) ++issues;

    const bool tpsPlausible = d.throttle_pot <= 255;
    addCheck(I18n::text(6835) /* EN: TPS */, QStringLiteral("%1 / 255").arg(d.throttle_pot),
             stateFor(tpsPlausible),
             (d.dtc1 & 0x80) || (d.dtc2 & 0x01)
                 ? I18n::text(6836) /* EN: A TPS/TPS-supply fault is reported: check supply, ground and sensor progression. */
                 : I18n::text(6837) /* EN: No known active TPS fault in the monitored bits. */);
    if ((d.dtc1 & 0x80) || (d.dtc2 & 0x01)) ++issues;

    const bool lambdaFault = (d.dtc2 & 0x04) || (d.dtc2 & 0x08);
    addCheck(I18n::text(6838) /* EN: lambda */,
             I18n::text(6839) /* EN: U=%1  freq=%2  duty=%3  status=%4 */
                 .arg(d.lambda_voltage).arg(d.lambda_sensor_frequency)
                 .arg(d.lambda_sensor_dutycycle).arg(d.lambda_sensor_status),
             lambdaFault ? I18n::text(6840) /* EN: anomaly */ : I18n::text(6841) /* EN: INFORMATION */,
             lambdaFault ? I18n::text(6842) /* EN: Lambda circuit/supply fault: check heater, supply and wiring. */
                         : (d.closed_loop ? I18n::text(6843) /* EN: Closed loop active: lambda corrections are being applied. */
                                          : I18n::text(6844) /* EN: Closed loop is currently inactive; interpret together with temperature and engine conditions. */));
    if (lambdaFault) ++issues;

    const int fuelTrim = int(d.short_term_fuel_trim) - 100;
    const bool trimWatch = qAbs(fuelTrim) > 20 || qAbs(int(d.long_term_fuel_trim) - 100) > 20;
    addCheck(I18n::text(6845) /* EN: Corrections fuel */,
             I18n::text(6846) /* EN: short-term=%1  long-term=%2 */
                 .arg(fuelTrim).arg(int(d.long_term_fuel_trim) - 100),
             stateFor(!trimWatch, true),
             trimWatch ? I18n::text(6847) /* EN: Large correction: check for air leaks, fuel pressure/supply, injection or lambda measurement before changing settings. */
                       : I18n::text(6848) /* EN: Corrections show no significant deviation according to this indicative criterion. */);
    if (trimWatch) ++warnings;

    // Temps de charge de la bobine : contrôle spécifique demandé pour
    // une tension batterie proche de 14 V. Hors de cette tension, le
    // logiciel affiche la mesure mais ne porte pas de jugement automatique.
    const double coilTime = static_cast<double>(d.coil_time) * 0.002;
    const bool batteryNear14 = battery >= 13.5 && battery <= 14.5;
    const bool coilOk = coilTime >= 1.9 && coilTime <= 3.1;
    QString coilState;
    QString coilAdvice;
    if (!batteryNear14) {
        coilState = I18n::text(6849) /* EN: NOT EVALUATED */;
        coilAdvice = I18n::text(6850) /* EN: Measurement displayed. The 1.9–3.1 ms check is applied automatically only when battery voltage is close to 14 V. */;
    } else if (coilOk) {
        coilState = I18n::text(6851) /* EN: OK */;
        coilAdvice = I18n::text(6852) /* EN: Coil charge time is within the 1.9–3.1 ms range at about 14 V. */;
    } else if (coilTime > 3.1) {
        coilState = I18n::text(6853) /* EN: anomaly */;
        coilAdvice = I18n::text(6854) /* EN: Coil charge time too high at about 14 V: first check the coil primary circuit, coil and wiring. */;
        ++issues;
    } else {
        coilState = I18n::text(6855) /* EN: SURVEILLER */;
        coilAdvice = I18n::text(6856) /* EN: Coil charge time below 1.9 ms at about 14 V: check the measurement, supply and control circuit before drawing a conclusion. */;
        ++warnings;
    }
    addCheck(I18n::text(6857) /* EN: Coil charge time */, I18n::text(6858) /* EN: %1 ms  | battery %2 V */.arg(coilTime, 0, 'f', 2).arg(battery, 0, 'f', 1),
             coilState, coilAdvice);

    // Décodage du champ 7D14-15. La correction de -3 n'est pas une constante :
    // elle correspond au réglage actuel « Position ralenti chaud » (idle_hot - 35).
    const int raw7d1415 = (static_cast<int>(d.idle_error2) << 8) | static_cast<int>(d.uk10);
    const int hotIdleCorrection = static_cast<int>(d.idle_hot) - 35;
    const int hotIdleErrorCorrected = (raw7d1415 - 32768) + hotIdleCorrection;
    const bool hotIdleErrorOk = qAbs(hotIdleErrorCorrected) <= 15;
    addCheck(I18n::text(6859) /* EN: Hot idle error */,
             I18n::text(6860) /* EN: %1 ECU | brut 7D14-15=%2 | correction=%3 */
                 .arg(hotIdleErrorCorrected).arg(raw7d1415).arg(hotIdleCorrection),
             stateFor(hotIdleErrorOk, true),
             I18n::text(6897) /* EN: Decoding: (raw 7D14-15 - 32768) + Hot idle position correction. The correction follows the value configured in the Settings tab; it is not hard-coded. The indicative value is consistent when it remains close to zero. */);
    if (!hotIdleErrorOk) ++warnings;

    const bool iacSuspicious = (d.iac_position == 0 && d.idle_error >= 50 && d.idle_switch == 0 && d.uk3 != 0);
    addCheck(I18n::text(6861) /* EN: IAC idle command */,
             I18n::text(6862) /* EN: position=%1  error=%2 */.arg(d.iac_position).arg(d.idle_error),
             iacSuspicious ? I18n::text(6863) /* EN: anomaly */ : I18n::text(6864) /* EN: OK */,
             iacSuspicious ? I18n::text(6865) /* EN: IAC at its limit with idle error: check stepper motor, stop, air leaks and mechanical adjustment. */
                           : I18n::text(6866) /* EN: No critical IAC/error combination detected. */);
    if (iacSuspicious) ++issues;

    if (m_reference.valid) {
        const int rpmDelta = int(d.engine_rpm) - int(m_reference.data.engine_rpm);
        const int mapDelta = int(d.map_kpa) - int(m_reference.data.map_kpa);
        const int tpsDelta = int(d.throttle_pot) - int(m_reference.data.throttle_pot);
        addCheck(I18n::text(6867) /* EN: Comparaison reference */,
                 I18n::text(6868) /* EN: ΔRPM=%1  ΔMAP=%2 kPa  ΔTPS=%3 */.arg(rpmDelta).arg(mapDelta).arg(tpsDelta),
                 I18n::text(6869) /* EN: INFO */,
                 I18n::text(6870) /* EN: Reference captured on %1. This comparison helps identify drift; it is not a manufacturer specification. */.arg(m_reference.timestamp));
    }

    const int total = issues * 2 + warnings;
    const QString level = issues == 0 ? (warnings == 0 ? I18n::text(6871) /* EN: NORMAL */ : I18n::text(6872) /* EN: SURVEILLER */)
                                      : I18n::text(6873) /* EN: ANOMALY TO INVESTIGATE */;
    m_score->setText(I18n::text(6874) /* EN: Diagnostics : %1  |  %2 anomaly(s), %3 warning(s) */.arg(level).arg(issues).arg(warnings));
    m_status->setText(I18n::text(6875) /* EN: Last analysis : %1  |  score interne=%2 */
                      .arg(QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss")).arg(total));
    m_report->setPlainText(buildReport());
}

void DiagnosticPanel::captureReference()
{
    if (!m_haveData) {
        QMessageBox::information(this, I18n::text(6876) /* EN: Diagnostics */, I18n::text(6877) /* EN: No ECU data available. */);
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
        return I18n::text(6878) /* EN: No ECU data available. */;

    const mems_data &d = m_last;
    QString text;
    text += I18n::text(6879) /* EN: ECU MEMS MANAGER - DIAGNOSTIC REPORT  */;
    text += I18n::text(6880) /* EN: Date : %1  */.arg(QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss"));
    if (!m_ecuId.isEmpty()) text += I18n::text(6881) /* EN: Identification : %1  */.arg(QString::fromLatin1(m_ecuId.toHex(' ').toUpper()));
    text += I18n::text(6882) /* EN: DTC : %1 %2 %3  */.arg(hexByte(d.dtc0), hexByte(d.dtc1), hexByte(d.dtc2));
    const int raw7d1415 = (static_cast<int>(d.idle_error2) << 8) | static_cast<int>(d.uk10);
    const int hotIdleCorrection = static_cast<int>(d.idle_hot) - 35;
    const int hotIdleErrorCorrected = (raw7d1415 - 32768) + hotIdleCorrection;
    text += I18n::text(6883) /* EN: Temps coil=%1 ms  */.arg(static_cast<double>(d.coil_time) * 0.002, 0, 'f', 2);
    text += I18n::text(6884) /* EN: raw 7D14-15=%1 | hot-idle correction=%2 | corrected hot-idle error=%3 ECU  */
        .arg(raw7d1415).arg(hotIdleCorrection).arg(hotIdleErrorCorrected);
    text += I18n::text(6885) /* EN: RPM=%1 | MAP=%2 kPa | battery=%3 V | TPS=%4 | IAC=%5 | idle error=%6  */
        .arg(d.engine_rpm).arg(d.map_kpa).arg(d.battery_voltage / 10.0, 0, 'f', 1)
        .arg(d.throttle_pot).arg(d.iac_position).arg(d.idle_error);
    text += I18n::text(6886) /* EN: raw coolant=%1 | raw intake air=%2 | lambda=%3 | closed loop=%4  */
        .arg(d.coolant_temp).arg(d.intake_air_temp).arg(d.lambda_voltage).arg(d.closed_loop ? I18n::text(6887) /* EN: oui */ : I18n::text(6888) /* EN: non */);
    if (m_reference.valid)
        text += I18n::text(6889) /* EN: Reference: %1  */.arg(m_reference.timestamp);
    text += I18n::text(6890) /* EN:  The statuses and guidance in this report are consistency checks and do not replace manufacturer specifications.  */;
    return text;
}

void DiagnosticPanel::exportReport()
{
    if (!m_haveData) {
        QMessageBox::information(this, I18n::text(6891) /* EN: Diagnostics */, I18n::text(6892) /* EN: No data to export. */);
        return;
    }
    const QString fileName = QFileDialog::getSaveFileName(this, I18n::text(6893) /* EN: Export report */,
        QStringLiteral("diagnostic_mems_%1.txt").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss"))),
        I18n::text(6894) /* EN: report texte (*.txt) */);
    if (fileName.isEmpty()) return;
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, I18n::text(6895) /* EN: Diagnostics */, I18n::text(6896) /* EN: Unable to write report. */);
        return;
    }
    file.write(m_report->toPlainText().toUtf8());
    file.close();
}
