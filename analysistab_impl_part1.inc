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
#include <QFrame>
#include <QShowEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QPalette>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QTime>
#include <QTextStream>
#include <QtMath>

namespace {

// Original AnalysisTab owns styling and resizing: no application-wide filter.
static void fitAnalysisModernSizing(QWidget *tab);
    static qreal analysisUiScale(QWidget *tab)
    {
        QWidget *top = tab ? tab->window() : nullptr;
        if (top && top->property("globalUiScale").isValid())
            return qBound<qreal>(0.62, top->property("globalUiScale").toDouble(), 1.18);
        return 1.0;
    }

    static void applyAnalysisModernStyle(QWidget *tab)
    {
        if (!tab)
            return;

        tab->setMinimumSize(0, 0);
        tab->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        tab->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        tab->setAttribute(Qt::WA_StyledBackground, true);
        tab->setStyleSheet(QStringLiteral(
            "AnalysisTab{background:#0b0f14;color:#dce2e7;}"
            "QWidget{color:#dce2e7;}"
            "QLabel{background:transparent;border:0;color:#dce2e7;}"
            "#analysisRightPanel{background:#0f151b;border:1px solid #26313b;border-radius:0;}"
            "#analysisParametersTitle,#analysisToolsTitle{color:#ff9828;font-weight:700;padding:3px 0;border-bottom:1px solid #2b3540;}"
            "#analysisFileLabel{color:#aeb8bf;font-style:italic;}"
            "QPushButton{background:#17202a;color:#dce2e7;border:1px solid #34414d;border-radius:2px;padding:4px 8px;font-weight:600;min-height:23px;}"
            "QPushButton:hover{border-color:#ff8a1c;color:#fff;}"
            "QPushButton:pressed{background:#202b36;}"
            "QPushButton:checked{background:#ff8a1c;color:#101419;border-color:#ff9b32;}"
            "QPushButton:disabled{background:#111820;color:#5f6972;border-color:#28323b;}"
            "QScrollArea{background:#0d1218;border:0;}"
            "QScrollArea>QWidget>QWidget{background:#0d1218;}"
            "QCheckBox{spacing:6px;padding:2px 1px;color:#dce2e7;background:transparent;}"
            "QScrollBar:vertical{background:#0c1116;width:8px;margin:1px;}"
            "QScrollBar::handle:vertical{background:#384550;border-radius:1px;min-height:24px;}"
            "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
        ));

        const QList<QScrollArea*> scrollAreas = tab->findChildren<QScrollArea*>();
        for (QScrollArea *scroll : scrollAreas)
        {
            scroll->setFrameShape(QFrame::NoFrame);
            scroll->setWidgetResizable(true);
            if (scroll->viewport())
            {
                scroll->viewport()->setAutoFillBackground(false);
                scroll->viewport()->setStyleSheet(QStringLiteral("background:#0d1218;"));
            }
        }

        fitAnalysisModernSizing(tab);
    }

