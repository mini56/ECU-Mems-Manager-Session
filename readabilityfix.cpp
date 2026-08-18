#include <QApplication>
#include <QColor>
#include <QCoreApplication>
#include <QEvent>
#include <QLabel>
#include <QMainWindow>
#include <QStringList>
#include <QTableWidget>
#include <QTimer>
#include <QWidget>

namespace {

static void forceReadableTable(QTableWidget *table)
{
    if (!table)
        return;

    const QString readableRule = QStringLiteral(
        "QTableWidget{color:#ffffff;}"
        "QTableWidget:disabled{color:#ffffff;}"
        "QTableWidget::item{color:#ffffff;}"
        "QTableWidget::item:disabled{color:#ffffff;}");

    QString style = table->styleSheet();
    if (!style.contains(readableRule)) {
        style += readableRule;
        table->setStyleSheet(style);
    }

    for (int row = 0; row < table->rowCount(); ++row) {
        for (int column = 0; column < table->columnCount(); ++column) {
            if (QTableWidgetItem *item = table->item(row, column))
                item->setData(Qt::ForegroundRole, QColor(QStringLiteral("#ffffff")));
        }
    }
}

static void applyRequestedReadableText(QMainWindow *window)
{
    if (!window)
        return;

    // Toutes les mesures : rendre lisibles uniquement les lignes des 3 tableaux.
    for (QWidget *widget : window->findChildren<QWidget*>()) {
        if (QString::fromLatin1(widget->metaObject()->className()) == QStringLiteral("SummaryTab")) {
            const QList<QTableWidget*> tables = widget->findChildren<QTableWidget*>();
            for (QTableWidget *table : tables)
                forceReadableTable(table);
            break;
        }
    }

    // Toutes les données : conserver les en-têtes orange et passer les lignes en blanc.
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

    // Diagnostic automatique : rendre lisibles uniquement les lignes du tableau.
    for (QWidget *widget : window->findChildren<QWidget*>()) {
        if (QString::fromLatin1(widget->metaObject()->className()) == QStringLiteral("DiagnosticPanel")) {
            forceReadableTable(widget->findChild<QTableWidget*>());
            break;
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
