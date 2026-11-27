#include "diagnosticpanel.h"

#include <QDateTime>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <QVector>
#include <QApplication>
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
#include "mems16na_presentation.h"
#include "mems219_units.h"
#include "memsextendeddtc_presentation.h"
#define tr I18n::text

namespace {
QString stateFor(bool ok, bool warn = false)
{
    return ok ? I18n::text(6800) /* EN: OK */ : (warn ? I18n::text(6801) /* EN: SURVEILLER */ : I18n::text(6802) /* EN: anomaly */);
}
}

class CoolantTrend : public QWidget {
public:
    explicit CoolantTrend(QWidget *parent):QWidget(parent){setMinimumSize(90,44);setMaximumHeight(52);setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);}
    void clear(){points.clear();update();}
    void append(qint64 ms,int c){
        if(!points.isEmpty()&&ms-points.last().x()<1000)return;
        if(points.size()>=600)points.remove(1);
        points.append(QPointF(qreal(ms),c));update();
    }
protected:
    void paintEvent(QPaintEvent*)override{
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
        const bool dark=qApp&&qApp->property("ecuDarkTheme").toBool();
        p.setPen(dark?QColor("#44505a"):QColor("#c1cbd3"));
        const QRectF r=QRectF(rect()).adjusted(35,4,-4,-15);p.drawRect(r);
        if(points.isEmpty())return;
        qreal lo=points.first().y(),hi=lo;
        for(const auto &v:points){lo=qMin(lo,v.y());hi=qMax(hi,v.y());}
        if(hi-lo<4){const qreal mid=(hi+lo)*0.5;lo=mid-2;hi=mid+2;}
        const qreal span=qMax(qreal(1000),points.last().x());
        QFont axis=p.font();axis.setPointSizeF(7);p.setFont(axis);
        p.setPen(dark?QColor("#d4dbe0"):QColor("#344250"));
        p.drawText(QRectF(0,0,34,14),Qt::AlignRight,QString::number(hi,'f',0)+QStringLiteral(" °C"));
        p.drawText(QRectF(0,r.bottom()-12,34,14),Qt::AlignRight,QString::number(lo,'f',0)+QStringLiteral(" °C"));
        p.drawText(QRectF(r.left(),r.bottom()+1,40,14),Qt::AlignLeft,QStringLiteral("0 s"));
        p.drawText(QRectF(r.right()-45,r.bottom()+1,45,14),Qt::AlignRight,QString::number(span/1000.0,'f',0)+QStringLiteral(" s"));
        QPainterPath path;
        for(int i=0;i<points.size();++i){
            const QPointF pt(r.left()+r.width()*points[i].x()/span,
                              r.bottom()-r.height()*(points[i].y()-lo)/(hi-lo));
            if(i==0)path.moveTo(pt);else path.lineTo(pt);
        }
        p.setPen(QPen(dark?QColor("#ff9828"):QColor("#a04a00"),2));
        p.drawPath(path);
    }
private:
    QVector<QPointF> points;
};

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

    QGroupBox *coolantBox=new QGroupBox(I18n::text(2971812),this);
    coolantBox->setObjectName(QStringLiteral("coolantTestCard"));
    QVBoxLayout *cv=new QVBoxLayout(coolantBox);cv->setContentsMargins(6,6,6,6);cv->setSpacing(3);
    QHBoxLayout *cr=new QHBoxLayout;
    m_coolantValues=new QLabel(coolantBox);m_coolantValues->setObjectName(QStringLiteral("coolantTestValues"));
    m_coolantValues->setWordWrap(true);m_coolantValues->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
    cr->addWidget(m_coolantValues,2);
    m_coolantTrend=new CoolantTrend(coolantBox);cr->addWidget(m_coolantTrend,1);
    m_coolantStart=new QPushButton(I18n::text(2971813),coolantBox);
    m_coolantStart->setObjectName(QStringLiteral("coolantTestStart"));
    m_coolantStop=new QPushButton(I18n::text(2971814),coolantBox);
    m_coolantStop->setObjectName(QStringLiteral("coolantTestStop"));
    QVBoxLayout *cb=new QVBoxLayout;cb->setSpacing(3);cb->addWidget(m_coolantStart);cb->addWidget(m_coolantStop);
    cr->addLayout(cb);cv->addLayout(cr);
    m_coolantStatus=new QLabel(coolantBox);m_coolantStatus->setObjectName(QStringLiteral("coolantTestStatus"));
    m_coolantStatus->setWordWrap(true);m_coolantStatus->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
    m_coolantStatus->setMinimumHeight(30);cv->addWidget(m_coolantStatus);
    root->addWidget(coolantBox);

    m_checks = new QTableWidget(0, 4, this);
    m_checks->setHorizontalHeaderLabels(QStringList()
        << I18n::text(6808) /* EN: Check */ << I18n::text(6809) /* EN: Value */
        << I18n::text(6810) /* EN: Status */ << I18n::text(6811) /* EN: Interpretation / action */);
    m_checks->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_checks->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_checks->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_checks->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_checks->horizontalHeader()->setMinimumSectionSize(55);
    m_checks->horizontalHeader()->setStretchLastSection(true);
    m_checks->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_checks->setWordWrap(false);
    m_checks->verticalHeader()->setVisible(false);
    // The diagnostic table owns its readability, without a late UI patch.
    m_checks->setStyleSheet(m_checks->styleSheet()+QStringLiteral(
        "QTableWidget{color:#ffffff;}"
        "QTableWidget:disabled{color:#ffffff;}"
        "QTableWidget::item{color:#ffffff;}"
        "QTableWidget::item:disabled{color:#ffffff;}"));
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
    connect(m_coolantStart,&QPushButton::clicked,this,[this](){
        if(!m_coolantFresh.isValid()||m_coolantFresh.elapsed()>3000)return;
        if(m_coolantTest.start(Mems218::coolantSample(m_coolantConnected,m_naEvidence))){
            m_coolantElapsed.start();m_coolantTrend->clear();
            m_coolantTrend->append(0,m_coolantTest.current);
        }
        refreshCoolantTest();if(m_haveData)m_report->setPlainText(buildReport());
    });
    connect(m_coolantStop,&QPushButton::clicked,this,[this](){
        m_coolantTest.stop();refreshCoolantTest();if(m_haveData)m_report->setPlainText(buildReport());
    });
    QTimer *freshness=new QTimer(this);freshness->setInterval(500);
    connect(freshness,&QTimer::timeout,this,[this](){
        if(m_coolantFresh.isValid()&&m_coolantFresh.elapsed()>3000){
            m_coolantFresh.invalidate();m_coolantTest.interrupt();refreshCoolantTest();
            if(m_haveData)m_report->setPlainText(buildReport());
        }
    });
    freshness->start();refreshCoolantTest();
}

