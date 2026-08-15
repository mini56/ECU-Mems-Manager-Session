#include <QAbstractItemView>
#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTabWidget>
#include <QTimer>
#include <QVariant>
#include <QVBoxLayout>
#include <QWidget>
#include "i18n.h"

namespace {

static QWidget *realPage(QWidget *tab)
{
    if(!tab) return nullptr;
    if(QScrollArea *scroll=qobject_cast<QScrollArea*>(tab)) return scroll->widget();
    return tab;
}

static QWidget *findInteractiveTab(QTabWidget *tabs)
{
    if(!tabs) return nullptr;
    for(int i=0;i<tabs->count();++i) {
        QWidget *tab=tabs->widget(i);
        QWidget *page=realPage(tab);
        if(page && page->objectName()==QStringLiteral("ECU"))
            return tab;
    }
    return nullptr;
}

static QWidget *findDatabaseTab(QTabWidget *tabs)
{
    if(!tabs) return nullptr;
    for(int i=0;i<tabs->count();++i) {
        QWidget *tab=tabs->widget(i);
        QWidget *page=realPage(tab);
        if(page && page->objectName()==QStringLiteral("database_tab"))
            return tab;
    }
    return nullptr;
}

static QLabel *dbLabel(int key,QWidget *parent)
{
    QLabel *label=new QLabel(parent);
    label->setProperty("i18nKey",key);
    label->setText(I18n::text(key));
    return label;
}

static QTableWidgetItem *dbItem(int key)
{
    QTableWidgetItem *item=new QTableWidgetItem;
    item->setData(Qt::UserRole,key);
    item->setText(I18n::text(key));
    item->setTextAlignment(Qt::AlignCenter);
    return item;
}

static QFrame *dbCard(QWidget *parent)
{
    QFrame *frame=new QFrame(parent);
    frame->setAttribute(Qt::WA_StyledBackground,true);
    frame->setStyleSheet(QStringLiteral(
        "QFrame{background:#10171d;border:1px solid #29343e;border-radius:5px;}"));
    return frame;
}

static QFrame *dbMetric(int valueKey,int titleKey,QWidget *parent)
{
    QFrame *frame=dbCard(parent);
    QVBoxLayout *layout=new QVBoxLayout(frame);
    layout->setContentsMargins(9,6,9,6);
    layout->setSpacing(0);

    QLabel *value=dbLabel(valueKey,frame);
    value->setAlignment(Qt::AlignCenter);
    QFont vf=value->font(); vf.setBold(true); vf.setPointSizeF(15.5); value->setFont(vf);
    value->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;border:0;"));

    QLabel *title=dbLabel(titleKey,frame);
    title->setAlignment(Qt::AlignCenter);
    title->setWordWrap(true);
    QFont tf=title->font(); tf.setBold(true); tf.setPointSizeF(7.4); title->setFont(tf);
    title->setStyleSheet(QStringLiteral("color:#aeb9c2;background:transparent;border:0;"));

    layout->addWidget(value);
    layout->addWidget(title);
    return frame;
}

static QLabel *dbSection(int key,QWidget *parent)
{
    QLabel *label=dbLabel(key,parent);
    QFont f=label->font(); f.setBold(true); f.setPointSizeF(8.4); label->setFont(f);
    label->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;border:0;"));
    return label;
}

static QLabel *dbInfo(int key,QWidget *parent,bool strong=false)
{
    QLabel *label=dbLabel(key,parent);
    label->setWordWrap(true);
    QFont f=label->font(); f.setPointSizeF(strong?8.1:7.8); f.setBold(strong); label->setFont(f);
    label->setStyleSheet(QStringLiteral("color:%1;background:transparent;border:0;padding:2px 0;")
                         .arg(strong?QStringLiteral("#e7ecef"):QStringLiteral("#b7c1c9")));
    return label;
}

static void refreshDatabaseTranslations(QWidget *database)
{
    if(!database) return;
    for(QLabel *label:database->findChildren<QLabel*>()) {
        const QVariant value=label->property("i18nKey");
        if(value.isValid()) label->setText(I18n::text(value.toInt()));
    }

    QTableWidget *table=database->findChild<QTableWidget*>(QStringLiteral("databaseGenerationTable"));
    if(!table) return;
    for(int c=0;c<table->columnCount();++c) {
        QTableWidgetItem *item=table->horizontalHeaderItem(c);
        if(item) item->setText(I18n::text(item->data(Qt::UserRole).toInt()));
    }
    for(int r=0;r<table->rowCount();++r)
        for(int c=0;c<table->columnCount();++c) {
            QTableWidgetItem *item=table->item(r,c);
            if(item) item->setText(I18n::text(item->data(Qt::UserRole).toInt()));
        }
}

static void buildDatabasePage(QWidget *database)
{
    if(!database) return;
    if(database->property("databaseFirstDraftBuilt").toBool()) {
        refreshDatabaseTranslations(database);
        return;
    }
    database->setProperty("databaseFirstDraftBuilt",true);
    database->setMinimumSize(0,0);
    database->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    database->setAttribute(Qt::WA_StyledBackground,true);
    database->setStyleSheet(QStringLiteral("#database_tab{background:#090e13;}"));

    QVBoxLayout *root=new QVBoxLayout(database);
    root->setContentsMargins(10,8,10,8);
    root->setSpacing(7);

    QFrame *hero=dbCard(database);
    hero->setStyleSheet(QStringLiteral(
        "QFrame{background:#10171d;border:1px solid #36434d;border-left:3px solid #ff7a00;border-radius:5px;}"));
    QVBoxLayout *heroLayout=new QVBoxLayout(hero);
    heroLayout->setContentsMargins(12,7,12,7);
    heroLayout->setSpacing(1);
    QLabel *title=dbLabel(7160,hero);
    QFont titleFont=title->font(); titleFont.setBold(true); titleFont.setPointSizeF(10.5); title->setFont(titleFont);
    title->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;border:0;"));
    QLabel *subtitle=dbLabel(7161,hero);
    QFont subtitleFont=subtitle->font(); subtitleFont.setPointSizeF(7.7); subtitle->setFont(subtitleFont);
    subtitle->setStyleSheet(QStringLiteral("color:#9aa7b1;background:transparent;border:0;"));
    heroLayout->addWidget(title);
    heroLayout->addWidget(subtitle);
    root->addWidget(hero);

    QFrame *scope=dbCard(database);
    QHBoxLayout *scopeLayout=new QHBoxLayout(scope);
    scopeLayout->setContentsMargins(9,5,9,5);
    scopeLayout->setSpacing(6);
    scopeLayout->addWidget(dbSection(7162,scope));
    for(int key:{7163,7164,7165,7166}) {
        QLabel *pill=dbLabel(key,scope);
        pill->setAlignment(Qt::AlignCenter);
        QFont pf=pill->font(); pf.setBold(true); pf.setPointSizeF(8.0); pill->setFont(pf);
        pill->setStyleSheet(QStringLiteral(
            "color:#f0f3f5;background:#172027;border:1px solid #46525c;border-radius:4px;padding:4px 10px;"));
        scopeLayout->addWidget(pill);
    }
    scopeLayout->addStretch(1);
    QLabel *excluded=dbLabel(7167,scope);
    QFont ef=excluded->font(); ef.setPointSizeF(7.5); excluded->setFont(ef);
    excluded->setStyleSheet(QStringLiteral("color:#89959f;background:transparent;border:0;"));
    scopeLayout->addWidget(excluded);
    root->addWidget(scope);

    QGridLayout *metrics=new QGridLayout;
    metrics->setContentsMargins(0,0,0,0);
    metrics->setHorizontalSpacing(6);
    metrics->setVerticalSpacing(0);
    const int valueKeys[]={7174,7175,7176,7177,7178,7179};
    const int titleKeys[]={7168,7169,7170,7171,7172,7173};
    for(int i=0;i<6;++i) {
        metrics->addWidget(dbMetric(valueKeys[i],titleKeys[i],database),0,i);
        metrics->setColumnStretch(i,1);
    }
    root->addLayout(metrics);

    QHBoxLayout *mainArea=new QHBoxLayout;
    mainArea->setContentsMargins(0,0,0,0);
    mainArea->setSpacing(7);

    QFrame *generationCard=dbCard(database);
    QVBoxLayout *generationLayout=new QVBoxLayout(generationCard);
    generationLayout->setContentsMargins(9,7,9,8);
    generationLayout->setSpacing(5);
    generationLayout->addWidget(dbSection(7180,generationCard));

    QTableWidget *table=new QTableWidget(4,6,generationCard);
    table->setObjectName(QStringLiteral("databaseGenerationTable"));
    const int headers[]={7181,7182,7183,7184,7185,7186};
    for(int c=0;c<6;++c) table->setHorizontalHeaderItem(c,dbItem(headers[c]));
    const int rows[4][6]={
        {7163,7187,7188,7189,7190,7191},
        {7164,7192,7193,7189,7190,7194},
        {7165,7195,7193,7189,7190,7196},
        {7166,7197,7193,7198,7199,7200}
    };
    for(int r=0;r<4;++r)
        for(int c=0;c<6;++c)
            table->setItem(r,c,dbItem(rows[r][c]));
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->horizontalHeader()->setMinimumSectionSize(72);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setFocusPolicy(Qt::NoFocus);
    table->setShowGrid(false);
    table->setAlternatingRowColors(true);
    table->setMinimumHeight(165);
    table->setMaximumHeight(215);
    table->setStyleSheet(QStringLiteral(
        "QTableWidget{background:#0a1117;color:#dce3e8;alternate-background-color:#0e151b;border:1px solid #27323b;outline:0;}"
        "QTableWidget::item{padding:3px 4px;border:0;}"
        "QHeaderView::section{background:#141c23;color:#ff9828;border:0;border-right:1px solid #29343e;border-bottom:2px solid #ff7a00;padding:4px 4px;font-weight:700;}"));
    for(int r=0;r<4;++r) table->setRowHeight(r,25);
    generationLayout->addWidget(table,1);
    mainArea->addWidget(generationCard,3);

    QFrame *contentCard=dbCard(database);
    QVBoxLayout *contentLayout=new QVBoxLayout(contentCard);
    contentLayout->setContentsMargins(10,7,10,8);
    contentLayout->setSpacing(3);
    contentLayout->addWidget(dbSection(7201,contentCard));
    for(int key:{7202,7203,7204,7205,7206}) contentLayout->addWidget(dbInfo(key,contentCard));
    contentLayout->addSpacing(4);
    contentLayout->addWidget(dbSection(7207,contentCard));
    for(const QPair<int,int> pair:{QPair<int,int>(7208,7209),QPair<int,int>(7210,7211),QPair<int,int>(7212,7213)}) {
        contentLayout->addWidget(dbInfo(pair.first,contentCard,true));
        contentLayout->addWidget(dbInfo(pair.second,contentCard));
    }
    contentLayout->addStretch(1);
    mainArea->addWidget(contentCard,2);

    root->addLayout(mainArea,1);

    QFrame *footer=dbCard(database);
    QHBoxLayout *footerLayout=new QHBoxLayout(footer);
    footerLayout->setContentsMargins(9,4,9,4);
    footerLayout->setSpacing(8);
    QLabel *draft=dbLabel(7214,footer);
    QFont df=draft->font(); df.setBold(true); df.setPointSizeF(7.6); draft->setFont(df);
    draft->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;border:0;"));
    QLabel *notice=dbLabel(7215,footer);
    QFont nf=notice->font(); nf.setPointSizeF(7.5); notice->setFont(nf);
    notice->setStyleSheet(QStringLiteral("color:#8f9aa4;background:transparent;border:0;"));
    footerLayout->addWidget(draft);
    footerLayout->addStretch(1);
    footerLayout->addWidget(notice);
    root->addWidget(footer);

    refreshDatabaseTranslations(database);
}

static void syncNavigation(QMainWindow *w,QTabWidget *tabs)
{
    if(!w || !tabs) return;
    QListWidget *nav=w->findChild<QListWidget*>(QStringLiteral("uiRebuildNav"));
    if(!nav) return;
    const QSignalBlocker blocker(nav);
    nav->clear();
    for(int i=0;i<tabs->count();++i)
        nav->addItem(tabs->tabText(i).trimmed());
    nav->setCurrentRow(qBound(0,tabs->currentIndex(),qMax(0,tabs->count()-1)));
}

static void prepareDatabaseAndInteractiveOrder(QMainWindow *w)
{
    if(!w) return;
    QTabWidget *tabs=w->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
    if(!tabs) return;

    QWidget *interactive=findInteractiveTab(tabs);
    if(!interactive) return;

    QWidget *database=findDatabaseTab(tabs);

    if(database && tabs->count()>=2 &&
       tabs->indexOf(database)==tabs->count()-2 &&
       tabs->indexOf(interactive)==tabs->count()-1) {
        buildDatabasePage(realPage(database));
        tabs->setTabText(tabs->indexOf(database),I18n::text(7152));
        syncNavigation(w,tabs);
        w->setProperty("databaseTabPrepared",true);
        return;
    }

    if(!database) {
        database=new QWidget(tabs);
        database->setObjectName(QStringLiteral("database_tab"));
        database->setMinimumSize(0,0);
        database->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        database->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    }
    buildDatabasePage(realPage(database));

    QWidget *current=tabs->currentWidget();
    const QString interactiveTitle=tabs->tabText(tabs->indexOf(interactive));

    const int databaseIndex=tabs->indexOf(database);
    if(databaseIndex>=0)
        tabs->removeTab(databaseIndex);

    const int interactiveIndex=tabs->indexOf(interactive);
    if(interactiveIndex>=0)
        tabs->removeTab(interactiveIndex);

    tabs->addTab(database,I18n::text(7152));
    tabs->addTab(interactive,interactiveTitle);

    if(current && tabs->indexOf(current)>=0)
        tabs->setCurrentWidget(current);

    syncNavigation(w,tabs);
    w->setProperty("databaseTabPrepared",true);
}

class TabOrderInstaller : public QObject
{
public:
    explicit TabOrderInstaller(QObject *parent=nullptr):QObject(parent){}
protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QMainWindow *w=qobject_cast<QMainWindow*>(watched);
        if(!w || w->objectName()!=QStringLiteral("MainWindow"))
            return QObject::eventFilter(watched,event);

        if((event->type()==QEvent::Show || event->type()==QEvent::Polish) &&
           !w->property("databaseTabScheduled").toBool()) {
            w->setProperty("databaseTabScheduled",true);
            QTimer::singleShot(1400,w,[w](){prepareDatabaseAndInteractiveOrder(w);});
            QTimer::singleShot(2400,w,[w](){prepareDatabaseAndInteractiveOrder(w);});
            QTimer::singleShot(3600,w,[w](){prepareDatabaseAndInteractiveOrder(w);});
        } else if(event->type()==QEvent::LanguageChange && w->property("databaseTabPrepared").toBool()) {
            QTimer::singleShot(0,w,[w](){prepareDatabaseAndInteractiveOrder(w);});
        }
        return QObject::eventFilter(watched,event);
    }
};

void installTabOrderPatch()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if(app) app->installEventFilter(new TabOrderInstaller(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installTabOrderPatch)
