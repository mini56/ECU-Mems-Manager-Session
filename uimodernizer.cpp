#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPainter>
#include <QPainterPath>
#include <QSettings>
#include <QTabBar>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QVector>

#include "analogwidgets/abstractmeter.h"

namespace {

class MiniTrendWidget : public QWidget
{
public:
    MiniTrendWidget(const QString &title, AbstractMeter *meter, QWidget *parent)
        : QWidget(parent), m_title(title), m_meter(meter)
    {
        setMinimumHeight(86);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_clock.start();
    }

    void sample()
    {
        if (!m_meter) return;
        const qint64 now = m_clock.elapsed();
        m_points.append(qMakePair(now, m_meter->value()));
        const qint64 cutoff = now - 120000; // 2 minutes
        int removeCount = 0;
        while (removeCount < m_points.size() && m_points.at(removeCount).first < cutoff)
            ++removeCount;
        if (removeCount) m_points.remove(0, removeCount);
        if (m_points.size() > 1500) m_points.remove(0, m_points.size() - 1500);
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QRectF box = rect().adjusted(1, 1, -1, -1);
        p.setPen(QColor("#343941"));
        p.setBrush(QColor("#15181c"));
        p.drawRoundedRect(box, 7, 7);

        p.setPen(QColor("#dfe4e8"));
        QFont f = p.font(); f.setPointSize(8); f.setBold(true); p.setFont(f);
        p.drawText(QRectF(9, 5, width()-18, 16), Qt::AlignLeft|Qt::AlignVCenter, m_title);

        const QRectF plot(9, 25, width()-18, height()-36);
        p.setPen(QColor("#292e34"));
        for (int i=1; i<4; ++i)
            p.drawLine(plot.left(), plot.top()+plot.height()*i/4.0, plot.right(), plot.top()+plot.height()*i/4.0);

        if (m_points.size() < 2) {
            p.setPen(QColor("#737b83"));
            p.drawText(plot, Qt::AlignCenter, QStringLiteral("2 min"));
            return;
        }

        double vMin = m_points.first().second;
        double vMax = vMin;
        for (const auto &point : m_points) { vMin=qMin(vMin, point.second); vMax=qMax(vMax, point.second); }
        if (qAbs(vMax-vMin) < 0.001) { vMin -= 1.0; vMax += 1.0; }
        const double margin=(vMax-vMin)*0.10; vMin-=margin; vMax+=margin;
        const qint64 tMax=m_points.last().first;
        const qint64 tMin=qMax<qint64>(0, tMax-120000);

        QPainterPath path;
        for (int i=0; i<m_points.size(); ++i) {
            const double x=plot.left()+((m_points[i].first-tMin)/120000.0)*plot.width();
            const double y=plot.bottom()-((m_points[i].second-vMin)/(vMax-vMin))*plot.height();
            if (i==0) path.moveTo(x,y); else path.lineTo(x,y);
        }
        p.setPen(QPen(QColor("#ff8a1c"), 1.7));
        p.drawPath(path);
        p.setPen(QColor("#8b939b"));
        f.setPointSize(7); f.setBold(false); p.setFont(f);
        p.drawText(QRectF(plot.left(), plot.bottom()-13, 50, 13), Qt::AlignLeft, QStringLiteral("-2 min"));
        p.drawText(QRectF(plot.right()-50, plot.bottom()-13, 50, 13), Qt::AlignRight, QStringLiteral("Maint."));
    }

private:
    QString m_title;
    AbstractMeter *m_meter;
    QElapsedTimer m_clock;
    QVector<QPair<qint64,double> > m_points;
};

class UiModernizer : public QObject
{
public:
    explicit UiModernizer(QMainWindow *window) : QObject(window), m_window(window)
    {
        m_tabs = window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
        if (!m_tabs) return;
        installModeSelector();
        installDatabasePlaceholder();
        installOverviewTrends();
        applyMode(m_modeBox ? m_modeBox->currentIndex() : 0);
    }

private:
    void installModeSelector()
    {
        QWidget *central=m_window->centralWidget();
        if (!central || !central->layout()) return;

        QFrame *bar=new QFrame(central);
        bar->setObjectName(QStringLiteral("modernModeBar"));
        bar->setStyleSheet("#modernModeBar{background:#20242a;border-bottom:1px solid #343941;} QLabel{color:#cfd5da;} QComboBox{background:#121519;color:#f4f6f8;border:1px solid #ff8a1c;border-radius:4px;padding:4px 9px;min-width:145px;}");
        QHBoxLayout *layout=new QHBoxLayout(bar);
        layout->setContentsMargins(12,4,12,4);
        QLabel *title=new QLabel(QStringLiteral("ECU MEMS MANAGER"),bar);
        QFont titleFont=title->font(); titleFont.setBold(true); title->setFont(titleFont);
        layout->addWidget(title);
        layout->addStretch();
        QLabel *modeLabel=new QLabel(QStringLiteral("Mode :"),bar);
        layout->addWidget(modeLabel);
        m_modeBox=new QComboBox(bar);
        m_modeBox->addItem(QStringLiteral("Simple"));
        m_modeBox->addItem(QStringLiteral("Expert"));
        QSettings settings(QStringLiteral("ECU MEMS Manager"), QStringLiteral("ECU MEMS Manager"));
        m_modeBox->setCurrentIndex(settings.value(QStringLiteral("InterfaceMode"),0).toInt()==1 ? 1 : 0);
        layout->addWidget(m_modeBox);
        central->layout()->addWidget(bar);
        connect(m_modeBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
            QSettings s(QStringLiteral("ECU MEMS Manager"), QStringLiteral("ECU MEMS Manager"));
            s.setValue(QStringLiteral("InterfaceMode"), index);
            applyMode(index);
        });
    }

