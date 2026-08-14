#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QWidget>

namespace {

class AnalysisModernizer : public QObject
{
public:
    explicit AnalysisModernizer(QObject *parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        QWidget *widget = qobject_cast<QWidget*>(watched);
        if (!widget) return QObject::eventFilter(watched, event);

        if ((event->type() == QEvent::Show || event->type() == QEvent::Polish) &&
            QString::fromLatin1(widget->metaObject()->className()) == QStringLiteral("AnalysisTab") &&
            !widget->property("analysisModernized").toBool()) {
            widget->setProperty("analysisModernized", true);
            const qreal initialPointSize = widget->font().pointSizeF() > 0 ? widget->font().pointSizeF() : 9.0;
            widget->setProperty("analysisBaseFontPointSize", initialPointSize);
            QTimer::singleShot(0, widget, [widget]() { apply(widget); });
        }

        if (event->type() == QEvent::Resize && widget->property("analysisModernized").toBool())
            fit(widget);

        return QObject::eventFilter(watched, event);
    }

private:
    static void matchApprovedLayout(QWidget *tab)
    {
        QWidget *legacyPanel = tab->findChild<QWidget*>(QStringLiteral("analysisLeftPanel"));
        if (!legacyPanel || tab->findChild<QWidget*>(QStringLiteral("analysisTopBar"))) return;

        QVBoxLayout *root = qobject_cast<QVBoxLayout*>(tab->layout());
        QHBoxLayout *mainRow = nullptr;
        if (root) {
            for (int i=0; i<root->count(); ++i) {
                if (QLayout *layout = root->itemAt(i)->layout()) {
                    mainRow = qobject_cast<QHBoxLayout*>(layout);
                    if (mainRow) break;
                }
            }
        }
        if (!root || !mainRow) return;

        QPushButton *load = nullptr, *all = nullptr, *none = nullptr, *overlay = nullptr;
        const QList<QPushButton*> buttons = legacyPanel->findChildren<QPushButton*>();
        if (buttons.size() > 0) load = buttons.at(0);
        if (buttons.size() > 1) all = buttons.at(1);
        if (buttons.size() > 2) none = buttons.at(2);
        if (buttons.size() > 3) overlay = buttons.at(3);
        QLabel *fileLabel = nullptr;
        const QList<QLabel*> labels = legacyPanel->findChildren<QLabel*>();
        if (!labels.isEmpty()) fileLabel = labels.at(0);
        QScrollArea *parameterScroll = legacyPanel->findChild<QScrollArea*>();

        QWidget *topBar = new QWidget(tab);
        topBar->setObjectName(QStringLiteral("analysisTopBar"));
        QHBoxLayout *top = new QHBoxLayout(topBar);
        top->setContentsMargins(10,7,10,7);
        top->setSpacing(8);
        if (load) { load->setParent(topBar); top->addWidget(load); }
        if (fileLabel) { fileLabel->setParent(topBar); fileLabel->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred); top->addWidget(fileLabel,1); }
        if (all) { all->setParent(topBar); top->addWidget(all); }
        if (none) { none->setParent(topBar); top->addWidget(none); }
        if (overlay) { overlay->setParent(topBar); top->addWidget(overlay); }
        root->insertWidget(0, topBar);

        QWidget *rightPanel = new QWidget(tab);
        rightPanel->setObjectName(QStringLiteral("analysisRightPanel"));
        QVBoxLayout *right = new QVBoxLayout(rightPanel);
        right->setContentsMargins(10,10,10,10);
        right->setSpacing(7);
        QLabel *title = new QLabel(QStringLiteral("PARAMETRES"), rightPanel);
        title->setObjectName(QStringLiteral("analysisParametersTitle"));
        right->addWidget(title);
        if (parameterScroll) { parameterScroll->setParent(rightPanel); right->addWidget(parameterScroll,1); }

        mainRow->removeWidget(legacyPanel);
        legacyPanel->hide();
        legacyPanel->setMaximumWidth(0);
        mainRow->addWidget(rightPanel);
        mainRow->setStretch(0,1);
        if (mainRow->count() > 1) mainRow->setStretch(1,0);
    }

