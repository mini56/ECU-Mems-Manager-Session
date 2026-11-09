#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFont>
#include <QFrame>
#include <QtMath>
#include <QTableView>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include "i18n.h"

namespace {

static QWidget *realPage(QWidget *tab)
{
    if (!tab) return nullptr;
    if (QScrollArea *s=qobject_cast<QScrollArea*>(tab)) return s->widget();
    return tab;
}

static QWidget *pageAt(QTabWidget *tabs,int index)
{
    if (!tabs || index<0 || index>=tabs->count()) return nullptr;
    return realPage(tabs->widget(index));
}

static void clearLayout(QLayout *layout)
{
    if (!layout) return;
    while (QLayoutItem *it=layout->takeAt(0)) {
        if (it->layout()) clearLayout(it->layout());
        delete it;
    }
}

static void replaceRoot(QWidget *page,QVBoxLayout *&root)
{
    if (!page) return;
    if (QLayout *old=page->layout()) {
        clearLayout(old);
        delete old;
    }
    root=new QVBoxLayout(page);
    root->setContentsMargins(8,7,8,7);
    root->setSpacing(7);
    root->setSizeConstraint(QLayout::SetDefaultConstraint);
    page->setMinimumSize(0,0);
    page->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    page->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
}

static QFrame *hero(QWidget *parent,const QString &title,const QString &subtitle)
{
    QFrame *f=new QFrame(parent);
    f->setObjectName(QStringLiteral("strictHero_%1").arg(qAbs(qHash(title))));
    f->setAttribute(Qt::WA_StyledBackground,true);
    f->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    f->setStyleSheet(QStringLiteral("QFrame{background:#10161c;border:1px solid #29343e;border-radius:5px;}"));
    QVBoxLayout *v=new QVBoxLayout(f);
    v->setContentsMargins(13,6,13,6);
    v->setSpacing(0);
    QLabel *t=new QLabel(title.toUpper(),f);
    QFont tf=t->font(); tf.setBold(true); tf.setPointSizeF(9.2); t->setFont(tf);
    t->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;border:0;"));
    QLabel *s=new QLabel(subtitle,f);
    QFont sf=s->font(); sf.setPointSizeF(6.9); s->setFont(sf);
    s->setStyleSheet(QStringLiteral("color:#8d99a3;background:transparent;border:0;"));
    v->addWidget(t); v->addWidget(s);
    f->setMaximumHeight(46);
    return f;
}

static QFrame *card(QWidget *parent,const QString &title)
{
    QFrame *f=new QFrame(parent);
    f->setAttribute(Qt::WA_StyledBackground,true);
    f->setMinimumSize(0,0);
    f->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    f->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    f->setStyleSheet(QStringLiteral("QFrame{background:#10161c;border:1px solid #29343e;border-radius:5px;}"));
    QVBoxLayout *v=new QVBoxLayout(f);
    v->setContentsMargins(10,8,10,10);
    v->setSpacing(6);
    QLabel *t=new QLabel(title,f);
    QFont tf=t->font(); tf.setBold(true); tf.setPointSizeF(8.2); t->setFont(tf);
    t->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;border:0;"));
    v->addWidget(t);
    QFrame *line=new QFrame(f);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet(QStringLiteral("background:#29343e;border:0;max-height:1px;"));
    v->addWidget(line);
    return f;
}

static void cleanupOldShell(QWidget *page)
{
    if (!page) return;
    for (QFrame *f:page->findChildren<QFrame*>()) {
        const QString n=f->objectName();
        if (n.startsWith(QStringLiteral("uiRebuildHero_")) || n.startsWith(QStringLiteral("uiRebuildCard_"))) f->hide();
    }
}

static void normalizeWidget(QWidget *w)
{
    if (!w) return;
    w->setMinimumSize(0,0);
    w->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    w->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    w->show();
}

static void polishTable(QTableWidget *t)
{
    if (!t) return;
    normalizeWidget(t);
    t->setAlternatingRowColors(true);
    t->setShowGrid(false);
    if (t->verticalHeader()) t->verticalHeader()->setVisible(false);
    if (t->horizontalHeader()) {
        t->horizontalHeader()->setStretchLastSection(true);
        t->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    }
}

static void rebuildSummary(QTabWidget *tabs)
{
    const int index=2;
    QWidget *page=pageAt(tabs,index);
    if (!page || page->property("strictSummaryBuilt").toBool()) return;
    QList<QTableWidget*> tables=page->findChildren<QTableWidget*>();
    if (tables.size()<3) return;
    page->setProperty("strictSummaryBuilt",true);
    cleanupOldShell(page);
    for (QTableWidget *t:tables) { t->setParent(page); polishTable(t); }
    QVBoxLayout *root=nullptr;
    replaceRoot(page,root);
    root->addWidget(hero(page,tabs->tabText(index),I18n::text(7106)));
    QHBoxLayout *body=new QHBoxLayout;
    body->setSpacing(6);
    for (int i=0;i<3;i++) body->addWidget(tables.at(i),1);
    root->addLayout(body,1);
}

static void styleRawBlock(QWidget *block)
{
    if (!block) return;
    normalizeWidget(block);
    for (QLabel *l:block->findChildren<QLabel*>()) {
        const QString n=l->objectName();
        const bool head=n.startsWith(QStringLiteral("header_")) || n.startsWith(QStringLiteral("Aheader_"));
        l->setStyleSheet(head
            ? QStringLiteral("color:#ff9828;background:transparent;font-weight:700;")
            : QStringLiteral("color:#ffffff;background:transparent;"));
    }
}

static void rebuildRaw(QTabWidget *tabs)
{
    const int index=5;
    QWidget *page=pageAt(tabs,index);
    if (!page || page->property("strictRawBuilt").toBool()) return;
    QWidget *a=page->findChild<QWidget*>(QStringLiteral("raw_1"));
    QWidget *b=page->findChild<QWidget*>(QStringLiteral("raw_2"));
    if (!a || !b) return;
    page->setProperty("strictRawBuilt",true);
    cleanupOldShell(page);
    a->setParent(page); b->setParent(page);
    styleRawBlock(a); styleRawBlock(b);
    QVBoxLayout *root=nullptr;
    replaceRoot(page,root);
    root->addWidget(hero(page,tabs->tabText(index),I18n::text(7109)));
    QHBoxLayout *body=new QHBoxLayout;
    body->setSpacing(7);
    QFrame *ca=card(page,I18n::text(7124));
    static_cast<QVBoxLayout*>(ca->layout())->addWidget(a,1);
    QFrame *cb=card(page,I18n::text(7125));
    static_cast<QVBoxLayout*>(cb->layout())->addWidget(b,1);
    body->addWidget(ca,1); body->addWidget(cb,1);
    root->addLayout(body,1);
}

static void moveLabel(QLabel *l,QWidget *parent,QVBoxLayout *layout,bool warning=false)
{
    if (!l || !parent || !layout) return;
    l->setParent(parent);
    l->setWordWrap(true);
    l->setMinimumHeight(0);
    l->setMaximumHeight(QWIDGETSIZE_MAX);
    l->setStyleSheet(warning
        ? QStringLiteral("color:#ff6559;background:#160d0d;border:1px solid #5d2c2c;border-radius:3px;padding:7px;font-weight:700;")
        : QStringLiteral("color:#dce3e8;background:transparent;border:0;"));
    l->show();
    layout->addWidget(l);
}

static void rebuildInteractive(QTabWidget *tabs)
{
    const int index=6;
    QWidget *page=pageAt(tabs,index);
    if (!page || page->property("strictInteractiveBuilt").toBool()) return;
    page->setProperty("strictInteractiveBuilt",true);
    cleanupOldShell(page);
    QLabel *l23=page->findChild<QLabel*>(QStringLiteral("label_23"));
    QLabel *l24=page->findChild<QLabel*>(QStringLiteral("label_24"));
    QLabel *l48=page->findChild<QLabel*>(QStringLiteral("label_48"));
    QLabel *l49=page->findChild<QLabel*>(QStringLiteral("label_49"));
    QLabel *l50=page->findChild<QLabel*>(QStringLiteral("label_50"));
    QLabel *l51=page->findChild<QLabel*>(QStringLiteral("label_51"));
    QLabel *l22=page->findChild<QLabel*>(QStringLiteral("label_22"));
    QPushButton *button=page->findChild<QPushButton*>(QStringLiteral("interactive_push_button"));
    QLineEdit *info=page->findChild<QLineEdit*>(QStringLiteral("lineEdit_2"));
    if (QLabel *empty=page->findChild<QLabel*>(QStringLiteral("label_9"))) empty->hide();

    QVBoxLayout *root=nullptr;
    replaceRoot(page,root);
    root->addWidget(hero(page,tabs->tabText(index),I18n::text(7110)));
    QHBoxLayout *body=new QHBoxLayout;
    body->setSpacing(7);
    QFrame *instructions=card(page,I18n::text(7126));
    QVBoxLayout *iv=static_cast<QVBoxLayout*>(instructions->layout());
    moveLabel(l23,instructions,iv); moveLabel(l24,instructions,iv); moveLabel(l48,instructions,iv); moveLabel(l49,instructions,iv); moveLabel(l50,instructions,iv);
    if (button) {
        button->setParent(instructions);
        button->setMinimumHeight(30);
        button->setMaximumWidth(220);
        button->show();
        iv->addWidget(button,0,Qt::AlignLeft);
    }
    iv->addStretch(1);
    QFrame *safety=card(page,I18n::text(7127));
    QVBoxLayout *sv=static_cast<QVBoxLayout*>(safety->layout());
    moveLabel(l51,safety,sv,true); moveLabel(l22,safety,sv,true);
    if (info) {
        info->setParent(safety);
        info->setReadOnly(true);
        info->setMinimumHeight(30);
        info->show();
        sv->addWidget(info);
    }
    sv->addStretch(1);
    body->addWidget(instructions,3); body->addWidget(safety,2);
    root->addLayout(body,1);
}

static void rebuildRosco(QTabWidget *tabs)
{
    const int index=8;
    QWidget *page=pageAt(tabs,index);
    if (!page || page->property("strictRoscoBuilt").toBool()) return;
    QList<QGroupBox*> groups=page->findChildren<QGroupBox*>();
    QList<QTextEdit*> edits=page->findChildren<QTextEdit*>();
    if (groups.isEmpty() || edits.isEmpty()) return;
    page->setProperty("strictRoscoBuilt",true);
    cleanupOldShell(page);
    QGroupBox *session=groups.first();
    QTextEdit *output=edits.first();
    session->setParent(page); output->setParent(page);
    normalizeWidget(session); normalizeWidget(output);
    QVBoxLayout *root=nullptr;
    replaceRoot(page,root);
    root->addWidget(hero(page,tabs->tabText(index),I18n::text(7112)));
    QHBoxLayout *body=new QHBoxLayout;
    body->setSpacing(7);
    QFrame *left=card(page,I18n::text(7128));
    static_cast<QVBoxLayout*>(left->layout())->addWidget(session,1);
    QFrame *right=card(page,I18n::text(7129));
    static_cast<QVBoxLayout*>(right->layout())->addWidget(output,1);
    body->addWidget(left,2); body->addWidget(right,3);
    root->addLayout(body,1);
}

static void rebuildDiagnostic(QTabWidget *tabs)
{
    const int index=9;
    QWidget *page=pageAt(tabs,index);
    if (!page || page->property("strictDiagnosticBuilt").toBool()) return;
    QList<QTableWidget*> tables=page->findChildren<QTableWidget*>();
    QList<QGroupBox*> groups=page->findChildren<QGroupBox*>();
    if (tables.isEmpty() || groups.isEmpty()) return;
    QTableWidget *table=tables.first();
    QGroupBox *report=groups.last();
    QList<QPushButton*> buttons=page->findChildren<QPushButton*>();
    QList<QLabel*> labels=page->findChildren<QLabel*>();
    page->setProperty("strictDiagnosticBuilt",true);
    cleanupOldShell(page);
    table->setParent(page); report->setParent(page);
    polishTable(table); normalizeWidget(report);

    QVBoxLayout *root=nullptr;
    replaceRoot(page,root);
    root->addWidget(hero(page,tabs->tabText(index),I18n::text(7113)));
    QFrame *toolbar=card(page,I18n::text(7130));
    QHBoxLayout *th=new QHBoxLayout;
    th->setSpacing(7);
    for (QLabel *l:labels) {
        if (!l || l->parentWidget()==report || report->isAncestorOf(l) || l->objectName().startsWith(QStringLiteral("strict"))) continue;
        l->setParent(toolbar); l->show(); th->addWidget(l);
    }
    th->addStretch(1);
    for (QPushButton *b:buttons) {
        if (!b || report->isAncestorOf(b)) continue;
        b->setParent(toolbar); b->setMinimumHeight(28); b->show(); th->addWidget(b);
    }
    static_cast<QVBoxLayout*>(toolbar->layout())->addLayout(th);
    root->addWidget(toolbar,0);

    QHBoxLayout *body=new QHBoxLayout;
    body->setSpacing(7);
    QFrame *checks=card(page,I18n::text(7131));
    static_cast<QVBoxLayout*>(checks->layout())->addWidget(table,1);
    QFrame *rep=card(page,I18n::text(7132));
    static_cast<QVBoxLayout*>(rep->layout())->addWidget(report,1);
    body->addWidget(checks,3); body->addWidget(rep,2);
    root->addLayout(body,1);
}

static void fitRawDataPage(QWidget *page)
{
    if (!page) return;
    QWidget *left=page->findChild<QWidget*>(QStringLiteral("raw_1"));
    QWidget *right=page->findChild<QWidget*>(QStringLiteral("raw_2"));
    if (!left || !right) return;

    page->setMinimumSize(0,0);
    page->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);

