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
#include "i18n.h"

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
            QTimer::singleShot(250, widget, [widget]() { fit(widget); });
        }

        if (event->type() == QEvent::Resize && widget->property("analysisModernized").toBool())
            QTimer::singleShot(0, widget, [widget]() { fit(widget); });

        return QObject::eventFilter(watched, event);
    }

private:
    static QLabel *sectionTitle(const QString &text, QWidget *parent, const QString &name)
    {
        QLabel *label = new QLabel(text, parent);
        label->setObjectName(name);
        return label;
    }

    static void matchApprovedLayout(QWidget *tab)
    {
        QWidget *legacyPanel = tab->findChild<QWidget*>(QStringLiteral("analysisLeftPanel"));
        if (!legacyPanel || tab->findChild<QWidget*>(QStringLiteral("analysisTopBar"))) return;

        QVBoxLayout *root = qobject_cast<QVBoxLayout*>(tab->layout());
        QHBoxLayout *mainRow = nullptr;
        if (root) {
            for (int i = 0; i < root->count(); ++i) {
                if (QLayout *layout = root->itemAt(i)->layout()) {
                    mainRow = qobject_cast<QHBoxLayout*>(layout);
                    if (mainRow) break;
                }
            }
        }
        if (!root || !mainRow) return;

        root->setContentsMargins(6,6,6,6);
        root->setSpacing(6);
        root->setSizeConstraint(QLayout::SetDefaultConstraint);
        mainRow->setSpacing(6);
        mainRow->setSizeConstraint(QLayout::SetDefaultConstraint);

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
        topBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        QHBoxLayout *top = new QHBoxLayout(topBar);
        top->setContentsMargins(7,4,7,4);
        top->setSpacing(6);
        if (load) { load->setParent(topBar); top->addWidget(load); }
        if (fileLabel) {
            fileLabel->setParent(topBar);
            fileLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            top->addWidget(fileLabel, 1);
        }
        root->insertWidget(0, topBar);

        QWidget *rightPanel = new QWidget(tab);
        rightPanel->setObjectName(QStringLiteral("analysisRightPanel"));
        rightPanel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        QVBoxLayout *right = new QVBoxLayout(rightPanel);
        right->setContentsMargins(7,7,7,7);
        right->setSpacing(5);

        right->addWidget(sectionTitle(I18n::text(7150), rightPanel, QStringLiteral("analysisParametersTitle")));
        if (parameterScroll) {
            parameterScroll->setParent(rightPanel);
            parameterScroll->setWidgetResizable(true);
            parameterScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            parameterScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            right->addWidget(parameterScroll, 1);
        }

        right->addWidget(sectionTitle(I18n::text(7151), rightPanel, QStringLiteral("analysisCursorTitle")));
        right->addWidget(sectionTitle(I18n::text(7100), rightPanel, QStringLiteral("analysisToolsTitle")));

        QVBoxLayout *tools = new QVBoxLayout();
        tools->setContentsMargins(0,0,0,0);
        tools->setSpacing(5);
        QHBoxLayout *selectionTools = new QHBoxLayout();
        selectionTools->setContentsMargins(0,0,0,0);
        selectionTools->setSpacing(5);
        if (all) { all->setParent(rightPanel); selectionTools->addWidget(all); }
        if (none) { none->setParent(rightPanel); selectionTools->addWidget(none); }
        tools->addLayout(selectionTools);
        if (overlay) {
            overlay->setParent(rightPanel);
            overlay->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            tools->addWidget(overlay);
        }
        right->addLayout(tools);

        mainRow->removeWidget(legacyPanel);
        legacyPanel->hide();
        legacyPanel->setMaximumWidth(0);
        mainRow->addWidget(rightPanel);
        for (int i=0;i<mainRow->count();++i) mainRow->setStretch(i, i==mainRow->count()-1 ? 0 : 1);
    }

    static void apply(QWidget *tab)
    {
        matchApprovedLayout(tab);
        tab->setMinimumSize(0,0);
        tab->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        tab->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        tab->setAttribute(Qt::WA_StyledBackground, true);
        tab->setStyleSheet(
            "AnalysisTab{background:#0b0f14;color:#dce2e7;}"
            "QWidget{color:#dce2e7;}"
            "QLabel{background:transparent;border:0;color:#dce2e7;}"
            "#analysisTopBar{background:#10161d;border:1px solid #26313b;border-radius:0px;}"
            "#analysisRightPanel{background:#0f151b;border:1px solid #26313b;border-radius:0px;}"
            "#analysisParametersTitle,#analysisCursorTitle,#analysisToolsTitle{color:#f0f3f5;font-weight:700;padding:3px 0px;border-bottom:1px solid #2b3540;}"
            "QPushButton{background:#17202a;color:#dce2e7;border:1px solid #34414d;border-radius:1px;padding:4px 8px;font-weight:600;min-height:23px;}"
            "QPushButton:hover{border-color:#ff8a1c;color:#ffffff;}"
            "QPushButton:pressed{background:#202b36;}"
            "QPushButton:checked{background:#ff8a1c;color:#101419;border-color:#ff9b32;}"
            "QScrollArea{background:#0d1218;border:0px;}"
            "QScrollArea>QWidget>QWidget{background:#0d1218;}"
            "QCheckBox{spacing:6px;padding:2px 1px;color:#dce2e7;background:transparent;}"
            "QCheckBox::indicator{width:12px;height:12px;border:1px solid #53606c;border-radius:1px;background:#0b1015;}"
            "QCheckBox::indicator:checked{background:#ff8a1c;border-color:#ff9b32;}"
            "QScrollBar:vertical{background:#0c1116;width:8px;margin:1px;}"
            "QScrollBar::handle:vertical{background:#384550;border-radius:1px;min-height:24px;}"
            "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
            "QScrollBar:horizontal{background:#0c1116;height:8px;margin:1px;}"
            "QScrollBar::handle:horizontal{background:#384550;border-radius:1px;min-width:24px;}"
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
            return qBound<qreal>(0.62, top->property("globalUiScale").toDouble(), 1.18);
        return 1.0;
    }

    static void fit(QWidget *tab)
    {
        if (!tab) return;
        const qreal scale = globalScale(tab);

        if (QLayout *root=tab->layout()) {
            const int m=qBound(4,qRound(6.0*scale),8);
            const int gap=qBound(4,qRound(6.0*scale),8);
            root->setContentsMargins(m,m,m,m);
            root->setSpacing(gap);
            root->setSizeConstraint(QLayout::SetDefaultConstraint);
        }

        QWidget *right = tab->findChild<QWidget*>(QStringLiteral("analysisRightPanel"));
        if (right) {
            const int target = qBound(176, qRound(tab->width()*0.19), 292);
            right->setMinimumWidth(target);
            right->setMaximumWidth(target);
            right->setMinimumHeight(0);
            right->setMaximumHeight(QWIDGETSIZE_MAX);
        }

        QWidget *topBar = tab->findChild<QWidget*>(QStringLiteral("analysisTopBar"));
        const int topHeight=qBound(48,qRound(48.0*scale),52);
        if (topBar) {
            topBar->setMinimumHeight(topHeight);
            topBar->setMaximumHeight(topHeight);
        }

        const qreal basePointSize = tab->property("analysisBaseFontPointSize").isValid()
            ? tab->property("analysisBaseFontPointSize").toDouble() : 9.0;
        QFont f = tab->font();
        f.setPointSizeF(qMax<qreal>(6.5, basePointSize * scale));
        tab->setFont(f);
        tab->setProperty("analysisScale", scale);

        const int buttonHeight = qBound(21, qRound(25.0 * scale), 29);
        for (QPushButton *button : tab->findChildren<QPushButton*>()) {
            if (topBar && button->parentWidget() == topBar)
                continue;
            button->setMinimumHeight(buttonHeight);
            button->setMaximumHeight(qMax(buttonHeight,34));
        }

        QList<QWidget*> charts;
        for (QWidget *child : tab->findChildren<QWidget*>()) {
            const QString className = QString::fromLatin1(child->metaObject()->className());
            if (className == QStringLiteral("SingleChartWidget") || className == QStringLiteral("ChartWidget"))
                charts.append(child);
        }
        if (!charts.isEmpty()) {
            const int usable=qMax(260,tab->height()-topHeight-28);
            const int target=qBound(82,(usable-qMax(0,charts.size()-1)*6)/qMax(1,charts.size()),220);
            for (QWidget *chart:charts) {
                chart->setMinimumHeight(target);
                chart->setMaximumHeight(QWIDGETSIZE_MAX);
                chart->setMinimumWidth(0);
                chart->setMaximumWidth(QWIDGETSIZE_MAX);
                chart->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
            }
        }
    }
};

void installAnalysisModernizerHook()
{
    QApplication *app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app) return;
    app->installEventFilter(new AnalysisModernizer(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installAnalysisModernizerHook)