    static void fitAnalysisModernSizing(QWidget *tab)
    {
        if (!tab)
            return;

        const qreal scale = analysisUiScale(tab);
        if (QLayout *root = tab->layout())
        {
            const int margin = qBound(4, qRound(6.0 * scale), 8);
            root->setContentsMargins(margin, margin, margin, margin);
            root->setSpacing(margin);
            root->setSizeConstraint(QLayout::SetDefaultConstraint);
        }

        QWidget *right = tab->findChild<QWidget*>(QStringLiteral("analysisRightPanel"));
        const int rightWidth = qBound(210, qRound(tab->width() * 0.20), 300);
        if (right)
        {
            right->setMinimumWidth(rightWidth);
            right->setMaximumWidth(rightWidth);
        }

        const qreal base = tab->property("analysisBaseFontPointSize").isValid()
            ? tab->property("analysisBaseFontPointSize").toDouble()
            : 9.0;
        QFont font = tab->font();
        font.setPointSizeF(qMax<qreal>(6.5, base * scale));
        tab->setFont(font);
        tab->setProperty("analysisScale", scale);

        const int buttonHeight = qBound(22, qRound(27.0 * scale), 32);
        const QList<QPushButton*> buttons = tab->findChildren<QPushButton*>();
        for (QPushButton *button : buttons)
        {
            button->setMinimumHeight(buttonHeight);
            button->setMaximumHeight(qMax(buttonHeight, 34));
        }

        const QList<QWidget*> children = tab->findChildren<QWidget*>();
        for (QWidget *child : children)
        {
            const QString cls = QString::fromLatin1(child->metaObject()->className());
            if (cls == QStringLiteral("SingleChartWidget") || cls == QStringLiteral("ChartWidget"))
            {
                child->setMinimumHeight(qBound(105, qRound(170.0 * scale), 220));
                child->setMaximumHeight(QWIDGETSIZE_MAX);
                child->setMinimumWidth(0);
                child->setMaximumWidth(QWIDGETSIZE_MAX);
                child->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            }
        }
    }


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

struct AxisSpec
{
    double minimum;
    double maximum;
    double step;
    int decimals;
};

static bool chartNameIs(const QString &name, int textId)
{
    return name == I18n::text(textId);
}

static bool isBinarySignal(const QString &name)
{
    return chartNameIs(name, 6408) ||
           chartNameIs(name, 6409) ||
           chartNameIs(name, 6418) ||
           chartNameIs(name, 6419) ||
           chartNameIs(name, 6426) ||
           chartNameIs(name, 6427);
}

static bool isFixedPercentSignal(const QString &name)
{
    return chartNameIs(name, 6407) ||
           chartNameIs(name, 6413) ||
           chartNameIs(name, 6425) ||
           chartNameIs(name, 6430);
}

static bool isZeroBasedSignal(const QString &name)
{
    return chartNameIs(name, 6400) ||
           chartNameIs(name, 6405) ||
           chartNameIs(name, 6410) ||
           chartNameIs(name, 6411) ||
           chartNameIs(name, 6412) ||
           chartNameIs(name, 6417) ||
           chartNameIs(name, 6421) ||
           chartNameIs(name, 6422) ||
           chartNameIs(name, 6423) ||
           chartNameIs(name, 6424) ||
           chartNameIs(name, 6431) ||
           chartNameIs(name, 6432) ||
           chartNameIs(name, 6433) ||
           chartNameIs(name, 6437) ||
           chartNameIs(name, 6438);
}

static bool isIntegerSignal(const QString &name)
{
    return chartNameIs(name, 6400) ||
           chartNameIs(name, 6410) ||
           chartNameIs(name, 6411) ||
           chartNameIs(name, 6412) ||
           chartNameIs(name, 6418) ||
           chartNameIs(name, 6419) ||
           chartNameIs(name, 6422) ||
           chartNameIs(name, 6426) ||
           chartNameIs(name, 6427) ||
           chartNameIs(name, 6431) ||
           chartNameIs(name, 6432) ||
           chartNameIs(name, 6433) ||
           chartNameIs(name, 6437) ||
           chartNameIs(name, 6438);
}

static double minimumUsefulSpan(const QString &name)
{
    if (chartNameIs(name, 6400))
        return 500.0;
    if (chartNameIs(name, 6401) || chartNameIs(name, 6402) ||
        chartNameIs(name, 6403) || chartNameIs(name, 6404))
        return 2.0;
    if (chartNameIs(name, 6406))
        return 0.8;
    if (chartNameIs(name, 6417))
        return 0.5;
    if (isIntegerSignal(name))
        return 4.0;
    return 1.0;
}

static double niceStep(double rawStep)
{
    if (rawStep <= 0.0)
        return 1.0;

    const double exponent = qFloor(qLn(rawStep) / qLn(10.0));
    const double power = qPow(10.0, exponent);
    const double fraction = rawStep / power;

    double niceFraction = 1.0;
    if (fraction <= 1.0)
        niceFraction = 1.0;
    else if (fraction <= 2.0)
        niceFraction = 2.0;
    else if (fraction <= 2.5)
        niceFraction = 2.5;
    else if (fraction <= 5.0)
        niceFraction = 5.0;
    else
        niceFraction = 10.0;

    return niceFraction * power;
}

static int decimalsForStep(double step)
{
    if (step >= 1.0)
        return 0;
    if (step >= 0.1)
        return 1;
    if (step >= 0.01)
        return 2;
    return 3;
}

static AxisSpec axisForChart(const QString &name, const QVector<double> &values)
{
    if (isBinarySignal(name))
        return { 0.0, 1.0, 1.0, 0 };

    if (isFixedPercentSignal(name))
        return { 0.0, 100.0, 25.0, 0 };

    double rawMin = values.isEmpty() ? 0.0 : values.first();
    double rawMax = rawMin;
    for (double value : values)
    {
        rawMin = qMin(rawMin, value);
        rawMax = qMax(rawMax, value);
    }

    const bool zeroBased = isZeroBasedSignal(name);
    const bool integerSignal = isIntegerSignal(name);
    const double minSpan = minimumUsefulSpan(name);
    double span = rawMax - rawMin;

    if (zeroBased && rawMin >= 0.0)
    {
        rawMin = 0.0;
        rawMax = qMax(rawMax * 1.08, minSpan);
    }
    else if (span < minSpan)
    {
        const double center = (rawMin + rawMax) * 0.5;
        rawMin = center - minSpan * 0.5;
        rawMax = center + minSpan * 0.5;
    }
    else
    {
        const double padding = span * 0.08;
        rawMin -= padding;
        rawMax += padding;
    }

    span = rawMax - rawMin;
    double step = niceStep(span / 4.0);
    if (integerSignal && step < 1.0)
        step = 1.0;

    double minimum = qFloor(rawMin / step) * step;
    double maximum = qCeil(rawMax / step) * step;

    if (zeroBased && minimum < 0.0)
        minimum = 0.0;
    if (maximum <= minimum)
        maximum = minimum + step;

    return { minimum, maximum, step, decimalsForStep(step) };
}

static void paintAnalysisChart(SingleChartWidget *chart)
{
    if (!chart)
        return;

    QPainter painter(chart);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(chart->rect(), QColor(QStringLiteral("#0b1116")));

    const QString &name = chart->chartName();
    const QColor &curveColor = chart->chartColor();
    const QVector<double> &time = chart->chartTime();
    const QVector<double> &values = chart->chartValues();

    painter.setPen(QColor(QStringLiteral("#e7edf0")));
    QFont titleFont = painter.font();
    titleFont.setBold(true);
    titleFont.setPointSize(9);
    painter.setFont(titleFont);
    painter.drawText(QRect(0, 4, chart->width(), 18), Qt::AlignHCenter, name);

    const int leftMargin = 55;
    const int rightMargin = 10;
    const int topMargin = 26;
    const int bottomMargin = 20;
    const QRect plotRect(leftMargin, topMargin,
                         chart->width() - leftMargin - rightMargin,
                         chart->height() - topMargin - bottomMargin);

    painter.setPen(QPen(QColor(QStringLiteral("#35414b")), 1));
    painter.setBrush(QColor(QStringLiteral("#151d23")));
    painter.drawRect(plotRect);

    if (time.count() < 2 || values.count() < 2)
    {
        painter.setPen(QColor(QStringLiteral("#aab5bd")));
        painter.drawText(plotRect, Qt::AlignCenter, I18n::text(6440));
        return;
    }

    const double tMin = time.first();
    const double tMax = time.last();
    const double tSpan = (tMax - tMin) > 0.0001 ? (tMax - tMin) : 1.0;

    const AxisSpec axis = axisForChart(name, values);
    const double vSpan = axis.maximum - axis.minimum;

    QFont axisFont = painter.font();
    axisFont.setBold(false);
    axisFont.setPointSize(7);
    painter.setFont(axisFont);

    const int tickCount = qMax(2, int(qRound(vSpan / axis.step)) + 1);
    for (int i = 0; i < tickCount; ++i)
    {
        const double value = axis.maximum - axis.step * i;
        if (value < axis.minimum - axis.step * 0.001)
            break;
        const double fraction = (axis.maximum - value) / vSpan;
        const int y = plotRect.top() + qRound(plotRect.height() * fraction);

        painter.setPen(QColor(QStringLiteral("#2a343d")));
        painter.drawLine(plotRect.left(), y, plotRect.right(), y);

        painter.setPen(QColor(QStringLiteral("#aab5bd")));
        painter.drawText(QRect(0, y - 8, leftMargin - 6, 16),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(value, 'f', axis.decimals));
    }

    painter.setPen(QPen(curveColor, 2));
    QPolygonF poly;
    const int count = qMin(time.count(), values.count());
    for (int i = 0; i < count; ++i)
    {
        const double xFraction = (time[i] - tMin) / tSpan;
        const double yFraction = (values[i] - axis.minimum) / vSpan;
        const double x = plotRect.left() + xFraction * plotRect.width();
        const double y = plotRect.bottom() - yFraction * plotRect.height();
        poly << QPointF(x, y);
    }
    painter.drawPolyline(poly);

    painter.setPen(QColor(QStringLiteral("#aab5bd")));
    for (int i = 0; i <= 5; ++i)
    {
        const double t = tMin + (tSpan * i) / 5.0;
        const int x = plotRect.left() + (plotRect.width() * i) / 5;
        painter.drawText(QRect(x - 30, plotRect.bottom() + 2, 60, 16),
                         Qt::AlignCenter,
                         QStringLiteral("%1 s").arg(t - tMin, 0, 'f', 0));
    }

    if (chart->chartHasCursor() && chart->chartCursorX() >= plotRect.left() &&
        chart->chartCursorX() <= plotRect.right())
    {
        const int cursorX = chart->chartCursorX();
        painter.setPen(QPen(QColor(QStringLiteral("#7f8b94")), 1, Qt::DashLine));
        painter.drawLine(cursorX, plotRect.top(), cursorX, plotRect.bottom());

        const double xFraction = double(cursorX - plotRect.left()) / double(plotRect.width());
        const double tAtCursor = tMin + xFraction * tSpan;
        int index = 0;
        double best = 1e18;
        for (int i = 0; i < time.count(); ++i)
        {
            const double distance = qAbs(time[i] - tAtCursor);
            if (distance < best)
            {
                best = distance;
                index = i;
            }
        }

        if (index < values.count())
        {
            const int valueDecimals = qMax(1, axis.decimals);
            const QString label = QString::number(values[index], 'f', valueDecimals);
            QFontMetrics metrics(axisFont);
            const int textWidth = metrics.horizontalAdvance(label) + 10;
            int boxX = cursorX + 6;
            if (boxX + textWidth > plotRect.right())
                boxX = cursorX - 6 - textWidth;

            painter.setPen(QPen(QColor(QStringLiteral("#3a4650")), 1));
            painter.setBrush(QColor(QStringLiteral("#0a1015")));
            painter.drawRect(boxX, plotRect.top() + 4, textWidth, 16);
            painter.setPen(curveColor);
            painter.drawText(QRect(boxX + 5, plotRect.top() + 4, textWidth - 5, 16),
                             Qt::AlignVCenter | Qt::AlignLeft, label);
        }
    }
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

// Authoritative chart drawing: previously intercepted by a global Paint
// event filter. This native method renders exactly the same chart.
void SingleChartWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    paintAnalysisChart(this);
}
