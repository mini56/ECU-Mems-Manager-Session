#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QMainWindow>
#include <QScrollArea>
#include <QTabWidget>
#include <QTimer>
#include <QWidget>

namespace {

class MockupThemeInstaller : public QObject
{
public:
    explicit MockupThemeInstaller(QObject *parent=nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if ((event->type()==QEvent::Show || event->type()==QEvent::Polish) && watched) {
            QMainWindow *window=qobject_cast<QMainWindow*>(watched);
            if (window && window->objectName()==QStringLiteral("MainWindow") &&
                !window->property("mockupThemeInstalled").toBool()) {
                window->setProperty("mockupThemeInstalled", true);
                QTimer::singleShot(0, window, [window](){ apply(window); });
            }
        }
        return QObject::eventFilter(watched,event);
    }

private:
    static QWidget *realPage(QWidget *tabPage, QScrollArea **scrollOut=nullptr)
    {
        if (scrollOut) *scrollOut=nullptr;
        if (!tabPage) return nullptr;
        if (QScrollArea *scroll=qobject_cast<QScrollArea*>(tabPage)) {
            if (scrollOut) *scrollOut=scroll;
            return scroll->widget();
        }
        return tabPage;
    }

    static void apply(QMainWindow *window)
    {
        QTabWidget *tabs=window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
        if (!tabs) return;

        const QString style = QStringLiteral(
            "QWidget{background:#0d1116;color:#dce2e7;}"
            "QLabel{background:transparent;color:#dce2e7;border:0;}"
            "QFrame{border-color:#2b343d;}"
            "QGroupBox{background:#12181e;color:#e5e9ec;border:1px solid #2b343d;"
            "border-radius:0px;margin-top:8px;font-weight:600;}"
            "QGroupBox::title{subcontrol-origin:margin;left:8px;padding:0 4px;color:#ff9b32;}"
            "QPushButton,QToolButton{background:#17202a;color:#eef2f5;border:1px solid #34414d;"
            "border-radius:0px;padding:3px 8px;font-weight:600;}"
            "QPushButton:hover,QToolButton:hover{background:#1c2833;border-color:#ff8a1c;color:#ffffff;}"
            "QPushButton:pressed,QToolButton:pressed{background:#111820;}"
            "QPushButton:checked,QToolButton:checked{background:#ff8a1c;color:#101419;border-color:#ff9b32;}"
            "QPushButton:disabled,QToolButton:disabled{background:#20262c;color:#68727c;border-color:#303841;}"
            "QLineEdit,QTextEdit,QPlainTextEdit,QSpinBox,QDoubleSpinBox,QComboBox{background:#0f151b;"
            "color:#f0f3f5;border:1px solid #303b45;border-radius:0px;padding:3px 5px;"
            "selection-background-color:#1769d2;}"
            "QLineEdit:focus,QTextEdit:focus,QPlainTextEdit:focus,QSpinBox:focus,QDoubleSpinBox:focus,QComboBox:focus{"
            "border-color:#596775;}"
            "QComboBox::drop-down{border:0;width:18px;}"
            "QCheckBox,QRadioButton{background:transparent;color:#dce2e7;spacing:5px;padding:1px 0;}"
            "QCheckBox::indicator,QRadioButton::indicator{width:13px;height:13px;}"
            "QTableWidget,QTableView,QTreeWidget,QListView{background:#0f151a;color:#dce2e7;"
            "alternate-background-color:#131a20;border:1px solid #2b343d;gridline-color:#263039;outline:0;}"
            "QTableWidget::item,QTableView::item,QTreeWidget::item,QListView::item{padding:2px 4px;}"
            "QTableWidget::item:selected,QTableView::item:selected,QTreeWidget::item:selected,QListView::item:selected{"
            "background:#1b2530;color:#ffffff;}"
            "QHeaderView::section{background:#141b21;color:#dce2e7;border:0;border-right:1px solid #2b343d;"
            "border-bottom:1px solid #2b343d;padding:3px 5px;font-weight:600;}"
            "QScrollArea{background:#0d1116;border:0;}"
            "QScrollArea>QWidget>QWidget{background:#0d1116;}"
            "QScrollBar:vertical{background:#0b1015;width:9px;margin:0;}"
            "QScrollBar::handle:vertical{background:#34414c;border-radius:0px;min-height:24px;}"
            "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
            "QScrollBar:horizontal{background:#0b1015;height:9px;margin:0;}"
            "QScrollBar::handle:horizontal{background:#34414c;border-radius:0px;min-width:24px;}"
            "QScrollBar::add-line:horizontal,QScrollBar::sub-line:horizontal{width:0;}"
            "QProgressBar{background:#0f151a;color:#dce2e7;border:1px solid #2b343d;border-radius:0px;text-align:center;}"
            "QProgressBar::chunk{background:#ff8a1c;border-radius:0px;}"
            "QSlider::groove:horizontal{height:4px;background:#27313a;border-radius:0px;}"
            "QSlider::handle:horizontal{width:12px;margin:-4px 0;background:#ff8a1c;border:1px solid #ff9b32;border-radius:0px;}"
        );

        for (int i=0;i<tabs->count();++i) {
            QScrollArea *scroll=nullptr;
            QWidget *page=realPage(tabs->widget(i), &scroll);
            if (!page) continue;

            if (scroll) {
                scroll->setWidgetResizable(true);
                scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
                scroll->setStyleSheet(style);
                if (scroll->viewport()) {
                    scroll->viewport()->setAutoFillBackground(false);
                    scroll->viewport()->setStyleSheet(QStringLiteral("background:#0d1116;"));
                }
            }

            if (page->objectName()==QStringLiteral("overview_tab")) continue;
            if (QString::fromLatin1(page->metaObject()->className())==QStringLiteral("AnalysisTab")) continue;

            page->setMinimumSize(0,0);
            page->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
            page->setAttribute(Qt::WA_StyledBackground,true);
            page->setStyleSheet(style);
        }
    }
};

void installMockupThemeHook()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app) return;
    MockupThemeInstaller *installer=new MockupThemeInstaller(app);
    app->installEventFilter(installer);
}

}

Q_COREAPP_STARTUP_FUNCTION(installMockupThemeHook)
