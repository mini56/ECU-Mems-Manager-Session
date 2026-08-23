#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFont>
#include <QLabel>
#include <QPalette>
#include <QProgressBar>
#include <QSplashScreen>
#include <QTimer>

#include "i18n.h"

namespace {

static void ensureBuildIdentity(QSplashScreen *splash)
{
    if (!splash || splash->findChild<QLabel*>(QStringLiteral("buildIdentityLabel")))
        return;

    QLabel *label = new QLabel(splash);
    label->setObjectName(QStringLiteral("buildIdentityLabel"));
    label->setGeometry(300, 88, 310, 22);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QFont font = label->font();
    font.setPointSize(9);
    label->setFont(font);
    label->setText(I18n::text(44)
        .arg(QStringLiteral(APP_BUILD_NUMBER))
        .arg(QStringLiteral(APP_COMMIT_SHA)));
    label->setProperty("splashWhiteTextApplied", true);
    QPalette palette = label->palette();
    palette.setColor(QPalette::WindowText, QColor(QStringLiteral("#AAB4BF")));
    label->setPalette(palette);
    label->setStyleSheet(QStringLiteral("color:#AAB4BF;background:transparent;"));
    label->show();
}

static void applySplashVisuals(QSplashScreen *splash)
{
    if (!splash)
        return;

    ensureBuildIdentity(splash);

    const QList<QProgressBar*> progressBars = splash->findChildren<QProgressBar*>();
    for (QProgressBar *progress : progressBars)
    {
        if (!progress || progress->property("splashWhiteTextApplied").toBool())
            continue;
        progress->setProperty("splashWhiteTextApplied", true);
        QPalette palette = progress->palette();
        palette.setColor(QPalette::Text, Qt::white);
        palette.setColor(QPalette::HighlightedText, Qt::white);
        progress->setPalette(palette);
        // Keep the exact historical visual result of the former patch.
        progress->setStyleSheet(QStringLiteral("QProgressBar{color:#ffffff;}"));
    }

    const QList<QLabel*> labels = splash->findChildren<QLabel*>();
    for (QLabel *label : labels)
    {
        if (!label || label->property("splashWhiteTextApplied").toBool())
            continue;
        label->setProperty("splashWhiteTextApplied", true);
        QPalette palette = label->palette();
        palette.setColor(QPalette::WindowText, Qt::white);
        palette.setColor(QPalette::Text, Qt::white);
        label->setPalette(palette);
        label->setStyleSheet(QStringLiteral("color:#ffffff;background:transparent;"));
    }
}

class SplashTextFixer : public QObject
{
public:
    explicit SplashTextFixer(QApplication *app)
        : QObject(app), m_app(app)
    {
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        QSplashScreen *splash = qobject_cast<QSplashScreen*>(watched);
        if (!splash || (event->type() != QEvent::Show && event->type() != QEvent::Polish))
            return QObject::eventFilter(watched, event);

        applySplashVisuals(splash);

        // The startup splash is fully built before it is shown. Once its
        // existing children have been styled, this application-wide filter is
        // no longer needed. Removing it avoids a permanent visual patch on
        // every QWidget event for the rest of the program lifetime.
        QTimer::singleShot(0, splash, [this, splash]() {
            applySplashVisuals(splash);
            if (m_app)
                m_app->removeEventFilter(this);
            deleteLater();
        });

        return QObject::eventFilter(watched, event);
    }

private:
    QApplication *m_app;
};

void installSplashTextFixer()
{
    QApplication *app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (app)
        app->installEventFilter(new SplashTextFixer(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installSplashTextFixer)
