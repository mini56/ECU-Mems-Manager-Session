#include "analysistab.h"
#include "i18n.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetrics>
#include <QGridLayout>
#include <QMessageBox>
#include <QPainter>
#include <QPalette>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QTime>
#include <QTextStream>
#include <QtMath>

namespace {

static QString friendlyColumnName(const QString &rawName)
{
    static QMap<QString, QString> names;
    if (names.isEmpty()) {
        names["80x01-02_engine-rpm"] = I18n::text(6400);
        names["80x03_coolant_temp"] = I18n::text(6401);
        names["80x04_ambient_temp"] = I18n::text(6402);
        names["80x05_intake_air_temp"] = I18n::text(6403);
        names["80x06_fuel_temp"] = I18n::text(6404);
        names["80x07_map_kpa"] = I18n::text(6405);
        names["80x08_battery_voltage"] = I18n::text(6406);
        names["80x09_throttle_pot"] = I18n::text(6407);
        names["80x0A_idle_switch"] = I18n::text(6408);
        names["80x0C_park_neutral_switch"] = I18n::text(6409);
        names["80x0D-0E_fault_codes"] = I18n::text(6410);
        names["80x0F_idle_set_point"] = I18n::text(6411);
        names["80x10_idle_hot"] = I18n::text(6412);
        names["80x12_iac_position"] = I18n::text(6413);
        names["80x13-14_idle_error"] = I18n::text(6414);
        names["80x15_ignition_advance_offset"] = I18n::text(6415);
        names["80x16_ignition_advance"] = I18n::text(6416);
        names["80x17-18_coil_time"] = I18n::text(6417);
        names["80x19_crankshaft_position_sensor"] = I18n::text(6418);
        names["7dx01_ignition_switch"] = I18n::text(6419);
        names["7dx02_throttle_angle"] = I18n::text(6420);
        names["7dx04_air_fuel_ratio"] = I18n::text(6421);
        names["7dx05_dtc2"] = I18n::text(6422);
        names["7dx06_lambda_voltage"] = I18n::text(6423);
        names["7dx07_lambda_sensor_frequency"] = I18n::text(6424);
        names["7dx08_lambda_sensor_dutycycle"] = I18n::text(6425);
        names["7dx09_lambda_sensor_status"] = I18n::text(6426);
        names["7dx0A_closed_loop"] = I18n::text(6427);
        names["7dx0B_long_term_fuel_trim"] = I18n::text(6428);
        names["7dx0C_short_term_fuel_trim"] = I18n::text(6429);
        names["7dx0D_carbon_canister_dutycycle"] = I18n::text(6430);
        names["7dx0E_dtc3"] = I18n::text(6431);
        names["7dx0F_idle_base_pos"] = I18n::text(6432);
        names["7dx11_dtc4"] = I18n::text(6433);
        names["7dx12_ignition_advance2"] = I18n::text(6434);
        names["7dx13_idle_speed_offset"] = I18n::text(6435);
        names["7dx14-15_idle_error_hot_corrected"] = I18n::text(6436);
        names["7dx14-15_raw"] = I18n::text(6437);
        names["7dx16_dtc5"] = I18n::text(6438);
    }
    if (names.contains(rawName)) return names.value(rawName);
    if (rawName.contains("_uk") || rawName.contains("uk1") || rawName.contains("uk2"))
        return I18n::text(6439).arg(rawName);
    return rawName;
}

static QStringList diagnosticRawNames()
{
    return QStringList()
        << "80x01-02_engine-rpm" << "80x03_coolant_temp" << "80x04_ambient_temp"
        << "80x05_intake_air_temp" << "80x06_fuel_temp" << "80x07_map_kpa"
        << "80x08_battery_voltage" << "80x09_throttle_pot" << "80x0A_idle_switch"
        << "80x0B_uk1" << "80x0C_park_neutral_switch" << "80x0D-0E_fault_codes"
        << "80x0F_idle_set_point" << "80x10_idle_hot" << "80x11_uk2"
        << "80x12_iac_position" << "80x13-14_idle_error" << "80x15_ignition_advance_offset"
        << "80x16_ignition_advance" << "80x17-18_coil_time" << "80x19_crankshaft_position_sensor"
        << "80x1A_uk4" << "80x1B_uk5"
        << "7dx01_ignition_switch" << "7dx02_throttle_angle" << "7dx03_uk6"
        << "7dx04_air_fuel_ratio" << "7dx05_dtc2" << "7dx06_lambda_voltage"
        << "7dx07_lambda_sensor_frequency" << "7dx08_lambda_sensor_dutycycle"
        << "7dx09_lambda_sensor_status" << "7dx0A_closed_loop"
        << "7dx0B_long_term_fuel_trim" << "7dx0C_short_term_fuel_trim"
        << "7dx0D_carbon_canister_dutycycle" << "7dx0E_dtc3" << "7dx0F_idle_base_pos"
        << "7dx10_uk7" << "7dx11_dtc4" << "7dx12_ignition_advance2"
        << "7dx13_idle_speed_offset" << "7dx14-15_idle_error_hot_corrected"
        << "7dx14-15_raw" << "7dx16_dtc5" << "7dx17_uk11" << "7dx18_uk12"
        << "7dx19_uk13" << "7dx1A_uk14" << "7dx1B_uk15" << "7dx1C_uk16"
        << "7dx1D_uk1A" << "7dx1E_uk1B" << "7dx1F_uk1C";
}

static QVector<double> diagnosticValues(const mems_data *d)
{
    QVector<double> v;
    if (!d) return v;
    const int raw7d1415 = (int(d->idle_error2) << 8) | int(d->uk10);
    const int idleHotCorrection = int(d->idle_hot) - 35;
    v << d->engine_rpm
      << (int(d->coolant_temp) - 55) << (int(d->ambient_temp) - 55)
      << (int(d->intake_air_temp) - 55) << (int(d->fuel_temp) - 55)
      << d->map_kpa << (d->battery_voltage / 10.0) << (d->throttle_pot * 0.02)
      << d->idle_switch << d->uk1 << d->park_neutral_switch
      << (((quint16)d->dtc1 << 8) | d->dtc0)
      << d->idle_set_point << d->idle_hot << d->uk2 << d->iac_position << d->idle_error
      << d->ignition_advance_offset << ((d->ignition_advance * 0.5) - 24.0)
      << (d->coil_time * 0.002) << d->uk3 << d->uk4 << d->uk5
      << d->ignition_switch << d->throttle_angle << d->uk6 << d->air_fuel_ratio << d->dtc2
      << (d->lambda_voltage * 5.0) << d->lambda_sensor_frequency << d->lambda_sensor_dutycycle
      << d->lambda_sensor_status << d->closed_loop << d->long_term_fuel_trim
      << d->short_term_fuel_trim << d->carbon_canister_dutycycle << d->dtc3
      << d->idle_base_pos << d->uk7 << d->dtc4 << d->ignition_advance2
      << d->idle_speed_offset << ((raw7d1415 - 32768) + idleHotCorrection) << raw7d1415
      << d->dtc5 << d->uk11 << d->uk12 << d->uk13 << d->uk14 << d->uk15 << d->uk16
      << d->uk1A << d->uk1B << d->uk1C;
    return v;
}

static void drawEmptyMessage(QPainter &p, const QRect &r, const QString &message)
{
    p.setPen(QColor("#8a93a6"));
    p.drawText(r, Qt::AlignCenter | Qt::TextWordWrap, message);
}

} // namespace

