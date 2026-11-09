#include <QAbstractItemView>
#include <QApplication>
#include <QBoxLayout>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QFontMetrics>
#include <QToolButton>
#include <QPixmap>
#include <QIcon>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QStyle>
#include <QTabBar>
#include <QTabWidget>
#include <QTableView>
#include <QTableWidget>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include "i18n.h"
#include "optionsdialog.h"

namespace {

static const char *kOrange = "#ff7a00";
static const char *kOrange2 = "#ff9828";
static const char *kBg = "#090e13";
static const char *kPanel = "#10161c";
static const char *kBorder = "#29343e";
static const char *kText = "#e7ecef";
static const char *kMuted = "#8d99a3";

static QIcon utilityIcon(const QString &kind)
{
    QPixmap pm(24,24);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing,true);
    p.setPen(QPen(QColor("#ff8a1c"),1.8,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);

    if(kind==QStringLiteral("record")) {
        p.setBrush(QColor("#ff8a1c"));
        p.drawEllipse(QPointF(12,12),5.2,5.2);
    } else if(kind==QStringLiteral("stop")) {
        p.setBrush(QColor("#ff8a1c"));
        p.drawRoundedRect(QRectF(7,7,10,10),1.5,1.5);
    } else if(kind==QStringLiteral("camera")) {
        p.drawRoundedRect(QRectF(4,7,16,11),2,2);
        p.drawEllipse(QPointF(12,12.5),3.3,3.3);
        p.drawLine(8,7,10,4.8); p.drawLine(10,4.8,14,4.8); p.drawLine(14,4.8,16,7);
    } else if(kind==QStringLiteral("gallery")) {
        p.drawRoundedRect(QRectF(4,5,16,14),2,2);
        p.drawEllipse(QPointF(15.7,9),1.5,1.5);
        QPolygonF q; q << QPointF(6,17) << QPointF(10.2,12.3) << QPointF(13,15) << QPointF(16,12.2) << QPointF(19,17);
        p.drawPolyline(q);
    } else if(kind==QStringLiteral("collapse")) {
        p.drawLine(5,4,5,20);
        p.drawLine(9,12,19,12);
        p.drawLine(9,12,14,7);
        p.drawLine(9,12,14,17);
    } else if(kind==QStringLiteral("expand")) {
        p.drawLine(19,4,19,20);
        p.drawLine(5,12,15,12);
        p.drawLine(15,12,10,7);
        p.drawLine(15,12,10,17);
    }
    return QIcon(pm);
}

static QPushButton *buttonByText(QMainWindow *w,const QString &text)
{
    if(!w) return nullptr;
    for(QPushButton *b:w->findChildren<QPushButton*>())
        if(b && b->text()==text) return b;
    return nullptr;
}

static void rememberButtonText(QPushButton *b)
{
    if(!b) return;
    if(!b->property("darkOriginalText").isValid()) b->setProperty("darkOriginalText",b->text());
}

static void styleSidebarButton(QPushButton *b,const QString &kind)
{
    if(!b) return;
    rememberButtonText(b);
    b->setIcon(utilityIcon(kind));
    b->setIconSize(QSize(18,18));
    b->setMinimumHeight(28);
    b->setMaximumHeight(30);
    b->setStyleSheet(QStringLiteral(
        "QPushButton{background:#111920;color:#e7edf0;border:1px solid #33414b;border-radius:5px;padding:3px 8px;text-align:left;font-weight:700;}"
        "QPushButton:hover{background:#1b252c;border-color:#ff7a00;color:#fff;}"
        "QPushButton:pressed{background:#090f14;}"
        "QPushButton:disabled{background:#11181e;color:#5f6c75;border-color:#27323b;}"));
}

static void applySidebarState(QMainWindow *w)
{
    QFrame *sidebar=w?w->findChild<QFrame*>(QStringLiteral("darkSidebar")):nullptr;
    QListWidget *nav=w?w->findChild<QListWidget*>(QStringLiteral("uiRebuildNav")):nullptr;
    if(!sidebar || !nav) return;

    const bool collapsed=sidebar->property("collapsed").toBool();
    QFrame *utils=sidebar->findChild<QFrame*>(QStringLiteral("darkSidebarUtilities"));
    QToolButton *toggle=sidebar->findChild<QToolButton*>(QStringLiteral("darkSidebarToggle"));

    if(toggle) {
        toggle->setIcon(utilityIcon(collapsed?QStringLiteral("expand"):QStringLiteral("collapse")));
        toggle->setIconSize(QSize(19,19));
        toggle->setToolButtonStyle(collapsed?Qt::ToolButtonIconOnly:Qt::ToolButtonTextBesideIcon);
        toggle->setText(collapsed?QString():I18n::text(7101));
        toggle->setToolTip(collapsed?I18n::text(7103):I18n::text(7102));
        toggle->setMinimumHeight(30); toggle->setMaximumHeight(32);
    }

    if(utils) {
        if(QLabel *title=utils->findChild<QLabel*>(QStringLiteral("darkUtilitiesTitle"))) title->setVisible(!collapsed);
        if(QLineEdit *file=w->findChild<QLineEdit*>(QStringLiteral("m_logFileNameBox"))) file->setVisible(!collapsed);
        for(QPushButton *b:utils->findChildren<QPushButton*>(QString(),Qt::FindDirectChildrenOnly)) {
            rememberButtonText(b);
            const QString original=b->property("darkOriginalText").toString();
            b->setText(collapsed?QString():original);
            if(collapsed) {
                b->setMinimumWidth(38); b->setMaximumWidth(38);
                b->setStyleSheet(b->styleSheet()+QStringLiteral("QPushButton{text-align:center;padding:3px;}"));
            } else {
                b->setMinimumWidth(0); b->setMaximumWidth(QWIDGETSIZE_MAX);
            }
        }
        utils->setMaximumHeight(collapsed?142:198);
    }

    QFontMetrics fm(nav->font());
    int longest=0;
    const int textRole=Qt::UserRole+42;
    const int sidebarHeight=sidebar->height()>300?sidebar->height():580;
    const int reserved=(toggle?toggle->height():31)+(utils?(collapsed?142:198):0)+20;
    const int navSpace=qMax(220,sidebarHeight-reserved);
    const int rowHeight=qBound(collapsed?30:25,navSpace/qMax(1,nav->count()),collapsed?40:35);

    for(int i=0;i<nav->count();++i) {
        QListWidgetItem *item=nav->item(i);
        if(!item) continue;
        if(!item->data(textRole).isValid()) item->setData(textRole,item->text());
        const QString original=item->data(textRole).toString();
        longest=qMax(longest,fm.horizontalAdvance(original));
        item->setText(collapsed?QString():original);
        item->setTextAlignment(collapsed?Qt::AlignCenter:(Qt::AlignLeft|Qt::AlignVCenter));
        item->setSizeHint(QSize(0,rowHeight));
    }

    const int expanded=qBound(198,longest+78,252);
    const int width=collapsed?58:expanded;
    sidebar->setMinimumWidth(width); sidebar->setMaximumWidth(width);
    nav->setMinimumWidth(width); nav->setMaximumWidth(width);
    nav->setIconSize(QSize(22,22));
    nav->setSpacing(0);
    nav->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    nav->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    nav->setStyleSheet(QStringLiteral(
        "#uiRebuildNav{background:#080e13;color:#cbd4da;border:0;padding:2px 0;}"
        "#uiRebuildNav::item{padding:3px %1px;border-left:3px solid transparent;border-bottom:1px solid #121a20;}"
        "#uiRebuildNav::item:hover{background:#151f26;color:#fff;}"
        "#uiRebuildNav::item:selected{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #332417,stop:1 #161d21);color:#ffad5c;border-left:3px solid #ff7a00;font-weight:700;}"
    ).arg(collapsed?12:11));
}

static void ensureSidebar(QMainWindow *w)
{
    if(!w) return;
    QWidget *workspace=w->findChild<QWidget*>(QStringLiteral("uiRebuildWorkspace"));
    QListWidget *nav=w->findChild<QListWidget*>(QStringLiteral("uiRebuildNav"));
    QHBoxLayout *workspaceLayout=workspace?qobject_cast<QHBoxLayout*>(workspace->layout()):nullptr;
    if(!workspace || !nav || !workspaceLayout) return;

    QFrame *sidebar=w->findChild<QFrame*>(QStringLiteral("darkSidebar"));
    QVBoxLayout *sideLayout=nullptr;
    if(!sidebar) {
        sidebar=new QFrame(workspace);
        sidebar->setObjectName(QStringLiteral("darkSidebar"));
        sidebar->setProperty("collapsed",false);
        sidebar->setAttribute(Qt::WA_StyledBackground,true);
        sidebar->setStyleSheet(QStringLiteral("#darkSidebar{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #070c11,stop:1 #0c141a);border:0;border-right:1px solid #2a3540;}"));
        sideLayout=new QVBoxLayout(sidebar);
        sideLayout->setContentsMargins(0,4,0,5);
        sideLayout->setSpacing(3);

        QToolButton *toggle=new QToolButton(sidebar);
        toggle->setObjectName(QStringLiteral("darkSidebarToggle"));
        toggle->setAutoRaise(false);
        toggle->setStyleSheet(QStringLiteral(
            "QToolButton{background:#101820;color:#e7edf0;border:0;border-left:3px solid transparent;padding:4px 11px;text-align:left;font-weight:700;}"
            "QToolButton:hover{background:#182229;color:#ffad5c;border-left:3px solid #ff7a00;}"));
        sideLayout->addWidget(toggle);

        workspaceLayout->removeWidget(nav);
        nav->setParent(sidebar);
        sideLayout->addWidget(nav,1);

        QFrame *utils=new QFrame(sidebar);
        utils->setObjectName(QStringLiteral("darkSidebarUtilities"));
        utils->setAttribute(Qt::WA_StyledBackground,true);
        utils->setStyleSheet(QStringLiteral("#darkSidebarUtilities{background:#0b1217;border-top:1px solid #27333c;}"));
        QVBoxLayout *uv=new QVBoxLayout(utils);
        uv->setContentsMargins(8,4,8,4);
        uv->setSpacing(4);
        QLabel *ut=new QLabel(I18n::text(7100),utils);
        ut->setObjectName(QStringLiteral("darkUtilitiesTitle"));
        QFont uf=ut->font(); uf.setBold(true); uf.setPointSizeF(qMax<qreal>(7.0,uf.pointSizeF()-1.0)); ut->setFont(uf);
        ut->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;border:0;padding:0 2px 1px 2px;"));
        uv->addWidget(ut);
        sideLayout->addWidget(utils,0);

        QObject::connect(toggle,&QToolButton::clicked,sidebar,[w,sidebar](){
            sidebar->setProperty("collapsed",!sidebar->property("collapsed").toBool());
            applySidebarState(w);
        });

        workspaceLayout->insertWidget(0,sidebar,0);
    } else {
        sideLayout=qobject_cast<QVBoxLayout*>(sidebar->layout());
    }

    QFrame *utils=sidebar->findChild<QFrame*>(QStringLiteral("darkSidebarUtilities"));
    QVBoxLayout *uv=utils?qobject_cast<QVBoxLayout*>(utils->layout()):nullptr;
    if(!utils || !uv) return;

    QLineEdit *file=w->findChild<QLineEdit*>(QStringLiteral("m_logFileNameBox"));
    QPushButton *start=w->findChild<QPushButton*>(QStringLiteral("m_startLoggingButton"));
    QPushButton *stop=w->findChild<QPushButton*>(QStringLiteral("m_stopLoggingButton"));
    QPushButton *snapshot=w->findChild<QPushButton*>(QStringLiteral("darkSnapshotButton"));
    QPushButton *view=w->findChild<QPushButton*>(QStringLiteral("darkViewCapturesButton"));
    if(!snapshot) snapshot=buttonByText(w,I18n::text(7015));
    if(!view) view=buttonByText(w,I18n::text(7016));
    if(snapshot) snapshot->setObjectName(QStringLiteral("darkSnapshotButton"));
    if(view) view->setObjectName(QStringLiteral("darkViewCapturesButton"));

    if(file) {
        file->setParent(utils);
        file->setMinimumHeight(27); file->setMaximumHeight(29);
        file->setMinimumWidth(0); file->setMaximumWidth(QWIDGETSIZE_MAX);
        file->setStyleSheet(QStringLiteral("QLineEdit{background:#071016;color:#e2e9ed;border:1px solid #34434e;border-radius:5px;padding:3px 7px;}QLineEdit:focus{border-color:#ff7a00;}"));
        if(uv->indexOf(file)<0) uv->addWidget(file);
    }
    if(start) { start->setParent(utils); styleSidebarButton(start,QStringLiteral("record")); if(uv->indexOf(start)<0) uv->addWidget(start); }
    if(stop) { stop->setParent(utils); styleSidebarButton(stop,QStringLiteral("stop")); if(uv->indexOf(stop)<0) uv->addWidget(stop); }
    if(snapshot) { snapshot->setParent(utils); styleSidebarButton(snapshot,QStringLiteral("camera")); if(uv->indexOf(snapshot)<0) uv->addWidget(snapshot); }
    if(view) { view->setParent(utils); styleSidebarButton(view,QStringLiteral("gallery")); if(uv->indexOf(view)<0) uv->addWidget(view); }

    if(QLabel *fileLabel=w->findChild<QLabel*>(QStringLiteral("m_logFileNameLabel"))) fileLabel->hide();
    applySidebarState(w);
}

static QWidget *realPage(QWidget *tab)
{
    if (!tab) return nullptr;
    if (QScrollArea *scroll = qobject_cast<QScrollArea*>(tab)) return scroll->widget();
    return tab;
}

static QString titleForIndex(QTabWidget *tabs, int i)
{
    return tabs ? tabs->tabText(i).trimmed() : QString();
}

static QString subtitleForIndex(int index)
{
    switch(index) {
    case 0: return I18n::text(7104);
    case 1: return I18n::text(7105);
    case 2: return I18n::text(7106);
    case 3: return I18n::text(7107);
    case 4: return I18n::text(7108);
    case 5: return I18n::text(7109);
    case 6: return I18n::text(7110);
    case 7: return I18n::text(7111);
    case 8: return I18n::text(7112);
    case 9: return I18n::text(7113);
    default: return QString();
    }
}

static QString globalStyle()
{
    return QStringLiteral(
        "QMainWindow,QWidget{background:%1;color:%2;}"
        "QLabel{color:%2;background:transparent;}"
        "QFrame{border-color:%3;}"
        "QGroupBox{background:%4;color:%2;border:1px solid %3;border-radius:4px;margin-top:13px;font-weight:700;}"
        "QGroupBox::title{subcontrol-origin:margin;left:12px;padding:0 6px;color:%5;}"
        "QPushButton,QToolButton{background:#151d25;color:#edf2f5;border:1px solid #35414c;border-radius:3px;padding:6px 11px;font-weight:700;}"
        "QPushButton:hover,QToolButton:hover{background:#1b252e;border-color:%6;color:white;}"
        "QPushButton:pressed,QToolButton:pressed{background:#0f151b;}"
        "QPushButton:checked,QToolButton:checked{background:%6;color:#101419;border-color:%5;}"
        "QPushButton:disabled,QToolButton:disabled{background:#182027;color:#63707a;border-color:#28323b;}"
        "QLineEdit,QTextEdit,QPlainTextEdit,QSpinBox,QDoubleSpinBox,QComboBox{background:#0a1117;color:#f0f4f6;border:1px solid #33404b;border-radius:3px;padding:5px 7px;selection-background-color:#a84f0a;}"
        "QLineEdit:focus,QTextEdit:focus,QPlainTextEdit:focus,QSpinBox:focus,QDoubleSpinBox:focus,QComboBox:focus{border-color:%6;}"
        "QTableWidget,QTableView,QTreeWidget,QListView{background:#0a1117;color:#dfe5e9;alternate-background-color:#10171e;border:1px solid %3;gridline-color:#222c34;outline:0;}"
        "QTableWidget::item,QTableView::item,QTreeWidget::item,QListView::item{padding:4px 6px;border:0;}"
        "QTableWidget::item:selected,QTableView::item:selected,QTreeWidget::item:selected,QListView::item:selected{background:#362514;color:white;}"
        "QHeaderView::section{background:#141b21;color:#dfe5e9;border:0;border-right:1px solid %3;border-bottom:1px solid %3;padding:5px 7px;font-weight:700;}"
        "QScrollArea{background:%1;border:0;}"
        "QScrollBar:vertical{background:#080d12;width:9px;margin:0;}"
        "QScrollBar::handle:vertical{background:#34414c;border-radius:3px;min-height:24px;}"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
        "QScrollBar:horizontal{background:#080d12;height:9px;margin:0;}"
        "QScrollBar::handle:horizontal{background:#34414c;border-radius:3px;min-width:24px;}"
        "QScrollBar::add-line:horizontal,QScrollBar::sub-line:horizontal{width:0;}"
        "QCheckBox,QRadioButton{color:#dfe5e9;background:transparent;spacing:6px;}"
        "QSlider::groove:horizontal{height:4px;background:#27323b;border-radius:2px;}"
        "QSlider::handle:horizontal{width:12px;margin:-4px 0;background:%6;border:1px solid %5;border-radius:6px;}"
        "QProgressBar{background:#0a1117;color:#dfe5e9;border:1px solid %3;border-radius:3px;text-align:center;}"
        "QProgressBar::chunk{background:%6;border-radius:2px;}"
    ).arg(QString::fromLatin1(kBg), QString::fromLatin1(kText), QString::fromLatin1(kBorder), QString::fromLatin1(kPanel),
          QString::fromLatin1(kOrange2), QString::fromLatin1(kOrange));
}

static QFrame *makeCard(QWidget *parent, const QString &objectName)
{
    QFrame *card = new QFrame(parent);
    card->setObjectName(objectName);
    card->setAttribute(Qt::WA_StyledBackground, true);
    card->setStyleSheet(QStringLiteral("#%1{background:%2;border:1px solid %3;border-radius:5px;}")
                        .arg(objectName, QString::fromLatin1(kPanel), QString::fromLatin1(kBorder)));
    return card;
}

static QFrame *makePageHeader(QWidget *parent, const QString &title, int index)
{
    QFrame *hero = makeCard(parent, QStringLiteral("uiRebuildHero_%1").arg(qAbs(qHash(title))));
    QVBoxLayout *v = new QVBoxLayout(hero);
    v->setContentsMargins(15,9,15,9);
    v->setSpacing(1);
    QLabel *t = new QLabel(title.toUpper(), hero);
    QFont tf=t->font(); tf.setBold(true); tf.setPointSizeF(10.5); t->setFont(tf);
    t->setStyleSheet(QStringLiteral("color:%1;background:transparent;border:0;").arg(QString::fromLatin1(kOrange2)));
    QLabel *s = new QLabel(subtitleForIndex(index), hero);
    QFont sf=s->font(); sf.setPointSizeF(7.6); sf.setLetterSpacing(QFont::AbsoluteSpacing,.45); s->setFont(sf);
    s->setStyleSheet(QStringLiteral("color:%1;background:transparent;border:0;").arg(QString::fromLatin1(kMuted)));
    v->addWidget(t); v->addWidget(s);
    return hero;
}

static QList<QWidget*> directLayoutBlocks(QWidget *page)
{
    QList<QWidget*> out;
    if (!page) return out;
    for (QWidget *w : page->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly)) {
        if (!w || w->objectName().startsWith(QStringLiteral("qt_"))) continue;
        if (w->layout() || qobject_cast<QGroupBox*>(w) || qobject_cast<QAbstractItemView*>(w) ||
            qobject_cast<QTextEdit*>(w) || qobject_cast<QPlainTextEdit*>(w)) out << w;
    }
    return out;
}

