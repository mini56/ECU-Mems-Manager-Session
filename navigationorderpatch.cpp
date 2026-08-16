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
#include "i18n.h"

namespace {

static const int kTextRole=Qt::UserRole+42;
static const int kTabPtrRole=Qt::UserRole+60;
static const int kRankRole=Qt::UserRole+61;

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

static int rankFromTranslatedTitle(const QString &title)
{
    const QString t=title.trimmed();
    if(t==I18n::text(1001).trimmed()) return 0; // Apercu
    if(t==I18n::text(2001).trimmed()) return 1; // Reglages
    if(t==I18n::text(4001).trimmed()) return 2; // Actionneurs
    if(t==I18n::text(3001).trimmed()) return 3; // Erreurs
    if(t==I18n::text(7013).trimmed()) return 4; // Diagnostic automatique
    if(t==I18n::text(7018).trimmed()) return 5; // Analyse
    if(t==I18n::text(7017).trimmed()) return 6; // Toutes les mesures
    if(t==I18n::text(7012).trimmed()) return 7; // ECU / ROSCO
    if(t==I18n::text(5001).trimmed()) return 8; // Toutes les donnees
    if(t==I18n::text(7152).trimmed()) return 9; // Base de donnees
    if(t==I18n::text(6002).trimmed()) return 10; // Mode interactif
    return 100;
}

static int pageRank(QTabWidget *tabs,int index)
{
    if(!tabs || index<0 || index>=tabs->count()) return 100;

    const int byTitle=rankFromTranslatedTitle(tabs->tabText(index));
    if(byTitle<100) return byTitle;

    QWidget *page=pageOf(tabs->widget(index));
    if(!page) return 100;
    const QString name=page->objectName();
    const QString cls=className(page);

    if(name==QStringLiteral("overview_tab")) return 0;
    if(name==QStringLiteral("emission_tab")) return 1;
    if(name==QStringLiteral("actuators")) return 2;
    if(name==QStringLiteral("errors")) return 3;
    if(cls==QStringLiteral("DiagnosticPanel")) return 4;
    if(cls==QStringLiteral("AnalysisTab")) return 5;
    if(cls==QStringLiteral("SummaryTab")) return 6;
    if(page->property("strictRoscoBuilt").toBool()) return 7;
    if(name==QStringLiteral("raw")) return 8;
    if(name==QStringLiteral("database_tab")) return 9;
    if(name==QStringLiteral("ECU")) return 10;
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
    case 0:
        p.drawRoundedRect(QRectF(3,4,7,6),1.2,1.2);
        p.drawRoundedRect(QRectF(12,4,7,6),1.2,1.2);
        p.drawRoundedRect(QRectF(3,12,7,6),1.2,1.2);
        p.drawRoundedRect(QRectF(12,12,7,6),1.2,1.2);
        break;
    case 1:
        p.drawLine(4,6,18,6); p.drawLine(4,11,18,11); p.drawLine(4,16,18,16);
        p.drawEllipse(QPointF(9,6),2,2); p.drawEllipse(QPointF(14,11),2,2); p.drawEllipse(QPointF(7,16),2,2);
        break;
    case 2:
        p.drawEllipse(QRectF(6,6,10,10)); p.drawEllipse(QPointF(11,11),2.4,2.4);
        p.drawLine(11,3,11,6); p.drawLine(11,16,11,19);
        p.drawLine(3,11,6,11); p.drawLine(16,11,19,11);
        p.drawLine(5.3,5.3,7.2,7.2); p.drawLine(14.8,14.8,16.7,16.7);
        break;
    case 3: {
        QPolygonF q; q << QPointF(11,3.5) << QPointF(19,18) << QPointF(3,18);
        p.drawPolygon(q); p.drawLine(11,8,11,13); p.drawPoint(QPointF(11,16));
        break;
    }
    case 4:
        p.drawEllipse(QRectF(4,4,11,11)); p.drawLine(14,14,19,19);
        p.drawLine(7,10,9.5,12.5); p.drawLine(9.5,12.5,13,8);
        break;
    case 5: {
        p.drawLine(4,18,4,5); p.drawLine(4,18,19,18);
        QPolygonF q; q << QPointF(5,15) << QPointF(9,10) << QPointF(12,12) << QPointF(18,6);
        p.drawPolyline(q);
        break;
    }
    case 6:
        p.drawRoundedRect(QRectF(4,12,3,6),.8,.8);
        p.drawRoundedRect(QRectF(9.5,8,3,10),.8,.8);
        p.drawRoundedRect(QRectF(15,4,3,14),.8,.8);
        p.drawLine(3,18.5,19,18.5);
        break;
    case 7:
        p.drawRoundedRect(QRectF(3,7,9,8),4,4);
        p.drawRoundedRect(QRectF(10,7,9,8),4,4);
        p.drawLine(8,11,14,11);
        break;
    case 8:
        p.drawRoundedRect(QRectF(3.5,3.5,15,15),1.5,1.5);
        p.drawLine(8.5,4,8.5,18); p.drawLine(13.5,4,13.5,18);
        p.drawLine(4,8.5,18,8.5); p.drawLine(4,13.5,18,13.5);
        p.drawPoint(QPointF(6,6)); p.drawPoint(QPointF(11,11)); p.drawPoint(QPointF(16,16));
        break;
    case 9:
        p.drawEllipse(QRectF(4,4,14,5));
        p.drawLine(4,6.5,4,16); p.drawLine(18,6.5,18,16);
        p.drawArc(QRectF(4,13.5,14,5),180*16,180*16);
        p.drawArc(QRectF(4,9,14,5),180*16,180*16);
        break;
    case 10:
        p.drawRoundedRect(QRectF(3,4,16,14),2,2);
        p.drawLine(6,8,9,11); p.drawLine(9,11,6,14);
        p.drawLine(11.5,14,16,14);
        break;
    default:
        p.drawEllipse(QPointF(11,11),7,7);
        break;
    }
    return QIcon(pm);
}

