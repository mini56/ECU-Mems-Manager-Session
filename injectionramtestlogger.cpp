#include "mainwindow.h"
#include "i18n.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QGridLayout>
#include <QGroupBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTimer>
#include <QUrl>
#include <QWidget>

namespace {

static QString logDirectory()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (base.isEmpty())
        base = QDir::homePath();

    const QString path = QDir(base).filePath(QStringLiteral("ECU MEMS Manager/InjectionTests"));
    QDir().mkpath(path);
    return path;
}

class InjectionRamTestLogSaver : public QObject
{
public:
    explicit InjectionRamTestLogSaver(QObject *parent = nullptr)
        : QObject(parent)
    {
        QTimer *timer = new QTimer(this);
        timer->setInterval(50);
        connect(timer, &QTimer::timeout, this, [this]() { scanDialogs(); });
        timer->start();
    }

private:
    static bool isFinalLog(const QString &text)
    {
        const QString trimmed = text.trimmed();
        if (trimmed.isEmpty())
            return false;

        if (trimmed == QStringLiteral("Contrôles de sécurité en cours...") ||
            trimmed == QStringLiteral("Contrôles de sécurité et lecture dynamique en cours..."))
        {
            return false;
        }

        return trimmed.startsWith(QStringLiteral("TEST RAM TEMPS INJECTION — AANMP002")) ||
               trimmed.startsWith(QStringLiteral("TEST DYNAMIQUE RAM TEMPS INJECTION — AANMP002"));
    }

    static void saveLog(QPlainTextEdit *output)
    {
        if (!output)
            return;

        const QString text = output->toPlainText();

        // Build #672: phase 1 and phase 2 use the same output widget. The phase-1
        // save sets injectionLogSaveAttempted=true; re-arm it when phase 2 starts
        // so the final phase-2 journal is written to a second file.
        if (text.trimmed().startsWith(QStringLiteral("Phase 2 :")))
        {
            output->setProperty("injectionLogSaveAttempted", false);
            return;
        }

        if (output->property("injectionLogSaveAttempted").toBool())
            return;

        if (!isFinalLog(text))
            return;

        output->setProperty("injectionLogSaveAttempted", true);

        const QString fileName = QStringLiteral("Injection_RAM_AANMP002_%1.txt")
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss-zzz")));
        const QString path = QDir(logDirectory()).filePath(fileName);

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        {
            output->appendPlainText(QStringLiteral("\nERREUR : impossible d'enregistrer automatiquement le journal du test."));
            return;
        }

        QByteArray bytes = text.toUtf8();
        if (!bytes.endsWith('\n'))
            bytes.append('\n');

        const qint64 written = file.write(bytes);
        file.close();

        if (written != bytes.size())
        {
            output->appendPlainText(QStringLiteral("\nERREUR : écriture incomplète du journal du test."));
            return;
        }

        output->setProperty("injectionLogSavedPath", path);
        output->appendPlainText(
            QStringLiteral("\nFichier enregistré automatiquement :\n%1")
                .arg(QDir::toNativeSeparators(path)));
    }

    void scanDialogs()
    {
        for (QWidget *top : QApplication::topLevelWidgets())
        {
            QDialog *dialog = qobject_cast<QDialog*>(top);
            if (!dialog)
                continue;

            const QString title = dialog->windowTitle();
            if (title != QStringLiteral("Test RAM temps injection — AANMP002") &&
                title != QStringLiteral("Test dynamique temps injection — AANMP002"))
            {
                continue;
            }

            QPlainTextEdit *output = dialog->findChild<QPlainTextEdit*>();
            if (!output || output->property("injectionLogAutoSaveConnected").toBool())
                continue;

            output->setProperty("injectionLogAutoSaveConnected", true);
            connect(output, &QPlainTextEdit::textChanged, output, [output]() {
                saveLog(output);
            });

            // If the final result was displayed before this watcher attached,
            // save it immediately as well.
            saveLog(output);
        }
    }
};

static QWidget *realTabPage(QWidget *tab)
{
    if (!tab)
        return nullptr;
    if (QScrollArea *scroll = qobject_cast<QScrollArea*>(tab))
        return scroll->widget();
    return tab;
}

class InjectionTestFolderButtonInstaller : public QObject
{
public:
    explicit InjectionTestFolderButtonInstaller(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event && event->type() == QEvent::Show)
        {
            if (MainWindow *window = qobject_cast<MainWindow*>(watched))
                installButton(window);
        }
        return QObject::eventFilter(watched, event);
    }

private:
    static bool installButton(MainWindow *window)
    {
        if (!window)
            return false;

        QTabWidget *tabs = window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
        if (!tabs)
            return false;

        QGroupBox *session = nullptr;
        for (int i = 0; i < tabs->count() && !session; ++i)
        {
            QWidget *page = realTabPage(tabs->widget(i));
            if (!page)
                continue;

            const QList<QGroupBox*> groups = page->findChildren<QGroupBox*>();
            for (QGroupBox *group : groups)
            {
                if (group && group->title().trimmed() == I18n::text(7000).trimmed())
                {
                    session = group;
                    break;
                }
            }
        }

        if (!session)
            return false;

        if (session->findChild<QPushButton*>(QStringLiteral("injectionTestFolderButton")))
            return true;

        QGridLayout *grid = qobject_cast<QGridLayout*>(session->layout());
        if (!grid)
            return false;

        const QString folder = logDirectory();
        QPushButton *button = new QPushButton(QStringLiteral("Ouvrir le dossier des tests"), session);
        button->setObjectName(QStringLiteral("injectionTestFolderButton"));
        button->setMinimumHeight(30);
        button->setToolTip(QDir::toNativeSeparators(folder));
        grid->addWidget(button, grid->rowCount(), 0, 1, 4);

        QObject::connect(button, &QPushButton::clicked, session, []()
        {
            const QString path = logDirectory();
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        });

        return true;
    }
};

static void installInjectionRamTestSupport()
{
    QApplication *app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app)
        return;

    new InjectionRamTestLogSaver(app);
    app->installEventFilter(new InjectionTestFolderButtonInstaller(app));
}

} // namespace

Q_COREAPP_STARTUP_FUNCTION(installInjectionRamTestSupport)