    QWidget *viewport=page->parentWidget();
    int visibleHeight=(viewport && viewport->height()>0)?viewport->height():page->height();
    visibleHeight=qMax(360,visibleHeight);

    auto gridFor=[](QWidget *block)->QGridLayout* {
        return block?qobject_cast<QGridLayout*>(block->layout()):nullptr;
    };
    QGridLayout *leftGrid=gridFor(left);
    QGridLayout *rightGrid=gridFor(right);
    const int rows=qMax(leftGrid?leftGrid->rowCount():1,rightGrid?rightGrid->rowCount():1);

    for (QWidget *block:{left,right}) {
        block->setMinimumSize(0,0);
        block->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        block->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        QWidget *parent=block->parentWidget();
        QLayout *parentLayout=parent?parent->layout():nullptr;
        if (!parentLayout || parentLayout->indexOf(block)<0) {
            const int top=qMax(0,block->y());
            block->resize(block->width(),qMax(300,visibleHeight-top-6));
        }
    }

    const int usableHeight=qMax(300,visibleHeight-14);
    const int rowHeight=qMax(10,qMin(21,usableHeight/qMax(1,rows)));
    const qreal fontSize=rowHeight<=10?6.7:(rowHeight<=11?6.9:(rowHeight<=12?7.1:(rowHeight<=13?7.4:(rowHeight<=14?7.6:(rowHeight<=15?7.8:(rowHeight<=16?8.0:8.2))))));