QString DiagnosticPanel::hexByte(quint8 v)
{
    return QStringLiteral("0x%1").arg(v, 2, 16, QLatin1Char('0')).toUpper();
}

void DiagnosticPanel::setEcuId(const QByteArray &id)
{
    if(m_naEvidence.identifiedProfile()&&id!=m_ecuId)endNaSession();
    m_ecuId = id;
    if (m_haveData)
        rebuild(m_last);
}

void DiagnosticPanel::updateData(const mems_data *data,const Mems16Na::SessionEvidence *na)
{
    if (!data)return;
    const auto *profile=na?na->identifiedProfile():nullptr;
    if(m_naEvidence.identifiedProfile()!=profile||
       (profile&&m_naEvidence.identity()!=na->identity()))m_naReference=Snapshot();
    if(!na || m_naEvidence.firmwareName()!=na->firmwareName() || m_naEvidence.identity()!=na->identity()){
        m_passive.reset();resetCoolantTest();
    }
    if(na)m_naEvidence=*na;
    else m_naEvidence.reset();
    m_passive.sample(m_naEvidence);
    m_coolantConnected=true;m_coolantFresh.start();
    m_coolantTest.sample(Mems218::coolantSample(true,m_naEvidence));
    if(m_coolantTest.state==Mems218::TestState::Running)
        m_coolantTrend->append(m_coolantElapsed.elapsed(),m_coolantTest.current);
    refreshCoolantTest();
    m_showing2J=false;
    m_last = *data;
    m_haveData = true;
    rebuild(m_last);
}

void DiagnosticPanel::endNaSession()
{
    restoreNaChecksStyle();
    m_passive.reset();resetCoolantTest();m_coolantConnected=false;refreshCoolantTest();
    if(!m_naEvidence.identifiedProfile()&&m_naEvidence.firmwareName().empty())return;
    m_naEvidence.reset();
    m_naReference=Snapshot();
    m_haveData=false;
    m_checks->setRowCount(0);
    m_status->setText(I18n::text(6803));
    m_score->setText(I18n::text(6804));
    m_report->clear();
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
    for (int col=0; col<4; ++col)
      if (QTableWidgetItem *item=m_checks->item(row,col))
        item->setData(Qt::ForegroundRole, QColor(QStringLiteral("#ffffff")));
}

