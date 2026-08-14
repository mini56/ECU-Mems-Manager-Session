#include <QApplication>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEvent>
#include <QLabel>
#include <QMainWindow>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QTimer>
#include <QVariant>
#include <QVector>
#include <QWidget>
#include <QtMath>

namespace {

class ModernGaugeCard : public QWidget
{
public:
    ModernGaugeCard(const QRect &geometry, QObject *source, const QString &title, QWidget *parent)
        : QWidget(parent), m_source(source), m_title(title)
    {
        setGeometry(geometry);
        setProperty("screenfitBaseGeometry", geometry);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        m_clock.start();
        show();
    }

    void sample()
    {
        if (!m_source) return;
        const QVariant vv = m_source->property("value");
        if (!vv.isValid()) return;

        const qint64 now = m_clock.elapsed();
        m_history.append(qMakePair(now, vv.toDouble()));
        const qint64 cutoff = now - 120000;
        while (!m_history.isEmpty() && m_history.first().first < cutoff)
            m_history.removeFirst();
        if (m_history.size() > 1000)
            m_history.remove(0, m_history.size() - 1000);
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        const qreal sx = width() / 238.0;
        const qreal sy = height() / 252.0;
        const qreal s = qBound<qreal>(0.62, qMin(sx, sy), 1.35);
        const qreal thin = qMax<qreal>(0.75, 0.9 * s);

        // One compact DARK PRO card, matching the approved reference proportions.
        const QRectF outer = rect().adjusted(thin, thin, -thin, -thin);
        p.setPen(QPen(QColor("#26313a"), thin));
        p.setBrush(QColor("#0b1015"));
        p.drawRoundedRect(outer, 3.0 * s, 3.0 * s);

        // Title strip.
        QFont titleFont = p.font();
        titleFont.setBold(true);
        titleFont.setPointSizeF(qBound<qreal>(5.8, 6.8 * s, 8.5));
        p.setFont(titleFont);
        p.setPen(QColor("#d7dde2"));
        p.drawText(QRectF(7*s, 5*s, width()-14*s, 17*s),
                   Qt::AlignHCenter | Qt::AlignVCenter, m_title.toUpper());
        p.setPen(QPen(QColor("#202830"), thin));
        p.drawLine(QPointF(7*s, 24*s), QPointF(width()-7*s, 24*s));

        const double value = m_source ? m_source->property("value").toDouble() : 0.0;
        double minv = m_source ? m_source->property("minimum").toDouble() : 0.0;
        double maxv = m_source ? m_source->property("maximum").toDouble() : 100.0;
        if (qFuzzyCompare(minv, maxv)) { minv = 0.0; maxv = 100.0; }

        QString suffix = m_source ? m_source->property("suffix").toString().trimmed() : QString();
        suffix.remove('[');
        suffix.remove(']');
        suffix = suffix.trimmed();

        // Dial dimensions follow the approved visual: large circle, little dead space.
        const QPointF center(width()/2.0, 103.0*s);
        const qreal radius = qMin<qreal>(86.0*s, (width()-30.0*s)/2.0);

        // Layered black face with thin metallic rings.
        QRadialGradient face(center - QPointF(13*s, 16*s), radius * 1.15);
        face.setColorAt(0.0, QColor("#171d22"));
        face.setColorAt(0.55, QColor("#0c1116"));
        face.setColorAt(1.0, QColor("#05080b"));
        p.setPen(QPen(QColor("#394650"), 1.0*s));
        p.setBrush(face);
        p.drawEllipse(center, radius, radius);

        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor("#121a20"), 4.2*s));
        p.drawEllipse(center, radius-4.2*s, radius-4.2*s);
        p.setPen(QPen(QColor("#33404a"), 0.85*s));
        p.drawEllipse(center, radius-8.5*s, radius-8.5*s);

        const qreal startDeg = -135.0;
        const qreal sweepDeg = 270.0;

        // Dense graduation pattern, with the same orange/red warning sector on the right.
        p.save();
        p.translate(center);
        for (int i = 0; i <= 60; ++i) {
            const bool major = (i % 10) == 0;
            const bool medium = (!major && (i % 5) == 0);
            const qreal angle = qDegreesToRadians(startDeg + sweepDeg * i / 60.0);
            const qreal outerR = radius - 9.0*s;
            const qreal tickLen = (major ? 11.0 : (medium ? 7.0 : 4.0)) * s;
            const QPointF a(qCos(angle) * outerR, qSin(angle) * outerR);
            const QPointF b(qCos(angle) * (outerR - tickLen), qSin(angle) * (outerR - tickLen));

            QColor tick("#78828b");
            if (major) tick = QColor("#e1e5e8");
            if (i >= 55) tick = QColor("#e34732");
            else if (i >= 50) tick = QColor("#f47b20");

            p.setPen(QPen(tick, (major ? 1.20 : (medium ? 0.95 : 0.65))*s,
                          Qt::SolidLine, Qt::FlatCap));
            p.drawLine(b, a);
        }
        p.restore();

