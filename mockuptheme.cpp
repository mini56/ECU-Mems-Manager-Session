#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QMainWindow>
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
                QTimer::singleShot(1300, window, [window](){ apply(window); });
            }
        }
        return QObject::eventFilter(watched,event);
    }

private:
    static void apply(QMainWindow *window)
    {
        QTabWidget *tabs=window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
        if (!tabs) return;

        const QString style = QStringLiteral(
            "QWidget{background:#0d1116;color:#dce2e7;}"
            "QLabel{background:transparent;color:#dce2e7;border:0;}"
            "QGroupBox{background:#14191f;color:#e5e9ec;border:1px solid #303842;"
            "border-radius:4px;margin-top:10px;font-weight:600;}"
            "QGroupBox::title{subcontrol-origin:margin;left:9px;padding:0 4px;color:#ff9b32;}"
            "QPushButton{background:#1769d2;color:#fff;border:1px solid #2d7ee8;border-radius:4px;"
            "padding:5px 10px;font-weight:600;}"
            "QPushButton:hover{background:#2378e6;border-color:#4c91ef;}"
            "QPushButton:pressed{background:#1257b0;}"
            "QPushButton:disabled{background:#252b31;color:#707983;border-color:#333b43;}"
            "QLineEdit,QTextEdit,QPlainTextEdit,QSpinBox,QDoubleSpinBox,QComboBox{background:#11161c;"
            "color:#f0f3f5;border:1px solid #343d46;border-radius:3px;padding:4px 6px;selection-background-color:#1769d2;}"
            "QComboBox::drop-down{border:0;width:20px;}"
            "QCheckBox,QRadioButton{background:transparent;color:#dce2e7;spacing:6px;}"
            "QTableWidget,QTableView,QTreeWidget,QListView{background:#10151a;color:#dce2e7;"
            "alternate-background-color:#151b21;border:1px solid #303842;gridline-color:#2b333b;}"
            "QHeaderView::section{background:#171d23;color:#dce2e7;border:0;border-right:1px solid #303842;"
            "border-bottom:1px solid #303842;padding:5px;font-weight:600;}"
            "QScrollArea{background:#0d1116;border:0;}"
            "QScrollArea>QWidget>QWidget{background:#0d1116;}"
            "QScrollBar:vertical{background:#0d1116;width:10px;margin:0;}"
            "QScrollBar::handle:vertical{background:#39434d;border-radius:4px;min-height:28px;}"
            "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
            "QScrollBar:horizontal{background:#0d1116;height:10px;margin:0;}"
            "QScrollBar::handle:horizontal{background:#39434d;border-radius:4px;min-width:28px;}"
            "QScrollBar::add-line:horizontal,QScrollBar::sub-line:horizontal{width:0;}"
            "QProgressBar{background:#10151a;color:#dce2e7;border:1px solid #303842;border-radius:3px;text-align:center;}"
            "QProgressBar::chunk{background:#ff8a1c;border-radius:2px;}"
        );

        for (int i=0;i<tabs->count();++i) {
            QWidget *page=tabs->widget(i);
            if (!page) continue;
            // Analysis and Overview keep their dedicated approved renderers.
            if (page->objectName()==QStringLiteral("overview_tab")) continue;
            if (QString::fromLatin1(page->metaObject()->className())==QStringLiteral("AnalysisTab")) continue;
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