void DiagnosticPanel::rebuild(const mems_data &d)
{
    if(m_naEvidence.identifiedProfile()) { rebuildNa(d); return; }
    restoreNaChecksStyle();
    m_checks->setRowCount(0);
    int issues = 0;
    int warnings = 0;

    const bool knownDtc2=MemsExtendedDtc::dtc2Known(m_naEvidence.firmwareName().c_str());
    const unsigned fault2=knownDtc2?m_naEvidence.extendedDtc(2).faults:d.dtc2;
    const bool hasDtc = (d.dtc0 != 0 || d.dtc1 != 0 || fault2 != 0);
    const bool incompleteDtc2=knownDtc2&&m_naEvidence.extendedDtc(2).status!=MemsExtendedDtc::Status::Decoded;
    addCheck(I18n::text(6813) /* EN: ECU faults */,
             I18n::text(6814) /* EN: DTC0=%1  DTC1=%2  DTC2=%3 */.arg(hexByte(d.dtc0), hexByte(d.dtc1), hexByte(d.dtc2)),
             hasDtc ? I18n::text(6815) /* EN: anomaly */ : incompleteDtc2?I18n::text(29002):I18n::text(6816) /* EN: OK */,
             hasDtc ? I18n::text(6817) /* EN: Read the relevant bits before clearing anything. */
                    : incompleteDtc2?I18n::text(29002):I18n::text(6818) /* EN: No active fault bit in 0x7D/0x80 frames. */);
    if (hasDtc) ++issues;
    for(int group=knownDtc2?2:3;group<=5;++group){
        const auto r=m_naEvidence.extendedDtc(group);
        const unsigned legacy=group==2?d.dtc2:group==3?d.dtc3:group==4?d.dtc4:d.dtc5;
        addCheck(QStringLiteral("DTC %1").arg(group),MemsExtendedDtc::raw(&m_naEvidence,group,legacy),
            r.faults?I18n::text(29051):I18n::text(6841),MemsExtendedDtc::text(&m_naEvidence,group));
        // DTC2/10 and DTC3/01 are the same stored synchronization fault on KBHBG009.
        const unsigned duplicate=(group==3&&knownDtc2&&(fault2&0x10))?1u:0u;
        if(group!=2&&(r.faults&~duplicate))++issues;
    }

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
    addCheck(I18n::text(2971903),coolantPlausible?QString::number(Mems219::temperatureC(d.coolant_temp))+QStringLiteral(" °C"):I18n::text(29002),
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

    const bool tpsFault = (d.dtc1 & 0x80) != 0 || (!knownDtc2 && (fault2 & 0x01) != 0);
    addCheck(I18n::text(6835) /* EN: TPS */, QStringLiteral("%1 / 255").arg(d.throttle_pot),
             tpsFault ? I18n::text(6802) : I18n::text(6849),
             tpsFault
                 ? I18n::text(6836) /* EN: A TPS/TPS-supply fault is reported: check supply, ground and sensor progression. */
                 : I18n::text(2971705));
    if(tpsFault) ++issues;

    const bool lambdaFault = (fault2 & 0x04) || (fault2 & 0x08);
    addCheck(I18n::text(6838) /* EN: lambda */,
             I18n::text(6839) /* EN: U=%1  freq=%2  duty=%3  status=%4 */
                 .arg(QString::number(Mems219::lambdaMv(d.lambda_voltage))+QStringLiteral(" mV")).arg(d.lambda_sensor_frequency)
                 .arg(d.lambda_sensor_dutycycle).arg(d.lambda_sensor_status),
             lambdaFault ? I18n::text(6840) /* EN: anomaly */ : I18n::text(6841) /* EN: INFORMATION */,
             lambdaFault ? I18n::text(6842) /* EN: Lambda circuit/supply fault: check heater, supply and wiring. */
                         : (d.closed_loop ? I18n::text(6843) /* EN: Closed loop active: lambda corrections are being applied. */
                                          : I18n::text(6844) /* EN: Closed loop is currently inactive; interpret together with temperature and engine conditions. */));
    if (lambdaFault) ++issues;

    const int shortFuelTrim = int(d.short_term_fuel_trim) - 100;
    const bool shortQualified=Mems219::lambdaQualified(m_naEvidence);
    const bool longQualified=Mems219::injectionAvailable(m_naEvidence);
    const bool trimWatch=shortQualified&&qAbs(shortFuelTrim)>20;
    addCheck(I18n::text(6845) /* EN: Corrections fuel */,
             I18n::text(6846) /* EN: short-term=%1  long-term=%2 */
                 .arg(shortQualified?QString::number(shortFuelTrim)+QStringLiteral(" %"):I18n::text(29002))
                 .arg(longQualified?QString::number(Mems217::injectionUs(Mems217::frame7d(m_naEvidence).bytes[11]))+QStringLiteral(" µs"):I18n::text(29002)),
             shortQualified?stateFor(!trimWatch,true):I18n::text(6849),
             trimWatch ? I18n::text(6847) /* EN: Large correction: check for air leaks, fuel pressure/supply, injection or lambda measurement before changing settings. */
                       : shortQualified?I18n::text(6848):I18n::text(29002));
    if (trimWatch) ++warnings;

    // Vérification indicative MEMS 1.3/1.6 : moteur tournant, tension
    // entre 13,5 et 14,5 V. Une durée isolée ne prouve pas une panne de bobine.
    const double coilTime = static_cast<double>(d.coil_time) * 0.002;
    const bool batteryNear14 = battery >= 13.5 && battery <= 14.5;
    const bool coilOk = coilTime >= 2.0 && coilTime <= 3.5;
    QString coilState;
    QString coilAdvice;
    if (d.engine_rpm == 0) {
        coilState = I18n::text(6849) /* EN: NOT EVALUATED */;
        coilAdvice = I18n::text(18122) /* EN: Engine stopped; no automatic coil diagnosis. */;
    } else if (!batteryNear14) {
        coilState = I18n::text(6849) /* EN: NOT EVALUATED */;
        coilAdvice = I18n::text(6850) /* EN: Reference check requires 13.5–14.5 V. */;
    } else if (coilOk) {
        coilState = I18n::text(6851) /* EN: OK */;
        coilAdvice = I18n::text(6852) /* EN: Indicative 2.0–3.5 ms range. */;
    } else if (coilTime > 4.0) {
        coilState = I18n::text(6853) /* EN: POSSIBLE ANOMALY */;
        coilAdvice = I18n::text(6854) /* EN: Over 4.0 ms: verify the primary circuit; not proof of a bad coil. */;
        ++issues;
    } else if (coilTime > 3.5) {
        coilState = I18n::text(6855) /* EN: WATCH */;
        coilAdvice = I18n::text(18121) /* EN: Over 3.5 to 4.0 ms: monitor. */;
        ++warnings;
    } else {
        coilState = I18n::text(6855) /* EN: WATCH */;
        coilAdvice = I18n::text(6856) /* EN: Below 2.0 ms: verify the reading and supply. */;
        ++warnings;
    }
    addCheck(I18n::text(6857) /* EN: Coil charge time */, I18n::text(6858) /* EN: %1 ms  | battery %2 V */.arg(coilTime, 0, 'f', 2).arg(battery, 0, 'f', 1),
             coilState, coilAdvice);

    // Décodage du champ 7D14-15. La correction de -3 n'est pas une constante :
    // elle correspond au réglage actuel « Position ralenti chaud » (idle_hot - 35).
    const int raw7d1415 = (static_cast<int>(d.idle_error2) << 8) | static_cast<int>(d.uk10);
    const int hotIdleCorrection = static_cast<int>(d.idle_hot) - 35;
    const int hotIdleErrorCorrected = (raw7d1415 - 32768) - hotIdleCorrection;
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

    addPassiveChecks();
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
    if (m_showing2J)
    {
        m_2jReference=m_mems2jValues;
        m_2jReferenceTime=QDateTime::currentDateTime().toString(QStringLiteral("dd/MM/yyyy hh:mm:ss"));
        update2JData(m_mems2jValues,m_mems2jFaults,m_structuredEcuName);
        return;
    }
    if (!m_haveData) {
        QMessageBox::information(this, I18n::text(6876) /* EN: Diagnostics */, I18n::text(6877) /* EN: No ECU data available. */);
        return;
    }
    if(m_naEvidence.identifiedProfile() && !m_naEvidence.ready()) {
        QMessageBox::information(this,I18n::text(6876),I18n::text(29048));
        return;
    }
    Snapshot &reference=m_naEvidence.identifiedProfile()?m_naReference:m_reference;
    reference.valid = true;
    reference.data = m_last;
    reference.timestamp = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
    rebuild(m_last);
}

void DiagnosticPanel::clearReference()
{
    if (m_showing2J)
    {
        m_2jReference.clear();
        m_2jReferenceTime.clear();
        update2JData(m_mems2jValues,m_mems2jFaults,m_structuredEcuName);
        return;
    }
    if(m_naEvidence.identifiedProfile())m_naReference=Snapshot();
    else m_reference = Snapshot();
    if (m_haveData) rebuild(m_last);
}

QString DiagnosticPanel::buildReport() const
{
    if (!m_haveData)
        return I18n::text(6878) /* EN: No ECU data available. */;
    if (m_showing2J)
    {
        QStringList report;
        report << QStringLiteral("ECU MEMS MANAGER — %1, lecture K-Line").arg(m_structuredEcuName)
               << QStringLiteral("Date : %1").arg(QDateTime::currentDateTime().toString(QStringLiteral("dd/MM/yyyy hh:mm:ss")))
               << (m_structuredEcuName==QStringLiteral("MEMS 2J")
                  ? QStringLiteral("Defauts : cartographie Mini MPi indicative ; variantes a confirmer")
                  : QStringLiteral("Defauts MEMS 3 : codes 16 bits, references RoverMEMS"));
        for (auto it=m_mems2jValues.cbegin();it!=m_mems2jValues.cend();++it)
            report << QStringLiteral("%1 : %2").arg(it.key(),it.value().toString());
        if (m_mems2jFaults.isEmpty())
            report << QStringLiteral("Aucun bit reconnu dans la cartographie Mini MPi.");
        else
            report << m_mems2jFaults;
        if (!m_2jReferenceTime.isEmpty())
            report << QStringLiteral("Référence enregistrée le : %1").arg(m_2jReferenceTime);
        return report.join(QLatin1Char('\n'));
    }

    if(m_naEvidence.identifiedProfile())return buildNaReport();
    const mems_data &d = m_last;
    QString text;
    text += I18n::text(6879) /* EN: ECU MEMS MANAGER - DIAGNOSTIC REPORT  */;
    text += I18n::text(6880) /* EN: Date : %1  */.arg(QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss"));
    if (!m_ecuId.isEmpty()) text += I18n::text(6881) /* EN: Identification : %1  */.arg(QString::fromLatin1(m_ecuId.toHex(' ').toUpper()));
    text += I18n::text(6882) /* EN: DTC : %1 %2 %3  */.arg(hexByte(d.dtc0), hexByte(d.dtc1), hexByte(d.dtc2));
    for(int group=2;group<=5;++group)if(group!=2||MemsExtendedDtc::dtc2Known(m_naEvidence.firmwareName().c_str()))
        text+=QStringLiteral("\nDTC %1 : ").arg(group)+MemsExtendedDtc::text(&m_naEvidence,group);
    const int raw7d1415 = (static_cast<int>(d.idle_error2) << 8) | static_cast<int>(d.uk10);
    const int hotIdleCorrection = static_cast<int>(d.idle_hot) - 35;
    const int hotIdleErrorCorrected = (raw7d1415 - 32768) - hotIdleCorrection;
    text += I18n::text(6883) /* EN: Temps coil=%1 ms  */.arg(static_cast<double>(d.coil_time) * 0.002, 0, 'f', 2);
    text += I18n::text(6884) /* EN: raw 7D14-15=%1 | hot-idle correction=%2 | corrected hot-idle error=%3 ECU  */
        .arg(raw7d1415).arg(hotIdleCorrection).arg(hotIdleErrorCorrected);
    text += I18n::text(6885) /* EN: RPM=%1 | MAP=%2 kPa | battery=%3 V | TPS=%4 | IAC=%5 | idle error=%6  */
        .arg(d.engine_rpm).arg(d.map_kpa).arg(d.battery_voltage / 10.0, 0, 'f', 1)
        .arg(d.throttle_pot).arg(d.iac_position).arg(d.idle_error);
    text += I18n::text(6886) /* EN: raw coolant=%1 | raw intake air=%2 | lambda=%3 | closed loop=%4  */
        .arg(d.coolant_temp).arg(d.intake_air_temp).arg(d.lambda_voltage).arg(d.closed_loop ? I18n::text(6887) /* EN: oui */ : I18n::text(6888) /* EN: non */);
    // Include the same passive results in the generic (MEMS 1.9) report.
    if(Mems217::passiveProfile(m_naEvidence.firmwareName())){
        for(int row=0;row<m_checks->rowCount();++row){
            const QString title=m_checks->item(row,0)->text();
            if(title!=I18n::text(2971710)&&title!=I18n::text(2971711))continue;
            QStringList fields;
            for(int col=0;col<4;++col)fields<<m_checks->item(row,col)->text();
            text+=QStringLiteral("\n")+fields.join(QStringLiteral(" | "))+QStringLiteral("\n");
        }
    }
    if(m_coolantTest.state!=Mems218::TestState::Idle)
        text+=QStringLiteral("\n")+coolantTestSummary()+QStringLiteral("\n");
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


void DiagnosticPanel::update2JData(const QVariantMap &values, const QStringList &faults,
                                   const QString &family)
{
    endNaSession();
    m_coolantConnected=true;refreshCoolantTest();
    m_showing2J=true;
    m_structuredEcuName=family;
    m_haveData=true;
    m_mems2jValues=values;
    m_mems2jFaults=faults;
    m_checks->setRowCount(0);
    m_status->setText(QStringLiteral("%1 — %2").arg(family).arg(
        QDateTime::currentDateTime().toString(QStringLiteral("dd/MM/yyyy hh:mm:ss"))));
    m_score->setText(QStringLiteral("%1 : %2")
         .arg(family==QStringLiteral("MEMS 3")?QStringLiteral("Codes DTC"):
                  QStringLiteral("Bits interprétés")).arg(faults.size()));

    addCheck(QStringLiteral("Calculateur"),family,
             QStringLiteral("Connecté"),family==QStringLiteral("MEMS 3")
               ? QStringLiteral("Session 9600 8E1, B8/13/F7 et XOR")
               : QStringLiteral("Session 10400 bauds, protocole 21/61"));
    for (auto it=values.cbegin();it!=values.cend();++it)
    {
        if(it.key()==QStringLiteral("_received"))continue;
        addCheck(it.key(),it.value().toString(),QStringLiteral("Lecture ECU"),
                 it.key().endsWith(QStringLiteral("_raw"))
                    ? QStringLiteral("Valeur brute — unité / interprétation non établie")
                    : QStringLiteral("Conversion documentée RoverMEMS ; à confirmer sur ECU réel"));
    }
    addCheck(family==QStringLiteral("MEMS 3")?QStringLiteral("Defauts 18 00"):
             QStringLiteral("Défauts 21 19"),
             values.value(QStringLiteral("faults_raw"),QStringLiteral("--")).toString(),
             QStringLiteral("Lecture brute"),
             family==QStringLiteral("MEMS 3")
                ? QStringLiteral("Codes DTC source RoverMEMS, variantes a verifier")
                : QStringLiteral("Cartographie documentée sur Mini MPi uniquement ; pas de codes Pxxxx inventés"));
    for (const QString &fault : faults)
        addCheck(family==QStringLiteral("MEMS 3")?QStringLiteral("DTC MEMS 3"):
                 QStringLiteral("Bit MEMS 2J"),fault,
                 QStringLiteral("Signalé"),QStringLiteral("État et montage capteur à vérifier selon variante"));
    if (faults.isEmpty())
        addCheck(QStringLiteral("Bits de défaut"),QStringLiteral("Aucun bit reconnu"),
                 QStringLiteral("Information"),QStringLiteral("Cela ne prouve pas qu'aucun défaut n'existe"));
    if (!m_2jReferenceTime.isEmpty())
        addCheck(QStringLiteral("Référence"),m_2jReferenceTime,
                 QStringLiteral("Information"),QStringLiteral("Capture MEMS 2J enregistrée"));
    m_report->setPlainText(buildReport());
}

void DiagnosticPanel::restoreNaChecksStyle()
{
    if(!m_naStyleActive)return;
    m_checks->setStyleSheet(m_naOriginalChecksStyle);
    m_naOriginalChecksStyle.clear();
    m_naStyleActive=false;
}

void DiagnosticPanel::rebuildNa(const mems_data &d)
{
    using namespace Mems16Na;
    const auto *p=m_naEvidence.identifiedProfile();
    if(!p)return;
    if(!m_naStyleActive) {
        m_naOriginalChecksStyle=m_checks->styleSheet();
        m_naStyleActive=true;
    }
    const bool dark=qApp&&qApp->property("ecuDarkTheme").toBool();
    const QString style=m_naOriginalChecksStyle+(dark?QString():QStringLiteral(
        "QTableWidget{color:#202936;}QTableWidget:disabled{color:#536172;}"
        "QTableWidget::item{color:#202936;}QTableWidget::item:disabled{color:#536172;}"));
    if(m_checks->styleSheet()!=style)m_checks->setStyleSheet(style);
    m_checks->setRowCount(0);
    int issues=0,warnings=0;
    const QString info=I18n::text(6841);
    const QString notEvaluated=I18n::text(6849);
    auto add=[&](const QString &name,const QString &value,const QString &state,const QString &advice) {
        addCheck(name,value,state,advice);
        const int row=m_checks->rowCount()-1;
        for(int col=0;col<4;++col) {
            auto *item=m_checks->item(row,col);
            // Respect the active palette and retain the complete multi-fault detail.
            item->setData(Qt::ForegroundRole,QVariant());
            item->setToolTip(name+QStringLiteral("\n")+value+QStringLiteral("\n")+state+QStringLiteral("\n")+advice);
        }
    };
    add(I18n::text(29049),QString::fromLatin1(p->firmware),info,I18n::text(29050));
    const Group groups[]={Group::Dtc0,Group::Dtc1,Group::Dtc2,Group::Dtc3,Group::Dtc4,Group::Dtc5,Group::Indirect7D10};
    for(int i=0;i<7;++i) {
        const auto r=m_naEvidence.dtc(groups[i]);
        for(unsigned bit=1;bit<=128;bit<<=1) {
            if(r.faultBits&bit)++issues;
            if(r.unknownBits&bit)++warnings;
        }
        const QString name=i<6?QStringLiteral("DTC %1").arg(i):QStringLiteral("7D/10");
        const QString state=r.availability==Availability::Missing?notEvaluated:
            r.faultBits?I18n::text(29051):r.unknownBits||r.availability==Availability::Unverified?I18n::text(29002):
            r.availability==Availability::NotUsed?info:r.stateBits?info:I18n::text(6816);
        add(name,rawDtc(r),state,dtcText(p,groups[i],r)+QStringLiteral("\n")+dtcHelp(p,groups[i]));
    }
    if(!m_naEvidence.ready()) {
        m_score->setText(I18n::text(29052));
        m_status->setText(I18n::text(29048));
        m_report->setPlainText(buildNaReport());
        return;
    }
    const auto m=m_naEvidence.measurements();
    // Retain the existing independent consistency checks. No threshold from
    // the old dimensionless correction or the old idle-error formula is reused.
    const double battery=d.battery_voltage/10.0;
    const bool batteryOk=battery>=11.5&&battery<=15.2;
    add(I18n::text(6819),I18n::text(6820).arg(battery,0,'f',1),stateFor(batteryOk,true),
        battery<11.5?I18n::text(6821):battery>15.2?I18n::text(6822):I18n::text(6823));
    if(!batteryOk)++warnings;
    const bool coolantOk=d.coolant_temp>0&&d.coolant_temp<250;
    add(I18n::text(2971903),coolantOk?QString::number(Mems219::temperatureC(d.coolant_temp))+QStringLiteral(" °C"):I18n::text(29002),stateFor(coolantOk,true),I18n::text(coolantOk?6825:6826));
    if(!coolantOk)++issues;
    const bool rpmOk=d.engine_rpm<=8000;
    add(I18n::text(6827),I18n::text(6828).arg(d.engine_rpm),stateFor(rpmOk,true),I18n::text(rpmOk?6829:6830));
    if(!rpmOk)++issues;
    const bool mapOk=d.map_kpa<=160;
    add(I18n::text(6831),I18n::text(6832).arg(d.map_kpa),stateFor(mapOk,true),I18n::text(mapOk?6833:6834));
    if(!mapOk)++issues;
    const auto dtc1=m_naEvidence.dtc(Group::Dtc1),dtc2=m_naEvidence.dtc(Group::Dtc2);
    add(I18n::text(29027),numeric(m.throttleVoltage,2,QStringLiteral(" V")),
        (dtc1.faultBits&0x80)?I18n::text(29051):info,
        (dtc1.faultBits&0x80)?describe(Circuit::ThrottlePot,0x80):I18n::text(6837));
    QString lambda=state(m.closedLoop,6843,6844);
    if(m.lambdaFault.availability==Availability::Available)
        lambda+=QStringLiteral(" ; ")+I18n::text(29053).arg(I18n::text(m.lambdaFault.calculated?29038:29039));
    else lambda+=QStringLiteral(" ; ")+I18n::text(29002);
    if((dtc2.faultBits&4)&&circuit(p,Group::Dtc2,4)==Circuit::LambdaHeater)
        lambda+=QStringLiteral(" ; ")+describe(Circuit::LambdaHeater,4);
    add(I18n::text(6838),numeric(m.lambdaMv,0,QStringLiteral(" mV")),info,lambda);
    add(I18n::text(29029),numeric(m.injectionTime,0,QStringLiteral(" µs")),notEvaluated,
        I18n::text(29044)+QStringLiteral(" ")+I18n::text(29054));
    add(I18n::text(29031),numeric(m.idleError,0,QStringLiteral(" RPM")),notEvaluated,
        I18n::text(29043)+QStringLiteral(" ")+I18n::text(29054));
    add(I18n::text(29025),steps(m.idleHot),info,I18n::text(29054));
    add(I18n::text(29030),steps(m.idleBase),info,I18n::text(29054));
    add(I18n::text(29046),numeric(m.ambient,0,QStringLiteral(" °C")),info,I18n::text(29054));
    add(I18n::text(29047),numeric(m.fuel,0,QStringLiteral(" °C")),info,I18n::text(29054));
    const double coilTime=static_cast<double>(d.coil_time)*0.002;
    QString coilState,coilAdvice;
    if(d.engine_rpm==0) {coilState=notEvaluated;coilAdvice=I18n::text(18122);}
    else if(battery<13.5||battery>14.5) {coilState=notEvaluated;coilAdvice=I18n::text(6850);}
    else if(coilTime>=2.0&&coilTime<=3.5) {coilState=I18n::text(6851);coilAdvice=I18n::text(6852);}
    else if(coilTime>4.0) {coilState=I18n::text(6853);coilAdvice=I18n::text(6854);++issues;}
    else {coilState=I18n::text(6855);coilAdvice=I18n::text(coilTime>3.5?18121:6856);++warnings;}
    add(I18n::text(6857),I18n::text(6858).arg(coilTime,0,'f',2).arg(battery,0,'f',1),coilState,coilAdvice);
    // Existing short-term correction retained as an indicative check only on
    // standard transmission profiles; the three alternate encodings are not
    // generalized. No new long-term µs threshold is invented.
    if(!p->alternateTransmission) {
        const int shortTrim=int(d.short_term_fuel_trim)-100;
        const bool watch=qAbs(shortTrim)>20;
        add(I18n::text(29055),QString::number(shortTrim)+QStringLiteral(" %"),stateFor(!watch,true),
            I18n::text(watch?6847:6848));
        if(watch)++warnings;
    } else add(I18n::text(29055),QString::number(d.short_term_fuel_trim),notEvaluated,I18n::text(29002));
    if(m_naReference.valid) {
        add(I18n::text(6867),I18n::text(6868).arg(int(d.engine_rpm)-int(m_naReference.data.engine_rpm))
            .arg(int(d.map_kpa)-int(m_naReference.data.map_kpa))
            .arg(int(d.throttle_pot)-int(m_naReference.data.throttle_pot)),info,I18n::text(6870).arg(m_naReference.timestamp));
    }
    addPassiveChecks();
    const QString level=issues?I18n::text(6873):warnings?I18n::text(6872):I18n::text(6871);
    m_score->setText(I18n::text(6874).arg(level).arg(issues).arg(warnings));
    m_status->setText(I18n::text(6875).arg(QDateTime::currentDateTime().toString(QStringLiteral("dd/MM/yyyy hh:mm:ss"))).arg(2*issues+warnings));
    m_report->setPlainText(buildNaReport());
}

void DiagnosticPanel::resetCoolantTest()
{
    m_coolantTest.reset();m_coolantFresh.invalidate();m_coolantElapsed.invalidate();
    m_coolantTrend->clear();
}

void DiagnosticPanel::invalidateCoolantTest()
{
    m_coolantConnected=false;m_coolantFresh.invalidate();m_coolantTest.interrupt();
    refreshCoolantTest();if(m_haveData)m_report->setPlainText(buildReport());
}

QString DiagnosticPanel::coolantTestSummary() const
{
    return I18n::text(2971812)+QStringLiteral(" : ")+m_coolantValues->text()+
        QStringLiteral(" | ")+m_coolantStatus->text();
}

void DiagnosticPanel::refreshCoolantTest()
{
    using namespace Mems218;
    const auto sample=coolantSample(m_coolantConnected,m_naEvidence);
    const bool fresh=m_coolantFresh.isValid()&&m_coolantFresh.elapsed()<=3000;
    const bool running=m_coolantTest.state==TestState::Running;
    m_coolantStart->setEnabled(fresh&&sample.availability==CoolantAvailability::Ready&&!running);
    m_coolantStop->setEnabled(running);
    const auto &t=m_coolantTest;
    auto celsius=[](int c){return QString::number(c)+QStringLiteral(" °C");};
    m_coolantValues->setText(I18n::text(2971815)
        .arg(t.samples?celsius(t.initial):QStringLiteral("--"),
             t.samples?celsius(t.current):QStringLiteral("--"),
             t.samples?celsius(t.current-t.initial):QStringLiteral("--"))+
        (t.samples?QStringLiteral("\n")+I18n::text(2971904).arg(celsius(t.minimum),celsius(t.maximum)):QString()));
    int key=2971816;
    if(t.state==TestState::Interrupted)key=2971822;
    else if(t.state==TestState::Finished)key=t.storedFaultSeen?2971821:t.warmingObserved()?2971820:2971819;
    else if(running)key=t.storedFaultSeen?2971821:2971818;
    else switch(sample.availability){
        case CoolantAvailability::Disconnected:key=2971805;break;
        case CoolantAvailability::Unsupported:key=2971806;break;
        case CoolantAvailability::AwaitData:key=2971808;break;
        case CoolantAvailability::EngineStopped:key=2971817;break;
        case CoolantAvailability::Ready:key=fresh?2971816:2971808;break;
    }
    const QString message=I18n::text(key);m_coolantStatus->setText(message);
    if(!m_coolantStart->isEnabled()){
        int why=key;
        if(running)why=2971818;
        else if(!m_coolantConnected)why=2971805;
        else if(sample.availability==CoolantAvailability::Unsupported)why=2971806;
        else if(sample.availability==CoolantAvailability::EngineStopped)why=2971817;
        else if(!fresh||sample.availability!=CoolantAvailability::Ready)why=2971808;
        m_coolantStart->setToolTip(I18n::text(why));
        if(t.state==TestState::Finished||t.state==TestState::Interrupted)
            m_coolantStatus->setText(message+QStringLiteral(" ")+I18n::text(why));
    } else m_coolantStart->setToolTip(I18n::text(2971816));
    m_coolantStop->setToolTip(running?I18n::text(2971814):I18n::text(2971823));
    const bool dark=qApp&&qApp->property("ecuDarkTheme").toBool();
    m_coolantStatus->setStyleSheet(dark?QStringLiteral("color:#ff9828;"):QStringLiteral("color:#a04a00;"));
}

void DiagnosticPanel::addPassiveChecks()
{
    if(!Mems217::passiveProfile(m_naEvidence.firmwareName()))return;
    const auto &o=m_passive;
    const QString unavailable=I18n::text(6849);
    auto add=[&](int title,const QString &value,int result,int advice){
        addCheck(I18n::text(title),value,result?I18n::text(result):unavailable,I18n::text(advice));
        const int row=m_checks->rowCount()-1;
        for(int col=0;col<4;++col) {
            auto *item=m_checks->item(row,col);
            item->setData(Qt::ForegroundRole,QVariant());
            item->setToolTip(I18n::text(advice));
        }
    };
    add(2971710,o.valid?QStringLiteral("%1 mV | %2").arg(o.mv).arg(o.crossings):QStringLiteral("--"),
        !o.valid?0:o.lambdaFault?6802:o.lambdaObserved?2971712:0,
        !o.valid?2971713:o.lambdaFault?2971714:!o.closedLoop?2971715:o.lambdaObserved?2971716:2971717);
    add(2971711,o.valid?QStringLiteral("%1 °C (%2–%3)").arg(int(o.coolantRaw)-55).arg(int(o.low)-55).arg(int(o.high)-55):QStringLiteral("--"),
        !o.valid?0:o.coolantFault?6802:o.coolantObserved?2971712:0,
        !o.valid?2971713:o.coolantFault?2971718:o.coolantObserved?2971719:2971720);
}

QString DiagnosticPanel::buildNaReport() const
{
    QStringList lines;
    lines<<I18n::text(6879).trimmed()
         <<I18n::text(6880).arg(QDateTime::currentDateTime().toString(QStringLiteral("dd/MM/yyyy hh:mm:ss"))).trimmed();
    if(!m_ecuId.isEmpty())lines<<I18n::text(6881).arg(QString::fromLatin1(m_ecuId.toHex(' ').toUpper())).trimmed();
    // Export exactly the same decoded and qualified values as the live table.
    for(int row=0;row<m_checks->rowCount();++row) {
        QStringList fields;
        for(int col=0;col<4;++col)fields<<m_checks->item(row,col)->text();
        lines<<fields.join(QStringLiteral(" | "));
    }
    if(m_coolantTest.state!=Mems218::TestState::Idle)lines<<coolantTestSummary();
    if(m_naReference.valid)lines<<I18n::text(6889).arg(m_naReference.timestamp).trimmed();
    lines<<I18n::text(6890).trimmed();
    return lines.join(QLatin1Char('\n'));
}
