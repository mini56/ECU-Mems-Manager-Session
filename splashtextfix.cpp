#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QLabel>
#include <QPalette>
#include <QProgressBar>
#include <QSplashScreen>
#include <QWidget>

namespace {

static bool belongsToStartupSplash(QWidget *widget)
{
    return widget && qobject_cast<QSplashScreen*>(widget->window());
}

static void applySplashTextFix(QWidget *widget)
{
    if(!belongsToStartupSplash(widget)) return;

    if(QProgressBar *progress=qobject_cast<QProgressBar*>(widget)) {
        if(progress->property("splashBlackTextApplied").toBool()) return;
        progress->setProperty("splashBlackTextApplied",true);
        QPalette palette=progress->palette();
        palette.setColor(QPalette::Text,Qt::black);
        palette.setColor(QPalette::HighlightedText,Qt::black);
        progress->setPalette(palette);
        progress->setStyleSheet(QStringLiteral("QProgressBar{color:#000000;}"));
        return;
    }

    if(QLabel *label=qobject_cast<QLabel*>(widget)) {
        if(label->property("splashBlackTextApplied").toBool()) return;
        label->setProperty("splashBlackTextApplied",true);
        QPalette palette=label->palette();
        palette.setColor(QPalette::WindowText,Qt::black);
        palette.setColor(QPalette::Text,Qt::black);
        label->setPalette(palette);
        label->setStyleSheet(QStringLiteral("color:#000000;background:transparent;"));
    }
}

class SplashTextFixer : public QObject
{
public:
    explicit SplashTextFixer(QObject *parent=nullptr):QObject(parent) {}

protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        if(event->type()==QEvent::Show || event->type()==QEvent::Polish) {
            if(QWidget *widget=qobject_cast<QWidget*>(watched))
                applySplashTextFix(widget);
        }
        return QObject::eventFilter(watched,event);
    }
};

void installSplashTextFixer()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if(app) app->installEventFilter(new SplashTextFixer(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installSplashTextFixer)
