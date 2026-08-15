#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
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

static void clearLayoutItems(QLayout *layout)
{
    if(!layout) return;
    while(QLayoutItem *item=layout->takeAt(0)) {
        if(QLayout *sub=item->layout()) {
            clearLayoutItems(sub);
            delete sub;
        }
        delete item;
    }
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

        if(QGridLayout *grid=qobject_cast<QGridLayout*>(form->layout()))
            grid->setVerticalSpacing(2);
    }

    if(QLineEdit *note=page->findChild<QLineEdit*>(QStringLiteral("lineEdit_3"))) {
        note->setMinimumWidth(300);
        note->setMaximumWidth(460);
        note->setMinimumHeight(22);
        note->setMaximumHeight(24);
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

static void moveNoteBelowCenterGauge(QWidget *page,QFrame *metrics)
{
    if(!page || !metrics) return;

    QWidget *rpm=page->findChild<QWidget*>(QStringLiteral("darkTuneGaugeRpm"));
    QLineEdit *note=page->findChild<QLineEdit*>(QStringLiteral("lineEdit_3"));
    if(!rpm || !note) return;

    QWidget *column=metrics->findChild<QWidget*>(QStringLiteral("settingsRpmColumn421"),Qt::FindDirectChildrenOnly);
    QGridLayout *gaugeGrid=nullptr;
    for(QGridLayout *grid:metrics->findChildren<QGridLayout*>()) {
        if(grid->indexOf(rpm)>=0) {
            gaugeGrid=grid;
            break;
        }
    }

    if(!column) {
        if(!gaugeGrid) return;
        gaugeGrid->removeWidget(rpm);

        column=new QWidget(metrics);
        column->setObjectName(QStringLiteral("settingsRpmColumn421"));
        column->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
        QVBoxLayout *columnLayout=new QVBoxLayout(column);
        columnLayout->setContentsMargins(0,0,0,0);
        columnLayout->setSpacing(1);

        rpm->setParent(column);
        columnLayout->addWidget(rpm,0,Qt::AlignHCenter|Qt::AlignTop);

        note->setParent(column);
        note->setReadOnly(true);
        note->show();
        columnLayout->addWidget(note,0,Qt::AlignHCenter|Qt::AlignTop);

        gaugeGrid->addWidget(column,0,1,2,1,Qt::AlignHCenter|Qt::AlignVCenter);
    } else {
        note->setParent(column);
        note->show();
        if(QVBoxLayout *columnLayout=qobject_cast<QVBoxLayout*>(column->layout())) {
            if(columnLayout->indexOf(rpm)<0)
                columnLayout->insertWidget(0,rpm,0,Qt::AlignHCenter|Qt::AlignTop);
            if(columnLayout->indexOf(note)<0)
                columnLayout->addWidget(note,0,Qt::AlignHCenter|Qt::AlignTop);
        }
    }
}

static void rebuildStatesCard(QWidget *page,QFrame *states,QFrame *adjust)
{
    if(!page || !states) return;

    QWidget *idleLed=page->findChild<QWidget*>(QStringLiteral("idleswitch_led"));
    QWidget *loopLed=page->findChild<QWidget*>(QStringLiteral("closedloop_led"));

    if(adjust) {
        const QList<QLabel*> directLabels=adjust->findChildren<QLabel*>(QString(),Qt::FindDirectChildrenOnly);
        for(QLabel *label:directLabels) {
            if(label && (label->text()==I18n::text(2014) || label->text()==I18n::text(2015)))
                label->hide();
        }
    }

    QVBoxLayout *layout=qobject_cast<QVBoxLayout*>(states->layout());
    if(!layout) return;
    clearLayoutItems(layout);
    const QList<QWidget*> oldChildren=states->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly);
    for(QWidget *child:oldChildren) if(child!=idleLed && child!=loopLed) child->hide();

    layout->setContentsMargins(12,8,12,9);
    layout->setSpacing(5);

    QLabel *title=new QLabel(I18n::text(7134),states);
    QFont titleFont=title->font();
    titleFont.setBold(true);
    titleFont.setPointSizeF(9.0);
    title->setFont(titleFont);
    title->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;border:0;"));
    layout->addWidget(title);

    QFrame *line=new QFrame(states);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet(QStringLiteral("background:#29343e;border:0;max-height:1px;"));
    layout->addWidget(line);

    QGridLayout *grid=new QGridLayout;
    grid->setContentsMargins(4,4,4,2);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(8);

    if(idleLed) {
        idleLed->setParent(states);
        idleLed->setFixedSize(24,24);
        idleLed->show();
        grid->addWidget(idleLed,0,0,Qt::AlignCenter);
    }
    QLabel *idleText=new QLabel(I18n::text(2015),states);
    idleText->setStyleSheet(QStringLiteral("color:#dce3e8;background:transparent;border:0;"));
    grid->addWidget(idleText,0,1,Qt::AlignLeft|Qt::AlignVCenter);

    if(loopLed) {
        loopLed->setParent(states);
        loopLed->setFixedSize(24,24);
        loopLed->show();
        grid->addWidget(loopLed,1,0,Qt::AlignCenter);
    }
    QLabel *loopText=new QLabel(I18n::text(2014),states);
    loopText->setStyleSheet(QStringLiteral("color:#dce3e8;background:transparent;border:0;"));
    grid->addWidget(loopText,1,1,Qt::AlignLeft|Qt::AlignVCenter);

    grid->setColumnStretch(1,1);
    layout->addLayout(grid);
    layout->addStretch(1);
    states->show();
}