        // Scale numbers around the dial.
        QFont scaleFont = p.font();
        scaleFont.setBold(false);
        scaleFont.setPointSizeF(qBound<qreal>(4.8, 5.55*s, 7.1));
        p.setFont(scaleFont);
        p.setPen(QColor("#c2c8cd"));
        for (int i = 0; i <= 6; ++i) {
            const double fv = minv + (maxv-minv) * i / 6.0;
            const qreal angle = qDegreesToRadians(startDeg + sweepDeg * i / 6.0);
            const qreal rr = radius - 24.0*s;
            const QPointF pos = center + QPointF(qCos(angle)*rr, qSin(angle)*rr);
            const double span = qAbs(maxv-minv);
            const QString txt = span <= 20.0 ? QString::number(fv, 'f', 1)
                                              : QString::number(fv, 'f', 0);
            p.drawText(QRectF(pos.x()-21*s, pos.y()-6*s, 42*s, 12*s), Qt::AlignCenter, txt);
        }

        // Needle position.
        double norm = (value-minv)/(maxv-minv);
        const bool inRange = value >= minv && value <= maxv;
        norm = qBound(0.0, norm, 1.0);
        const qreal needleDeg = startDeg + sweepDeg*norm;
        const qreal na = qDegreesToRadians(needleDeg);
        const QPointF direction(qCos(na), qSin(na));
        const QPointF normal(-direction.y(), direction.x());
        const QPointF tip = center + direction*(radius-20*s);
        const QPointF rear = center - direction*(10*s);

        // Approved needle is a tapered orange pointer, not a thick line.
        QPainterPath needleShadow;
        needleShadow.moveTo(tip + normal*1.5*s);
        needleShadow.lineTo(rear + normal*3.3*s);
        needleShadow.lineTo(rear - normal*3.3*s);
        needleShadow.lineTo(tip - normal*1.5*s);
        needleShadow.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 104, 0, 55));
        p.drawPath(needleShadow.translated(0.8*s, 0.8*s));

        QPainterPath needle;
        needle.moveTo(tip);
        needle.lineTo(center + normal*2.4*s);
        needle.lineTo(rear);
        needle.lineTo(center - normal*2.4*s);
        needle.closeSubpath();
        p.setBrush(QColor("#f47b20"));
        p.drawPath(needle);

        // Dark metallic hub.
        p.setPen(QPen(QColor("#4e5962"), 0.8*s));
        p.setBrush(QColor("#1b2228"));
        p.drawEllipse(center, 5.2*s, 5.2*s);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#303941"));
        p.drawEllipse(center, 2.5*s, 2.5*s);

        // Large numeric readout in the lower centre, exactly like the reference hierarchy.
        QFont valueFont = p.font();
        valueFont.setBold(true);
        valueFont.setPointSizeF(qBound<qreal>(10.0, 14.5*s, 18.0));
        p.setFont(valueFont);
        p.setPen(QColor("#f5f7f8"));
        QString valueText;
        if (qAbs(value) < 10.0 && qAbs(maxv-minv) <= 40.0)
            valueText = QString::number(value, 'f', 1);
        else
            valueText = QString::number(value, 'f', 0);
        p.drawText(QRectF(center.x()-58*s, center.y()+14*s, 116*s, 24*s),
                   Qt::AlignCenter, valueText);

        if (!suffix.isEmpty()) {
            QFont unitFont = p.font();
            unitFont.setBold(false);
            unitFont.setPointSizeF(qBound<qreal>(4.8, 5.8*s, 7.2));
            p.setFont(unitFont);
            p.setPen(QColor("#aeb6bd"));
            p.drawText(QRectF(center.x()-52*s, center.y()+35*s, 104*s, 12*s),
                       Qt::AlignCenter, suffix);
        }

        // Green NORMAL line directly under the dial.
        QFont statusFont = p.font();
        statusFont.setBold(true);
        statusFont.setPointSizeF(qBound<qreal>(4.7, 5.5*s, 6.7));
        p.setFont(statusFont);
        p.setPen(inRange ? QColor("#62d86b") : QColor("#ff5541"));
        p.drawText(QRectF(8*s, 181*s, width()-16*s, 13*s), Qt::AlignCenter,
                   inRange ? QStringLiteral("NORMAL") : QStringLiteral("HORS PLAGE"));

        // Separate 2-minute trace panel immediately below the gauge, as requested.
        const QRectF trendRect(7*s, 201*s, width()-14*s, 43*s);
        p.setPen(QPen(QColor("#26323b"), thin));
        p.setBrush(QColor("#080d11"));
        p.drawRoundedRect(trendRect, 2.0*s, 2.0*s);

        for (int i = 1; i < 4; ++i) {
            const qreal y = trendRect.top() + trendRect.height()*i/4.0;
            p.setPen(QPen(QColor("#182128"), qMax<qreal>(0.55, 0.65*s)));
            p.drawLine(trendRect.left()+3*s, y, trendRect.right()-3*s, y);
        }
        for (int i = 1; i < 6; ++i) {
            const qreal x = trendRect.left() + trendRect.width()*i/6.0;
            p.setPen(QPen(QColor("#141c22"), qMax<qreal>(0.50, 0.60*s)));
            p.drawLine(x, trendRect.top()+3*s, x, trendRect.bottom()-3*s);
        }

        if (m_history.size() > 1) {
            double hmin = m_history.first().second;
            double hmax = hmin;
            for (const auto &pt : m_history) {
                hmin = qMin(hmin, pt.second);
                hmax = qMax(hmax, pt.second);
            }
            if (qAbs(hmax-hmin) < 0.001) { hmin -= 1.0; hmax += 1.0; }

            const qint64 tmax = m_history.last().first;
            const qint64 tmin = qMax<qint64>(0, tmax-120000);
            QPainterPath trace;
            bool first = true;
            for (const auto &pt : m_history) {
                if (pt.first < tmin) continue;
                const qreal x = trendRect.left()+3*s +
                    ((pt.first-tmin)/120000.0)*(trendRect.width()-6*s);
                const qreal y = trendRect.bottom()-3*s -
                    ((pt.second-hmin)/(hmax-hmin))*(trendRect.height()-6*s);
                if (first) { trace.moveTo(x,y); first=false; }
                else trace.lineTo(x,y);
            }
            p.setPen(QPen(QColor(244,123,32,45), 3.0*s, Qt::SolidLine, Qt::RoundCap));
            p.drawPath(trace);
            p.setPen(QPen(QColor("#f47b20"), qMax<qreal>(0.9,1.20*s),
                          Qt::SolidLine, Qt::RoundCap));
            p.drawPath(trace);
        }

        QFont traceFont = p.font();
        traceFont.setBold(false);
        traceFont.setPointSizeF(qBound<qreal>(4.1, 4.7*s, 5.7));
        p.setFont(traceFont);
        p.setPen(QColor("#58636c"));
        p.drawText(QRectF(trendRect.left()+4*s, trendRect.top()+2*s, 34*s, 9*s),
                   Qt::AlignLeft, QStringLiteral("-2 min"));
    }