static void prepareViewWidgets(QWidget *page)
{
    for (QTableView *v : page->findChildren<QTableView*>()) {
        v->setAlternatingRowColors(true);
        if (v->horizontalHeader()) { v->horizontalHeader()->setStretchLastSection(true); v->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft|Qt::AlignVCenter); }
        if (v->verticalHeader()) v->verticalHeader()->setVisible(false);
    }
    for (QTableWidget *v : page->findChildren<QTableWidget*>()) {
        v->setAlternatingRowColors(true); v->setShowGrid(false);
        if (v->horizontalHeader()) { v->horizontalHeader()->setStretchLastSection(true); v->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft|Qt::AlignVCenter); }
        if (v->verticalHeader()) v->verticalHeader()->setVisible(false);
    }
}

static bool usesDedicatedLayout(QWidget *page,int index)
{
    if (!page) return true;
    const QString name=page->objectName();
    const QString cls=QString::fromLatin1(page->metaObject()->className());
    if (index>=0 && index<=9) return true;
    if (name==QStringLiteral("overview_tab") || name==QStringLiteral("emission_tab") ||
        name==QStringLiteral("errors") || name==QStringLiteral("actuators") ||
        name==QStringLiteral("raw") || name==QStringLiteral("ECU")) return true;
    return cls==QStringLiteral("SummaryTab") || cls==QStringLiteral("AnalysisTab") || cls==QStringLiteral("DiagnosticPanel");
}

