#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QSizePolicy>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace {

static void applyGaugeFrame(QWidget *gauge,int size)
{
    if(!gauge) return;
    gauge->setMinimumSize(size,size);
    gauge->setMaximumSize(size,size);
    gauge->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    gauge->setAttribute(Qt::WA_StyledBackground,true);
    gauge->setStyleSheet(QStringLiteral(
        "background:#080d11;"
        "border:1px solid #34414a;"
        "border-radius:5px;"));
    gauge->show();
}

static void compactAdjustForm(QWidget *page)
{
    if(!page) return;

    QWidget *form=page->findChild<QWidget*>(QStringLiteral("darkSettingsAdjustForm"));
    if(form) {
        form->setMaximumWidth(760);
        form->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);

        const QList<QPushButton*> buttons=form->findChildren<QPushButton*>();
        for(QPushButton *button:buttons) {
            button->setMinimumWidth(108);
            button->setMaximumWidth(142);
        }

        const QList<QLabel*> labels=form->findChildren<QLabel*>();
        for(QLabel *label:labels) {
            if(label->minimumWidth()>=200) {
                label->setMinimumWidth(185);
                label->setMaximumWidth(215);
            }
        }
    }

    if(QLineEdit *note=page->findChild<QLineEdit*>(QStringLiteral("lineEdit_3"))) {
        note->setMinimumWidth(460);
        note->setMaximumWidth(620);
        note->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
        QFont font=note->font();
        font.setPointSizeF(8.0);
        note->setFont(font);
        note->setStyleSheet(QStringLiteral(
            "QLineEdit{background:transparent;color:#b8c1c7;"
            "border:0;padding:1px 2px;}"));
    }
}

static void applySettingsVisualPatch(QMainWindow *window)
{
    if(!window) return;

    QWidget *page=window->findChild<QWidget*>(QStringLiteral("emission_tab"));
    QFrame *metrics=window->findChild<QFrame*>(QStringLiteral("settingsMetrics"));
    QFrame *adjust=window->findChild<QFrame*>(QStringLiteral("settingsAdjust"));
    if(!page || !metrics || !adjust) return;

    if(!page->property("settingsDark410Built").toBool()) {
        QTimer::singleShot(350,window,[window](){applySettingsVisualPatch(window);});
        return;
    }

    const int width=qMax(760,page->width());
    const int height=qMax(500,page->height());
    const int centerByWidth=qRound(width*.34);
    const int centerByHeight=qRound((height-190)*.96);
    const int center=qBound(345,qMin(centerByWidth,centerByHeight),400);
    const int outer=qBound(185,qRound(center*.53),210);

    applyGaugeFrame(page->findChild<QWidget*>(QStringLiteral("darkTuneGaugeRpm")),center);
    applyGaugeFrame(page->findChild<QWidget*>(QStringLiteral("darkTuneGaugeIdle")),outer);
    applyGaugeFrame(page->findChild<QWidget*>(QStringLiteral("darkTuneGaugeTrim")),outer);
    applyGaugeFrame(page->findChild<QWidget*>(QStringLiteral("darkTuneGaugeIgnition")),outer);
    applyGaugeFrame(page->findChild<QWidget*>(QStringLiteral("darkTuneGaugeLambda")),outer);

    const int metricsHeight=qMax(center+16,outer*2+8);
    metrics->setMinimumHeight(metricsHeight);
    metrics->setMaximumHeight(metricsHeight+8);
    metrics->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    metrics->setStyleSheet(QStringLiteral(
        "QFrame#settingsMetrics{background:#0d1318;"
        "border:1px solid #2f3b45;border-radius:6px;}"));

    const int adjustHeight=qBound(210,qRound(height*.34),232);
    adjust->setMinimumHeight(adjustHeight);
    adjust->setMaximumHeight(adjustHeight+12);
    adjust->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    adjust->setStyleSheet(QStringLiteral(
        "QFrame#settingsAdjust{background:#10161c;"
        "border:1px solid #34414a;border-radius:6px;}"));

    if(QVBoxLayout *layout=qobject_cast<QVBoxLayout*>(adjust->layout())) {
        layout->setContentsMargins(14,12,14,11);
        layout->setSpacing(6);
    }

    compactAdjustForm(page);

    for(QGridLayout *grid:page->findChildren<QGridLayout*>()) {
        if(grid->indexOf(metrics)>=0 && grid->indexOf(adjust)>=0) {
            grid->setVerticalSpacing(12);
            break;
        }
    }
}

class SettingsVisualPatchInstaller : public QObject
{
public:
    explicit SettingsVisualPatchInstaller(QObject *parent=nullptr):QObject(parent) {}

protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QMainWindow *window=qobject_cast<QMainWindow*>(watched);
        if(!window || window->objectName()!=QStringLiteral("MainWindow"))
            return QObject::eventFilter(watched,event);

        if(event->type()==QEvent::Show &&
           !window->property("settingsVisualPatchScheduled").toBool()) {
            window->setProperty("settingsVisualPatchScheduled",true);
            QTimer::singleShot(2800,window,[window](){applySettingsVisualPatch(window);});
            QTimer::singleShot(3300,window,[window](){applySettingsVisualPatch(window);});
        } else if(event->type()==QEvent::Resize &&
                  window->property("settingsVisualPatchScheduled").toBool()) {
            QTimer::singleShot(460,window,[window](){applySettingsVisualPatch(window);});
        }
        return QObject::eventFilter(watched,event);
    }
};

void installSettingsVisualPatch()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if(app) app->installEventFilter(new SettingsVisualPatchInstaller(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installSettingsVisualPatch)
