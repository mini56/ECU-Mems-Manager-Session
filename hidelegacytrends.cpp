#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QMainWindow>
#include <QTimer>
#include <QWidget>

namespace {
class LegacyTrendHider : public QObject
{
public:
    explicit LegacyTrendHider(QObject *parent=nullptr):QObject(parent){}
protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        if ((event->type()==QEvent::Show || event->type()==QEvent::Polish) && watched) {
            QMainWindow *window=qobject_cast<QMainWindow*>(watched);
            if (window && window->objectName()==QStringLiteral("MainWindow") && !window->property("legacyTrendHidden").toBool()) {
                window->setProperty("legacyTrendHidden",true);
                QTimer::singleShot(350,window,[window](){
                    QWidget *panel=window->findChild<QWidget*>(QStringLiteral("trendPanel2min"));
                    if(panel) panel->hide();
                    QWidget *overview=window->findChild<QWidget*>(QStringLiteral("overview_tab"));
                    if(overview) overview->setMinimumHeight(0);
                });
            }
        }
        return QObject::eventFilter(watched,event);
    }
};
void installLegacyTrendHider()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if(!app) return;
    LegacyTrendHider *hider=new LegacyTrendHider(app);
    app->installEventFilter(hider);
}
}
Q_COREAPP_STARTUP_FUNCTION(installLegacyTrendHider)