static void composeGenericPage(QWidget *page, const QString &title, int index)
{
    if (!page || page->property("uiRebuiltPage").toBool()) return;
    page->setProperty("uiRebuiltPage", true);
    page->setMinimumSize(0,0);
    page->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    page->setAttribute(Qt::WA_StyledBackground,true);

    if (QBoxLayout *existing=qobject_cast<QBoxLayout*>(page->layout())) {
        existing->setContentsMargins(15,12,15,12);
        existing->setSpacing(qMax(9,existing->spacing()));
        existing->insertWidget(0,makePageHeader(page,title,index));
        prepareViewWidgets(page);
        return;
    }

    QList<QWidget*> blocks=directLayoutBlocks(page);
    QVBoxLayout *root=new QVBoxLayout(page);
    root->setContentsMargins(15,12,15,12);
    root->setSpacing(10);
    root->addWidget(makePageHeader(page,title,index));
    QGridLayout *grid=new QGridLayout;
    grid->setContentsMargins(0,0,0,0);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(10);
    grid->setColumnStretch(0,1);
    grid->setColumnStretch(1,1);
    int row=0,col=0;
    for (QWidget *block:blocks) {
        if (!block || block->parentWidget()!=page) continue;
        QFrame *card=makeCard(page,QStringLiteral("uiRebuildCard_%1_%2_%3").arg(qAbs(qHash(title))).arg(row).arg(col));
        QVBoxLayout *cardLayout=new QVBoxLayout(card);
        cardLayout->setContentsMargins(12,10,12,10);
        cardLayout->setSpacing(6);
        block->setParent(card);
        block->setMinimumSize(0,0);
        block->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        block->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        cardLayout->addWidget(block,1);
        grid->addWidget(card,row,col);
        if (++col>=2) { col=0; ++row; }
    }
    root->addLayout(grid,1);
    prepareViewWidgets(page);
}

