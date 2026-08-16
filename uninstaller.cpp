#include <QApplication>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSet>
#include <QStringList>

#include <algorithm>

#include "i18n.h"

namespace
{
QString batchQuoted(QString path)
{
    path = QDir::toNativeSeparators(path);
    path.replace(QLatin1Char('%'), QStringLiteral("%%"));
    return QStringLiteral("\"") + path + QStringLiteral("\"");
}

bool validRelativePath(const QString &path)
{
    if (path.trimmed().isEmpty() || QDir::isAbsolutePath(path))
        return false;

    const QString clean = QDir::cleanPath(path);
    return clean != QStringLiteral("..") &&
           !clean.startsWith(QStringLiteral("../")) &&
           !clean.startsWith(QStringLiteral("..\\"));
}

QStringList loadInstallManifest(const QString &appDir)
{
    QFile manifest(QDir(appDir).filePath(QStringLiteral("install_manifest.txt")));
    if (!manifest.open(QIODevice::ReadOnly | QIODevice::Text))
        return QStringList();

    QStringList files;
    QTextStream stream(&manifest);
    stream.setCodec("UTF-8");
    while (!stream.atEnd())
    {
        const QString relative = stream.readLine().trimmed();
        if (!validRelativePath(relative))
            continue;
        files.append(QDir::cleanPath(relative));
    }
    files.removeDuplicates();
    return files;
}

QStringList manifestDirectories(const QStringList &files)
{
    QSet<QString> directories;
    for (const QString &relative : files)
    {
        QString dir = QFileInfo(relative).path();
        while (!dir.isEmpty() && dir != QStringLiteral("."))
        {
            directories.insert(QDir::cleanPath(dir));
            dir = QFileInfo(dir).path();
        }
    }

    QStringList result = directories.values();
    std::sort(result.begin(), result.end(), [](const QString &a, const QString &b) {
        const int depthA = a.count(QLatin1Char('/')) + a.count(QLatin1Char('\\'));
        const int depthB = b.count(QLatin1Char('/')) + b.count(QLatin1Char('\\'));
        if (depthA != depthB)
            return depthA > depthB;
        return a.size() > b.size();
    });
    return result;
}

bool mainApplicationRunning()
{
#ifdef Q_OS_WIN
    QProcess process;
    process.start(QStringLiteral("tasklist.exe"),
                  QStringList() << QStringLiteral("/FI")
                                << QStringLiteral("IMAGENAME eq ecu_mems_manager.exe")
                                << QStringLiteral("/FO") << QStringLiteral("CSV")
                                << QStringLiteral("/NH"));
    if (!process.waitForFinished(3000))
        return false;
    return QString::fromLocal8Bit(process.readAllStandardOutput())
        .contains(QStringLiteral("ecu_mems_manager.exe"), Qt::CaseInsensitive);
#else
    return false;
#endif
}

bool safeApplicationDataPath(const QString &path)
{
    const QString clean = QDir::cleanPath(path);
    const QString home = QDir::cleanPath(QDir::homePath());
    if (clean.isEmpty() || home.isEmpty())
        return false;
#ifdef Q_OS_WIN
    return clean.startsWith(home + QLatin1Char('/'), Qt::CaseInsensitive) ||
           clean.startsWith(home + QLatin1Char('\\'), Qt::CaseInsensitive);
#else
    return clean.startsWith(home + QLatin1Char('/'));
#endif
}

bool writeRemovalScript(const QString &scriptPath,
                        const QString &appDir,
                        const QStringList &files,
                        bool keepProfile)
{
    QFile script(scriptPath);
    if (!script.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return false;

    QTextStream out(&script);
    out.setCodec("UTF-8");
    out << "@echo off\r\n";
    out << "setlocal DisableDelayedExpansion\r\n";
    out << "ping 127.0.0.1 -n 3 >nul\r\n";

    const QString desktopShortcut = QDir(
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation))
        .filePath(QStringLiteral(PROJECTNAME) + QStringLiteral(".lnk"));
    out << "del /f /q " << batchQuoted(desktopShortcut) << " >nul 2>&1\r\n";

    if (!keepProfile)
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, PROJECTNAME);
        const QString settingsFile = settings.fileName();
        if (!settingsFile.isEmpty() && safeApplicationDataPath(settingsFile))
            out << "del /f /q " << batchQuoted(settingsFile) << " >nul 2>&1\r\n";