    auto fitBlock=[&](QWidget *block,QGridLayout *grid) {
        if (!block) return;
        if (grid) {
            grid->setContentsMargins(0,0,0,0);
            grid->setHorizontalSpacing(8);
            grid->setVerticalSpacing(0);
            grid->setSizeConstraint(QLayout::SetDefaultConstraint);
            for (int r=0;r<grid->rowCount();++r) grid->setRowMinimumHeight(r,rowHeight);
        }
        for (QLabel *label:block->findChildren<QLabel*>()) {
            const QString name=label->objectName();
            const bool header=name.startsWith(QStringLiteral("header_")) || name.startsWith(QStringLiteral("Aheader_"));
            QFont f=label->font();
            f.setPointSizeF(fontSize);
            f.setBold(header);
            label->setFont(f);
            label->setWordWrap(false);
            label->setMinimumHeight(rowHeight);
            label->setMaximumHeight(rowHeight);
            label->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
            label->setStyleSheet(header
                ? QStringLiteral("color:#ff9828;background:transparent;border:0;font-weight:700;")
                : QStringLiteral("color:#ffffff;background:transparent;border:0;"));
        }
    };

    fitBlock(left,leftGrid);
    fitBlock(right,rightGrid);
}

static void fitBuiltPages(QMainWindow *w)
{
    QTabWidget *tabs=w?w->findChild<QTabWidget*>(QStringLiteral("Tab_main")):nullptr;
    if (!tabs) return;
    const qreal s=w->property("globalUiScale").isValid()
        ? qBound<qreal>(0.64,w->property("globalUiScale").toDouble(),1.30)
        : 1.0;
    const int margin=qBound(3,qRound(7*s),10),gap=qBound(3,qRound(6*s),8);
    const int row=qBound(19,qRound(24*s),28);
    for(int i=0;i<tabs->count();++i) {
        QWidget *page=realPage(tabs->widget(i));
        if(!page) continue;
        fitRawDataPage(page);
        const bool built=page->property("strictSummaryBuilt").toBool() ||
            page->property("strictRawBuilt").toBool() ||
            page->property("strictInteractiveBuilt").toBool() ||
            page->property("strictRoscoBuilt").toBool() ||
            page->property("strictDiagnosticBuilt").toBool();
        if(!built) continue;
        page->setMinimumSize(0,0);
        page->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        if(QLayout *l=page->layout()) {
            l->setContentsMargins(margin,margin,margin,margin);
            l->setSpacing(gap);
        }
        for(QFrame *card:page->findChildren<QFrame*>())
            if(card->objectName().startsWith(QStringLiteral("strictHero_")))
                card->setMaximumHeight(qBound(36,qRound(44*s),48));
        for(QTableView *table:page->findChildren<QTableView*>()) {
            table->setMinimumSize(0,0);
            table->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
            if(table->verticalHeader()) table->verticalHeader()->setDefaultSectionSize(row);
            if(table->horizontalHeader()) table->horizontalHeader()->setStretchLastSection(true);
        }
    }
}

