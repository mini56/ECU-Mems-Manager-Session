#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFont>
#include <QLabel>
#include <QPalette>
#include <QProgressBar>
#include <QSplashScreen>
#include <QWidget>

#include "i18n.h"

namespace {

static bool belongsToStartupSplash(QWidget *widget)
{
    return widget && qobject_cast<QSplashScreen*>(widget->window());
}

static void ensureBuildIdentity(QSplashScreen *splash)
{
    if(!splash || splash->findChild<QLabel*>(QStringLiteral("buildIdentityLabel")))
        return;

    QLabel *label=new QLabel(splash);
    label->setObjectName(QStringLiteral("buildIdentityLabel"));
    label->setGeometry(300,88,310,22);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QFont font=label->font();
    font.setPointSize(9);
    label->setFont(font);
    label->setText(I18n::text(44)
        .arg(QStringLiteral(APP_BUILD_NUMBER))
        .arg(QStringLiteral(APP_COMMIT_SHA)));
    label->setProperty("splashWhiteTextApplied",true);
    QPalette palette=label->palette();
    palette.setColor(QPalette::WindowText,QColor(QStringLiteral("#AAB4BF")));
    label->setPalette(palette);
    label->setStyleSheet(QStringLiteral("color:#AAB4BF;background:transparent;"));
    label->show();
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
            if(QWidget *widget=qobject_cast<QWidget*>(watched)) {
                if(QSplashScreen *splash=qobject_cast<QSplashScreen*>(widget))
                    ensureBuildIdentity(splash);
                applySplashTextFix(widget);
            }
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
