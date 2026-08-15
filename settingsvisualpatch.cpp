#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QLCDNumber>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QSizePolicy>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include "i18n.h"

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

static void compactAdjustForm(QWidget *page,QFrame *adjust)
{
    if(!page || !adjust) return;

    QWidget *form=page->findChild<QWidget*>(QStringLiteral("darkSettingsAdjustForm"));
    if(form) {
        form->setMaximumWidth(760);
        form->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
        form->setStyleSheet(QStringLiteral("background:transparent;border:0;"));

        if(QGridLayout *grid=qobject_cast<QGridLayout*>(form->layout())) {
            grid->setContentsMargins(0,0,0,0);
            grid->setVerticalSpacing(0);
        }

        const QList<QPushButton*> buttons=form->findChildren<QPushButton*>();
        for(QPushButton *button:buttons) {
            button->setMinimumWidth(108);
            button->setMaximumWidth(142);
            button->setMinimumHeight(21);
            button->setMaximumHeight(22);
        }

        const QList<QLCDNumber*> lcds=form->findChildren<QLCDNumber*>();
        for(QLCDNumber *lcd:lcds) {
            lcd->setMinimumHeight(21);
            lcd->setMaximumHeight(22);
        }

        const QList<QLabel*> labels=form->findChildren<QLabel*>();
        for(QLabel *label:labels) {
            label->setMinimumHeight(20);
            label->setMaximumHeight(22);
            if(label->minimumWidth()>=200) {
                label->setMinimumWidth(185);
                label->setMaximumWidth(215);
            }
        }
    }

    const QList<QPushButton*> resetButtons=adjust->findChildren<QPushButton*>(QString(),Qt::FindDirectChildrenOnly);
    for(QPushButton *button:resetButtons) {
        button->setMinimumHeight(21);
        button->setMaximumHeight(23);
    }

    if(QLineEdit *note=page->findChild<QLineEdit*>(QStringLiteral("lineEdit_3"))) {
        note->setMinimumWidth(0);
        note->setMaximumWidth(QWIDGETSIZE_MAX);
        note->setMinimumHeight(19);
        note->setMaximumHeight(21);
        note->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
        note->setAlignment(Qt::AlignCenter);
        QFont font=note->font();
        font.setPointSizeF(8.0);
        note->setFont(font);
        note->setStyleSheet(QStringLiteral(
            "QLineEdit{background:transparent;color:#b8c1c7;"
            "border:0;padding:0 2px;}"));
    }
}

static void moveIndicatorsToStates(QWidget *page,QFrame *states,QFrame *adjust)
{
    if(!page || !states || !adjust) return;

    QWidget *idleLed=page->findChild<QWidget*>(QStringLiteral("idleswitch_led"));
    QWidget *loopLed=page->findChild<QWidget*>(QStringLiteral("closedloop_led"));
    if(!idleLed || !loopLed) return;

    for(QHBoxLayout *row:adjust->findChildren<QHBoxLayout*>()) {
        if(row->indexOf(idleLed)>=0) row->removeWidget(idleLed);
        if(row->indexOf(loopLed)>=0) row->removeWidget(loopLed);
    }

    for(QLabel *label:adjust->findChildren<QLabel*>(QString(),Qt::FindDirectChildrenOnly)) {
        if(label->text()==I18n::text(2014) || label->text()==I18n::text(2015))
            label->hide();
    }

    QLabel *idleText=nullptr;
    QLabel *loopText=nullptr;
    for(QLabel *label:states->findChildren<QLabel*>()) {
        if(!idleText && label->text()==I18n::text(2015)) idleText=label;
        if(!loopText && label->text()==I18n::text(2014)) loopText=label;
    }

    QGridLayout *stateGrid=nullptr;
    for(QGridLayout *grid:states->findChildren<QGridLayout*>()) {
        if((idleText && grid->indexOf(idleText)>=0) || (loopText && grid->indexOf(loopText)>=0)) {
            stateGrid=grid;
            break;
        }
    }
    if(!stateGrid) return;

    idleLed->setParent(states);
    idleLed->setFixedSize(24,24);
    idleLed->show();
    stateGrid->addWidget(idleLed,0,0,Qt::AlignCenter);

    loopLed->setParent(states);
    loopLed->setFixedSize(24,24);
    loopLed->show();
    stateGrid->addWidget(loopLed,1,0,Qt::AlignCenter);

    if(idleText) idleText->show();
    if(loopText) loopText->show();
    states->show();
}