SingleChartWidget::SingleChartWidget(const QString &name, const QColor &color, QWidget *parent)
    : QWidget(parent), m_name(name), m_color(color), m_hasCursor(false), m_cursorX(0)
{
    setMinimumHeight(150);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
}

void SingleChartWidget::setData(const QVector<double> &time, const QVector<double> &values)
{
    m_time = time;
    m_values = values;
    update();
}

void SingleChartWidget::mouseMoveEvent(QMouseEvent *event)
{
    m_hasCursor = true;
    m_cursorX = event->pos().x();
    update();
}

void SingleChartWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    m_hasCursor = false;
    update();
}

void SingleChartWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor("#0d1218"));

    QFont title = p.font(); title.setBold(true); title.setPointSize(9); p.setFont(title);
    p.setPen(QColor("#edf2f4"));
    p.drawText(QRect(0, 3, width(), 20), Qt::AlignHCenter | Qt::AlignVCenter, m_name);

    const int left = 55, right = 10, top = 27, bottom = 22;
    QRect plot(left, top, qMax(1, width()-left-right), qMax(1, height()-top-bottom));
    p.setPen(QPen(QColor("#26313b"), 1));
    p.setBrush(QColor("#090e13"));
    p.drawRect(plot);

    if (m_time.size() < 2 || m_values.size() < 2) {
        drawEmptyMessage(p, plot, I18n::text(6440));
        return;
    }

    double tMin=m_time.first(), tMax=m_time.last();
    double tSpan=qMax(0.001,tMax-tMin);
    double vMin=m_values.first(), vMax=m_values.first();
    for (double x:m_values) { vMin=qMin(vMin,x); vMax=qMax(vMax,x); }
    if (qAbs(vMax-vMin)<0.0001) { vMin-=1.0; vMax+=1.0; }
    const double margin=(vMax-vMin)*0.08; vMin-=margin; vMax+=margin;
    const double vSpan=vMax-vMin;

    QFont axis=p.font(); axis.setBold(false); axis.setPointSize(7); p.setFont(axis);
    for(int i=0;i<=4;++i){
        int y=plot.top()+plot.height()*i/4;
        p.setPen(QColor("#233047")); p.drawLine(plot.left(),y,plot.right(),y);
        double val=vMax-vSpan*i/4.0;
        p.setPen(QColor("#9aa4ad"));
        p.drawText(QRect(0,y-8,left-6,16),Qt::AlignRight|Qt::AlignVCenter,
                   QString::number(val,'f',qAbs(val)<10?2:1));
    }

    QPolygonF poly;