static void arrangeBottomCards(QWidget *page,QFrame *metrics,QFrame *states,QFrame *adjust)
{
    if(!page || !metrics || !states || !adjust) return;

    for(QGridLayout *grid:page->findChildren<QGridLayout*>()) {
        if(grid->indexOf(metrics)<0 || grid->indexOf(adjust)<0) continue;

        grid->removeWidget(metrics);
        grid->removeWidget(states);
        grid->removeWidget(adjust);
        grid->addWidget(metrics,0,0,1,2);
        grid->addWidget(states,1,0);
        grid->addWidget(adjust,1,1);
        grid->setColumnStretch(0,1);
        grid->setColumnStretch(1,4);
        grid->setRowStretch(0,3);
        grid->setRowStretch(1,2);
        grid->setHorizontalSpacing(8);
        grid->setVerticalSpacing(5);
        break;
    }

    states->setMinimumWidth(205);
    states->setMaximumWidth(275);
    states->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
    adjust->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
}

static void composeSettings421(QWidget *page,QFrame *metrics,QFrame *states,QFrame *adjust)
{
    if(!page || !metrics || !states || !adjust) return;
    if(page->property("settingsLayout421Composed").toBool()) return;

    moveNoteBelowCenterGauge(page,metrics);
    rebuildStatesCard(page,states,adjust);
    arrangeBottomCards(page,metrics,states,adjust);

    page->setProperty("settingsLayout421Composed",true);
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

    composeSettings421(page,metrics,states,adjust);

    const int width=qMax(760,page->width());
    const int height=qMax(500,page->height());
    const int centerByWidth=qRound(width*.34);
    const int centerByHeight=qRound((height-190)*.96);
    const int center=qBound(345,qMin(centerByWidth,centerByHeight),400);
    const int outer=qBound(185,qRound(center*.53),210);

    // Do not change the gauge sizes established in build #418.
    applyGaugeFrame(page->findChild<QWidget*>(QStringLiteral("darkTuneGaugeRpm")),center);
    applyGaugeFrame(page->findChild<QWidget*>(QStringLiteral("darkTuneGaugeIdle")),outer);
    applyGaugeFrame(page->findChild<QWidget*>(QStringLiteral("darkTuneGaugeTrim")),outer);
    applyGaugeFrame(page->findChild<QWidget*>(QStringLiteral("darkTuneGaugeIgnition")),outer);
    applyGaugeFrame(page->findChild<QWidget*>(QStringLiteral("darkTuneGaugeLambda")),outer);

    // The note now uses the free space directly below the centre gauge.
    const int metricsHeight=qMax(center+32,outer*2+8);
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

    states->setMinimumHeight(adjustHeight);
    states->setMaximumHeight(adjustHeight+12);
    states->setStyleSheet(QStringLiteral(
        "QFrame#settingsStates{background:#10161c;"
        "border:1px solid #34414a;border-radius:6px;}"));

    if(QVBoxLayout *layout=qobject_cast<QVBoxLayout*>(adjust->layout())) {
        // Slightly tighter than #418: raises the settings rows without changing controls.
        layout->setContentsMargins(12,7,12,8);
        layout->setSpacing(4);
    }

    compactAdjustForm(page);

    for(QGridLayout *grid:page->findChildren<QGridLayout*>()) {
        if(grid->indexOf(metrics)>=0 && grid->indexOf(adjust)>=0) {
            grid->setVerticalSpacing(5);
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