static void moveNoteBelowRpm(QWidget *page,QFrame *metrics,QFrame *adjust)
{
    if(!page || !metrics || !adjust) return;

    QLineEdit *note=page->findChild<QLineEdit*>(QStringLiteral("lineEdit_3"));
    QWidget *rpm=page->findChild<QWidget*>(QStringLiteral("darkTuneGaugeRpm"));
    if(!note || !rpm) return;

    for(QHBoxLayout *row:adjust->findChildren<QHBoxLayout*>())
        if(row->indexOf(note)>=0) row->removeWidget(note);

    note->setParent(rpm);
    note->setReadOnly(true);
    note->show();
    note->raise();

    const int noteHeight=qBound(19,note->sizeHint().height(),21);
    const int margin=8;
    const int noteWidth=qMax(1,rpm->width()-margin*2);
    const int x=margin;
    const int y=qMax(2,rpm->height()-noteHeight-4);
    note->setGeometry(x,y,noteWidth,noteHeight);
}

static void arrangeBottomPanels(QWidget *page,QFrame *metrics,QFrame *states,QFrame *adjust)
{
    if(!page || !metrics || !states || !adjust) return;

    QGridLayout *body=nullptr;
    for(QGridLayout *grid:page->findChildren<QGridLayout*>()) {
        if(grid->indexOf(metrics)>=0 && grid->indexOf(adjust)>=0) {
            body=grid;
            break;
        }
    }
    if(!body) return;

    body->removeWidget(states);
    body->removeWidget(adjust);
    body->addWidget(states,1,0);
    body->addWidget(adjust,1,1);
    body->setColumnStretch(0,1);
    body->setColumnStretch(1,4);
    body->setHorizontalSpacing(8);
    body->setVerticalSpacing(4);

    states->setMinimumWidth(205);
    states->setMaximumWidth(275);
    states->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
    adjust->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
}

static void shortenSettingsSeparator(QFrame *adjust)
{
    if(!adjust) return;
    QVBoxLayout *layout=qobject_cast<QVBoxLayout*>(adjust->layout());
    if(!layout) return;

    for(QFrame *line:adjust->findChildren<QFrame*>(QString(),Qt::FindDirectChildrenOnly)) {
        if(line->frameShape()!=QFrame::HLine) continue;
        line->setMinimumWidth(0);
        line->setMaximumWidth(650);
        line->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
        layout->setAlignment(line,Qt::AlignLeft);
        break;
    }
}

static void applySettingsVisualPatch(QMainWindow *window)
{
    if(!window) return;

    QWidget *page=window->findChild<QWidget*>(QStringLiteral("emission_tab"));
    QFrame *metrics=window->findChild<QFrame*>(QStringLiteral("settingsMetrics"));
    QFrame *states=window->findChild<QFrame*>(QStringLiteral("settingsStates"));
    QFrame *adjust=window->findChild<QFrame*>(QStringLiteral("settingsAdjust"));
    if(!page || !metrics || !states || !adjust) return;

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

    const int adjustHeight=qBound(138,qRound(height*.23),154);
    adjust->setMinimumHeight(adjustHeight);
    adjust->setMaximumHeight(adjustHeight+4);
    adjust->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    adjust->setStyleSheet(QStringLiteral(
        "QFrame#settingsAdjust{background:#10161c;"
        "border:1px solid #34414a;border-radius:6px;}"));

    states->setMinimumHeight(adjustHeight);
    states->setMaximumHeight(adjustHeight+4);
    states->setStyleSheet(QStringLiteral(
        "QFrame#settingsStates{background:#10161c;"
        "border:1px solid #34414a;border-radius:6px;}"));

    if(QVBoxLayout *layout=qobject_cast<QVBoxLayout*>(adjust->layout())) {
        layout->setContentsMargins(10,3,10,4);
        layout->setSpacing(1);
    }

    compactAdjustForm(page,adjust);
    moveIndicatorsToStates(page,states,adjust);
    arrangeBottomPanels(page,metrics,states,adjust);
    shortenSettingsSeparator(adjust);
    moveNoteBelowRpm(page,metrics,adjust);
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