struct MenuEntry
{
    QWidget *tab=nullptr;
    QString title;
    int rank=100;
    int originalIndex=0;
};

static QWidget *tabFromItem(QListWidgetItem *item)
{
    if(!item) return nullptr;
    const qulonglong raw=item->data(kTabPtrRole).toULongLong();
    return reinterpret_cast<QWidget*>(static_cast<quintptr>(raw));
}

static void installMappedConnections(QMainWindow *window,QTabWidget *tabs,QListWidget *nav)
{
    if(!window || !tabs || !nav || nav->property("mappedNavigationConnections").toBool()) return;

    QObject::disconnect(nav,&QListWidget::currentRowChanged,tabs,&QTabWidget::setCurrentIndex);
    QObject::disconnect(tabs,nullptr,nav,nullptr);

    QObject::connect(nav,&QListWidget::currentRowChanged,window,[nav,tabs](int row){
        if(row<0 || row>=nav->count()) return;
        QWidget *target=tabFromItem(nav->item(row));
        if(!target) return;
        const int index=tabs->indexOf(target);
        if(index>=0 && tabs->currentIndex()!=index) tabs->setCurrentIndex(index);
    });

    QObject::connect(tabs,&QTabWidget::currentChanged,window,[nav,tabs](int index){
        if(index<0 || index>=tabs->count()) return;
        QWidget *current=tabs->widget(index);
        for(int row=0;row<nav->count();++row) {
            if(tabFromItem(nav->item(row))==current) {
                const QSignalBlocker blocker(nav);
                nav->setCurrentRow(row);
                break;
            }
        }
    });

    nav->setProperty("mappedNavigationConnections",true);
}

static void applyRequestedMenu(QMainWindow *window)
{
    if(!window) return;
    QTabWidget *tabs=window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
    QListWidget *nav=window->findChild<QListWidget*>(QStringLiteral("uiRebuildNav"));
    if(!tabs || !nav || tabs->count()<11) return;

    QVector<MenuEntry> entries;
    entries.reserve(tabs->count());
    bool seen[11]={false,false,false,false,false,false,false,false,false,false,false};

    for(int i=0;i<tabs->count();++i) {
        const int rank=pageRank(tabs,i);
        if(rank<0 || rank>=11 || seen[rank]) continue;
        seen[rank]=true;
        MenuEntry entry;
        entry.tab=tabs->widget(i);
        entry.title=tabs->tabText(i).trimmed();
        entry.rank=rank;
        entry.originalIndex=i;
        entries.append(entry);
    }

    for(int rank=0;rank<11;++rank)
        if(!seen[rank]) return;

    std::stable_sort(entries.begin(),entries.end(),[](const MenuEntry &a,const MenuEntry &b){
        return a.rank<b.rank;
    });

    bool collapsed=false;
    if(QFrame *sidebar=window->findChild<QFrame*>(QStringLiteral("darkSidebar")))
        collapsed=sidebar->property("collapsed").toBool();

    QWidget *current=tabs->currentWidget();
    {
        const QSignalBlocker blocker(nav);
        nav->clear();
        nav->setIconSize(QSize(20,20));
        int currentRow=-1;
        for(int row=0;row<entries.size();++row) {
            const MenuEntry &entry=entries.at(row);
            QListWidgetItem *item=new QListWidgetItem(menuIcon(entry.rank),collapsed?QString():entry.title,nav);
            item->setData(kTextRole,entry.title);
            item->setData(kTabPtrRole,static_cast<qulonglong>(reinterpret_cast<quintptr>(entry.tab)));
            item->setData(kRankRole,entry.rank);
            item->setToolTip(entry.title);
            if(entry.tab==current) currentRow=row;
        }
        if(currentRow>=0) nav->setCurrentRow(currentRow);
    }

    installMappedConnections(window,tabs,nav);
    window->setProperty("requestedNavigationMenuApplied",true);
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
           !window->property("requestedNavigationMenuScheduled").toBool()) {
            window->setProperty("requestedNavigationMenuScheduled",true);
            QTimer::singleShot(4000,window,[window](){applyRequestedMenu(window);});
            QTimer::singleShot(4700,window,[window](){applyRequestedMenu(window);});
            QTimer::singleShot(5600,window,[window](){applyRequestedMenu(window);});
        } else if(event->type()==QEvent::LanguageChange) {
            QTimer::singleShot(550,window,[window](){applyRequestedMenu(window);});
        } else if(event->type()==QEvent::Resize && window->property("requestedNavigationMenuApplied").toBool()) {
            QTimer::singleShot(180,window,[window](){applyRequestedMenu(window);});
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