        const QString localData = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
        if (!localData.isEmpty() && safeApplicationDataPath(localData))
            out << "rmdir /s /q " << batchQuoted(localData) << " >nul 2>&1\r\n";
    }

    for (const QString &relative : files)
    {
        const QString fullPath = QDir(appDir).filePath(relative);
        out << "del /f /q " << batchQuoted(fullPath) << " >nul 2>&1\r\n";
    }

    const QStringList directories = manifestDirectories(files);
    for (const QString &relativeDir : directories)
    {
        const QString fullDir = QDir(appDir).filePath(relativeDir);
        out << "rmdir " << batchQuoted(fullDir) << " >nul 2>&1\r\n";
    }

    // The application directory is removed only when empty. Any unrelated
    // file that the user placed next to the application is preserved.
    out << "rmdir " << batchQuoted(appDir) << " >nul 2>&1\r\n";
    out << "del /f /q \"%~f0\" >nul 2>&1\r\n";
    out << "endlocal\r\n";
    return true;
}
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("ECU Mems Manager"));
    QApplication::setApplicationVersion(QStringLiteral(APP_VERSION));
    QApplication::setOrganizationName(QStringLiteral("ECU Mems Manager"));

    QSettings languageSettings(QSettings::IniFormat, QSettings::UserScope, PROJECTNAME);
    languageSettings.beginGroup(QStringLiteral("Settings"));
    QString language = languageSettings.value(QStringLiteral("Language"), QStringLiteral("fr"))
                           .toString().toLower();
    languageSettings.endGroup();
    const QStringList supported = QStringList()
        << QStringLiteral("fr") << QStringLiteral("en") << QStringLiteral("es")
        << QStringLiteral("it") << QStringLiteral("pt") << QStringLiteral("de");
    if (!supported.contains(language))
        language = QStringLiteral("fr");
    I18n::load(language);
    I18n::install(&app);

    if (mainApplicationRunning())
    {
        QMessageBox::warning(nullptr, I18n::text(33), I18n::text(42));
        return 1;
    }

    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList manifestFiles = loadInstallManifest(appDir);
    if (manifestFiles.isEmpty())
    {
        QMessageBox::critical(nullptr, I18n::text(33), I18n::text(40));
        return 1;
    }

    QDialog dialog;
    dialog.setWindowTitle(I18n::text(33));
    dialog.setMinimumWidth(540);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QLabel *description = new QLabel(I18n::text(34), &dialog);
    description->setWordWrap(true);
    layout->addWidget(description);

    QCheckBox *keepProfile = new QCheckBox(I18n::text(35), &dialog);
    keepProfile->setChecked(true);
    layout->addWidget(keepProfile);

    QLabel *details = new QLabel(I18n::text(36), &dialog);
    details->setWordWrap(true);
    layout->addWidget(details);

    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addStretch(1);
    QPushButton *cancelButton = new QPushButton(I18n::text(38), &dialog);
    QPushButton *uninstallButton = new QPushButton(I18n::text(37), &dialog);
    uninstallButton->setDefault(true);
    buttons->addWidget(cancelButton);
    buttons->addWidget(uninstallButton);
    layout->addLayout(buttons);

    QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(uninstallButton, &QPushButton::clicked, &dialog, [&]() {
        const QString confirmation = keepProfile->isChecked() ? I18n::text(39) : I18n::text(43);
        if (QMessageBox::question(&dialog, I18n::text(33), confirmation,
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No) != QMessageBox::Yes)
            return;

        const QString tempRoot = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        const QString scriptPath = QDir(tempRoot).filePath(
            QStringLiteral("ecu_mems_uninstall_%1.cmd").arg(QCoreApplication::applicationPid()));

        if (!writeRemovalScript(scriptPath, appDir, manifestFiles, keepProfile->isChecked()))
        {
            QMessageBox::critical(&dialog, I18n::text(33), I18n::text(41));
            return;
        }

        const bool started = QProcess::startDetached(
            QStringLiteral("cmd.exe"),
            QStringList() << QStringLiteral("/C") << QDir::toNativeSeparators(scriptPath),
            tempRoot);
        if (!started)
        {
            QFile::remove(scriptPath);
            QMessageBox::critical(&dialog, I18n::text(33), I18n::text(41));
            return;
        }

        dialog.accept();
    });

    dialog.exec();
    return 0;
}
