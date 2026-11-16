#include "taskbarautohide.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QProcess>
#include <QSettings>
#include <QThread>

#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#endif

namespace {
const QString kPending=QStringLiteral("TaskbarSession/RestorePending");
const QString kWasHidden=QStringLiteral("TaskbarSession/OriginalAutoHide");
const QString kOwner=QStringLiteral("TaskbarSession/OwnerPid");
const QString kToken=QStringLiteral("TaskbarSession/Token");
bool gEnabled=false;
bool gOwnsChange=false;

void clearSavedState(QSettings &record)
{
    record.remove(QStringLiteral("TaskbarSession"));
    record.sync();
}

#ifdef Q_OS_WIN
int currentState()
{
    APPBARDATA data={};
    data.cbSize=sizeof(data);
    data.hWnd=FindWindowW(L"Shell_TrayWnd",nullptr);
    if(!data.hWnd)
        return -1;
    const UINT_PTR flags=SHAppBarMessage(ABM_GETSTATE,&data);
    return (flags & ABS_AUTOHIDE) ? 1 : 0;
}

bool setState(bool autohide)
{
    APPBARDATA data={};
    data.cbSize=sizeof(data);
    data.hWnd=FindWindowW(L"Shell_TrayWnd",nullptr);
    if(!data.hWnd)
        return false;
    // Keep Windows' always-on-top default. This request changes only the
    // autohide flag; it does NOT hide the Shell_TrayWnd forcibly.
    data.lParam=(autohide ? ABS_AUTOHIDE : 0) | ABS_ALWAYSONTOP;
    SHAppBarMessage(ABM_SETSTATE,&data);
    QThread::msleep(120);
    return currentState()==(autohide ? 1 : 0);
}

bool processStillRunning(DWORD processId)
{
    if(!processId)return false;
    HANDLE process=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,
                               FALSE,processId);
    if(!process)return false;
    const DWORD state=WaitForSingleObject(process,0);
    CloseHandle(process);
    return state==WAIT_TIMEOUT;
}
#endif
}

namespace TaskbarAutoHide {

void recoverPreviousSession()
{
#ifdef Q_OS_WIN
    QSettings record(QSettings::IniFormat,QSettings::UserScope,PROJECTNAME);
    if(!record.value(kPending,false).toBool())
        return;
    const quint32 processId=record.value(kOwner).toUInt();
    // A second instance must never undo the taskbar setting held by an
    // already running process.
    if(processStillRunning(processId) && processId!=GetCurrentProcessId())
        return;
    if(currentState()==1 &&
       !setState(record.value(kWasHidden,false).toBool()))
        return; // retain marker for a later recovery attempt
    clearSavedState(record);
#endif
}

bool apply(bool requested)
{
#ifndef Q_OS_WIN
    return !requested;
#else
    if(requested && gEnabled)
        return true;
    if(!requested){
        if(gOwnsChange){
            QSettings record(QSettings::IniFormat,QSettings::UserScope,PROJECTNAME);
            if(currentState()==1 &&
                !setState(record.value(kWasHidden,false).toBool()))
                return false;
            clearSavedState(record);
        }
        gEnabled=false;
        gOwnsChange=false;
        return true;
    }

    const int existing=currentState();
    if(existing<0)
        return false;
    if(existing==1){
        // The user had already enabled autohide in Windows. Never overwrite
        // that independent preference on application shutdown.
        gEnabled=true;
        return true;
    }
    QSettings record(QSettings::IniFormat,QSettings::UserScope,PROJECTNAME);
    const quint32 pid=GetCurrentProcessId();
    const quint64 token=static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());
    record.setValue(kPending,true);
    record.setValue(kWasHidden,false);
    record.setValue(kOwner,pid);
    record.setValue(kToken,QString::number(token));
    record.sync();
    if(record.status()!=QSettings::NoError)
        return false;

    if(!setState(true)){
        clearSavedState(record);
        return false;
    }

    // Independent watcher runs from the same installed binary, with no
    // QApplication, window, network or ECU access. It wakes on abnormal
    // parent termination and restores the saved Windows state.
    const bool watcher=QProcess::startDetached(
        QCoreApplication::applicationFilePath(),
        QStringList{QStringLiteral("--taskbar-restore-watchdog"),
                    QString::number(pid),QString::number(token)});
    if(!watcher){
        setState(false);
        clearSavedState(record);
        return false;
    }
    gEnabled=true;
    gOwnsChange=true;
    return true;
#endif
}

void restore()
{
    apply(false);
}

int monitorParent(quint32 parentProcessId,quint64 token)
{
#ifndef Q_OS_WIN
    Q_UNUSED(parentProcessId);
    Q_UNUSED(token);
    return 1;
#else
    // This process is deliberately created before QApplication and before
    // the single-instance UI manager: it must have no visible window.
    HANDLE parent=OpenProcess(SYNCHRONIZE,FALSE,parentProcessId);
    if(parent){
        WaitForSingleObject(parent,INFINITE);
        CloseHandle(parent);
    }
    QSettings record(QSettings::IniFormat,QSettings::UserScope,PROJECTNAME);
    const bool matching=record.value(kPending,false).toBool() &&
        record.value(kOwner).toUInt()==parentProcessId &&
        record.value(kToken).toString()==QString::number(token);
    if(!matching)
        return 0; // normal app exit or another session has superseded us
    const bool original=record.value(kWasHidden,false).toBool();
    const bool recovered=currentState()!=1 || setState(original);
    if(recovered)
        clearSavedState(record);
    // If recovery is blocked by Windows, the marker remains for the next
    // normal application launch; do not silently claim restoration.
    return recovered ? 0 : 2;
#endif
}
}
