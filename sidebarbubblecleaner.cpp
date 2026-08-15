#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QLabel>
#include <QMainWindow>
#include <QTimer>
#include <QToolButton>

namespace {

static void removeSidebarBubble(QMainWindow *w)
{
    if(!w) return;
    QToolButton *toggle=w->findChild<QToolButton*>(QStringLiteral("darkSidebarToggle"));
    if(!toggle) return;

    // No tooltip here: the control is already explicitly labelled when the
    // sidebar is expanded, so the generic help-bubble manager must not add
    // its speech-bubble icon beside it.
    toggle->setToolTip(QString());
    if(QLabel *bubble=toggle->findChild<QLabel*>(QStringLiteral("_ecuHelpBubble"),Qt::FindDirectChildrenOnly))
        bubble->hide();
}

class SidebarBubbleCleaner : public QObject
{
public:
    explicit SidebarBubbleCleaner(QObject *parent=nullptr):QObject(parent){}
protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QMainWindow *w=qobject_cast<QMainWindow*>(watched);
        if(!w || w->objectName()!=QStringLiteral("MainWindow"))
            return QObject::eventFilter(watched,event);

        if(event->type()==QEvent::Show || event->type()==QEvent::Polish) {
            QTimer::singleShot(1900,w,[w](){removeSidebarBubble(w);});
            QTimer::singleShot(2400,w,[w](){removeSidebarBubble(w);});
        } else if(event->type()==QEvent::Resize) {
            QTimer::singleShot(0,w,[w](){removeSidebarBubble(w);});
        }
        return QObject::eventFilter(watched,event);
    }
};

void installSidebarBubbleCleaner()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if(app) app->installEventFilter(new SidebarBubbleCleaner(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installSidebarBubbleCleaner)
