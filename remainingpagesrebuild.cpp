#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QEvent>
#include <QFont>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QTableView>
#include <QTableWidget>
#include <QTextEdit>
#include <QTimer>
#include <QWidget>

namespace {

static QWidget *realPage(QWidget *tab)
{
    if (!tab) return nullptr;
    if (QScrollArea *scroll = qobject_cast<QScrollArea*>(tab)) {
        scroll->setWidgetResizable(true);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        return scroll->widget();
    }
    return tab;
}

static bool isTarget(const QString &title)
{
    const QString t = title.toLower();
    return t.contains(QStringLiteral("toutes les mesures")) ||
           t.contains(QStringLiteral("toutes les données")) ||
           t.contains(QStringLiteral("interactif")) ||
           t.contains(QStringLiteral("rosco")) ||
           t.contains(QStringLiteral("diagnostic")) ||
           t.contains(QStringLiteral("enregistre"));
}

static void polishTables(QWidget *page, bool dense)
{
    for (QTableView *v : page->findChildren<QTableView*>()) {
        v->setAlternatingRowColors(true);
        v->setShowGrid(false);
        v->setSelectionBehavior(QAbstractItemView::SelectRows);
        v->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        v->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        v->setMinimumHeight(dense ? 410 : 300);
        v->verticalHeader()->setVisible(false);
        v->verticalHeader()->setDefaultSectionSize(dense ? 27 : 30);
        if (v->horizontalHeader()) {
            v->horizontalHeader()->setMinimumHeight(30);
            v->horizontalHeader()->setStretchLastSection(true);
        }
        v->setStyleSheet(QStringLiteral(
            "QTableView{background:#0a1117;color:#e1e7eb;alternate-background-color:#0f171e;border:1px solid #2d3943;border-radius:4px;gridline-color:#202a32;}"
            "QTableView::item{padding:4px 7px;border-bottom:1px solid #182129;}"
            "QTableView::item:selected{background:#3b2918;color:white;}"
            "QHeaderView::section{background:#151d24;color:#f0f3f5;border:0;border-right:1px solid #29343e;border-bottom:1px solid #29343e;padding:5px 7px;font-weight:700;}"));
    }

    for (QTableWidget *v : page->findChildren<QTableWidget*>()) {
        v->setAlternatingRowColors(true);
        v->setShowGrid(false);
        v->setSelectionBehavior(QAbstractItemView::SelectRows);
        v->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        v->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        v->setMinimumHeight(dense ? 410 : 300);
        v->verticalHeader()->setVisible(false);
        v->verticalHeader()->setDefaultSectionSize(dense ? 27 : 30);
        if (v->horizontalHeader()) {
            v->horizontalHeader()->setMinimumHeight(30);
            v->horizontalHeader()->setStretchLastSection(true);
        }
        v->setStyleSheet(QStringLiteral(
            "QTableWidget{background:#0a1117;color:#e1e7eb;alternate-background-color:#0f171e;border:1px solid #2d3943;border-radius:4px;gridline-color:#202a32;}"
            "QTableWidget::item{padding:4px 7px;border-bottom:1px solid #182129;}"
            "QTableWidget::item:selected{background:#3b2918;color:white;}"
            "QHeaderView::section{background:#151d24;color:#f0f3f5;border:0;border-right:1px solid #29343e;border-bottom:1px solid #29343e;padding:5px 7px;font-weight:700;}"));
    }
}

static void polishEditors(QWidget *page, int minimumHeight)
{
    const QString style = QStringLiteral(
        "background:#090f14;color:#dfe6eb;border:1px solid #2d3943;border-radius:4px;padding:8px;"
        "selection-background-color:#7b3d0c;selection-color:white;");

    for (QPlainTextEdit *e : page->findChildren<QPlainTextEdit*>()) {
        e->setMinimumHeight(minimumHeight);
        e->setStyleSheet(style);
        QFont f = e->font();
        f.setFamily(QStringLiteral("Consolas"));
        f.setPointSizeF(8.5);
        e->setFont(f);
    }
    for (QTextEdit *e : page->findChildren<QTextEdit*>()) {
        e->setMinimumHeight(minimumHeight);
        e->setStyleSheet(style);
    }
}

static void polishControls(QWidget *page)
{
    for (QGroupBox *g : page->findChildren<QGroupBox*>()) {
        g->setStyleSheet(QStringLiteral(
            "QGroupBox{background:#10161c;color:#e7ecef;border:1px solid #2b3741;border-radius:5px;margin-top:15px;font-weight:700;padding-top:4px;}"
            "QGroupBox::title{subcontrol-origin:margin;left:12px;padding:0 6px;color:#ff9828;}"));
    }

    for (QPushButton *b : page->findChildren<QPushButton*>()) {
        b->setMinimumHeight(31);
        b->setMinimumWidth(qMax(88, b->minimumWidth()));
        const QString text = b->text().toLower();
        const bool danger = text.contains(QStringLiteral("effac")) ||
                            text.contains(QStringLiteral("supprim")) ||
                            text.contains(QStringLiteral("clear")) ||
                            text.contains(QStringLiteral("reset"));
        if (danger) {
            b->setStyleSheet(QStringLiteral(
                "QPushButton{background:#351517;color:#f5d9da;border:1px solid #7d2c30;border-radius:3px;padding:6px 12px;font-weight:700;}"
                "QPushButton:hover{background:#4a1b1e;border-color:#c44a50;color:white;}"
                "QPushButton:disabled{background:#191f24;color:#66717a;border-color:#29333c;}"));
        } else {
            b->setStyleSheet(QStringLiteral(
                "QPushButton{background:#151d25;color:#edf2f5;border:1px solid #36434e;border-radius:3px;padding:6px 12px;font-weight:700;}"
                "QPushButton:hover{background:#202932;border-color:#ff7a00;color:white;}"
                "QPushButton:pressed{background:#10161c;}"
                "QPushButton:disabled{background:#191f24;color:#66717a;border-color:#29333c;}"));
        }
    }

    for (QLineEdit *e : page->findChildren<QLineEdit*>()) {
        e->setMinimumHeight(31);
        e->setStyleSheet(QStringLiteral(
            "QLineEdit{background:#091016;color:#edf2f5;border:1px solid #34414c;border-radius:3px;padding:5px 8px;}"
            "QLineEdit:focus{border-color:#ff7a00;}"));
    }

    for (QComboBox *c : page->findChildren<QComboBox*>()) {
        c->setMinimumHeight(31);
    }
}

static void polishLabels(QWidget *page, const QString &title)
{
    const QString t = title.toLower();
    for (QLabel *label : page->findChildren<QLabel*>()) {
        if (label->objectName().startsWith(QStringLiteral("uiRebuild"))) continue;
        label->setStyleSheet(QStringLiteral("color:#dce3e8;background:transparent;"));
        if (label->text().size() > 90) label->setWordWrap(true);

        if (t.contains(QStringLiteral("diagnostic")) &&
            label->text().toLower().contains(QStringLiteral("diagnostic"))) {
            QFont f = label->font();
            f.setBold(true);
            f.setPointSizeF(11.0);
            label->setFont(f);
            label->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;"));
        }
    }
}

static void styleTargetPage(QWidget *page, const QString &title)
{
    if (!page || page->property("remainingVisualRebuild").toBool()) return;
    page->setProperty("remainingVisualRebuild", true);
    page->setMinimumSize(0,0);
    page->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    page->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    page->setAttribute(Qt::WA_StyledBackground,true);
    page->setStyleSheet(QStringLiteral("background:#090e13;color:#e7ecef;"));

    if (QLayout *layout = page->layout()) {
        layout->setContentsMargins(15,12,15,12);
        layout->setSpacing(10);
    }

    const QString t = title.toLower();
    polishControls(page);
    polishLabels(page,title);

    if (t.contains(QStringLiteral("toutes les mesures"))) {
        polishTables(page,true);
        polishEditors(page,360);
        for (QAbstractItemView *v : page->findChildren<QAbstractItemView*>())
            v->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    } else if (t.contains(QStringLiteral("toutes les données"))) {
        polishTables(page,true);
        polishEditors(page,410);
    } else if (t.contains(QStringLiteral("interactif"))) {
        polishTables(page,false);
        polishEditors(page,280);
        for (QPushButton *b : page->findChildren<QPushButton*>()) b->setMinimumWidth(120);
    } else if (t.contains(QStringLiteral("rosco"))) {
        polishTables(page,false);
        polishEditors(page,330);
        for (QPushButton *b : page->findChildren<QPushButton*>()) b->setMinimumWidth(118);
    } else if (t.contains(QStringLiteral("diagnostic"))) {
        polishTables(page,false);
        polishEditors(page,175);
        for (QGroupBox *g : page->findChildren<QGroupBox*>()) g->setMinimumHeight(180);
    } else if (t.contains(QStringLiteral("enregistre"))) {
        polishEditors(page,180);
        for (QLineEdit *e : page->findChildren<QLineEdit*>()) e->setMinimumWidth(260);
        for (QPushButton *b : page->findChildren<QPushButton*>()) b->setMinimumWidth(110);
    }
}

static void applyToTabs(QMainWindow *window)
{
    if (!window) return;
    QTabWidget *tabs = window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
    if (!tabs) return;

    for (int i=0;i<tabs->count();++i) {
        const QString title = tabs->tabText(i).trimmed();
        if (!isTarget(title)) continue;
        QWidget *page = realPage(tabs->widget(i));
        styleTargetPage(page,title);
    }
}

class RemainingPagesInstaller : public QObject
{
public:
    explicit RemainingPagesInstaller(QObject *parent=nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (!watched || (event->type()!=QEvent::Show && event->type()!=QEvent::Polish))
            return QObject::eventFilter(watched,event);

        QMainWindow *window = qobject_cast<QMainWindow*>(watched);
        if (!window || window->objectName()!=QStringLiteral("MainWindow") ||
            window->property("remainingPagesInstaller").toBool())
            return QObject::eventFilter(watched,event);

        window->setProperty("remainingPagesInstaller",true);
        QTimer::singleShot(700,window,[window](){ applyToTabs(window); });
        QTimer::singleShot(1500,window,[window](){ applyToTabs(window); });
        return QObject::eventFilter(watched,event);
    }
};

void installRemainingPages()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app) return;
    app->installEventFilter(new RemainingPagesInstaller(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installRemainingPages)