    void installDatabasePlaceholder()
    {
        QWidget *database=new QWidget(m_tabs);
        database->setObjectName(QStringLiteral("database_tab"));
        QVBoxLayout *layout=new QVBoxLayout(database);
        layout->setContentsMargins(28,28,28,28);
        QLabel *heading=new QLabel(QStringLiteral("Base de données ECU"),database);
        QFont f=heading->font(); f.setPointSize(16); f.setBold(true); heading->setFont(f);
        QLabel *info=new QLabel(QStringLiteral("Espace préparé pour la future base MEMS : identification ECU, versions, trames, commandes, DTC, actionneurs, réglages et documentation technique."),database);
        info->setWordWrap(true);
        layout->addWidget(heading);
        layout->addWidget(info);
        layout->addStretch();
        m_tabs->addTab(database, QStringLiteral("Base de données"));
    }

    void installOverviewTrends()
    {
        QWidget *overview=m_window->findChild<QWidget*>(QStringLiteral("overview_tab"));
        if (!overview) return;

        QFrame *panel=new QFrame(overview);
        panel->setObjectName(QStringLiteral("trendPanel2min"));
        panel->setGeometry(25, 600, 1240, 205);
        panel->setStyleSheet("#trendPanel2min{background:#20242a;border:1px solid #343941;border-radius:8px;} QLabel{color:#e7ebee;}");
        QVBoxLayout *outer=new QVBoxLayout(panel);
        outer->setContentsMargins(10,8,10,10);
        QLabel *heading=new QLabel(QStringLiteral("Historique rapide — 2 dernières minutes"),panel);
        QFont hf=heading->font(); hf.setBold(true); heading->setFont(hf);
        outer->addWidget(heading);
        QGridLayout *grid=new QGridLayout();
        grid->setSpacing(7);
        outer->addLayout(grid);

        const struct { const char *name; const char *title; } gauges[] = {
            {"m_revCounter","Régime moteur"}, {"m_mapGauge","Pression MAP"}, {"m_waterTempGauge","Température moteur"},
            {"m_throttle_pos","Papillon"}, {"m_lambda_voltage","Lambda"}, {"m_battery","Batterie"}
        };
        for (int i=0;i<6;++i) {
            AbstractMeter *meter=m_window->findChild<AbstractMeter*>(QString::fromLatin1(gauges[i].name));
            MiniTrendWidget *trend=new MiniTrendWidget(QString::fromUtf8(gauges[i].title),meter,panel);
            grid->addWidget(trend,i/3,i%3);
            m_trends.append(trend);
        }
        overview->setMinimumHeight(qMax(overview->minimumHeight(),820));
        QTimer *timer=new QTimer(this);
        timer->setInterval(500);
        connect(timer,&QTimer::timeout,this,[this](){ for (MiniTrendWidget *trend:m_trends) trend->sample(); });
        timer->start();
    }

    void applyMode(int mode)
    {
        if (!m_tabs) return;
        const bool expert=(mode==1);
        for (int i=0;i<m_tabs->count();++i) {
            QWidget *page=m_tabs->widget(i);
            if (!page) continue;
            const QString name=page->objectName();
            const QString title=m_tabs->tabText(i).toLower();
            const bool expertOnly = name==QStringLiteral("raw") || name==QStringLiteral("ECU") ||
                title.contains(QStringLiteral("rosco")) || title.contains(QStringLiteral("analyse")) ||
                title.contains(QStringLiteral("analysis"));
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            m_tabs->setTabVisible(i, !expertOnly || expert);
#else
            m_tabs->setTabEnabled(i, !expertOnly || expert);
#endif
        }
        m_tabs->setCurrentIndex(0);
    }

    QMainWindow *m_window=nullptr;
    QTabWidget *m_tabs=nullptr;
    QComboBox *m_modeBox=nullptr;
    QVector<MiniTrendWidget*> m_trends;
};

class ModernUiAutoInstaller : public QObject
{
public:
    explicit ModernUiAutoInstaller(QObject *parent=nullptr):QObject(parent){}
protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        if ((event->type()==QEvent::Show || event->type()==QEvent::Polish) && watched) {
            QMainWindow *window=qobject_cast<QMainWindow*>(watched);
            if (window && window->objectName()==QStringLiteral("MainWindow") && !window->property("modernUiInstalled").toBool()) {
                QTabWidget *tabs=window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
                if (tabs && tabs->count()>=8) {
                    window->setProperty("modernUiInstalled",true);
                    QTimer::singleShot(0,window,[window](){ new UiModernizer(window); });
                }
            }
        }
        return QObject::eventFilter(watched,event);
    }
};

void installModernUiHook()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app) return;
    ModernUiAutoInstaller *installer=new ModernUiAutoInstaller(app);
    app->installEventFilter(installer);
}

}

Q_COREAPP_STARTUP_FUNCTION(installModernUiHook)
