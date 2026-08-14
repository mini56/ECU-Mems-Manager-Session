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
        if (!widget)
            return QObject::eventFilter(watched, event);

        if ((event->type() == QEvent::Show || event->type() == QEvent::Polish) &&
            QString::fromLatin1(widget->metaObject()->className()) == QStringLiteral("AnalysisTab") &&
            !widget->property("analysisModernized").toBool()) {
            widget->setProperty("analysisModernized", true);
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
            "QScrollBar:vertical{background:#10151a;width:10px;margin:1px;}"
            "QScrollBar::handle:vertical{background:#39434d;border-radius:4px;min-height:28px;}"
            "QScrollBar::handle:vertical:hover{background:#4a5662;}"
            "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
            "QScrollBar:horizontal{background:#10151a;height:10px;margin:1px;}"
            "QScrollBar::handle:horizontal{background:#39434d;border-radius:4px;min-width:28px;}"
            "QScrollBar::add-line:horizontal,QScrollBar::sub-line:horizontal{width:0;}"
        );

        if (QWidget *left = tab->findChild<QWidget*>(QStringLiteral("analysisLeftPanel"))) {
            left->setStyleSheet(
                "#analysisLeftPanel{background:#151a20;border:1px solid #313943;border-radius:7px;}"
            );
        }

        const QList<QScrollArea*> scrolls = tab->findChildren<QScrollArea*>();
        for (QScrollArea *scroll : scrolls) {
            scroll->setFrameShape(QFrame::NoFrame);
            scroll->setWidgetResizable(true);
        }

        tab->installEventFilter(qApp);
        fit(tab);
    }

    static void fit(QWidget *tab)
    {
        QWidget *left = tab->findChild<QWidget*>(QStringLiteral("analysisLeftPanel"));
        if (!left)
            return;

        const int available = qMax(640, tab->width());
        // Preserve the approved composition: selector panel remains about a
        // quarter of the working area, but scales smoothly on smaller screens.
        const int target = qBound(220, qRound(available * 0.245), 330);
        left->setMinimumWidth(target);
        left->setMaximumWidth(target);

        const qreal scale = qBound<qreal>(0.78, available / 1280.0, 1.12);
        QFont f = tab->font();
        const qreal base = f.pointSizeF() > 0 ? f.pointSizeF() : 9.0;
        f.setPointSizeF(qBound<qreal>(8.0, base * scale, 11.0));
        tab->setFont(f);
    }
};

void installAnalysisModernizerHook()
{
    QApplication *app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app)
        return;
    AnalysisModernizer *modernizer = new AnalysisModernizer(app);
    app->installEventFilter(modernizer);
}

}

Q_COREAPP_STARTUP_FUNCTION(installAnalysisModernizerHook)
