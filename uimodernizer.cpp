#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QTabBar>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace {

class UiModernizer : public QObject
{
public:
    explicit UiModernizer(QMainWindow *window) : QObject(window), m_window(window)
    {
        m_tabs = window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
        if (!m_tabs) return;
        if (qApp) qApp->setProperty("ecuDarkTheme", true);
        extractRecorderPage();
        installNavigation();
        modernizeOverview();
    }

private:
    void extractRecorderPage()
    {
        if (!m_tabs || m_tabs->findChild<QWidget*>(QStringLiteral("recorder_tab"))) return;
        QWidget *overview = m_window->findChild<QWidget*>(QStringLiteral("overview_tab"));
        if (!overview) return;

        QWidget *loggerBox = overview->findChild<QWidget*>(QStringLiteral("layoutWidget"), Qt::FindDirectChildrenOnly);
        if (!loggerBox) return;

        QWidget *recorder = new QWidget(m_tabs);
        recorder->setObjectName(QStringLiteral("recorder_tab"));
        recorder->setAttribute(Qt::WA_StyledBackground, true);
        recorder->setStyleSheet(
            "#recorder_tab{background:#0d1116;color:#dce2e7;}"
            "#recorderHero{background:#10161c;border:1px solid #2b343d;border-radius:4px;}"
            "#recorderCard{background:#111820;border:1px solid #303943;border-radius:4px;}"
            "#recorder_tab QLabel{color:#dce2e7;background:transparent;}"
            "#recorder_tab QLineEdit{background:#0c1217;color:#f0f3f5;border:1px solid #35414c;"
            "border-radius:2px;padding:7px 9px;}"
            "#recorder_tab QPushButton{background:#151d25;color:#eef2f5;border:1px solid #3a4652;"
            "border-radius:2px;padding:7px 14px;font-weight:700;}"
            "#recorder_tab QPushButton:hover{border-color:#ff8a1c;color:#ffffff;background:#1b242d;}"
            "#recorder_tab QPushButton:disabled{background:#1b2229;color:#65707a;border-color:#2a333b;}"
        );

        QVBoxLayout *root = new QVBoxLayout(recorder);
        root->setContentsMargins(22,18,22,18);
        root->setSpacing(14);

        QFrame *hero = new QFrame(recorder);
        hero->setObjectName(QStringLiteral("recorderHero"));
        QVBoxLayout *heroLayout = new QVBoxLayout(hero);
        heroLayout->setContentsMargins(18,13,18,13);
        heroLayout->setSpacing(3);
        QLabel *title = new QLabel(QStringLiteral("ENREGISTREUR"), hero);
        QFont tf = title->font();
        tf.setBold(true);
        tf.setPointSizeF(12.0);
        title->setFont(tf);
        title->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;"));
        QLabel *subtitle = new QLabel(QStringLiteral("ENREGISTREMENT DE SESSION ECU"), hero);
        QFont sf = subtitle->font();
        sf.setPointSizeF(8.5);
        sf.setLetterSpacing(QFont::AbsoluteSpacing, 0.5);
        subtitle->setFont(sf);
        subtitle->setStyleSheet(QStringLiteral("color:#8e99a3;background:transparent;"));
        heroLayout->addWidget(title);
        heroLayout->addWidget(subtitle);
        root->addWidget(hero);

        QFrame *card = new QFrame(recorder);
        card->setObjectName(QStringLiteral("recorderCard"));
        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(20,18,20,20);
        cardLayout->setSpacing(12);

        QLabel *section = new QLabel(QStringLiteral("SESSION D'ENREGISTREMENT"), card);
        QFont sectionFont = section->font();
        sectionFont.setBold(true);
        sectionFont.setPointSizeF(10.0);
        section->setFont(sectionFont);
        section->setStyleSheet(QStringLiteral("color:#f0f3f5;background:transparent;"));
        cardLayout->addWidget(section);

        QFrame *line = new QFrame(card);
        line->setFrameShape(QFrame::HLine);
        line->setStyleSheet(QStringLiteral("background:#2b343d;border:0;max-height:1px;"));
        cardLayout->addWidget(line);

        loggerBox->setParent(card);
        loggerBox->setMinimumWidth(520);
        loggerBox->setMaximumWidth(760);
        loggerBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        loggerBox->setStyleSheet(QStringLiteral("background:transparent;"));
        cardLayout->addWidget(loggerBox, 0, Qt::AlignLeft | Qt::AlignTop);
        cardLayout->addStretch(1);

        QHBoxLayout *body = new QHBoxLayout;
        body->setContentsMargins(0,0,0,0);
        body->setSpacing(14);
        body->addWidget(card, 3);

        QFrame *info = new QFrame(recorder);
        info->setObjectName(QStringLiteral("recorderCard"));
        info->setMinimumWidth(230);
        info->setMaximumWidth(320);
        QVBoxLayout *infoLayout = new QVBoxLayout(info);
        infoLayout->setContentsMargins(18,18,18,18);
        infoLayout->setSpacing(9);
        QLabel *infoTitle = new QLabel(QStringLiteral("SESSION"), info);
        QFont ifont = infoTitle->font();
        ifont.setBold(true);
        ifont.setPointSizeF(10.0);
        infoTitle->setFont(ifont);
        infoTitle->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;"));
        QLabel *infoText = new QLabel(QStringLiteral("Les commandes ci-contre utilisent l'enregistreur réel de MEMS Manager.\n\nLe nom de fichier et les boutons Enregistrement / Arrêter restent reliés à la logique existante."), info);
        infoText->setWordWrap(true);
        infoText->setStyleSheet(QStringLiteral("color:#9fa9b2;background:transparent;line-height:1.3;"));
        infoLayout->addWidget(infoTitle);
        infoLayout->addWidget(infoText);
        infoLayout->addStretch(1);
        body->addWidget(info, 1);
        root->addLayout(body, 1);

        m_tabs->addTab(recorder, QStringLiteral("Enregistreur"));
    }

