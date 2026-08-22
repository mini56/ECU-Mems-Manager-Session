#include "navigationorderpatch.h"

#include <QApplication>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QPainter>
#include <QPixmap>
#include <QPolygonF>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTimer>
#include <QVariant>
#include <QWidget>

#include "i18n.h"
#include "mainwindow.h"
#include "mems19testtab.h"

namespace {

static const int kTextRole=Qt::UserRole+42;
static const int kTabPtrRole=Qt::UserRole+60;
static const int kFinalTabCount=13;

static QWidget *realPage(QWidget *tab)
{
    if(!tab)return nullptr;
    if(QScrollArea *scroll=qobject_cast<QScrollArea*>(tab))return scroll->widget();
    return tab;
}

static int rankFromTitle(const QString &title)
{
    const QString t=title.trimmed();
    if(t==I18n::text(1001).trimmed())return 0; // Aperçu
    if(t==I18n::text(7200).trimmed())return 1; // Injection
    if(t==I18n::text(2001).trimmed())return 2; // Réglages
    if(t==I18n::text(4001).trimmed())return 3; // Actionneurs
    if(t==I18n::text(3001).trimmed())return 4; // Erreurs
    if(t==I18n::text(7013).trimmed())return 5; // Diagnostic auto
    if(t==I18n::text(7018).trimmed())return 6; // Analyse
    if(t==I18n::text(7017).trimmed())return 7; // Toutes les mesures
    if(t==I18n::text(7012).trimmed())return 8; // ECU/ROSCO
    if(t==I18n::text(5001).trimmed())return 9; // Toutes les données
    if(t==I18n::text(7152).trimmed())return 10; // Base données
    if(t==I18n::text(6002).trimmed()||t==I18n::text(6003).trimmed())return 11; // Interactif
    if(t==I18n::text(7930).trimmed())return 12; // Test ECU 1.9
    return -1;
}

static int fallbackRank(QWidget *tab)
{
    QWidget *page=realPage(tab);if(!page)return -1;
    const QString name=page->objectName().toLower();const QString cls=QString::fromLatin1(page->metaObject()->className()).toLower();
    if(name==QStringLiteral("overview_tab"))return 0;
    if(name==QStringLiteral("injection_tab"))return 1;
    if(name==QStringLiteral("emission_tab"))return 2;
    if(name==QStringLiteral("actuators"))return 3;
    if(name==QStringLiteral("errors"))return 4;
    if(cls.contains(QStringLiteral("diagnostic")))return 5;
    if(cls.contains(QStringLiteral("analysis")))return 6;
    if(cls.contains(QStringLiteral("summary")))return 7;
    if(name==QStringLiteral("ecu"))return 8;
    if(name==QStringLiteral("raw"))return 9;
    if(name==QStringLiteral("database_tab"))return 10;
    if(name.contains(QStringLiteral("interactive")))return 11;
    if(name==QStringLiteral("mems19_test_tab"))return 12;
    return -1;
}

static int rankForTab(QTabWidget *tabs,int index)
{
    if(!tabs||index<0||index>=tabs->count())return -1;
    const int byTitle=rankFromTitle(tabs->tabText(index));return byTitle>=0?byTitle:fallbackRank(tabs->widget(index));
}

static QIcon iconForRank(int rank)
{
    QPixmap pm(22,22);pm.fill(Qt::transparent);QPainter p(&pm);p.setRenderHint(QPainter::Antialiasing,true);p.setPen(QPen(QColor("#ff8a1c"),1.7,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));p.setBrush(Qt::NoBrush);
    switch(rank){
    case 0:
        p.drawRoundedRect(QRectF(3,4,7,6),1.2,1.2);p.drawRoundedRect(QRectF(12,4,7,6),1.2,1.2);p.drawRoundedRect(QRectF(3,12,7,6),1.2,1.2);p.drawRoundedRect(QRectF(12,12,7,6),1.2,1.2);break;
    case 1: // injector
        p.drawLine(5,17,16,6);p.drawLine(7,19,18,8);p.drawLine(13,5,19,11);p.drawLine(4,14,9,19);p.drawLine(4,18,2.5,19.5);break;
    case 2:
        p.drawLine(4,6,18,6);p.drawLine(4,11,18,11);p.drawLine(4,16,18,16);p.drawEllipse(QPointF(9,6),2,2);p.drawEllipse(QPointF(14,11),2,2);p.drawEllipse(QPointF(7,16),2,2);break;
    case 3:
        p.drawEllipse(QRectF(6,6,10,10));p.drawEllipse(QPointF(11,11),2.4,2.4);p.drawLine(11,3,11,6);p.drawLine(11,16,11,19);p.drawLine(3,11,6,11);p.drawLine(16,11,19,11);break;
    case 4:{QPolygonF q;q<<QPointF(11,3.5)<<QPointF(19,18)<<QPointF(3,18);p.drawPolygon(q);p.drawLine(11,8,11,13);p.drawPoint(QPointF(11,16));break;}
    case 5:
        p.drawEllipse(QRectF(4,4,11,11));p.drawLine(14,14,19,19);p.drawLine(7,10,9.5,12.5);p.drawLine(9.5,12.5,13,8);break;
    case 6:{p.drawLine(4,18,4,5);p.drawLine(4,18,19,18);QPolygonF q;q<<QPointF(5,15)<<QPointF(9,10)<<QPointF(12,12)<<QPointF(18,6);p.drawPolyline(q);break;}
    case 7:
        p.drawRoundedRect(QRectF(4,12,3,6),.8,.8);p.drawRoundedRect(QRectF(9.5,8,3,10),.8,.8);p.drawRoundedRect(QRectF(15,4,3,14),.8,.8);p.drawLine(3,18.5,19,18.5);break;
    case 8:
        p.drawRoundedRect(QRectF(3,7,9,8),4,4);p.drawRoundedRect(QRectF(10,7,9,8),4,4);p.drawLine(8,11,14,11);break;
    case 9:
        p.drawRoundedRect(QRectF(3.5,3.5,15,15),1.5,1.5);p.drawLine(8.5,4,8.5,18);p.drawLine(13.5,4,13.5,18);p.drawLine(4,8.5,18,8.5);p.drawLine(4,13.5,18,13.5);break;
    case 10:
        p.drawEllipse(QRectF(4,4,14,5));p.drawLine(4,6.5,4,16);p.drawLine(18,6.5,18,16);p.drawArc(QRectF(4,13.5,14,5),180*16,180*16);p.drawArc(QRectF(4,9,14,5),180*16,180*16);break;
    case 11:
        p.drawRoundedRect(QRectF(3,4,16,14),2,2);p.drawLine(6,8,9,11);p.drawLine(9,11,6,14);p.drawLine(11.5,14,16,14);break;
    case 12:
        p.drawRoundedRect(QRectF(3,5,10,12),2,2);p.drawLine(13,9,18,9);p.drawLine(13,13,18,13);p.drawLine(18,7,18,15);p.drawLine(5,8,10,8);p.drawLine(5,11,10,11);p.drawLine(5,14,10,14);break;
    default:p.drawEllipse(QPointF(11,11),7,7);break;
    }
    return QIcon(pm);
}

static void fitRawSpacing(QMainWindow *window)
{
    if(!window)return;QTabWidget *tabs=window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));if(!tabs)return;
    QWidget *page=nullptr;for(int i=0;i<tabs->count();++i){QWidget *candidate=realPage(tabs->widget(i));if(candidate&&candidate->objectName()==QStringLiteral("raw")){page=candidate;break;}}
    if(!page)return;QWidget *left=page->findChild<QWidget*>(QStringLiteral("raw_1"));QWidget *right=page->findChild<QWidget*>(QStringLiteral("raw_2"));if(!left||!right)return;
    QGridLayout *leftGrid=qobject_cast<QGridLayout*>(left->layout());QGridLayout *rightGrid=qobject_cast<QGridLayout*>(right->layout());const int rows=qMax(leftGrid?leftGrid->rowCount():1,rightGrid?rightGrid->rowCount():1);if(rows<=0)return;
    const int pageHeight=qMax(page->height(),tabs->height());const bool wrapped=left->parentWidget()&&left->parentWidget()!=page;const int reserve=wrapped?48:20;const int usableHeight=qMax(320,pageHeight-reserve);const int rowHeight=qBound(13,usableHeight/rows,18);const int blockHeight=rowHeight*rows;
    auto fitBlock=[page,rowHeight,blockHeight](QWidget *block,QGridLayout *grid){if(!block||!grid)return;grid->setContentsMargins(0,0,0,0);grid->setHorizontalSpacing(8);grid->setVerticalSpacing(0);grid->setSizeConstraint(QLayout::SetDefaultConstraint);for(int r=0;r<grid->rowCount();++r)grid->setRowMinimumHeight(r,rowHeight);block->setMinimumHeight(blockHeight);block->setMaximumHeight(blockHeight);block->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);QWidget *container=block->parentWidget();if(container&&container!=page){container->setMinimumHeight(blockHeight+42);container->setMaximumHeight(QWIDGETSIZE_MAX);container->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);}else if(container==page){const int y=qMax(6,block->y());block->setGeometry(block->x(),y,block->width(),blockHeight);}for(QLabel *label:block->findChildren<QLabel*>()){label->setWordWrap(false);label->setMinimumHeight(rowHeight);label->setMaximumHeight(rowHeight);label->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);}};
    fitBlock(left,leftGrid);fitBlock(right,rightGrid);
}

