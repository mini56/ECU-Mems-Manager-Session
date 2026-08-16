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
        if(progress->property("splashWhiteTextApplied").toBool()) return;
        progress->setProperty("splashWhiteTextApplied",true);
        QPalette palette=progress->palette();
        palette.setColor(QPalette::Text,Qt::white);
        palette.setColor(QPalette::HighlightedText,Qt::white);
        progress->setPalette(palette);
        progress->setStyleSheet(QStringLiteral("QProgressBar{color:#ffffff;}"));
        return;
    }

    if(QLabel *label=qobject_cast<QLabel*>(widget)) {
        if(label->property("splashWhiteTextApplied").toBool()) return;
        label->setProperty("splashWhiteTextApplied",true);
        QPalette palette=label->palette();
        palette.setColor(QPalette::WindowText,Qt::white);
        palette.setColor(QPalette::Text,Qt::white);
        label->setPalette(palette);
        label->setStyleSheet(QStringLiteral("color:#ffffff;background:transparent;"));
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
