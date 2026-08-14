#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QMainWindow>
#include <QPainter>
#include <QTimer>
#include <QWidget>

namespace {

class GaugeCard : public QWidget
{
public:
    GaugeCard(const QRect &geometry, QWidget *parent) : QWidget(parent)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setGeometry(geometry);
        setProperty("screenfitBaseGeometry", geometry);
        show();
        lower();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QRectF r = rect().adjusted(1, 1, -1, -1);
        p.setPen(QPen(QColor("#343b43"), 1));
        p.setBrush(QColor("#1b2026"));
        p.drawRoundedRect(r, 10, 10);

        // Thin MEMS Manager orange signature, deliberately restrained.
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#ff8a1c"));
        p.drawRoundedRect(QRectF(12, 7, 38, 3), 1.5, 1.5);
    }
};

class OverviewCardsInstaller : public QObject
{
public:
    explicit OverviewCardsInstaller(QObject *parent=nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if ((event->type()==QEvent::Show || event->type()==QEvent::Polish) && watched) {
            QMainWindow *window=qobject_cast<QMainWindow*>(watched);
            if (window && window->objectName()==QStringLiteral("MainWindow") &&
                !window->property("overviewCardsInstalled").toBool()) {
                QWidget *overview=window->findChild<QWidget*>(QStringLiteral("overview_tab"));
                if (overview) {
                    window->setProperty("overviewCardsInstalled", true);
                    QTimer::singleShot(100, window, [overview](){
                        // Keep the original technical layout: five columns, two rows.
                        // The whole composition is scaled uniformly by screenfit.cpp.
                        const int xs[5] = {35, 285, 535, 785, 1035};
                        for (int row=0; row<2; ++row) {
                            for (int col=0; col<5; ++col) {
                                GaugeCard *card=new GaugeCard(QRect(xs[col], row==0 ? 0 : 250, 230, 245), overview);
                                card->setObjectName(QStringLiteral("overviewGaugeCard_%1_%2").arg(row).arg(col));
                            }
                        }
                        // Ensure all real controls remain above the decorative cards.
                        const QList<QWidget*> children=overview->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
                        for (QWidget *w: children)
                            if (!w->objectName().startsWith(QStringLiteral("overviewGaugeCard_"))) w->raise();
                    });
                }
            }
        }
        return QObject::eventFilter(watched,event);
    }
};

void installOverviewCardsHook()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app) return;
    OverviewCardsInstaller *installer=new OverviewCardsInstaller(app);
    app->installEventFilter(installer);
}

}

Q_COREAPP_STARTUP_FUNCTION(installOverviewCardsHook)