static QWidget *tabFromItem(QListWidgetItem *item)
{
    if(!item)return nullptr;const qulonglong raw=item->data(kTabPtrRole).toULongLong();return reinterpret_cast<QWidget*>(static_cast<quintptr>(raw));
}

static void installMappedConnections(QMainWindow *window,QTabWidget *tabs,QListWidget *nav)
{
    if(!window||!tabs||!nav||nav->property("finalNavigationMapped").toBool())return;
    QObject::disconnect(nav,&QListWidget::currentRowChanged,tabs,&QTabWidget::setCurrentIndex);QObject::disconnect(tabs,nullptr,nav,nullptr);
    QObject::connect(nav,&QListWidget::currentRowChanged,window,[nav,tabs](int row){if(row<0||row>=nav->count())return;QWidget *target=tabFromItem(nav->item(row));if(!target)return;const int index=tabs->indexOf(target);if(index>=0&&tabs->currentIndex()!=index)tabs->setCurrentIndex(index);});
    QObject::connect(tabs,&QTabWidget::currentChanged,window,[window,nav,tabs](int index){if(index<0||index>=tabs->count())return;QWidget *current=tabs->widget(index);for(int row=0;row<nav->count();++row){if(tabFromItem(nav->item(row))==current){const QSignalBlocker blocker(nav);nav->setCurrentRow(row);break;}}QTimer::singleShot(60,window,[window](){fitRawSpacing(window);});});
    nav->setProperty("finalNavigationMapped",true);
}

