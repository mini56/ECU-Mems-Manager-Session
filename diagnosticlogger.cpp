#include "diagnosticlogger.h"

#include <cstdio>
#include <string>

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegExp>
#include <QSettings>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTextStream>
#include <QThread>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {
QString g_logPath;
QMutex g_logMutex;

QString stamp()
{
    return QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
}

void appendRaw(const QString &line)
{
    if (g_logPath.isEmpty())
        return;

    QMutexLocker locker(&g_logMutex);
    QFile file(g_logPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return;
    file.write(line.toUtf8());
    file.write("\r\n");
    file.flush();
}

#ifdef Q_OS_WIN
std::wstring g_crashLogPath;

LONG WINAPI diagnosticUnhandledExceptionFilter(EXCEPTION_POINTERS *info)
{
    if (g_crashLogPath.empty())
        return EXCEPTION_CONTINUE_SEARCH;

    HANDLE file = CreateFileW(g_crashLogPath.c_str(), FILE_APPEND_DATA,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return EXCEPTION_CONTINUE_SEARCH;

    SYSTEMTIME now;
    GetLocalTime(&now);
    DWORD code = 0;
    void *address = nullptr;
    if (info && info->ExceptionRecord) {
        code = info->ExceptionRecord->ExceptionCode;
        address = info->ExceptionRecord->ExceptionAddress;
    }

    char buffer[768] = {};
    const int length = _snprintf_s(
        buffer, sizeof(buffer), _TRUNCATE,
        "%04u-%02u-%02u %02u:%02u:%02u.%03u [CRASH] pid=%lu tid=%lu exception=0x%08lX address=%p\r\n",
        now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond,
        now.wMilliseconds, GetCurrentProcessId(), GetCurrentThreadId(), code, address);

    if (length > 0) {
        DWORD written = 0;
        WriteFile(file, buffer, static_cast<DWORD>(length), &written, nullptr);
        FlushFileBuffers(file);
    }
    CloseHandle(file);
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

void logFilesUnder(const QString &label, const QString &root, int maximum)
{
    if (root.isEmpty()) {
        appendRaw(QStringLiteral("PERSISTENT_SCAN_%1: path is empty").arg(label));
        return;
    }

    QDir directory(root);
    if (!directory.exists()) {
        appendRaw(QStringLiteral("PERSISTENT_SCAN_%1: directory does not exist: %2").arg(label, root));
        return;
    }

    appendRaw(QStringLiteral("PERSISTENT_SCAN_%1_BEGIN: %2").arg(label, root));
    QDirIterator it(root, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    int count = 0;
    while (it.hasNext() && count < maximum) {
        const QString path = it.next();
        const QFileInfo info(path);
        appendRaw(QStringLiteral("PERSISTENT_%1_FILE: %2 | size=%3 | modified=%4")
                      .arg(label, QDir(root).relativeFilePath(path))
                      .arg(info.size())
                      .arg(info.lastModified().toString(Qt::ISODateWithMs)));
        ++count;
    }
    if (it.hasNext())
        appendRaw(QStringLiteral("PERSISTENT_SCAN_%1: truncated after %2 files").arg(label).arg(maximum));
    appendRaw(QStringLiteral("PERSISTENT_SCAN_%1_END: files_logged=%2").arg(label).arg(count));
}

#ifdef Q_OS_WIN
void logTaskForPid(const QString &pid)
{
    if (pid.isEmpty() || pid == QStringLiteral("0"))
        return;

    QProcess task;
    task.start(QStringLiteral("tasklist.exe"),
               QStringList() << QStringLiteral("/FI") << QStringLiteral("PID eq %1").arg(pid)
                             << QStringLiteral("/FO") << QStringLiteral("CSV")
                             << QStringLiteral("/NH"));
    if (!task.waitForFinished(2000)) {
        task.kill();
        task.waitForFinished(300);
        appendRaw(QStringLiteral("PORT_18089_TASKLIST: pid=%1 timeout").arg(pid));
        return;
    }
    const QString taskOutput = QString::fromLocal8Bit(task.readAllStandardOutput()).simplified();
    appendRaw(QStringLiteral("PORT_18089_TASKLIST: pid=%1 %2")
                  .arg(pid, taskOutput.isEmpty() ? QStringLiteral("<no result>") : taskOutput));
}
#endif

void logPortOwners()
{
#ifdef Q_OS_WIN
    QProcess process;
    process.start(QStringLiteral("netstat.exe"),
                  QStringList() << QStringLiteral("-ano") << QStringLiteral("-p") << QStringLiteral("tcp"));
    if (!process.waitForFinished(2500)) {
        process.kill();
        process.waitForFinished(500);
        appendRaw(QStringLiteral("PORT_18089_NETSTAT: timeout"));
        return;
    }

    const QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    QStringList matches;
    QStringList pids;
    for (const QString &line : output.split(QRegExp(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts)) {
        if (!line.contains(QStringLiteral(":18089")))
            continue;
        const QString simple = line.simplified();
        matches.append(simple);
        const QStringList parts = simple.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (!parts.isEmpty()) {
            const QString pid = parts.last();
            if (!pids.contains(pid))
                pids.append(pid);
        }
    }
    if (matches.isEmpty())
        appendRaw(QStringLiteral("PORT_18089_NETSTAT: no TCP listener/connection found"));
    else {
        for (const QString &line : matches)
            appendRaw(QStringLiteral("PORT_18089_NETSTAT: %1").arg(line));
        for (const QString &pid : pids)
            logTaskForPid(pid);
    }
#else
    appendRaw(QStringLiteral("PORT_18089_NETSTAT: non-Windows build"));
#endif
}

void logSettingsValues(QSettings &settings)
{
    settings.beginGroup(QStringLiteral("Settings"));
    const QStringList keys = settings.allKeys();
    if (keys.isEmpty())
        appendRaw(QStringLiteral("QSETTINGS_VALUES: Settings group is empty"));
    for (const QString &key : keys)
        appendRaw(QStringLiteral("QSETTINGS_VALUE: %1=%2").arg(key, settings.value(key).toString()));
    settings.endGroup();
}

void logWindowsEnvironmentRegistry()
{
#ifdef Q_OS_WIN
    QSettings userEnvironment(QStringLiteral("HKEY_CURRENT_USER\\Environment"), QSettings::NativeFormat);
    QSettings machineEnvironment(
        QStringLiteral("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"),
        QSettings::NativeFormat);

    const QString userServer = userEnvironment.value(QStringLiteral("MEMS_AI_SERVER")).toString();
    const QString userModel = userEnvironment.value(QStringLiteral("MEMS_AI_MODEL")).toString();
    const QString machineServer = machineEnvironment.value(QStringLiteral("MEMS_AI_SERVER")).toString();
    const QString machineModel = machineEnvironment.value(QStringLiteral("MEMS_AI_MODEL")).toString();

    appendRaw(QStringLiteral("REGISTRY HKCU MEMS_AI_SERVER=%1")
                  .arg(userServer.isEmpty() ? QStringLiteral("<unset>") : userServer));
    appendRaw(QStringLiteral("REGISTRY HKCU MEMS_AI_MODEL=%1")
                  .arg(userModel.isEmpty() ? QStringLiteral("<unset>") : userModel));
    appendRaw(QStringLiteral("REGISTRY HKLM MEMS_AI_SERVER=%1")
                  .arg(machineServer.isEmpty() ? QStringLiteral("<unset>") : machineServer));
    appendRaw(QStringLiteral("REGISTRY HKLM MEMS_AI_MODEL=%1")
                  .arg(machineModel.isEmpty() ? QStringLiteral("<unset>") : machineModel));
#endif
}
}

namespace DiagnosticLogger
{
void initialize()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    g_logPath = QDir(appDir).filePath(QStringLiteral("MEMS_X64_DIAGNOSTIC.txt"));
    const QString previous = QDir(appDir).filePath(QStringLiteral("MEMS_X64_DIAGNOSTIC_previous.txt"));

    if (QFileInfo::exists(g_logPath)) {
        QFile::remove(previous);
        QFile::copy(g_logPath, previous);
    }
    QFile fresh(g_logPath);
    fresh.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    fresh.close();

#ifdef Q_OS_WIN
    g_crashLogPath = g_logPath.toStdWString();
    SetUnhandledExceptionFilter(diagnosticUnhandledExceptionFilter);
#endif

    appendRaw(QStringLiteral("============================================================"));
    appendRaw(QStringLiteral("ECU MEMS MANAGER x64 DIAGNOSTIC SESSION"));
    appendRaw(QStringLiteral("started=%1").arg(stamp()));
    appendRaw(QStringLiteral("version=%1 build=%2 commit=%3")
                  .arg(QStringLiteral(APP_VERSION), QStringLiteral(APP_BUILD_NUMBER), QStringLiteral(APP_COMMIT_SHA)));
    appendRaw(QStringLiteral("pid=%1").arg(QCoreApplication::applicationPid()));
    appendRaw(QStringLiteral("applicationFilePath=%1").arg(QCoreApplication::applicationFilePath()));
    appendRaw(QStringLiteral("applicationDirPath=%1").arg(appDir));
    appendRaw(QStringLiteral("currentPath=%1").arg(QDir::currentPath()));
    appendRaw(QStringLiteral("prettyProductName=%1").arg(QSysInfo::prettyProductName()));
    appendRaw(QStringLiteral("currentCpuArchitecture=%1").arg(QSysInfo::currentCpuArchitecture()));
    appendRaw(QStringLiteral("buildCpuArchitecture=%1").arg(QSysInfo::buildCpuArchitecture()));

    const QString appLocal = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString cache = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    const QString temp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString documents = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString desktop = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    appendRaw(QStringLiteral("AppLocalDataLocation=%1").arg(appLocal));
    appendRaw(QStringLiteral("AppDataLocation=%1").arg(appData));
    appendRaw(QStringLiteral("CacheLocation=%1").arg(cache));
    appendRaw(QStringLiteral("TempLocation=%1").arg(temp));
    appendRaw(QStringLiteral("DocumentsLocation=%1").arg(documents));
    appendRaw(QStringLiteral("DesktopLocation=%1").arg(desktop));

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, PROJECTNAME);
    const QString settingsPath = settings.fileName();
    const QFileInfo settingsInfo(settingsPath);
    appendRaw(QStringLiteral("QSettings.UserScope.Ini=%1 | exists=%2 | size=%3")
                  .arg(settingsPath)
                  .arg(settingsInfo.exists() ? QStringLiteral("yes") : QStringLiteral("no"))
                  .arg(settingsInfo.exists() ? settingsInfo.size() : 0));
    logSettingsValues(settings);

    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString aiServer = env.value(QStringLiteral("MEMS_AI_SERVER"));
    const QString aiModel = env.value(QStringLiteral("MEMS_AI_MODEL"));
    appendRaw(QStringLiteral("ENV MEMS_AI_SERVER=%1").arg(aiServer.isEmpty() ? QStringLiteral("<unset>") : aiServer));
    appendRaw(QStringLiteral("ENV MEMS_AI_MODEL=%1").arg(aiModel.isEmpty() ? QStringLiteral("<unset>") : aiModel));
    logWindowsEnvironmentRegistry();

    const QString desktopShortcut = QDir(desktop).filePath(QStringLiteral(PROJECTNAME) + QStringLiteral(".lnk"));
    const QFileInfo shortcutInfo(desktopShortcut);
    appendRaw(QStringLiteral("DESKTOP_SHORTCUT=%1 | exists=%2 | size=%3")
                  .arg(desktopShortcut)
                  .arg(shortcutInfo.exists() ? QStringLiteral("yes") : QStringLiteral("no"))
                  .arg(shortcutInfo.exists() ? shortcutInfo.size() : 0));

    logPortOwners();
    logFilesUnder(QStringLiteral("APPLOCAL"), appLocal, 300);
    logFilesUnder(QStringLiteral("DOCUMENTS_MEMS"),
                  QDir(documents).filePath(QStringLiteral("ECU MEMS Manager")), 150);
    appendRaw(QStringLiteral("DIAGNOSTIC_LOG_PATH=%1").arg(g_logPath));
    appendRaw(QStringLiteral("============================================================"));
}

void log(const QString &message)
{
#ifdef Q_OS_WIN
    const quint64 tid = static_cast<quint64>(GetCurrentThreadId());
#else
    const quint64 tid = reinterpret_cast<quintptr>(QThread::currentThreadId());
#endif
    appendRaw(QStringLiteral("%1 [pid=%2 tid=%3] %4")
                  .arg(stamp())
                  .arg(QCoreApplication::applicationPid())
                  .arg(tid)
                  .arg(message));
}

void checkpoint(const QString &name)
{
    log(QStringLiteral("CHECKPOINT: %1").arg(name));
}

QString logPath()
{
    return g_logPath;
}
}
