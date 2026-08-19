#include <QApplication>
#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QPlainTextEdit>
#include <QStandardPaths>
#include <QTimer>
#include <QWidget>

namespace {

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
        if (trimmed == QStringLiteral("Contrôles de sécurité en cours..."))
            return false;
        return true;
    }

    static QString logDirectory()
    {
        QString base = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        if (base.isEmpty())
            base = QDir::homePath();

        const QString path = QDir(base).filePath(QStringLiteral("ECU MEMS Manager/InjectionTests"));
        QDir().mkpath(path);
        return path;
    }

    static void saveLog(QPlainTextEdit *output)
    {
        if (!output || output->property("injectionLogSaveAttempted").toBool())
            return;

        const QString text = output->toPlainText();
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
            if (!dialog || dialog->windowTitle() != QStringLiteral("Test RAM temps injection — AANMP002"))
                continue;

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

static void installInjectionRamTestLogSaver()
{
    if (qApp)
        new InjectionRamTestLogSaver(qApp);
}

} // namespace

Q_COREAPP_STARTUP_FUNCTION(installInjectionRamTestLogSaver)