static QFrame *buildHeader(QMainWindow *window, QWidget *legacy)
{
    if (!window || !legacy) return nullptr;
    QPushButton *connectButton=legacy->findChild<QPushButton*>(QStringLiteral("m_connectButton"));
    QPushButton *disconnectButton=legacy->findChild<QPushButton*>(QStringLiteral("m_disconnectButton"));
    QLabel *ecu=legacy->findChild<QLabel*>(QStringLiteral("m_ecuIdLabel"));
    QLabel *comm=legacy->findChild<QLabel*>(QStringLiteral("m_communicationsStatusLabel"));
    QWidget *good=legacy->findChild<QWidget*>(QStringLiteral("m_commsGoodLed"));
    QWidget *bad=legacy->findChild<QWidget*>(QStringLiteral("m_commsBadLed"));
    if (!connectButton || !disconnectButton || !ecu || !comm) return nullptr;
    QFrame *header=new QFrame(window->centralWidget());
    header->setObjectName(QStringLiteral("uiRebuildHeader")); header->setAttribute(Qt::WA_StyledBackground,true);
    header->setStyleSheet(QStringLiteral("#uiRebuildHeader{background:#080d12;border:0;border-bottom:1px solid %1;}#uiRebuildHeader QLabel{background:transparent;border:0;}#m_connectButton{background:%2;color:#11161a;border:1px solid %3;font-weight:800;}#m_disconnectButton{background:#b52323;color:white;border:1px solid #d83a3a;font-weight:800;}#m_connectButton:disabled,#m_disconnectButton:disabled{background:#171e24;color:#5f6972;border-color:#29323a;}").arg(QString::fromLatin1(kBorder),QString::fromLatin1(kOrange),QString::fromLatin1(kOrange2)));
    QHBoxLayout *h=new QHBoxLayout(header); h->setContentsMargins(12,5,10,5); h->setSpacing(8);
    QLabel *logo=new QLabel(header); logo->setObjectName(QStringLiteral("uiRebuildLogo")); QPixmap p(QStringLiteral(":/icons/key.png")); if(!p.isNull())logo->setPixmap(p.scaled(34,34,Qt::KeepAspectRatio,Qt::SmoothTransformation)); logo->setAlignment(Qt::AlignCenter); h->addWidget(logo);
    QVBoxLayout *brandV=new QVBoxLayout; brandV->setContentsMargins(0,0,0,0); brandV->setSpacing(0);
    QLabel *brand=new QLabel(QStringLiteral("ECU MEMS\nMANAGER SESSION"),header); brand->setObjectName(QStringLiteral("uiRebuildBrand")); QFont bf=brand->font(); bf.setBold(true); bf.setPointSizeF(9.0); brand->setFont(bf); brand->setStyleSheet(QStringLiteral("color:#f5f7f8;letter-spacing:.5px;"));
    QLabel *ver=new QLabel(QStringLiteral("v%1.%2.%3").arg(VER_MAJOR).arg(VER_MINOR).arg(VER_PATCH),header); ver->setStyleSheet(QStringLiteral("color:%1;font-weight:700;").arg(QString::fromLatin1(kOrange2))); brandV->addWidget(brand); brandV->addWidget(ver); h->addLayout(brandV);
    QFrame *sep=new QFrame(header); sep->setFrameShape(QFrame::VLine); sep->setStyleSheet(QStringLiteral("background:%1;max-width:1px;").arg(QString::fromLatin1(kBorder))); h->addWidget(sep);
    ecu->setParent(header); ecu->setStyleSheet(QStringLiteral("color:#dfe4e8;font-weight:700;padding:0 8px;")); h->addWidget(ecu);
    h->addStretch(1);
    QWidget *commBox=new QWidget(header); QHBoxLayout *ch=new QHBoxLayout(commBox); ch->setContentsMargins(0,0,0,0); ch->setSpacing(5); comm->setParent(commBox); comm->setStyleSheet(QStringLiteral("color:#cbd3d9;font-weight:700;")); ch->addWidget(comm); if(good){good->setParent(commBox);ch->addWidget(good);} if(bad){bad->setParent(commBox);ch->addWidget(bad);} h->addWidget(commBox);
    connectButton->setParent(header); disconnectButton->setParent(header); h->addWidget(connectButton); h->addWidget(disconnectButton);
    legacy->hide(); legacy->setMaximumSize(0,0); return header;
}

