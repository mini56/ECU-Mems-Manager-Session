#include <QApplication>
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
    static void apply(QWidget *tab)
    {
        tab->setAttribute(Qt::WA_StyledBackground, true);
        tab->setStyleSheet(
            "AnalysisTab{background:#0e1318;color:#dce2e7;}"
            "QWidget{color:#dce2e7;}"
            "QLabel{background:transparent;border:0;color:#dce2e7;}"
            "QPushButton{background:#1769d2;color:#ffffff;border:1px solid #2d7ee8;"
            "border-radius:4px;padding:6px 12px;font-weight:600;min-height:28px;}"
            "QPushButton:hover{background:#2378e6;border-color:#4c91ef;}"
            "QPushButton:pressed{background:#1257b0;}"
            "QPushButton:checked{background:#ff8a1c;color:#11151a;border-color:#ff9b32;}"
            "QScrollArea{background:#11161c;border:1px solid #313943;border-radius:6px;}"
            "QScrollArea>QWidget>QWidget{background:#11161c;}"
            "QCheckBox{spacing:7px;padding:4px 2px;color:#dce2e7;background:transparent;}"
            "QCheckBox::indicator{width:14px;height:14px;border:1px solid #47515c;"
            "border-radius:3px;background:#0d1116;}"
            "QCheckBox::indicator:checked{background:#1769d2;border-color:#2d7ee8;}"
            "QScrollBar:vertical{background:#10151a;width:10px;margin:1px;}"
            "QScrollBar::handle:vertical{background:#39434d;border-radius:4px;min-height:28px;}"
            "QScrollBar::handle:vertical:hover{background:#4a5662;}"
            "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
            "QScrollBar:horizontal{background:#10151a;height:10px;margin:1px;}"
            "QScrollBar::handle:horizontal{background:#39434d;border-radius:4px;min-width:28px;}"
            "QScrollBar::handle:horizontal:hover{background:#4a5662;}"
            "QScrollBar::add-line:horizontal,QScrollBar::sub-line:horizontal{width:0;}"
        );

        if (QWidget *left = tab->findChild<QWidget*>(QStringLiteral("analysisLeftPanel")))
            left->setStyleSheet("#analysisLeftPanel{background:#151a20;border:1px solid #313943;border-radius:7px;}");

        const QList<QScrollArea*> scrolls = tab->findChildren<QScrollArea*>();
        for (QScrollArea *scroll : scrolls) {
            scroll->setFrameShape(QFrame::NoFrame);
            scroll->setWidgetResizable(true);
            if (scroll->viewport()) {
                scroll->viewport()->setAutoFillBackground(false);
                scroll->viewport()->setStyleSheet("background:#11161c;");
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
        QWidget *left = tab->findChild<QWidget*>(QStringLiteral("analysisLeftPanel"));
        if (!left) return;

        const qreal scale = globalScale(tab);
        const int target = qBound(190, qRound(300.0 * scale), 348);
        left->setMinimumWidth(target);
        left->setMaximumWidth(target);

        const qreal basePointSize = tab->property("analysisBaseFontPointSize").isValid()
            ? tab->property("analysisBaseFontPointSize").toDouble() : 9.0;
        QFont f = tab->font();
        f.setPointSizeF(qMax<qreal>(6.5, basePointSize * scale));
        tab->setFont(f);
        tab->setProperty("analysisScale", scale);

        const int buttonHeight = qBound(22, qRound(30.0 * scale), 35);
        const QList<QPushButton*> buttons = tab->findChildren<QPushButton*>();
        for (QPushButton *button : buttons) button->setMinimumHeight(buttonHeight);

        const int indicatorSize = qBound(10, qRound(14.0 * scale), 16);
        const QList<QCheckBox*> checks = tab->findChildren<QCheckBox*>();
        for (QCheckBox *check : checks)
            check->setStyleSheet(QStringLiteral("QCheckBox::indicator{width:%1px;height:%1px;}").arg(indicatorSize));

        const QList<QWidget*> children = tab->findChildren<QWidget*>();
        for (QWidget *child : children) {
            const QString className = QString::fromLatin1(child->metaObject()->className());
            if (className == QStringLiteral("SingleChartWidget")) {
                const int chartHeight = qBound(118, qRound(190.0 * scale), 220);
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
