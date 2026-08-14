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

        // The approved mock-up contains only the real application pages.
        // Do not create a mode selector, a placeholder Database tab or a second
        // trends panel here. Their previous creation caused visible layout jumps
        // and duplicated elements that were not present in the reference.
        installNavigation();
        modernizeOverview();
    }

private:
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
            "QListWidget{background:#10151a;color:#b8c0c7;border:0;border-right:1px solid #293038;"
            "padding:5px 3px;outline:0;}"
            "QListWidget::item{min-height:28px;padding:2px 9px;border-radius:1px;margin:1px 0px;}"
            "QListWidget::item:hover{background:#171d23;color:#ffffff;}"
            "QListWidget::item:selected{background:#191f25;color:#ff9b32;border-left:3px solid #ff8a1c;}"
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
        for (int i = 0; i < m_tabs->count(); ++i)
            m_nav->addItem(m_tabs->tabText(i));
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
            "#overview_tab QGroupBox{color:#e7ebee;background:#14191f;border:1px solid #303842;"
            "border-radius:4px;margin-top:10px;font-weight:600;}"
            "#overview_tab QGroupBox::title{subcontrol-origin:margin;left:9px;padding:0 4px;color:#ff9b32;}"
            "#overview_tab QPushButton{background:#1769d2;color:#ffffff;border:1px solid #2d7ee8;"
            "border-radius:4px;padding:5px 10px;font-weight:600;}"
            "#overview_tab QPushButton:hover{background:#2378e6;border-color:#4c91ef;}"
            "#overview_tab QPushButton:disabled{background:#252b31;color:#707983;border-color:#333b43;}"
            "#overview_tab QProgressBar{background:#10151a;color:#dce2e7;border:1px solid #303842;"
            "border-radius:3px;text-align:center;}"
            "#overview_tab QProgressBar::chunk{background:#ff8a1c;border-radius:2px;}"
        );
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
    ModernUiAutoInstaller *installer = new ModernUiAutoInstaller(app);
    app->installEventFilter(installer);
}

}

Q_COREAPP_STARTUP_FUNCTION(installModernUiHook)
