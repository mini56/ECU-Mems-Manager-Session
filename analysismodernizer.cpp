#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace {

class AnalysisModernizer : public QObject
{
public:
    explicit AnalysisModernizer(QApplication *app)
        : QObject(app), m_app(app)
    {
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (!event)
            return QObject::eventFilter(watched, event);

        if (!m_tab)
        {
            QWidget *candidate = qobject_cast<QWidget*>(watched);
            if (!candidate || QString::fromLatin1(candidate->metaObject()->className()) != QStringLiteral("AnalysisTab"))
                return QObject::eventFilter(watched, event);

            if (event->type() != QEvent::Show && event->type() != QEvent::Polish)
                return QObject::eventFilter(watched, event);

            m_tab = candidate;
            const qreal pt = candidate->font().pointSizeF() > 0 ? candidate->font().pointSizeF() : 9.0;
            candidate->setProperty("analysisModernized", true);
            candidate->setProperty("analysisBaseFontPointSize", pt);

            // From this point on only the Analysis tab needs to be observed.
            // Stop filtering every QApplication event and keep the resize logic
            // local to the widget it actually belongs to.
            candidate->installEventFilter(this);
            if (m_app)
                m_app->removeEventFilter(this);

            QTimer::singleShot(0, candidate, [this]() {
                if (m_tab)
                    apply(m_tab);
            });
            return QObject::eventFilter(watched, event);
        }

        if (watched == m_tab && event->type() == QEvent::Resize)
        {
            QTimer::singleShot(0, m_tab, [this]() {
                if (m_tab)
                    fit(m_tab);
            });
        }

        return QObject::eventFilter(watched, event);
    }

private:
    static qreal globalScale(QWidget *tab)
    {
        QWidget *top = tab ? tab->window() : nullptr;
        if (top && top->property("globalUiScale").isValid())
            return qBound<qreal>(0.62, top->property("globalUiScale").toDouble(), 1.18);
        return 1.0;
    }

    static void apply(QWidget *tab)
    {
        if (!tab)
            return;

        tab->setMinimumSize(0, 0);
        tab->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        tab->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        tab->setAttribute(Qt::WA_StyledBackground, true);
        tab->setStyleSheet(QStringLiteral(
            "AnalysisTab{background:#0b0f14;color:#dce2e7;}"
            "QWidget{color:#dce2e7;}"
            "QLabel{background:transparent;border:0;color:#dce2e7;}"
            "#analysisRightPanel{background:#0f151b;border:1px solid #26313b;border-radius:0;}"
            "#analysisParametersTitle,#analysisToolsTitle{color:#ff9828;font-weight:700;padding:3px 0;border-bottom:1px solid #2b3540;}"
            "#analysisFileLabel{color:#aeb8bf;font-style:italic;}"
            "QPushButton{background:#17202a;color:#dce2e7;border:1px solid #34414d;border-radius:2px;padding:4px 8px;font-weight:600;min-height:23px;}"
            "QPushButton:hover{border-color:#ff8a1c;color:#fff;}"
            "QPushButton:pressed{background:#202b36;}"
            "QPushButton:checked{background:#ff8a1c;color:#101419;border-color:#ff9b32;}"
            "QPushButton:disabled{background:#111820;color:#5f6972;border-color:#28323b;}"
            "QScrollArea{background:#0d1218;border:0;}"
            "QScrollArea>QWidget>QWidget{background:#0d1218;}"
            "QCheckBox{spacing:6px;padding:2px 1px;color:#dce2e7;background:transparent;}"
            "QScrollBar:vertical{background:#0c1116;width:8px;margin:1px;}"
            "QScrollBar::handle:vertical{background:#384550;border-radius:1px;min-height:24px;}"
            "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
        ));

        const QList<QScrollArea*> scrollAreas = tab->findChildren<QScrollArea*>();
        for (QScrollArea *scroll : scrollAreas)
        {
            scroll->setFrameShape(QFrame::NoFrame);
            scroll->setWidgetResizable(true);
            if (scroll->viewport())
            {
                scroll->viewport()->setAutoFillBackground(false);
                scroll->viewport()->setStyleSheet(QStringLiteral("background:#0d1218;"));
            }
        }

        fit(tab);
    }

    static void fit(QWidget *tab)
    {
        if (!tab)
            return;

        const qreal scale = globalScale(tab);
        if (QLayout *root = tab->layout())
        {
            const int margin = qBound(4, qRound(6.0 * scale), 8);
            root->setContentsMargins(margin, margin, margin, margin);
            root->setSpacing(margin);
            root->setSizeConstraint(QLayout::SetDefaultConstraint);
        }

        QWidget *right = tab->findChild<QWidget*>(QStringLiteral("analysisRightPanel"));
        const int rightWidth = qBound(210, qRound(tab->width() * 0.20), 300);
        if (right)
        {
            right->setMinimumWidth(rightWidth);
            right->setMaximumWidth(rightWidth);
        }

        const qreal base = tab->property("analysisBaseFontPointSize").isValid()
            ? tab->property("analysisBaseFontPointSize").toDouble()
            : 9.0;
        QFont font = tab->font();
        font.setPointSizeF(qMax<qreal>(6.5, base * scale));
        tab->setFont(font);
        tab->setProperty("analysisScale", scale);

        const int buttonHeight = qBound(22, qRound(27.0 * scale), 32);
        const QList<QPushButton*> buttons = tab->findChildren<QPushButton*>();
        for (QPushButton *button : buttons)
        {
            button->setMinimumHeight(buttonHeight);
            button->setMaximumHeight(qMax(buttonHeight, 34));
        }

        const QList<QWidget*> children = tab->findChildren<QWidget*>();
        for (QWidget *child : children)
        {
            const QString cls = QString::fromLatin1(child->metaObject()->className());
            if (cls == QStringLiteral("SingleChartWidget") || cls == QStringLiteral("ChartWidget"))
            {
                child->setMinimumHeight(qBound(105, qRound(170.0 * scale), 220));
                child->setMaximumHeight(QWIDGETSIZE_MAX);
                child->setMinimumWidth(0);
                child->setMaximumWidth(QWIDGETSIZE_MAX);
                child->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            }
        }
    }

    QApplication *m_app;
    QPointer<QWidget> m_tab;
};

void installAnalysisModernizerHook()
{
    QApplication *app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (app)
        app->installEventFilter(new AnalysisModernizer(app));
}

} // namespace

Q_COREAPP_STARTUP_FUNCTION(installAnalysisModernizerHook)
