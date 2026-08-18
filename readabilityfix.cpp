#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QLabel>
#include <QMainWindow>
#include <QStringList>
#include <QTableWidget>
#include <QTimer>
#include <QWidget>

namespace {

static void applyRequestedReadableText(QMainWindow *window)
{
    if (!window)
        return;

    QWidget *rawPage = window->findChild<QWidget*>(QStringLiteral("raw"));
    if (rawPage) {
        const QStringList blocks = { QStringLiteral("raw_1"), QStringLiteral("raw_2") };
        for (const QString &blockName : blocks) {
            QWidget *block = rawPage->findChild<QWidget*>(blockName);
            if (!block)
                continue;

            for (QLabel *label : block->findChildren<QLabel*>()) {
                const QString name = label->objectName();
                const bool header = name.startsWith(QStringLiteral("header_")) ||
                                    name.startsWith(QStringLiteral("Aheader_"));
                if (!header)
                    label->setStyleSheet(QStringLiteral("color:#ffffff;background:transparent;"));
            }
        }
    }

    QWidget *diagnosticPage = nullptr;
    for (QWidget *widget : window->findChildren<QWidget*>()) {
        if (QString::fromLatin1(widget->metaObject()->className()) == QStringLiteral("DiagnosticPanel")) {
            diagnosticPage = widget;
            break;
        }
    }

    if (diagnosticPage) {
        QTableWidget *table = diagnosticPage->findChild<QTableWidget*>();
        if (table) {
            const QString readableRule = QStringLiteral(
                "QTableWidget{color:#ffffff;}"
                "QTableWidget::item{color:#ffffff;}");
            QString style = table->styleSheet();
            if (!style.contains(readableRule)) {
                style += readableRule;
                table->setStyleSheet(style);
            }
        }
    }
}

class ReadabilityFixInstaller : public QObject
{
public:
    explicit ReadabilityFixInstaller(QObject *parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        QMainWindow *window = qobject_cast<QMainWindow*>(watched);
        if (!window || window->objectName() != QStringLiteral("MainWindow"))
            return QObject::eventFilter(watched, event);

        if ((event->type() == QEvent::Show || event->type() == QEvent::Polish) &&
            !window->property("requestedReadableTextScheduled").toBool()) {
            window->setProperty("requestedReadableTextScheduled", true);
            QTimer::singleShot(1500, window, [window]() { applyRequestedReadableText(window); });
        } else if (event->type() == QEvent::Resize &&
                   window->property("requestedReadableTextScheduled").toBool()) {
            QTimer::singleShot(120, window, [window]() { applyRequestedReadableText(window); });
        }

        return QObject::eventFilter(watched, event);
    }
};

void installRequestedReadableTextFix()
{
    QApplication *app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (app)
        app->installEventFilter(new ReadabilityFixInstaller(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installRequestedReadableTextFix)