private:
    QObject *m_source = nullptr;
    QString m_title;
    QElapsedTimer m_clock;
    QVector<QPair<qint64,double> > m_history;
};

class OverviewCardsInstaller : public QObject
{
public:
    explicit OverviewCardsInstaller(QObject *parent=nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if ((event->type()==QEvent::Show || event->type()==QEvent::Polish) && watched) {
            QMainWindow *window = qobject_cast<QMainWindow*>(watched);
            if (window && window->objectName()==QStringLiteral("MainWindow") &&
                !window->property("overviewCardsInstalled").toBool()) {
                QWidget *overview = window->findChild<QWidget*>(QStringLiteral("overview_tab"));
                if (overview) {
                    window->setProperty("overviewCardsInstalled", true);
                    QTimer::singleShot(0, window, [window,overview](){
                        struct Item { const char *source; const char *label; };
                        const Item items[10] = {
                            {"m_revCounter","m_revCounterLabel"},
                            {"m_mapGauge","m_mapGaugeLabel"},
                            {"m_throttle_pos","m_throttle_pos_label"},
                            {"m_idle_position","m_idlebypass_pos_label"},
                            {"m_battery","m_battery_voltage_label"},
                            {"m_waterTempGauge","m_waterTempLabel"},
                            {"m_airTempGauge","m_airTempLabel"},
                            {"m_lambda_voltage","m_lambda_label"},
                            {"m_short_term_correction","m_short_term_label"},
                            {"m_ignition_advance","m_ignition_advance_label"}
                        };

                        const int xs[5] = {28,278,528,778,1028};
                        QVector<ModernGaugeCard*> cards;
                        for (int i=0; i<10; ++i) {
                            QWidget *source = window->findChild<QWidget*>(QString::fromLatin1(items[i].source));
                            QLabel *label = window->findChild<QLabel*>(QString::fromLatin1(items[i].label));
                            if (!source) continue;

                            const QString title = label ? label->text() : QString();
                            ModernGaugeCard *card = new ModernGaugeCard(
                                QRect(xs[i%5], i<5 ? 8 : 272, 238, 252), source, title, overview);
                            card->setObjectName(QStringLiteral("modernGaugeCard_%1").arg(i));
                            card->raise();
                            cards.append(card);

                            source->hide();
                            if (label) label->hide();
                        }

                        QTimer *timer = new QTimer(overview);
                        timer->setInterval(500);
                        QObject::connect(timer, &QTimer::timeout, overview, [cards](){
                            for (ModernGaugeCard *card : cards)
                                if (card) card->sample();
                        });
                        timer->start();
                    });
                }
            }
        }
        return QObject::eventFilter(watched,event);
    }
};

void installOverviewCardsHook()
{
    QApplication *app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app) return;
    OverviewCardsInstaller *installer = new OverviewCardsInstaller(app);
    app->installEventFilter(installer);
}

}

Q_COREAPP_STARTUP_FUNCTION(installOverviewCardsHook)
