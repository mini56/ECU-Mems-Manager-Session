#include "desktopshortcut.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shobjidl.h>
#include <objbase.h>
#endif

static QString shortcutPath()
{
    const QString desktop = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    return QDir(desktop).filePath(QStringLiteral(PROJECTNAME) + ".lnk");
}

void DesktopShortcut::ensureIfEnabled()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, PROJECTNAME);
    settings.beginGroup("Settings");
    const bool enabled = settings.value("DesktopShortcut", false).toBool();
    settings.endGroup();

    if (!enabled)
        return;

    // Refresh the shortcut at each startup while this option is enabled.
    // This also upgrades older shortcuts so they use the icon embedded in
    // the executable instead of an external icon file.
    create();
}

bool DesktopShortcut::create()
{
#ifdef Q_OS_WIN
    const QString exePath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    const QString appDir = QDir::toNativeSeparators(QCoreApplication::applicationDirPath());
    const QString linkPath = QDir::toNativeSeparators(shortcutPath());

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool uninit = SUCCEEDED(hr);
    if (hr == RPC_E_CHANGED_MODE)
        hr = S_OK;
    if (FAILED(hr))
        return false;

    IShellLinkW *shellLink = nullptr;
    hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                          IID_IShellLinkW, reinterpret_cast<void **>(&shellLink));
    if (SUCCEEDED(hr) && shellLink)
    {
        shellLink->SetPath(reinterpret_cast<LPCWSTR>(exePath.utf16()));
        shellLink->SetWorkingDirectory(reinterpret_cast<LPCWSTR>(appDir.utf16()));
        shellLink->SetIconLocation(reinterpret_cast<LPCWSTR>(exePath.utf16()), 0);

        IPersistFile *persistFile = nullptr;
        hr = shellLink->QueryInterface(IID_IPersistFile,
                                       reinterpret_cast<void **>(&persistFile));
        if (SUCCEEDED(hr) && persistFile)
        {
            hr = persistFile->Save(reinterpret_cast<LPCOLESTR>(linkPath.utf16()), TRUE);
            persistFile->Release();
        }
        shellLink->Release();
    }

    if (uninit)
        CoUninitialize();

    return SUCCEEDED(hr);
#else
    return false;
#endif
}