    void installNavigation()
    {
        QWidget *central = m_window->centralWidget();
        QBoxLayout *root = central ? qobject_cast<QBoxLayout*>(central->layout()) : nullptr;
        if (!central || !root || !m_tabs) return;

        const int tabsIndex = root->indexOf(m_tabs);
        if (tabsIndex < 0) return;
        root->removeWidget(m_tabs);

        QFrame *workspace = new QFrame(central);
        workspace->setObjectName(QStringLiteral("modernWorkspace"));
        workspace->setStyleSheet(
            "#modernWorkspace{background:#0d1116;}"
            "QListWidget{background:#0e1419;color:#c4ccd3;border:0;border-right:1px solid #293038;"
            "padding:7px 0;outline:0;}"
            "QListWidget::item{min-height:31px;padding:2px 12px;border-radius:0;margin:0;font-weight:600;}"
            "QListWidget::item:hover{background:#161d23;color:#ffffff;}"
            "QListWidget::item:selected{background:#1a2026;color:#ff9828;border-left:3px solid #ff8a1c;}"
            "QTabWidget::pane{border:0;background:#0d1116;}"
        );

        QHBoxLayout *layout = new QHBoxLayout(workspace);
        layout->setContentsMargins(0,0,0,0);
        layout->setSpacing(0);

        m_nav = new QListWidget(workspace);
        m_nav->setObjectName(QStringLiteral("modernNavigation"));
        m_nav->setFixedWidth(176);
        m_nav->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_nav->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        m_nav->setSelectionMode(QAbstractItemView::SingleSelection);
        m_nav->setFocusPolicy(Qt::NoFocus);
        m_nav->setUniformItemSizes(true);
        m_nav->setSpacing(0);
        for (int i = 0; i < m_tabs->count(); ++i) {
            QListWidgetItem *item = new QListWidgetItem(m_tabs->tabText(i), m_nav);
            item->setToolTip(m_tabs->tabText(i));
        }
        m_nav->setCurrentRow(m_tabs->currentIndex());

        m_tabs->tabBar()->hide();
        layout->addWidget(m_nav);
        layout->addWidget(m_tabs, 1);
        root->insertWidget(tabsIndex, workspace, 1);

        connect(m_nav, &QListWidget::currentRowChanged, this, [this](int row){
            if (row >= 0 && row < m_tabs->count()) m_tabs->setCurrentIndex(row);
        });
        connect(m_tabs, &QTabWidget::currentChanged, this, [this](int index){
            if (m_nav && index >= 0 && index < m_nav->count() && m_nav->currentRow() != index)
                m_nav->setCurrentRow(index);
        });
    }