static void applyFinalMenu(QMainWindow *window)
{
    if(!window)return;QTabWidget *tabs=window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));QListWidget *nav=window->findChild<QListWidget*>(QStringLiteral("uiRebuildNav"));if(!tabs||!nav||tabs->count()<kFinalTabCount)return;
    QWidget *tabsByRank[kFinalTabCount]={nullptr};QString titlesByRank[kFinalTabCount];
    for(int i=0;i<tabs->count();++i){const int rank=rankForTab(tabs,i);if(rank<0||rank>=kFinalTabCount||tabsByRank[rank])continue;tabsByRank[rank]=tabs->widget(i);titlesByRank[rank]=tabs->tabText(i).trimmed();}
    for(int rank=0;rank<kFinalTabCount;++rank)if(!tabsByRank[rank])return;
    bool collapsed=false;if(QFrame *sidebar=window->findChild<QFrame*>(QStringLiteral("darkSidebar")))collapsed=sidebar->property("collapsed").toBool();QWidget *current=tabs->currentWidget();int currentRow=-1;
    {const QSignalBlocker blocker(nav);nav->clear();nav->setIconSize(QSize(20,20));for(int rank=0;rank<kFinalTabCount;++rank){QListWidgetItem *item=new QListWidgetItem(iconForRank(rank),collapsed?QString():titlesByRank[rank],nav);item->setData(kTextRole,titlesByRank[rank]);item->setData(kTabPtrRole,static_cast<qulonglong>(reinterpret_cast<quintptr>(tabsByRank[rank])));item->setToolTip(titlesByRank[rank]);if(tabsByRank[rank]==current)currentRow=rank;}if(currentRow>=0)nav->setCurrentRow(currentRow);}
    installMappedConnections(window,tabs,nav);fitRawSpacing(window);window->setProperty("finalNavigationApplied",true);
}

class FinalNavigationFilter:public QObject
{
public:explicit FinalNavigationFilter(QMainWindow *window):QObject(window),m_window(window){}
protected:bool eventFilter(QObject *watched,QEvent *event) override{if(watched==m_window&&m_window){if(event->type()==QEvent::Resize&&m_window->property("finalNavigationApplied").toBool())QTimer::singleShot(180,m_window,[this](){if(m_window)applyFinalMenu(m_window);});else if(event->type()==QEvent::LanguageChange)QTimer::singleShot(300,m_window,[this](){if(m_window)applyFinalMenu(m_window);});}return QObject::eventFilter(watched,event);}private:QMainWindow *m_window;
};

} // namespace

void installFinalNavigation(QApplication *app,QMainWindow *window)
{
    if(!app||!window||window->property("finalNavigationInstalled").toBool())return;
    if(MainWindow *mainWindow=qobject_cast<MainWindow*>(window)){
        if(QTabWidget *tabs=mainWindow->findChild<QTabWidget*>(QStringLiteral("Tab_main"))){
            bool exists=false;for(int i=0;i<tabs->count();++i){QWidget *page=realPage(tabs->widget(i));if(page&&page->objectName()==QStringLiteral("mems19_test_tab")){exists=true;break;}}
            if(!exists&&mainWindow->memsInterface())tabs->addTab(new Mems19TestTab(mainWindow->memsInterface(),tabs),I18n::text(7930));
        }
    }
    window->setProperty("finalNavigationInstalled",true);window->installEventFilter(new FinalNavigationFilter(window));const int delays[]={80,450,1550,2550,3750,4700};for(int delay:delays)QTimer::singleShot(delay,window,[window](){applyFinalMenu(window);});
}
