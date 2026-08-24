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

void logPersistentFiles(const QString &root)
{
    if (root.isEmpty()) {
        appendRaw(QStringLiteral("PERSISTENT_SCAN: AppLocalDataLocation is empty"));
        return;
    }

    QDir directory(root);
    if (!directory.exists()) {
        appendRaw(QStringLiteral("PERSISTENT_SCAN: directory does not exist: %1").arg(root));
        return;
    }

    appendRaw(QStringLiteral("PERSISTENT_SCAN_BEGIN: %1").arg(root));
    QDirIterator it(root, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    int count = 0;
    while (it.hasNext() && count < 250) {
        const QString path = it.next();
        const QFileInfo info(path);
        appendRaw(QStringLiteral("PERSISTENT_FILE: %1 | size=%2 | modified=%3")
                      .arg(QDir(root).relativeFilePath(path))
                      .arg(info.size())
                      .arg(info.lastModified().toString(Qt::ISODateWithMs)));
        ++count;
    }
    if (it.hasNext())
        appendRaw(QStringLiteral("PERSISTENT_SCAN: truncated after 250 files"));
    appendRaw(QStringLiteral("PERSISTENT_SCAN_END: files_logged=%1").arg(count));
}

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
    for (const QString &line : output.split(QRegExp(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts)) {
        if (line.contains(QStringLiteral(":18089")))
            matches.append(line.simplified());
    }
    if (matches.isEmpty())
        appendRaw(QStringLiteral("PORT_18089_NETSTAT: no TCP listener/connection found"));
    else
        for (const QString &line : matches)
            appendRaw(QStringLiteral("PORT_18089_NETSTAT: %1").arg(line));
#else
    appendRaw(QStringLiteral("PORT_18089_NETSTAT: non-Windows build"));
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
    appendRaw(QStringLiteral("AppLocalDataLocation=%1").arg(appLocal));
    appendRaw(QStringLiteral("AppDataLocation=%1").arg(appData));
    appendRaw(QStringLiteral("CacheLocation=%1").arg(cache));
    appendRaw(QStringLiteral("TempLocation=%1").arg(temp));

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, PROJECTNAME);
    const QString settingsPath = settings.fileName();
    const QFileInfo settingsInfo(settingsPath);
    appendRaw(QStringLiteral("QSettings.UserScope.Ini=%1 | exists=%2 | size=%3")
                  .arg(settingsPath)
                  .arg(settingsInfo.exists() ? QStringLiteral("yes") : QStringLiteral("no"))
                  .arg(settingsInfo.exists() ? settingsInfo.size() : 0));

    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString aiServer = env.value(QStringLiteral("MEMS_AI_SERVER"));
    const QString aiModel = env.value(QStringLiteral("MEMS_AI_MODEL"));
    appendRaw(QStringLiteral("ENV MEMS_AI_SERVER=%1").arg(aiServer.isEmpty() ? QStringLiteral("<unset>") : aiServer));
    appendRaw(QStringLiteral("ENV MEMS_AI_MODEL=%1").arg(aiModel.isEmpty() ? QStringLiteral("<unset>") : aiModel));

    logPortOwners();
    logPersistentFiles(appLocal);
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