static QFrame *buildBottomStatus(QMainWindow *window)
{
    QFrame *bar=new QFrame(window->centralWidget());
    bar->setObjectName(QStringLiteral("uiRebuildStatus"));
    bar->setAttribute(Qt::WA_StyledBackground,true);
    bar->setStyleSheet(QStringLiteral("#uiRebuildStatus{background:#080d12;border-top:1px solid %1;}#uiRebuildStatus QLabel{background:transparent;border-right:1px solid %1;color:#c9d1d7;padding:0 10px;}").arg(QString::fromLatin1(kBorder)));

    QHBoxLayout *h=new QHBoxLayout(bar);
    h->setContentsMargins(0,0,0,0);
    h->setSpacing(0);

    QString fileName=QStringLiteral("--");
    if(QLineEdit *f=window->findChild<QLineEdit*>(QStringLiteral("m_logFileNameBox"))){
        const QString current=f->text().trimmed();
        if(!current.isEmpty())fileName=current;
    }

    QSettings settings(QSettings::IniFormat,QSettings::UserScope,PROJECTNAME);
    settings.beginGroup(QStringLiteral("Settings"));
    QString portName=settings.value(QStringLiteral("SerialDevice"),QString()).toString().trimmed();
    settings.endGroup();
    if(portName.isEmpty())portName=QStringLiteral("--");

    QLabel *file=new QLabel(I18n::text(7115).arg(fileName),bar);
    file->setObjectName(QStringLiteral("statusFile"));
    QLabel *mode=new QLabel(I18n::text(7909).arg(I18n::text(7912)),bar);
    mode->setObjectName(QStringLiteral("statusReadMode"));
    QLabel *port=new QLabel(QStringLiteral("%1 : %2").arg(I18n::text(7114),portName),bar);
    port->setObjectName(QStringLiteral("statusPort"));
    QLabel *firmware=new QLabel(I18n::text(7916).arg(QStringLiteral("--")),bar);
    firmware->setObjectName(QStringLiteral("statusFirmware"));

    for(QLabel *label:{file,mode,port,firmware}){
        label->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
        label->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    }
    mode->setStyleSheet(QStringLiteral("color:#8d99a3;background:transparent;border-right:1px solid %1;padding:0 10px;font-weight:700;").arg(QString::fromLatin1(kBorder)));

    h->addWidget(file,3);
    h->addWidget(mode,2);
    h->addWidget(port,1);
    h->addWidget(firmware,1);
    return bar;
}

