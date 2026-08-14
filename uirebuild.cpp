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

namespace {

static const char *kOrange = "#ff7a00";
static const char *kOrange2 = "#ff9828";
static const char *kBg = "#090e13";
static const char *kPanel = "#10161c";
static const char *kBorder = "#29343e";
static const char *kText = "#e7ecef";
static const char *kMuted = "#8d99a3";

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

static QString subtitleFor(const QString &title)
{
    const QString t = title.toLower();
    if (t.contains(QStringLiteral("aper"))) return QStringLiteral("VUE D'ENSEMBLE DES PARAMÈTRES CLÉS");
    if (t.contains(QStringLiteral("régl"))) return QStringLiteral("CONFIGURATION ET RÉGLAGES ECU");
    if (t.contains(QStringLiteral("toutes les mesures"))) return QStringLiteral("MESURES ECU EN TEMPS RÉEL");
    if (t.contains(QStringLiteral("erreur"))) return QStringLiteral("DÉFAUTS, ANOMALIES ET ÉTATS SURVEILLÉS");
    if (t.contains(QStringLiteral("actionneur"))) return QStringLiteral("COMMANDES ET TESTS DES ACTIONNEURS");
    if (t.contains(QStringLiteral("toutes les données"))) return QStringLiteral("DONNÉES BRUTES ET VALEURS DÉCODÉES");
    if (t.contains(QStringLiteral("interactif"))) return QStringLiteral("COMMANDES MANUELLES ET MODE TECHNIQUE");
    if (t.contains(QStringLiteral("analyse"))) return QStringLiteral("ANALYSE DÉTAILLÉE, COURBES ET CURSEURS");
    if (t.contains(QStringLiteral("rosco"))) return QStringLiteral("INFORMATIONS TECHNIQUES ET SESSION ECU / ROSCO");
    if (t.contains(QStringLiteral("diagnostic"))) return QStringLiteral("VÉRIFICATION COMPLÈTE DU SYSTÈME ET RAPPORT DE SANTÉ");
    if (t.contains(QStringLiteral("enregistre"))) return QStringLiteral("ENREGISTREMENT DES SESSIONS DE DIAGNOSTIC");
    return QStringLiteral("ECU MEMS MANAGER");
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
    ).arg(QString::fromLatin1(kBg), QString::fromLatin1(kText), QString::fromLatin1(kBorder),
          QString::fromLatin1(kPanel), QString::fromLatin1(kOrange2), QString::fromLatin1(kOrange));
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

static QFrame *makePageHeader(QWidget *parent, const QString &title)
{
    QFrame *hero = makeCard(parent, QStringLiteral("uiRebuildHero_%1").arg(qAbs(qHash(title))));
    QVBoxLayout *v = new QVBoxLayout(hero);
    v->setContentsMargins(15,9,15,9);
    v->setSpacing(1);
    QLabel *t = new QLabel(title.toUpper(), hero);
    QFont tf=t->font(); tf.setBold(true); tf.setPointSizeF(10.5); t->setFont(tf);
    t->setStyleSheet(QStringLiteral("color:%1;background:transparent;border:0;").arg(QString::fromLatin1(kOrange2)));
    QLabel *s = new QLabel(subtitleFor(title), hero);
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

static void composeGenericPage(QWidget *page, const QString &title)
{
    if (!page || page->property("uiRebuiltPage").toBool()) return;
    page->setProperty("uiRebuiltPage", true);
    page->setMinimumSize(0,0);
    page->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    page->setAttribute(Qt::WA_StyledBackground,true);

    if (QBoxLayout *existing=qobject_cast<QBoxLayout*>(page->layout())) {
        existing->setContentsMargins(15,12,15,12);
        existing->setSpacing(qMax(9,existing->spacing()));
        existing->insertWidget(0,makePageHeader(page,title));
        prepareViewWidgets(page);
        return;
    }

    QList<QWidget*> blocks=directLayoutBlocks(page);
    QVBoxLayout *root=new QVBoxLayout(page);
    root->setContentsMargins(15,12,15,12);
    root->setSpacing(10);
    root->addWidget(makePageHeader(page,title));
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

static QWidget *createRecorderTab(QTabWidget *tabs, QWidget *overview)
{
    if (!tabs || !overview) return nullptr;
    QWidget *loggerBox=overview->findChild<QWidget*>(QStringLiteral("layoutWidget"),Qt::FindDirectChildrenOnly);
    if (!loggerBox) return nullptr;
    QWidget *page=new QWidget(tabs);
    page->setObjectName(QStringLiteral("recorder_tab"));
    page->setAttribute(Qt::WA_StyledBackground,true);
    QVBoxLayout *root=new QVBoxLayout(page);
    root->setContentsMargins(15,12,15,12); root->setSpacing(10);
    root->addWidget(makePageHeader(page,QStringLiteral("Enregistreur")));
    QHBoxLayout *body=new QHBoxLayout; body->setSpacing(10);
    QFrame *mainCard=makeCard(page,QStringLiteral("recorderMainCard"));
    QVBoxLayout *mainLayout=new QVBoxLayout(mainCard);
    mainLayout->setContentsMargins(18,16,18,18); mainLayout->setSpacing(10);
    QLabel *heading=new QLabel(QStringLiteral("SESSION D'ENREGISTREMENT"),mainCard);
    QFont hf=heading->font(); hf.setBold(true); hf.setPointSizeF(10); heading->setFont(hf);
    heading->setStyleSheet(QStringLiteral("color:#f0f3f5;background:transparent;"));
    mainLayout->addWidget(heading);
    QFrame *line=new QFrame(mainCard); line->setFrameShape(QFrame::HLine); line->setStyleSheet(QStringLiteral("background:%1;border:0;max-height:1px;").arg(QString::fromLatin1(kBorder))); mainLayout->addWidget(line);
    loggerBox->setParent(mainCard); loggerBox->setMinimumWidth(0); loggerBox->setMaximumWidth(QWIDGETSIZE_MAX); loggerBox->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed); mainLayout->addWidget(loggerBox,0); mainLayout->addStretch(1);
    body->addWidget(mainCard,3);
    QFrame *side=makeCard(page,QStringLiteral("recorderSideCard")); side->setObjectName(QStringLiteral("recorderSideCard"));
    QVBoxLayout *sv=new QVBoxLayout(side); sv->setContentsMargins(16,16,16,16);
    QLabel *st=new QLabel(QStringLiteral("ENREGISTREMENT"),side); QFont stf=st->font(); stf.setBold(true); stf.setPointSizeF(10); st->setFont(stf); st->setStyleSheet(QStringLiteral("color:%1;background:transparent;").arg(QString::fromLatin1(kOrange2)));
    QLabel *desc=new QLabel(QStringLiteral("Nom de fichier, démarrage et arrêt de la session. Les commandes restent reliées à l'enregistreur réel du programme."),side); desc->setWordWrap(true); desc->setStyleSheet(QStringLiteral("color:%1;background:transparent;").arg(QString::fromLatin1(kMuted)));
    sv->addWidget(st); sv->addWidget(desc); sv->addStretch(1); body->addWidget(side,1);
    root->addLayout(body,1);
    return page;
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
    QLabel *brand=new QLabel(QStringLiteral("ECU MEMS MANAGER"),header); brand->setObjectName(QStringLiteral("uiRebuildBrand")); QFont bf=brand->font(); bf.setBold(true); bf.setPointSizeF(10.5); brand->setFont(bf); brand->setStyleSheet(QStringLiteral("color:#f5f7f8;letter-spacing:.5px;"));
    QLabel *ver=new QLabel(QStringLiteral("v%1.%2.%3").arg(VER_MAJOR).arg(VER_MINOR).arg(VER_PATCH),header); ver->setStyleSheet(QStringLiteral("color:%1;font-weight:700;").arg(QString::fromLatin1(kOrange2))); brandV->addWidget(brand); brandV->addWidget(ver); h->addLayout(brandV);
    QFrame *sep=new QFrame(header); sep->setFrameShape(QFrame::VLine); sep->setStyleSheet(QStringLiteral("background:%1;max-width:1px;").arg(QString::fromLatin1(kBorder))); h->addWidget(sep);
    ecu->setParent(header); ecu->setStyleSheet(QStringLiteral("color:#dfe4e8;font-weight:700;padding:0 8px;")); h->addWidget(ecu);
    QSettings s(QSettings::IniFormat,QSettings::UserScope,PROJECTNAME); s.beginGroup(QStringLiteral("Settings")); QString port=s.value(QStringLiteral("SerialDevice"),QStringLiteral("--")).toString(); s.endGroup(); if(port.trimmed().isEmpty())port=QStringLiteral("--");
    QLabel *portLabel=new QLabel(QStringLiteral("Port\n%1").arg(port),header); portLabel->setAlignment(Qt::AlignCenter); portLabel->setStyleSheet(QStringLiteral("color:#cbd3d9;font-weight:600;")); h->addWidget(portLabel);
    QLabel *freqLabel=new QLabel(QStringLiteral("Fréquence\nauto"),header); freqLabel->setAlignment(Qt::AlignCenter); freqLabel->setStyleSheet(QStringLiteral("color:#cbd3d9;font-weight:600;")); h->addWidget(freqLabel); h->addStretch(1);
    QWidget *commBox=new QWidget(header); QHBoxLayout *ch=new QHBoxLayout(commBox); ch->setContentsMargins(0,0,0,0); ch->setSpacing(5); comm->setParent(commBox); comm->setStyleSheet(QStringLiteral("color:#cbd3d9;font-weight:700;")); ch->addWidget(comm); if(good){good->setParent(commBox);ch->addWidget(good);} if(bad){bad->setParent(commBox);ch->addWidget(bad);} h->addWidget(commBox);
    connectButton->setParent(header); disconnectButton->setParent(header); h->addWidget(connectButton); h->addWidget(disconnectButton);
    legacy->hide(); legacy->setMaximumSize(0,0); return header;
}

static QFrame *buildBottomStatus(QMainWindow *window)
{
    QFrame *bar=new QFrame(window->centralWidget()); bar->setObjectName(QStringLiteral("uiRebuildStatus")); bar->setAttribute(Qt::WA_StyledBackground,true);
    bar->setStyleSheet(QStringLiteral("#uiRebuildStatus{background:#080d12;border-top:1px solid %1;}#uiRebuildStatus QLabel{background:transparent;border-right:1px solid %1;color:#c9d1d7;padding:0 8px;}#uiRebuildStatus QPushButton{margin:3px 5px;padding:2px 10px;}").arg(QString::fromLatin1(kBorder)));
    QHBoxLayout *h=new QHBoxLayout(bar); h->setContentsMargins(0,0,0,0); h->setSpacing(0);
    QLabel *file=new QLabel(QStringLiteral("Fichier : --"),bar); QLabel *loop=new QLabel(QStringLiteral("Boucle : --"),bar); QLabel *lambda=new QLabel(QStringLiteral("Lambda : --"),bar); QLabel *system=new QLabel(QStringLiteral("Système : --"),bar); system->setStyleSheet(QStringLiteral("color:#65db79;background:transparent;border-right:1px solid %1;padding:0 8px;").arg(QString::fromLatin1(kBorder))); QLabel *inject=new QLabel(QStringLiteral("Injection : -- ms"),bar); QLabel *air=new QLabel(QStringLiteral("Air : -- °C"),bar);
    h->addWidget(file,2);h->addWidget(loop,1);h->addWidget(lambda,1);h->addWidget(system,1);h->addWidget(inject,1);h->addWidget(air,1); QPushButton *capture=new QPushButton(QStringLiteral("Capture écran"),bar);h->addWidget(capture,1);
    QObject::connect(capture,&QPushButton::clicked,window,[window](){QMetaObject::invokeMethod(window,"onSnapshotClicked",Qt::QueuedConnection);});
    QTimer *timer=new QTimer(bar);timer->setInterval(400);QObject::connect(timer,&QTimer::timeout,bar,[=](){if(QLineEdit *f=window->findChild<QLineEdit*>(QStringLiteral("m_logFileNameBox")))file->setText(QStringLiteral("Fichier : %1").arg(f->text()));if(QWidget *w=window->findChild<QWidget*>(QStringLiteral("m_closedLoopLed")))loop->setText(QStringLiteral("Boucle : %1").arg(w->property("checked").toBool()?QStringLiteral("fermée"):QStringLiteral("ouverte")));if(QWidget *w=window->findChild<QWidget*>(QStringLiteral("m_lambda_voltage")))lambda->setText(QStringLiteral("Lambda : %1").arg(w->property("value").toString()));if(QWidget *w=window->findChild<QWidget*>(QStringLiteral("m_engine_error")))system->setText(w->property("checked").toBool()?QStringLiteral("Système : défaut"):QStringLiteral("Système : OK"));if(QWidget *w=window->findChild<QWidget*>(QStringLiteral("m_injector_time")))inject->setText(QStringLiteral("Injection : %1 ms").arg(w->property("value").toString()));if(QWidget *w=window->findChild<QWidget*>(QStringLiteral("m_airTempGauge")))air->setText(QStringLiteral("Air : %1").arg(w->property("value").toString()));});timer->start();return bar;
}

static qreal responsiveScale(QMainWindow *window)
{
    if(!window) return 1.0;
    const QSize sz=window->centralWidget()?window->centralWidget()->size():window->size();
    const qreal sx=sz.width()/1366.0;
    const qreal sy=sz.height()/768.0;
    return qBound<qreal>(0.78,qMin(sx,sy),1.28);
}

static void applyResponsive(QMainWindow *window)
{
    if(!window) return;
    const qreal s=responsiveScale(window);
    window->setProperty("globalUiScale",s);

    if(QFrame *header=window->findChild<QFrame*>(QStringLiteral("uiRebuildHeader")))
        header->setFixedHeight(qBound(44,qRound(56.0*s),68));
    if(QLabel *logo=window->findChild<QLabel*>(QStringLiteral("uiRebuildLogo"))) {
        const int side=qBound(28,qRound(38.0*s),46);
        logo->setFixedSize(side,side);
        QPixmap p(QStringLiteral(":/icons/key.png"));
        if(!p.isNull())logo->setPixmap(p.scaled(side-4,side-4,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    }
    if(QLabel *brand=window->findChild<QLabel*>(QStringLiteral("uiRebuildBrand"))) {
        QFont f=brand->font(); f.setPointSizeF(qBound<qreal>(8.0,10.5*s,12.5)); brand->setFont(f);
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
        const int navW=qBound(155,qRound(190.0*s),220);
        const int rowH=qBound(29,qRound(35.0*s),42);
        const int pad=qBound(8,qRound(14.0*s),17);
        nav->setFixedWidth(navW);
        nav->setStyleSheet(QStringLiteral("#uiRebuildNav{background:#0d1318;color:#c7d0d6;border:0;border-right:1px solid %1;padding:6px 0;}#uiRebuildNav::item{min-height:%2px;padding:2px %3px;border-left:3px solid transparent;font-weight:650;}#uiRebuildNav::item:hover{background:#161d23;color:white;}#uiRebuildNav::item:selected{background:#1c211f;color:%4;border-left:3px solid %5;}")
            .arg(QString::fromLatin1(kBorder)).arg(rowH).arg(pad).arg(QString::fromLatin1(kOrange2)).arg(QString::fromLatin1(kOrange)));
    }

    if(QFrame *status=window->findChild<QFrame*>(QStringLiteral("uiRebuildStatus")))
        status->setFixedHeight(qBound(29,qRound(36.0*s),43));

    if(QFrame *side=window->findChild<QFrame*>(QStringLiteral("recorderSideCard")))
        side->setMaximumWidth(qBound(210,qRound(310.0*s),360));

    const QList<QScrollArea*> scrolls=window->findChildren<QScrollArea*>();
    for(QScrollArea *scroll:scrolls){
        scroll->setWidgetResizable(true);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        if(scroll->widget()){
            scroll->widget()->setMinimumWidth(0);
            scroll->widget()->setMaximumWidth(QWIDGETSIZE_MAX);
        }
    }
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
    static void rebuild(QMainWindow *window,QWidget *central,QVBoxLayout *root,QTabWidget *tabs,QWidget *legacy)
    {
        qApp->setStyleSheet(globalStyle()); central->setAttribute(Qt::WA_StyledBackground,true); central->setStyleSheet(QStringLiteral("background:%1;").arg(QString::fromLatin1(kBg)));
        if(QFrame *header=buildHeader(window,legacy))root->insertWidget(0,header);
        QWidget *overview=nullptr;for(int i=0;i<tabs->count();++i){QWidget *p=realPage(tabs->widget(i));if(p&&p->objectName()==QStringLiteral("overview_tab")){overview=p;break;}}
        if(overview&&tabs->findChild<QWidget*>(QStringLiteral("recorder_tab"))==nullptr)if(QWidget *rec=createRecorderTab(tabs,overview))tabs->addTab(rec,QStringLiteral("Enregistreur"));
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
        for(int i=0;i<tabs->count();++i){QWidget *page=realPage(tabs->widget(i));if(!page)continue;if(QScrollArea *scroll=qobject_cast<QScrollArea*>(tabs->widget(i))){scroll->setWidgetResizable(true);scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);page->setMinimumSize(0,0);}const QString title=titleForIndex(tabs,i);if(page->objectName()!=QStringLiteral("overview_tab")&&page->objectName()!=QStringLiteral("recorder_tab")&&QString::fromLatin1(page->metaObject()->className())!=QStringLiteral("AnalysisTab"))composeGenericPage(page,title);}
        root->addWidget(buildBottomStatus(window));tabs->setCurrentIndex(0);
        applyResponsive(window);
        QTimer::singleShot(120,window,[window](){applyResponsive(window);});
    }
};

void installUiRebuild(){QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());if(!app)return;UiRebuildInstaller *installer=new UiRebuildInstaller(app);app->installEventFilter(installer);} 

}

Q_COREAPP_STARTUP_FUNCTION(installUiRebuild)