static void rebuildAll(QMainWindow *w)
{
    QTabWidget *tabs=w?w->findChild<QTabWidget*>(QStringLiteral("Tab_main")):nullptr;
    if (!tabs) return;
    rebuildSummary(tabs);
    rebuildRaw(tabs);
    rebuildInteractive(tabs);
    rebuildRosco(tabs);
    rebuildDiagnostic(tabs);
    fitBuiltPages(w);
}

class DedicatedInstaller : public QObject
{
public:
    explicit DedicatedInstaller(QObject *p=nullptr):QObject(p) {}
protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QMainWindow *w=qobject_cast<QMainWindow*>(watched);
        if (!w || w->objectName()!=QStringLiteral("MainWindow")) return QObject::eventFilter(watched,event);
        if ((event->type()==QEvent::Show || event->type()==QEvent::Polish) && !w->property("dedicatedPagesScheduled").toBool()) {
            w->setProperty("dedicatedPagesScheduled",true);
            QTimer::singleShot(180,w,[w](){rebuildAll(w);});
            QTimer::singleShot(700,w,[w](){rebuildAll(w);});
        } else if (event->type()==QEvent::Resize && w->property("dedicatedPagesScheduled").toBool()) {
            QTimer::singleShot(0,w,[w](){fitBuiltPages(w);});
        }
        return QObject::eventFilter(watched,event);
    }
};

void installDedicatedPages()
{
    QApplication *a=qobject_cast<QApplication*>(QCoreApplication::instance());
    if (a) a->installEventFilter(new DedicatedInstaller(a));
}

}

Q_COREAPP_STARTUP_FUNCTION(installDedicatedPages)