static qreal responsiveScale(QMainWindow *window)
{
    if(!window) return 1.0;
    QWidget *workspace=window->findChild<QWidget*>(QStringLiteral("uiRebuildWorkspace"));
    const QSize sz=(workspace&&workspace->width()>0&&workspace->height()>0)?workspace->size():(window->centralWidget()?window->centralWidget()->size():window->size());
    return qBound<qreal>(0.64,qMin(sz.width()/1450.0,sz.height()/720.0),1.28);
}

static void applyResponsive(QMainWindow *window)
{
    if(!window) return;
    const qreal s=responsiveScale(window);
    window->setProperty("globalUiScale",s);

    if(QFrame *header=window->findChild<QFrame*>(QStringLiteral("uiRebuildHeader")))
        header->setFixedHeight(qBound(42,qRound(54.0*s),64));
    if(QLabel *logo=window->findChild<QLabel*>(QStringLiteral("uiRebuildLogo"))) {
        const int side=qBound(28,qRound(38.0*s),46);
        logo->setFixedSize(side,side);
        QPixmap p(QStringLiteral(":/icons/key.png"));
        if(!p.isNull())logo->setPixmap(p.scaled(side-4,side-4,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    }
    if(QLabel *brand=window->findChild<QLabel*>(QStringLiteral("uiRebuildBrand"))) {
        QFont f=brand->font(); f.setPointSizeF(qBound<qreal>(8.0,9.5*s,9.5)); brand->setFont(f);
    }
    if(QPushButton *b=window->findChild<QPushButton*>(QStringLiteral("m_connectButton"))) {
        b->setMinimumWidth(qBound(76,qRound(92.0*s),112));
        b->setMinimumHeight(qBound(24,qRound(28.0*s),34));
    }
    if(QPushButton *b=window->findChild<QPushButton*>(QStringLiteral("m_disconnectButton"))) {
        b->setMinimumWidth(qBound(84,qRound(102.0*s),124));
        b->setMinimumHeight(qBound(24,qRound(28.0*s),34));
    }
    if(QWidget *good=window->findChild<QWidget*>(QStringLiteral("m_commsGoodLed"))) {
        const int side=qBound(15,qRound(20.0*s),24); good->setFixedSize(side,side);
    }
    if(QWidget *bad=window->findChild<QWidget*>(QStringLiteral("m_commsBadLed"))) {
        const int side=qBound(15,qRound(20.0*s),24); bad->setFixedSize(side,side);
    }

    if(QListWidget *nav=window->findChild<QListWidget*>(QStringLiteral("uiRebuildNav"))) {
        const QSize a=window->findChild<QWidget*>(QStringLiteral("uiRebuildWorkspace"))?window->findChild<QWidget*>(QStringLiteral("uiRebuildWorkspace"))->size():(window->centralWidget()?window->centralWidget()->size():window->size());
        const int navW=qBound(142,qRound(a.width()*0.12),214);
        const int rowH=qBound(27,qRound(34.0*s),40);
        const int pad=qBound(8,qRound(13.0*s),16);
        nav->setFixedWidth(navW);
        nav->setStyleSheet(QStringLiteral("#uiRebuildNav{background:#0d1318;color:#c7d0d6;border:0;border-right:1px solid %1;padding:5px 0;}#uiRebuildNav::item{min-height:%2px;padding:2px %3px;border-left:3px solid transparent;font-weight:650;}#uiRebuildNav::item:hover{background:#161d23;color:white;}#uiRebuildNav::item:selected{background:#1c211f;color:%4;border-left:3px solid %5;}QToolTip{color:#ffffff;}")
            .arg(QString::fromLatin1(kBorder)).arg(rowH).arg(pad).arg(QString::fromLatin1(kOrange2)).arg(QString::fromLatin1(kOrange)));
    }

    if(QFrame *status=window->findChild<QFrame*>(QStringLiteral("uiRebuildStatus")))
        status->setFixedHeight(qBound(29,qRound(35.0*s),40));

    const QList<QScrollArea*> scrolls=window->findChildren<QScrollArea*>();
    for(QScrollArea *scroll:scrolls){
        scroll->setWidgetResizable(true);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        if(scroll->widget()){
            scroll->widget()->setMinimumWidth(0);
            scroll->widget()->setMaximumWidth(QWIDGETSIZE_MAX);
        }
    }
    if(window->findChild<QFrame*>(QStringLiteral("darkSidebar")))
        applySidebarState(window);
}

class UiRebuildInstaller : public QObject
{
public: explicit UiRebuildInstaller(QObject *parent=nullptr):QObject(parent){}
protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QMainWindow *window=qobject_cast<QMainWindow*>(watched);
        if(!window||window->objectName()!=QStringLiteral("MainWindow"))return QObject::eventFilter(watched,event);

        if(event->type()==QEvent::Resize && window->property("uiRebuildInstalled").toBool()) {
            applyResponsive(window);
            return QObject::eventFilter(watched,event);
        }

        if((event->type()!=QEvent::Show&&event->type()!=QEvent::Polish)||window->property("uiRebuildInstalled").toBool())return QObject::eventFilter(watched,event);
        QTabWidget *tabs=window->findChild<QTabWidget*>(QStringLiteral("Tab_main")); QWidget *central=window->centralWidget(); QVBoxLayout *root=central?qobject_cast<QVBoxLayout*>(central->layout()):nullptr; QWidget *legacy=central?central->findChild<QWidget*>(QStringLiteral("layoutWidget_7")):nullptr;
        if(!tabs||!root||!legacy)return QObject::eventFilter(watched,event);
        window->setProperty("uiRebuildInstalled",true); QTimer::singleShot(0,window,[=](){rebuild(window,central,root,tabs,legacy);}); return QObject::eventFilter(watched,event);
    }
private:
    // Typography only. The 14 original BUILD117 page layouts remain intact.
    // All-data page at rank 10 is the validated reference and is untouched.
    static void applyReadablePageFonts(QTabWidget *tabs)
    {
        if (!tabs) return;
        constexpr qreal kMinimumPoints = 9.8;
        for (int i=0; i<tabs->count(); ++i) {
            if (i==10) continue;
            QWidget *page=realPage(tabs->widget(i));
            if (!page) continue;
            for (QWidget *widget:page->findChildren<QWidget*>()) {
                if (!widget) continue;
                if (!(qobject_cast<QLabel*>(widget) ||
                      qobject_cast<QAbstractItemView*>(widget) ||
                      widget->inherits("QAbstractButton") ||
                      widget->inherits("QLineEdit") ||
                      widget->inherits("QComboBox") ||
                      widget->inherits("QGroupBox") ||
                      widget->inherits("QTextEdit") ||
                      widget->inherits("QPlainTextEdit"))) continue;
                QFont font=widget->font();
                const qreal pt=font.pointSizeF();
                if (pt>0 && pt<kMinimumPoints) {
                    font.setPointSizeF(kMinimumPoints);
                    widget->setFont(font);
                }
            }
        }
    }

    static void rebuild(QMainWindow *window,QWidget *central,QVBoxLayout *root,QTabWidget *tabs,QWidget *legacy)
    {
        qApp->setStyleSheet(globalStyle()); central->setAttribute(Qt::WA_StyledBackground,true); central->setStyleSheet(QStringLiteral("background:%1;").arg(QString::fromLatin1(kBg)));
        if(QFrame *header=buildHeader(window,legacy))root->insertWidget(0,header);
        const int oldIndex=root->indexOf(tabs);if(oldIndex>=0)root->removeWidget(tabs);
        QFrame *workspace=new QFrame(central);workspace->setObjectName(QStringLiteral("uiRebuildWorkspace"));workspace->setAttribute(Qt::WA_StyledBackground,true);workspace->setStyleSheet(QStringLiteral("#uiRebuildWorkspace{background:%1;border:0;}").arg(QString::fromLatin1(kBg)));QHBoxLayout *wh=new QHBoxLayout(workspace);wh->setContentsMargins(0,0,0,0);wh->setSpacing(0);
        QListWidget *nav=new QListWidget(workspace);nav->setObjectName(QStringLiteral("uiRebuildNav"));nav->setFocusPolicy(Qt::NoFocus);nav->setUniformItemSizes(true);nav->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        for(int i=0;i<tabs->count();++i)nav->addItem(titleForIndex(tabs,i));
        nav->setCurrentRow(tabs->currentIndex());
        tabs->tabBar()->hide();
        tabs->setStyleSheet(QStringLiteral("QTabWidget::pane{border:0;background:%1;}").arg(QString::fromLatin1(kBg)));
        wh->addWidget(nav);wh->addWidget(tabs,1);root->insertWidget(oldIndex<0?1:oldIndex,workspace,1);
        QObject::connect(nav,&QListWidget::currentRowChanged,tabs,&QTabWidget::setCurrentIndex);
        QObject::connect(tabs,&QTabWidget::currentChanged,nav,[nav](int row){ nav->setCurrentRow(row); });
        for(int i=0;i<tabs->count();++i){QWidget *page=realPage(tabs->widget(i));if(!page)continue;if(QScrollArea *scroll=qobject_cast<QScrollArea*>(tabs->widget(i))){scroll->setWidgetResizable(true);scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);page->setMinimumSize(0,0);}const QString title=titleForIndex(tabs,i);if(!usesDedicatedLayout(page,i))composeGenericPage(page,title,i);}
        root->addWidget(buildBottomStatus(window));tabs->setCurrentIndex(0);
        applyResponsive(window);
        // The sidebar and its utility actions belong to the native window
        // shell instead of a delayed global patch.
        ensureSidebar(window);
        QTimer::singleShot(120,window,[window](){applyResponsive(window);});
        // Once the original settings and page installers finish, improve
        // text only: no position, size, card, dial or table changes.
        QTimer::singleShot(2800,window,[tabs](){applyReadablePageFonts(tabs);});
    }
};

void installUiRebuild(){QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());if(!app)return;UiRebuildInstaller *installer=new UiRebuildInstaller(app);app->installEventFilter(installer);} 

}

Q_COREAPP_STARTUP_FUNCTION(installUiRebuild)
