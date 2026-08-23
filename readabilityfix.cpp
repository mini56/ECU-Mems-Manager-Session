#include <QApplication>
#include <QColor>
#include <QCoreApplication>
#include <QEvent>
#include <QFontMetrics>
#include <QLabel>
#include <QMainWindow>
#include <QPainter>
#include <QPointer>
#include <QPolygonF>
#include <QStringList>
#include <QTableWidget>
#include <QTimer>
#include <QWidget>
#include <QtMath>

#include "analysistab.h"
#include "i18n.h"

namespace {

static void forceReadableTable(QTableWidget *table)
{
    if (!table)
        return;

    const QString readableRule = QStringLiteral(
        "QTableWidget{color:#ffffff;}"
        "QTableWidget:disabled{color:#ffffff;}"
        "QTableWidget::item{color:#ffffff;}"
        "QTableWidget::item:disabled{color:#ffffff;}");

    QString style = table->styleSheet();
    if (!style.contains(readableRule))
    {
        style += readableRule;
        table->setStyleSheet(style);
    }

    for (int row = 0; row < table->rowCount(); ++row)
    {
        for (int column = 0; column < table->columnCount(); ++column)
        {
            if (QTableWidgetItem *item = table->item(row, column))
                item->setData(Qt::ForegroundRole, QColor(QStringLiteral("#ffffff")));
        }
    }
}

static void applyRequestedReadableText(QMainWindow *window)
{
    if (!window)
        return;

    // Toutes les mesures : rendre lisibles uniquement les lignes des 3 tableaux.
    for (QWidget *widget : window->findChildren<QWidget*>())
    {
        if (QString::fromLatin1(widget->metaObject()->className()) == QStringLiteral("SummaryTab"))
        {
            const QList<QTableWidget*> tables = widget->findChildren<QTableWidget*>();
            for (QTableWidget *table : tables)
                forceReadableTable(table);
            break;
        }
    }

    // Toutes les données : conserver les en-têtes orange et passer les lignes en blanc.
    QWidget *rawPage = window->findChild<QWidget*>(QStringLiteral("raw"));
    if (rawPage)
    {
        const QStringList blocks = { QStringLiteral("raw_1"), QStringLiteral("raw_2") };
        for (const QString &blockName : blocks)
        {
            QWidget *block = rawPage->findChild<QWidget*>(blockName);
            if (!block)
                continue;

            for (QLabel *label : block->findChildren<QLabel*>())
            {
                const QString name = label->objectName();
                const bool header = name.startsWith(QStringLiteral("header_")) ||
                                    name.startsWith(QStringLiteral("Aheader_"));
                if (!header)
                    label->setStyleSheet(QStringLiteral("color:#ffffff;background:transparent;"));
            }
        }
    }

    // Diagnostic automatique : rendre lisibles uniquement les lignes du tableau.
    for (QWidget *widget : window->findChildren<QWidget*>())
    {
        if (QString::fromLatin1(widget->metaObject()->className()) == QStringLiteral("DiagnosticPanel"))
        {
            forceReadableTable(widget->findChild<QTableWidget*>());
            break;
        }
    }
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

class ChartReadabilityFilter : public QObject
{
public:
    explicit ChartReadabilityFilter(QObject *parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (!event || event->type() != QEvent::Paint)
            return QObject::eventFilter(watched, event);

        if (SingleChartWidget *chart = qobject_cast<SingleChartWidget*>(watched))
        {
            paintAnalysisChart(chart);
            return true;
        }

        return QObject::eventFilter(watched, event);
    }
};

class WindowReadabilityFilter : public QObject
{
public:
    explicit WindowReadabilityFilter(QApplication *app)
        : QObject(app), m_app(app)
    {
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (!event)
            return QObject::eventFilter(watched, event);

        if (!m_window)
        {
            QMainWindow *candidate = qobject_cast<QMainWindow*>(watched);
            if (!candidate || candidate->objectName() != QStringLiteral("MainWindow"))
                return QObject::eventFilter(watched, event);

            if (event->type() != QEvent::Show && event->type() != QEvent::Polish)
                return QObject::eventFilter(watched, event);

            m_window = candidate;
            candidate->setProperty("requestedReadableTextScheduled", true);
            candidate->installEventFilter(this);
            if (m_app)
                m_app->removeEventFilter(this);

            QTimer::singleShot(1500, candidate, [this]() {
                if (m_window)
                    applyRequestedReadableText(m_window);
            });
            return QObject::eventFilter(watched, event);
        }

        if (watched == m_window && event->type() == QEvent::Resize)
        {
            QTimer::singleShot(120, m_window, [this]() {
                if (m_window)
                    applyRequestedReadableText(m_window);
            });
        }

        return QObject::eventFilter(watched, event);
    }

private:
    QApplication *m_app;
    QPointer<QMainWindow> m_window;
};

void installRequestedReadableTextFix()
{
    QApplication *app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app)
        return;

    // The chart override still needs to see paint events for dynamically
    // created SingleChartWidget instances. It now receives paint events only;
    // all other MainWindow readability work is moved to a local window filter.
    app->installEventFilter(new ChartReadabilityFilter(app));
    app->installEventFilter(new WindowReadabilityFilter(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installRequestedReadableTextFix)
