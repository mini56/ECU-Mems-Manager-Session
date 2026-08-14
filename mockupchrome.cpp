#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

namespace {

static QLabel *makeStatusCell(const QString &title, QWidget *parent)
{
    QLabel *label = new QLabel(title, parent);
    label->setAlignment(Qt::AlignCenter);
    label->setMinimumHeight(22);
    label->setStyleSheet(
        "QLabel{color:#cfd5da;background:#0f1419;border-right:1px solid #2b323a;padding:2px 7px;}"
    );
    return label;
}

static QString valueText(QObject *obj, const QString &suffix = QString())
{
    if (!obj) return QStringLiteral("--") + suffix;
    QVariant v = obj->property("value");
    if (!v.isValid()) v = obj->property("text");
    if (!v.isValid()) return QStringLiteral("--") + suffix;
    if (v.type() == QVariant::Double || v.type() == QVariant::Int || v.type() == QVariant::UInt)
        return QString::number(v.toDouble(), 'f', 1) + suffix;
    const QString text=v.toString().trimmed();
    return (text.isEmpty() ? QStringLiteral("--") : text) + suffix;
}

class MockupChromeInstaller : public QObject
{
public:
    explicit MockupChromeInstaller(QObject *parent=nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        QMainWindow *window = qobject_cast<QMainWindow*>(watched);
        if (!window) return QObject::eventFilter(watched, event);

        if ((event->type()==QEvent::Show || event->type()==QEvent::Polish) &&
            window->objectName()==QStringLiteral("MainWindow") &&
            !window->property("mockupChromeInstalled").toBool()) {
            window->setProperty("mockupChromeInstalled", true);
            QTimer::singleShot(260, window, [window](){ install(window); });
        }
        return QObject::eventFilter(watched, event);
    }

private:
    static void install(QMainWindow *window)
    {
        QWidget *central = window->centralWidget();
        QVBoxLayout *root = central ? qobject_cast<QVBoxLayout*>(central->layout()) : nullptr;
        if (!central || !root) return;

        if (QFrame *workspace = central->findChild<QFrame*>(QStringLiteral("modernWorkspace"))) {
            workspace->setStyleSheet(
                "#modernWorkspace{background:#0d1116;border-top:1px solid #252c33;}"
                "QTabWidget::pane{border:0;background:#0d1116;}"
            );
        }

        QListWidget *nav = central->findChild<QListWidget*>(QStringLiteral("modernNavigation"));
        if (nav) {
            nav->setStyleSheet(
                "QListWidget{background:#10151a;color:#b8c0c7;border:0;border-right:1px solid #293038;"
                "padding:5px 3px;outline:0;}"
                "QListWidget::item{min-height:28px;padding:2px 9px;border-radius:1px;margin:1px 0px;}"
                "QListWidget::item:hover{background:#171d23;color:#ffffff;}"
                "QListWidget::item:selected{background:#191f25;color:#ff9b32;border-left:3px solid #ff8a1c;}"
            );
            nav->setFocusPolicy(Qt::NoFocus);
            nav->setUniformItemSizes(true);
            nav->setSpacing(0);
        }

        QFrame *status = new QFrame(central);
        status->setObjectName(QStringLiteral("mockupBottomStatus"));
        status->setFixedHeight(26);
        status->setStyleSheet(
            "#mockupBottomStatus{background:#0f1419;border-top:1px solid #2a3138;}"
            "#mockupBottomStatus QPushButton{background:#121820;color:#cfd5da;border:0;border-left:1px solid #2b323a;"
            "border-radius:0;padding:2px 9px;}"
            "#mockupBottomStatus QPushButton:hover{color:#ffffff;background:#18212a;}"
        );
        QHBoxLayout *sl = new QHBoxLayout(status);
        sl->setContentsMargins(0,0,0,0);
        sl->setSpacing(0);

        QLabel *fileCell = makeStatusCell(QStringLiteral("Fichier : --"), status);
        QLabel *loopCell = makeStatusCell(QStringLiteral("Boucle : --"), status);
        QLabel *lambdaCell = makeStatusCell(QStringLiteral("Lambda : --"), status);
        QLabel *systemCell = makeStatusCell(QStringLiteral("Système : --"), status);
        QLabel *injCell = makeStatusCell(QStringLiteral("Injection : -- ms"), status);
        QLabel *airCell = makeStatusCell(QStringLiteral("Air : -- °C"), status);
        QPushButton *captureButton = new QPushButton(QStringLiteral("Capture écran"), status);
        captureButton->setCursor(Qt::PointingHandCursor);

        sl->addWidget(fileCell, 2);
        sl->addWidget(loopCell, 1);
        sl->addWidget(lambdaCell, 1);
        sl->addWidget(systemCell, 1);
        sl->addWidget(injCell, 1);
        sl->addWidget(airCell, 1);
        sl->addWidget(captureButton, 1);

        root->addWidget(status);

        QObject *closedLoop = window->findChild<QObject*>(QStringLiteral("m_closed_loop"));
        if (!closedLoop) closedLoop = window->findChild<QObject*>(QStringLiteral("m_closedLoopLed"));
        QObject *lambda = window->findChild<QObject*>(QStringLiteral("m_lambda_voltage"));
        QObject *inj = window->findChild<QObject*>(QStringLiteral("m_injector_time"));
        if (!inj) inj = window->findChild<QObject*>(QStringLiteral("m_injection_time"));
        QObject *air = window->findChild<QObject*>(QStringLiteral("m_airTempGauge"));
        QObject *system = window->findChild<QObject*>(QStringLiteral("m_engine_error"));
        QLineEdit *logName = window->findChild<QLineEdit*>(QStringLiteral("m_logFileNameBox"));

        QObject::connect(captureButton,&QPushButton::clicked,window,[window](){
            QMetaObject::invokeMethod(window,"onSnapshotClicked",Qt::QueuedConnection);
        });

        const QList<QLabel*> cells = {fileCell,loopCell,lambdaCell,systemCell,injCell,airCell};
        QTimer *sync = new QTimer(status);
        sync->setInterval(220);
        QObject::connect(sync, &QTimer::timeout, status, [=](){
            const qreal scale = window->property("globalUiScale").isValid()
                ? window->property("globalUiScale").toDouble() : 1.0;
            const int navWidth = qBound(126, qRound(176.0 * scale), 204);
            if (nav && nav->width()!=navWidth) nav->setFixedWidth(navWidth);
            const int statusH = qBound(21, qRound(26.0 * scale), 30);
            if (status->height()!=statusH) status->setFixedHeight(statusH);

            QFont sf=status->font();
            sf.setPointSizeF(qBound<qreal>(6.3,8.0*scale,9.2));
            for (QLabel *cell : cells) cell->setFont(sf);
            captureButton->setFont(sf);

            if (logName) {
                const QString name=logName->text().trimmed();
                fileCell->setText(QStringLiteral("Fichier : ") + (name.isEmpty()?QStringLiteral("--"):name));
            }
            if (closedLoop) {
                const bool on = closedLoop->property("checked").isValid()
                    ? closedLoop->property("checked").toBool()
                    : closedLoop->property("value").toInt()!=0;
                loopCell->setText(on ? QStringLiteral("Boucle : fermée") : QStringLiteral("Boucle : ouverte"));
            }
            lambdaCell->setText(QStringLiteral("Lambda : ") + valueText(lambda, QStringLiteral(" V")));
            injCell->setText(QStringLiteral("Injection : ") + valueText(inj, QStringLiteral(" ms")));
            airCell->setText(QStringLiteral("Air : ") + valueText(air, QStringLiteral(" °C")));
            if (system) {
                const bool err = system->property("checked").toBool();
                systemCell->setText(err ? QStringLiteral("Système : défaut") : QStringLiteral("Système : OK"));
                systemCell->setStyleSheet(err
                    ? "QLabel{color:#ff6b6b;background:#0f1419;border-right:1px solid #2b323a;padding:2px 7px;}"
                    : "QLabel{color:#6edc8b;background:#0f1419;border-right:1px solid #2b323a;padding:2px 7px;}");
            }
        });
        sync->start();
    }
};

void installMockupChromeHook()
{
    QApplication *app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app) return;
    MockupChromeInstaller *installer = new MockupChromeInstaller(app);
    app->installEventFilter(installer);
}

}

Q_COREAPP_STARTUP_FUNCTION(installMockupChromeHook)
