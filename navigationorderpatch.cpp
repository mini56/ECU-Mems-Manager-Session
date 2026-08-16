#include <algorithm>
#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QIcon>
#include <QListWidget>
#include <QMainWindow>
#include <QPainter>
#include <QPixmap>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTimer>
#include <QVector>
#include <QWidget>

namespace {

static QWidget *pageOf(QWidget *tab)
{
    if(!tab) return nullptr;
    if(QScrollArea *scroll=qobject_cast<QScrollArea*>(tab)) return scroll->widget();
    return tab;
}

static QString className(QObject *object)
{
    return object ? QString::fromLatin1(object->metaObject()->className()) : QString();
}

// Final menu order requested by the user:
// 1,2,7,6,8,5,3,9,4,10,11 from the previously numbered list.
static int pageRank(QWidget *tab)
{
    QWidget *page=pageOf(tab);
    if(!page) return 100;

    const QString name=page->objectName();
    const QString cls=className(page);

    if(name==QStringLiteral("overview_tab")) return 0;                         // Apercu
    if(name==QStringLiteral("emission_tab")) return 1;                         // Reglages
    if(name==QStringLiteral("actuators")) return 2;                            // Actionneurs
    if(name==QStringLiteral("errors")) return 3;                               // Erreurs
    if(cls==QStringLiteral("DiagnosticPanel") || page->property("strictDiagnosticBuilt").toBool()) return 4;
    if(cls==QStringLiteral("AnalysisTab")) return 5;                           // Analyse
    if(cls==QStringLiteral("SummaryTab") || page->property("strictSummaryBuilt").toBool()) return 6;
    if(page->property("strictRoscoBuilt").toBool()) return 7;                  // ECU / ROSCO
    if(name==QStringLiteral("raw") || page->property("strictRawBuilt").toBool()) return 8;
    if(name==QStringLiteral("database_tab")) return 9;                         // Base de donnees
    if(name==QStringLiteral("ECU") || page->property("strictInteractiveBuilt").toBool()) return 10;

    return 100;
}

static QIcon menuIcon(int rank)
{
    QPixmap pm(22,22);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing,true);
    p.setPen(QPen(QColor(QStringLiteral("#ff8a1c")),1.7,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);

    switch(rank) {
    case 0: // Apercu : quatre cadrans
        p.drawRoundedRect(QRectF(3,4,7,6),1.2,1.2);
        p.drawRoundedRect(QRectF(12,4,7,6),1.2,1.2);
        p.drawRoundedRect(QRectF(3,12,7,6),1.2,1.2);
        p.drawRoundedRect(QRectF(12,12,7,6),1.2,1.2);
        break;
    case 1: // Reglages : curseurs
        p.drawLine(4,6,18,6); p.drawLine(4,11,18,11); p.drawLine(4,16,18,16);
        p.drawEllipse(QPointF(9,6),2,2); p.drawEllipse(QPointF(14,11),2,2); p.drawEllipse(QPointF(7,16),2,2);
        break;
    case 2: // Actionneurs : moteur/engrenage
        p.drawEllipse(QRectF(6,6,10,10)); p.drawEllipse(QPointF(11,11),2.4,2.4);
        p.drawLine(11,3,11,6); p.drawLine(11,16,11,19);
        p.drawLine(3,11,6,11); p.drawLine(16,11,19,11);
        p.drawLine(5.3,5.3,7.2,7.2); p.drawLine(14.8,14.8,16.7,16.7);
        break;
    case 3: { // Erreurs : alerte
        QPolygonF q; q << QPointF(11,3.5) << QPointF(19,18) << QPointF(3,18);
        p.drawPolygon(q); p.drawLine(11,8,11,13); p.drawPoint(QPointF(11,16));
        break;
    }
    case 4: // Diagnostic automatique : loupe + validation
        p.drawEllipse(QRectF(4,4,11,11)); p.drawLine(14,14,19,19);
        p.drawLine(7,10,9.5,12.5); p.drawLine(9.5,12.5,13,8);
        break;
    case 5: { // Analyse : courbe
        p.drawLine(4,18,4,5); p.drawLine(4,18,19,18);
        QPolygonF q; q << QPointF(5,15) << QPointF(9,10) << QPointF(12,12) << QPointF(18,6);
        p.drawPolyline(q);
        break;
    }
    case 6: // Toutes les mesures : barres de mesures
        p.drawRoundedRect(QRectF(4,12,3,6),.8,.8);
        p.drawRoundedRect(QRectF(9.5,8,3,10),.8,.8);
        p.drawRoundedRect(QRectF(15,4,3,14),.8,.8);
        p.drawLine(3,18.5,19,18.5);
        break;
    case 7: // ECU / ROSCO : liaison
        p.drawRoundedRect(QRectF(3,7,9,8),4,4);
        p.drawRoundedRect(QRectF(10,7,9,8),4,4);
        p.drawLine(8,11,14,11);
        break;
    case 8: // Toutes les donnees : matrice brute
        p.drawRoundedRect(QRectF(3.5,3.5,15,15),1.5,1.5);
        p.drawLine(8.5,4,8.5,18); p.drawLine(13.5,4,13.5,18);
        p.drawLine(4,8.5,18,8.5); p.drawLine(4,13.5,18,13.5);
        p.drawPoint(QPointF(6,6)); p.drawPoint(QPointF(11,11)); p.drawPoint(QPointF(16,16));
        break;
    case 9: // Base de donnees : cylindre
        p.drawEllipse(QRectF(4,4,14,5));
        p.drawLine(4,6.5,4,16); p.drawLine(18,6.5,18,16);
        p.drawArc(QRectF(4,13.5,14,5),180*16,180*16);
        p.drawArc(QRectF(4,9,14,5),180*16,180*16);
        break;
    case 10: // Mode interactif : terminal
        p.drawRoundedRect(QRectF(3,4,16,14),2,2);
        p.drawLine(6,8,9,11); p.drawLine(9,11,6,14);
        p.drawLine(11.5,14,16,14);
        break;
    default:
        p.drawRoundedRect(QRectF(4,4,14,14),2,2);
        break;
    }

    return QIcon(pm);
}

struct TabInfo
{
    QWidget *tab=nullptr;
    QString title;
    QString toolTip;
    QIcon icon;
    int rank=100;
    int original=0;
};

static void syncSidebar(QMainWindow *window,QTabWidget *tabs)
{
    if(!window || !tabs) return;
    QListWidget *nav=window->findChild<QListWidget*>(QStringLiteral("uiRebuildNav"));
    if(!nav) return;

    const QSignalBlocker blocker(nav);
    const int textRole=Qt::UserRole+42;
    bool collapsed=false;
    if(QFrame *sidebar=window->findChild<QFrame*>(QStringLiteral("darkSidebar")))
        collapsed=sidebar->property("collapsed").toBool();

    nav->clear();
    nav->setIconSize(QSize(20,20));
    for(int i=0;i<tabs->count();++i) {
        const QString title=tabs->tabText(i).trimmed();
        const int rank=pageRank(tabs->widget(i));
        QListWidgetItem *item=new QListWidgetItem(menuIcon(rank),collapsed?QString():title,nav);
        item->setData(textRole,title);
        item->setToolTip(title);
    }
    nav->setCurrentRow(tabs->currentIndex());
}

static void applyIconsOnly(QMainWindow *window)
{
    if(!window) return;
    QTabWidget *tabs=window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
    QListWidget *nav=window->findChild<QListWidget*>(QStringLiteral("uiRebuildNav"));
    if(!tabs || !nav || nav->count()!=tabs->count()) return;

    for(int i=0;i<tabs->count();++i) {
        const int rank=pageRank(tabs->widget(i));
        if(QListWidgetItem *item=nav->item(i)) item->setIcon(menuIcon(rank));
    }
}

static void applyRequestedOrder(QMainWindow *window)
{
    if(!window) return;
    QTabWidget *tabs=window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
    if(!tabs || tabs->count()!=11) return;

    QVector<TabInfo> entries;
    entries.reserve(tabs->count());
    int rankCounts[11]={0,0,0,0,0,0,0,0,0,0,0};

    for(int i=0;i<tabs->count();++i) {
        TabInfo info;
        info.tab=tabs->widget(i);
        info.title=tabs->tabText(i);
        info.toolTip=tabs->tabToolTip(i);
        info.icon=tabs->tabIcon(i);
        info.rank=pageRank(info.tab);
        info.original=i;
        if(info.rank>=0 && info.rank<11) ++rankCounts[info.rank];
        entries.append(info);
    }

    for(int rank=0;rank<11;++rank)
        if(rankCounts[rank]!=1) return;

    std::stable_sort(entries.begin(),entries.end(),[](const TabInfo &a,const TabInfo &b){
        if(a.rank!=b.rank) return a.rank<b.rank;
        return a.original<b.original;
    });

    QWidget *current=tabs->currentWidget();
    for(int i=tabs->count()-1;i>=0;--i) tabs->removeTab(i);

    for(const TabInfo &info:entries) {
        const int index=tabs->addTab(info.tab,menuIcon(info.rank),info.title);
        tabs->setTabToolTip(index,info.toolTip);
    }

    if(current && tabs->indexOf(current)>=0) tabs->setCurrentWidget(current);
    syncSidebar(window,tabs);
    window->setProperty("requestedNavigationOrderApplied",true);
}

class NavigationOrderInstaller : public QObject
{
public:
    explicit NavigationOrderInstaller(QObject *parent=nullptr):QObject(parent){}
protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QMainWindow *window=qobject_cast<QMainWindow*>(watched);
        if(!window || window->objectName()!=QStringLiteral("MainWindow"))
            return QObject::eventFilter(watched,event);

        if((event->type()==QEvent::Show || event->type()==QEvent::Polish) &&
           !window->property("requestedNavigationOrderScheduled").toBool()) {
            window->setProperty("requestedNavigationOrderScheduled",true);
            // Run after the legacy dedicated-page rebuild and Database-tab insertion.
            QTimer::singleShot(4300,window,[window](){applyRequestedOrder(window);});
            QTimer::singleShot(5400,window,[window](){applyRequestedOrder(window);});
        } else if(event->type()==QEvent::LanguageChange) {
            QTimer::singleShot(160,window,[window](){applyRequestedOrder(window);});
        } else if(event->type()==QEvent::Resize && window->property("requestedNavigationOrderApplied").toBool()) {
            // darkstyle.cpp restyles the navigation shortly after a resize;
            // restore the semantic, unique icons just after that visual pass.
            QTimer::singleShot(150,window,[window](){applyIconsOnly(window);});
        }
        return QObject::eventFilter(watched,event);
    }
};

void installNavigationOrderPatch()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if(app) app->installEventFilter(new NavigationOrderInstaller(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installNavigationOrderPatch)