    static void apply(QWidget *tab)
    {
        matchApprovedLayout(tab);
        tab->setAttribute(Qt::WA_StyledBackground, true);
        tab->setStyleSheet(
            "AnalysisTab{background:#0b0f14;color:#dce2e7;}"
            "QWidget{color:#dce2e7;}"
            "QLabel{background:transparent;border:0;color:#dce2e7;}"
            "#analysisTopBar{background:#111820;border:1px solid #26313b;border-radius:3px;}"
            "#analysisRightPanel{background:#10161d;border:1px solid #26313b;border-radius:3px;}"
            "#analysisParametersTitle{color:#f0f3f5;font-weight:700;padding:3px 0px;border-bottom:1px solid #2b3540;}"
            "QPushButton{background:#17202a;color:#dce2e7;border:1px solid #34414d;"
            "border-radius:3px;padding:5px 10px;font-weight:600;min-height:25px;}"
            "QPushButton:hover{border-color:#ff8a1c;color:#ffffff;}"
            "QPushButton:pressed{background:#202b36;}"
            "QPushButton:checked{background:#ff8a1c;color:#101419;border-color:#ff9b32;}"
            "QScrollArea{background:#0d1218;border:0px;}"
            "QScrollArea>QWidget>QWidget{background:#0d1218;}"
            "QCheckBox{spacing:7px;padding:3px 2px;color:#dce2e7;background:transparent;}"
            "QCheckBox::indicator{width:13px;height:13px;border:1px solid #53606c;border-radius:2px;background:#0b1015;}"
            "QCheckBox::indicator:checked{background:#ff8a1c;border-color:#ff9b32;}"
            "QScrollBar:vertical{background:#0c1116;width:9px;margin:1px;}"
            "QScrollBar::handle:vertical{background:#384550;border-radius:3px;min-height:26px;}"
            "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
            "QScrollBar:horizontal{background:#0c1116;height:9px;margin:1px;}"
            "QScrollBar::handle:horizontal{background:#384550;border-radius:3px;min-width:26px;}"
            "QScrollBar::add-line:horizontal,QScrollBar::sub-line:horizontal{width:0;}"
        );

        const QList<QScrollArea*> scrolls = tab->findChildren<QScrollArea*>();
        for (QScrollArea *scroll : scrolls) {
            scroll->setFrameShape(QFrame::NoFrame);
            scroll->setWidgetResizable(true);
            if (scroll->viewport()) {
                scroll->viewport()->setAutoFillBackground(false);
                scroll->viewport()->setStyleSheet("background:#0d1218;");
            }
        }
        fit(tab);
    }

    static qreal globalScale(QWidget *tab)
    {
        if (!tab) return 1.0;
        QWidget *top = tab->window();
        if (top && top->property("globalUiScale").isValid())
            return qBound<qreal>(0.62, top->property("globalUiScale").toDouble(), 1.16);
        return 1.0;
    }

    static void fit(QWidget *tab)
    {
        const qreal scale = globalScale(tab);
        QWidget *right = tab->findChild<QWidget*>(QStringLiteral("analysisRightPanel"));
        if (right) {
            const int target = qBound(205, qRound(275.0 * scale), 320);
            right->setMinimumWidth(target);
            right->setMaximumWidth(target);
        }
        QWidget *topBar = tab->findChild<QWidget*>(QStringLiteral("analysisTopBar"));
        if (topBar) topBar->setMinimumHeight(qBound(36,qRound(45.0*scale),52));

        const qreal basePointSize = tab->property("analysisBaseFontPointSize").isValid()
            ? tab->property("analysisBaseFontPointSize").toDouble() : 9.0;
        QFont f = tab->font();
        f.setPointSizeF(qMax<qreal>(6.5, basePointSize * scale));
        tab->setFont(f);
        tab->setProperty("analysisScale", scale);

        const int buttonHeight = qBound(22, qRound(28.0 * scale), 33);
        const QList<QPushButton*> buttons = tab->findChildren<QPushButton*>();
        for (QPushButton *button : buttons) button->setMinimumHeight(buttonHeight);

        const QList<QWidget*> children = tab->findChildren<QWidget*>();
        for (QWidget *child : children) {
            const QString className = QString::fromLatin1(child->metaObject()->className());
            if (className == QStringLiteral("SingleChartWidget")) {
                const int chartHeight = qBound(105, qRound(158.0 * scale), 185);
                child->setMinimumHeight(chartHeight);
                child->setMaximumHeight(chartHeight);
            } else if (className == QStringLiteral("ChartWidget")) {
                child->setMinimumHeight(qBound(95, qRound(150.0 * scale), 175));
            }
        }
    }
};

void installAnalysisModernizerHook()
{
    QApplication *app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app) return;
    AnalysisModernizer *modernizer = new AnalysisModernizer(app);
    app->installEventFilter(modernizer);
}

}

Q_COREAPP_STARTUP_FUNCTION(installAnalysisModernizerHook)
