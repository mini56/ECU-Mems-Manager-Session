#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QSettings>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include "i18n.h"

namespace {

class StatusDot : public QWidget
{
public:
    explicit StatusDot(const QColor &color, QWidget *parent=nullptr) : QWidget(parent), m_color(color)
    {
        setFixedSize(10,10);
    }
    void setActive(bool active) { m_active=active; update(); }
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(m_active ? m_color.lighter(150) : QColor("#48505a"), 1));
        p.setBrush(m_active ? m_color : QColor("#252b31"));
        p.drawEllipse(rect().adjusted(1,1,-1,-1));
    }
private:
    QColor m_color;
    bool m_active=false;
};

class MockupTopBarInstaller : public QObject
{
public:
    explicit MockupTopBarInstaller(QObject *parent=nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if ((event->type()!=QEvent::Show && event->type()!=QEvent::Polish) || !watched)
            return QObject::eventFilter(watched,event);

        QMainWindow *window=qobject_cast<QMainWindow*>(watched);
        if (!window || window->objectName()!=QStringLiteral("MainWindow") || window->property("mockupTopBarInstalled").toBool())
            return QObject::eventFilter(watched,event);

        QWidget *central=window->centralWidget();
        QVBoxLayout *root=central ? qobject_cast<QVBoxLayout*>(central->layout()) : nullptr;
        if (!central || !root) return QObject::eventFilter(watched,event);

        window->setProperty("mockupTopBarInstalled",true);
        QTimer::singleShot(300,window,[window,central,root](){
            QWidget *legacy=central->findChild<QWidget*>(QStringLiteral("layoutWidget_7"));
            if (legacy) legacy->hide();
            if (window->menuBar()) window->menuBar()->hide();
            if (QWidget *modernMode=central->findChild<QWidget*>(QStringLiteral("modernModeBar"))) modernMode->hide();

            QFrame *bar=new QFrame(central);
            bar->setObjectName(QStringLiteral("mockupTopBar"));
            bar->setFixedHeight(38);
            bar->setStyleSheet(
                "#mockupTopBar{background:#11161b;border-bottom:1px solid #2b323a;}"
                "#mockupTopBar QLabel{color:#dce2e7;background:transparent;}"
                "#mockupTopBar QPushButton,#mockupTopBar QToolButton{background:#182028;color:#f1f4f6;border:1px solid #303943;border-radius:4px;padding:5px 10px;}"
                "#mockupTopBar QPushButton:hover,#mockupTopBar QToolButton:hover{border-color:#ff8a1c;}"
                "#mockupTopBar QComboBox{background:#151b21;color:#f1f4f6;border:1px solid #424b55;border-radius:4px;padding:4px 8px;min-width:92px;}"
                "#mockupTopBar QComboBox:focus{border-color:#ff8a1c;}"
            );
            QHBoxLayout *l=new QHBoxLayout(bar);
            l->setContentsMargins(8,4,8,4);
            l->setSpacing(8);

            QToolButton *menuButton=new QToolButton(bar);
            menuButton->setText(QString::fromUtf8("☰"));
            menuButton->setFixedWidth(30);
            menuButton->setPopupMode(QToolButton::InstantPopup);
            QMenu *menu=new QMenu(menuButton);
            QAction *connectAction=menu->addAction(I18n::text(101));
            QAction *disconnectAction=menu->addAction(I18n::text(103));
            menu->addSeparator();
            if (QMenu *fileMenu=window->findChild<QMenu*>(QStringLiteral("m_fileMenu"))) menu->addAction(fileMenu->menuAction());
            if (QMenu *optionsMenu=window->findChild<QMenu*>(QStringLiteral("m_optionsMenu"))) menu->addAction(optionsMenu->menuAction());
            if (QMenu *helpMenu=window->findChild<QMenu*>(QStringLiteral("m_helpMenu"))) menu->addAction(helpMenu->menuAction());
            menuButton->setMenu(menu);
            QObject::connect(connectAction,&QAction::triggered,window,[window](){ QMetaObject::invokeMethod(window,"onConnectClicked",Qt::QueuedConnection); });
            QObject::connect(disconnectAction,&QAction::triggered,window,[window](){ QMetaObject::invokeMethod(window,"onDisconnectClicked",Qt::QueuedConnection); });
            l->addWidget(menuButton);

            QLabel *title=new QLabel(QStringLiteral("ECU MEMS MANAGER"),bar);
            QFont tf=title->font(); tf.setBold(true); tf.setPointSize(9); title->setFont(tf);
            l->addWidget(title);
            l->addSpacing(18);

            auto capsule=[bar](const QString &text, StatusDot **dot, const QColor &color){
                QFrame *f=new QFrame(bar);
                f->setStyleSheet("QFrame{background:#171d23;border:1px solid #2e3740;border-radius:4px;} QLabel{border:0;} ");
                QHBoxLayout *hl=new QHBoxLayout(f); hl->setContentsMargins(8,3,7,3); hl->setSpacing(6);
                hl->addWidget(new QLabel(text,f));
                *dot=new StatusDot(color,f); hl->addWidget(*dot);
                return f;
            };

            StatusDot *commsDot=nullptr;
            StatusDot *errorDot=nullptr;
            l->addWidget(capsule(I18n::text(102),&commsDot,QColor("#22c55e")));
            l->addWidget(capsule(I18n::text(105),&errorDot,QColor("#ef4444")));
            l->addStretch(1);

            QLabel *ecuId=new QLabel(I18n::text(104),bar);
            ecuId->setStyleSheet("QLabel{background:#171d23;border:1px solid #2e3740;border-radius:4px;padding:5px 10px;color:#dce2e7;}");
            ecuId->setMinimumWidth(170);
            ecuId->setAlignment(Qt::AlignCenter);
            l->addWidget(ecuId);

            QLabel *modeLabel=new QLabel(I18n::text(7100),bar);
            l->addWidget(modeLabel);
            QComboBox *mode=new QComboBox(bar);
            mode->addItem(I18n::text(7101));
            mode->addItem(I18n::text(7102));
            QSettings settings(QStringLiteral("ECU MEMS Manager"),QStringLiteral("ECU MEMS Manager"));
            mode->setCurrentIndex(settings.value(QStringLiteral("InterfaceMode"),0).toInt()==1?1:0);
            l->addWidget(mode);

            QPushButton *snapshot=new QPushButton(I18n::text(7015),bar);
            QPushButton *captures=new QPushButton(I18n::text(7016),bar);
            snapshot->setStyleSheet("QPushButton{background:#2467c8;color:white;border:1px solid #4c86da;border-radius:4px;padding:5px 12px;} QPushButton:hover{background:#2d74db;}");
            captures->setStyleSheet(snapshot->styleSheet());
            QObject::connect(snapshot,&QPushButton::clicked,window,[window](){ QMetaObject::invokeMethod(window,"onSnapshotClicked",Qt::QueuedConnection); });
            QObject::connect(captures,&QPushButton::clicked,window,[window](){ QMetaObject::invokeMethod(window,"onViewCapturesClicked",Qt::QueuedConnection); });
            l->addWidget(snapshot);
            l->addWidget(captures);

            QToolButton *settingsButton=new QToolButton(bar);
            settingsButton->setText(QString::fromUtf8("⚙"));
            settingsButton->setFixedWidth(34);
            QObject::connect(settingsButton,&QToolButton::clicked,window,[window](){ QMetaObject::invokeMethod(window,"onEditOptionsClicked",Qt::QueuedConnection); });
            l->addWidget(settingsButton);

            root->insertWidget(0,bar);

            QTimer *sync=new QTimer(bar);
            sync->setInterval(250);
            QObject::connect(sync,&QTimer::timeout,bar,[window,central,ecuId,commsDot,errorDot,mode](){
                QLabel *oldId=window->findChild<QLabel*>(QStringLiteral("m_ecuIdLabel"));
                if (oldId && !oldId->text().isEmpty()) ecuId->setText(oldId->text());
                QAbstractButton *good=window->findChild<QAbstractButton*>(QStringLiteral("m_commsGoodLed"));
                QAbstractButton *err=window->findChild<QAbstractButton*>(QStringLiteral("m_engine_error"));
                if (good) commsDot->setActive(good->isChecked());
                if (err) errorDot->setActive(err->isChecked());
                QWidget *modernMode=central->findChild<QWidget*>(QStringLiteral("modernModeBar"));
                QComboBox *source=modernMode ? modernMode->findChild<QComboBox*>() : nullptr;
                if (source && source->currentIndex()!=mode->currentIndex()) mode->setCurrentIndex(source->currentIndex());
            });
            QObject::connect(mode,QOverload<int>::of(&QComboBox::currentIndexChanged),bar,[central](int index){
                QSettings s(QStringLiteral("ECU MEMS Manager"),QStringLiteral("ECU MEMS Manager"));
                s.setValue(QStringLiteral("InterfaceMode"),index);
                QWidget *modernMode=central->findChild<QWidget*>(QStringLiteral("modernModeBar"));
                QComboBox *source=modernMode ? modernMode->findChild<QComboBox*>() : nullptr;
                if (source && source->currentIndex()!=index) source->setCurrentIndex(index);
            });
            sync->start();
        });
        return QObject::eventFilter(watched,event);
    }
};

void installMockupTopBar()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app) return;
    app->installEventFilter(new MockupTopBarInstaller(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installMockupTopBar)
