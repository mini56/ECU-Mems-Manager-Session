#include <QAbstractButton>
#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QEvent>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QTabWidget>
#include <QTextEdit>
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

    static bool isProtectedVisual(QWidget *w)
    {
        if (!w) return true;
        const QString cls=QString::fromLatin1(w->metaObject()->className()).toLower();
        const QString name=w->objectName().toLower();
        return cls.contains(QStringLiteral("manometer")) ||
               cls.contains(QStringLiteral("meter")) ||
               cls.contains(QStringLiteral("led")) ||
               cls.contains(QStringLiteral("gauge")) ||
               cls.contains(QStringLiteral("analysisplot")) ||
               name.startsWith(QStringLiteral("moderngaugecard"));
    }

    static QString pageStyle()
    {
        return QStringLiteral(
            "QWidget{background:#0d1116;color:#dce2e7;}"
            "QLabel{background:transparent;color:#dce2e7;border:0;}"
            "QFrame{border-color:#2b343d;}"
            "QGroupBox{background:#11171d;color:#e5e9ec;border:1px solid #2b343d;"
            "border-radius:3px;margin-top:11px;font-weight:600;}"
            "QGroupBox::title{subcontrol-origin:margin;left:10px;padding:0 5px;color:#ff9828;}"
            "QPushButton,QToolButton{background:#151d25;color:#eef2f5;border:1px solid #34414d;"
            "border-radius:2px;padding:5px 10px;font-weight:600;}"
            "QPushButton:hover,QToolButton:hover{background:#1b242d;border-color:#ff8a1c;color:#ffffff;}"
            "QPushButton:pressed,QToolButton:pressed{background:#10161c;}"
            "QPushButton:checked,QToolButton:checked{background:#ff8a1c;color:#101419;border-color:#ff9b32;}"
            "QPushButton:disabled,QToolButton:disabled{background:#1b2229;color:#65707a;border-color:#2a333b;}"
            "QLineEdit,QTextEdit,QPlainTextEdit,QSpinBox,QDoubleSpinBox,QComboBox{background:#0c1217;"
            "color:#f0f3f5;border:1px solid #303b45;border-radius:2px;padding:4px 6px;"
            "selection-background-color:#b85b12;}"
            "QLineEdit:focus,QTextEdit:focus,QPlainTextEdit:focus,QSpinBox:focus,QDoubleSpinBox:focus,QComboBox:focus{"
            "border-color:#ff8a1c;}"
            "QComboBox::drop-down{border:0;width:18px;}"
            "QCheckBox,QRadioButton{background:transparent;color:#dce2e7;spacing:5px;padding:1px 0;}"
            "QCheckBox::indicator,QRadioButton::indicator{width:13px;height:13px;}"
            "QTableWidget,QTableView,QTreeWidget,QListView{background:#0c1217;color:#dce2e7;"
            "alternate-background-color:#111820;border:1px solid #2b343d;gridline-color:#222c34;outline:0;}"
            "QTableWidget::item,QTableView::item,QTreeWidget::item,QListView::item{padding:4px 6px;border:0;}"
            "QTableWidget::item:selected,QTableView::item:selected,QTreeWidget::item:selected,QListView::item:selected{"
            "background:#332313;color:#ffffff;}"
            "QHeaderView::section{background:#151c22;color:#dce2e7;border:0;border-right:1px solid #2b343d;"
            "border-bottom:1px solid #2b343d;padding:5px 7px;font-weight:700;}"
            "QScrollArea{background:#0d1116;border:0;}"
            "QScrollArea>QWidget>QWidget{background:#0d1116;}"
            "QScrollBar:vertical{background:#0b1015;width:9px;margin:0;}"
            "QScrollBar::handle:vertical{background:#34414c;border-radius:2px;min-height:24px;}"
            "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
            "QScrollBar:horizontal{background:#0b1015;height:9px;margin:0;}"
            "QScrollBar::handle:horizontal{background:#34414c;border-radius:2px;min-width:24px;}"
            "QScrollBar::add-line:horizontal,QScrollBar::sub-line:horizontal{width:0;}"
            "QProgressBar{background:#0c1217;color:#dce2e7;border:1px solid #2b343d;border-radius:2px;text-align:center;}"
            "QProgressBar::chunk{background:#ff8a1c;border-radius:1px;}"
            "QSlider::groove:horizontal{height:4px;background:#27313a;border-radius:2px;}"
            "QSlider::handle:horizontal{width:12px;margin:-4px 0;background:#ff8a1c;border:1px solid #ff9b32;border-radius:6px;}"
        );
    }

    static void forceChildStyles(QWidget *page)
    {
        if (!page) return;

        const QString buttonCss=QStringLiteral(
            "QPushButton,QToolButton{background:#151d25;color:#eef2f5;border:1px solid #34414d;border-radius:2px;"
            "padding:5px 10px;font-weight:600;}"
            "QPushButton:hover,QToolButton:hover{background:#1b242d;border-color:#ff8a1c;color:#fff;}"
            "QPushButton:checked,QToolButton:checked{background:#ff8a1c;color:#101419;border-color:#ff9b32;}"
            "QPushButton:disabled,QToolButton:disabled{background:#1b2229;color:#65707a;border-color:#2a333b;}"
        );
        const QString editCss=QStringLiteral(
            "QLineEdit,QTextEdit,QPlainTextEdit,QSpinBox,QDoubleSpinBox,QComboBox{background:#0c1217;color:#f0f3f5;"
            "border:1px solid #303b45;border-radius:2px;padding:4px 6px;selection-background-color:#b85b12;}"
            "QLineEdit:focus,QTextEdit:focus,QPlainTextEdit:focus,QSpinBox:focus,QDoubleSpinBox:focus,QComboBox:focus{border-color:#ff8a1c;}"
        );
        const QString viewCss=QStringLiteral(
            "QTableWidget,QTableView,QTreeWidget,QListView{background:#0c1217;color:#dce2e7;alternate-background-color:#111820;"
            "border:1px solid #2b343d;gridline-color:#222c34;outline:0;}"
            "QTableWidget::item,QTableView::item,QTreeWidget::item,QListView::item{padding:4px 6px;border:0;}"
            "QTableWidget::item:selected,QTableView::item:selected,QTreeWidget::item:selected,QListView::item:selected{background:#332313;color:#fff;}"
            "QHeaderView::section{background:#151c22;color:#dce2e7;border:0;border-right:1px solid #2b343d;border-bottom:1px solid #2b343d;padding:5px 7px;font-weight:700;}"
        );
        const QString groupCss=QStringLiteral(
            "QGroupBox{background:#11171d;color:#e5e9ec;border:1px solid #2b343d;border-radius:3px;margin-top:11px;font-weight:600;}"
            "QGroupBox::title{subcontrol-origin:margin;left:10px;padding:0 5px;color:#ff9828;}"
        );

        const QList<QWidget*> widgets=page->findChildren<QWidget*>();
        for (QWidget *w : widgets) {
            if (!w || isProtectedVisual(w)) continue;
            if (qobject_cast<QAbstractButton*>(w)) {
                w->setStyleSheet(buttonCss);
            } else if (qobject_cast<QAbstractItemView*>(w)) {
                w->setStyleSheet(viewCss);
            } else if (qobject_cast<QLineEdit*>(w) || qobject_cast<QTextEdit*>(w) ||
                       qobject_cast<QPlainTextEdit*>(w) || qobject_cast<QSpinBox*>(w) ||
                       qobject_cast<QDoubleSpinBox*>(w) || qobject_cast<QComboBox*>(w)) {
                w->setStyleSheet(editCss);
            } else if (qobject_cast<QGroupBox*>(w)) {
                w->setStyleSheet(groupCss);
            } else if (QLabel *label=qobject_cast<QLabel*>(w)) {
                if (!label->styleSheet().contains(QStringLiteral("color:#ff"), Qt::CaseInsensitive) &&
                    !label->styleSheet().contains(QStringLiteral("color: red"), Qt::CaseInsensitive) &&
                    !label->styleSheet().contains(QStringLiteral("color:red"), Qt::CaseInsensitive)) {
                    label->setStyleSheet(QStringLiteral("background:transparent;color:#dce2e7;border:0;"));
                }
            }
        }
    }

    static void apply(QMainWindow *window)
    {
        QTabWidget *tabs=window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
        if (!tabs) return;
        const QString style=pageStyle();

        for (int i=0;i<tabs->count();++i) {
            QScrollArea *scroll=nullptr;
            QWidget *page=realPage(tabs->widget(i), &scroll);
            if (!page) continue;

            if (scroll) {
                scroll->setWidgetResizable(true);
                scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
                scroll->setStyleSheet(QStringLiteral("QScrollArea{background:#0d1116;border:0;}"));
                if (scroll->viewport()) {
                    scroll->viewport()->setAutoFillBackground(false);
                    scroll->viewport()->setStyleSheet(QStringLiteral("background:#0d1116;"));
                }
            }

            page->setMinimumSize(0,0);
            page->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
            page->setAttribute(Qt::WA_StyledBackground,true);
            if (page->objectName()!=QStringLiteral("overview_tab") &&
                QString::fromLatin1(page->metaObject()->className())!=QStringLiteral("AnalysisTab")) {
                page->setStyleSheet(style);
                forceChildStyles(page);
            }
        }

        // Some pages are populated after startup. Re-apply the direct control styling
        // a few times so runtime-created diagnostic/table widgets cannot fall back to
        // the legacy white/blue appearance.
        QTimer *refresh=new QTimer(window);
        refresh->setInterval(700);
        int *remaining=new int(6);
        QObject::connect(refresh,&QTimer::timeout,window,[tabs,refresh,remaining](){
            for (int i=0;i<tabs->count();++i) {
                QWidget *page=realPage(tabs->widget(i));
                if (!page) continue;
                if (page->objectName()==QStringLiteral("overview_tab") ||
                    QString::fromLatin1(page->metaObject()->className())==QStringLiteral("AnalysisTab")) continue;
                forceChildStyles(page);
            }
            if (--(*remaining)<=0) {
                refresh->stop();
                delete remaining;
                refresh->deleteLater();
            }
        });
        refresh->start();
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