    void modernizeOverview()
    {
        QWidget *overview = m_window->findChild<QWidget*>(QStringLiteral("overview_tab"));
        if (!overview) return;
        overview->setStyleSheet(
            "#overview_tab{background:#0d1116;color:#e7ebee;}"
            "#overview_tab QLabel{color:#dfe4e8;background:transparent;}"
            "#overview_tab QGroupBox{color:#e7ebee;background:#12181e;border:1px solid #2b343d;"
            "border-radius:3px;margin-top:8px;font-weight:600;}"
            "#overview_tab QGroupBox::title{subcontrol-origin:margin;left:8px;padding:0 4px;color:#ff9b32;}"
            "#overview_tab QPushButton{background:#17202a;color:#eef2f5;border:1px solid #34414d;"
            "border-radius:2px;padding:4px 9px;font-weight:600;}"
            "#overview_tab QPushButton:hover{background:#1c2833;border-color:#ff8a1c;color:#ffffff;}"
            "#overview_tab QPushButton:pressed{background:#111820;}"
            "#overview_tab QPushButton:checked{background:#ff8a1c;color:#101419;border-color:#ff9b32;}"
            "#overview_tab QPushButton:disabled{background:#20262c;color:#68727c;border-color:#303841;}"
            "#overview_tab QProgressBar{background:#0f151a;color:#dce2e7;border:1px solid #2b343d;"
            "border-radius:2px;text-align:center;}"
            "#overview_tab QProgressBar::chunk{background:#ff8a1c;border-radius:1px;}"
        );

        const QStringList legacyOverlayNames = {
            QStringLiteral("m_closed_loop_label"),
            QStringLiteral("m_closedLoopLed"),
            QStringLiteral("m_idleSwitchLabel"),
            QStringLiteral("m_idleSwitchLed")
        };
        for (const QString &name : legacyOverlayNames) {
            if (QWidget *legacy = overview->findChild<QWidget*>(name)) legacy->hide();
        }
    }

    QMainWindow *m_window = nullptr;
    QTabWidget *m_tabs = nullptr;
    QListWidget *m_nav = nullptr;
};

class ModernUiAutoInstaller : public QObject
{
public:
    explicit ModernUiAutoInstaller(QObject *parent=nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if ((event->type()==QEvent::Show || event->type()==QEvent::Polish) && watched) {
            QMainWindow *window = qobject_cast<QMainWindow*>(watched);
            if (window && window->objectName()==QStringLiteral("MainWindow") &&
                !window->property("modernUiInstalled").toBool()) {
                QTabWidget *tabs = window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
                if (tabs && tabs->count() >= 8) {
                    window->setProperty("modernUiInstalled", true);
                    QTimer::singleShot(0, window, [window](){ new UiModernizer(window); });
                }
            }
        }
        return QObject::eventFilter(watched,event);
    }
};

void installModernUiHook()
{
    QApplication *app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app) return;
    ModernUiAutoInstaller *installer=new ModernUiAutoInstaller(app);
    app->installEventFilter(installer);
}

}

Q_COREAPP_STARTUP_FUNCTION(installModernUiHook)
