#include <QAbstractButton>
#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QString>
#include <QTimer>

#include "i18n.h"
#include "optionsdialog.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {

static void applyNativeTitleBar(QWidget *window, bool dark)
{
#ifdef Q_OS_WIN
    if (!window)
        return;

    const BOOL useDark = dark ? TRUE : FALSE;
    HWND hwnd = reinterpret_cast<HWND>(window->winId());
    HMODULE dwmApi = LoadLibraryW(L"dwmapi.dll");
    if (!dwmApi)
        return;

    typedef HRESULT (WINAPI *DwmSetWindowAttributeFn)(HWND, DWORD, LPCVOID, DWORD);
    DwmSetWindowAttributeFn setAttribute = reinterpret_cast<DwmSetWindowAttributeFn>(
        GetProcAddress(dwmApi, "DwmSetWindowAttribute"));
    if (setAttribute)
    {
        const DWORD attributes[] = {20, 19};
        for (DWORD attribute : attributes)
        {
            if (SUCCEEDED(setAttribute(hwnd, attribute, &useDark, sizeof(useDark))))
                break;
        }
    }

    FreeLibrary(dwmApi);
#else
    Q_UNUSED(window);
    Q_UNUSED(dark);
#endif
}

static void applyMainWindowTitleBar(QMainWindow *window)
{
    const bool dark = qApp && qApp->property("ecuDarkTheme").toBool();
    applyNativeTitleBar(window, dark);
}

static bool isPrimaryMainWindow(QMainWindow *window)
{
    return window && QString::fromLatin1(window->metaObject()->className()) == QStringLiteral("MainWindow");
}

static QString serialPortForDisplay(OptionsDialog *options)
{
    QString port = options ? options->getSerialDeviceName().trimmed() : QString();
#ifdef Q_OS_WIN
    const QString windowsPrefix = QStringLiteral("\\\\.\\");
    if (port.startsWith(windowsPrefix, Qt::CaseInsensitive))
        port.remove(0, windowsPrefix.size());
#endif
    return port.isEmpty() ? QStringLiteral("--") : port;
}

static void syncConnectionHeader(QMainWindow *window)
{
    if (!isPrimaryMainWindow(window))
        return;

    QPushButton *connectButton = window->findChild<QPushButton*>(QStringLiteral("m_connectButton"));
    QPushButton *disconnectButton = window->findChild<QPushButton*>(QStringLiteral("m_disconnectButton"));
    QAbstractButton *goodLed = window->findChild<QAbstractButton*>(QStringLiteral("m_commsGoodLed"));
    QAbstractButton *badLed = window->findChild<QAbstractButton*>(QStringLiteral("m_commsBadLed"));
    QLabel *portLabel = window->findChild<QLabel*>(QStringLiteral("uiRebuildPortLabel"));
    OptionsDialog *options = window->findChild<OptionsDialog*>();

    if (portLabel && options)
    {
        const QString wantedPortText = I18n::text(7114) + QStringLiteral("\n") + serialPortForDisplay(options);
        if (portLabel->text() != wantedPortText)
            portLabel->setText(wantedPortText);
    }

    const QString connectionButtonStyle = QStringLiteral(
        "QPushButton{background:#ff7a00;color:#11161a;border:1px solid #ff9828;border-radius:3px;padding:6px 11px;font-weight:800;}"
        "QPushButton:hover{background:#ff9828;}"
        "QPushButton:pressed{background:#d96500;}"
        "QPushButton:disabled{background:#171e24;color:#5f6972;border-color:#29323a;}");

    if (connectButton && !connectButton->property("ecuConnectionStyleInstalled").toBool())
    {
        connectButton->setStyleSheet(connectionButtonStyle);
        connectButton->setProperty("ecuConnectionStyleInstalled", true);
    }
    if (disconnectButton && !disconnectButton->property("ecuConnectionStyleInstalled").toBool())
    {
        disconnectButton->setStyleSheet(connectionButtonStyle);
        disconnectButton->setProperty("ecuConnectionStyleInstalled", true);
    }

    if (connectButton && disconnectButton && goodLed && badLed)
    {
        const bool disconnected = connectButton->isEnabled() && !disconnectButton->isEnabled();
        if (disconnected)
        {
            if (goodLed->isChecked())
                goodLed->setChecked(false);
            if (!badLed->isChecked())
                badLed->setChecked(true);
        }
        else if (goodLed->isChecked() && badLed->isChecked())
        {
            badLed->setChecked(false);
        }
    }
}

struct ExitConfirmationText
{
    QString title;
    QString message;
    QString yes;
    QString no;
};

static ExitConfirmationText exitConfirmationText()
{
    const QString language = I18n::language().toLower();

    if (language == QStringLiteral("en"))
        return {QStringLiteral("Exit ECU MEMS Manager"),
                QStringLiteral("Do you really want to exit ECU MEMS Manager?"),
                QStringLiteral("Yes"), QStringLiteral("No")};
    if (language == QStringLiteral("es"))
        return {QString::fromUtf8("Salir de ECU MEMS Manager"),
                QString::fromUtf8("¿Realmente desea salir de ECU MEMS Manager?"),
                QString::fromUtf8("Sí"), QStringLiteral("No")};
    if (language == QStringLiteral("it"))
        return {QStringLiteral("Uscire da ECU MEMS Manager"),
                QStringLiteral("Vuoi davvero uscire da ECU MEMS Manager?"),
                QString::fromUtf8("Sì"), QStringLiteral("No")};
    if (language == QStringLiteral("pt"))
        return {QStringLiteral("Sair do ECU MEMS Manager"),
                QStringLiteral("Deseja realmente sair do ECU MEMS Manager?"),
                QStringLiteral("Sim"), QString::fromUtf8("Não")};
    if (language == QStringLiteral("de"))
        return {QStringLiteral("ECU MEMS Manager beenden"),
                QString::fromUtf8("Möchten Sie ECU MEMS Manager wirklich beenden?"),
                QStringLiteral("Ja"), QStringLiteral("Nein")};

    return {QStringLiteral("Quitter ECU MEMS Manager"),
            QString::fromUtf8("Voulez-vous vraiment quitter ECU MEMS Manager ?"),
            QStringLiteral("Oui"), QStringLiteral("Non")};
}

static bool confirmMainWindowClose(QMainWindow *window)
{
    if (!window)
        return true;

    const ExitConfirmationText text = exitConfirmationText();
    QMessageBox box(QMessageBox::Question, text.title, text.message,
                    QMessageBox::NoButton, window);

    QPushButton *yesButton = box.addButton(text.yes, QMessageBox::YesRole);
    QPushButton *noButton = box.addButton(text.no, QMessageBox::NoRole);
    box.setDefaultButton(noButton);
    box.setEscapeButton(noButton);

    const bool dark = qApp && qApp->property("ecuDarkTheme").toBool();
    if (dark)
    {
        box.setStyleSheet(QStringLiteral(
            "QMessageBox{background:#090e13;color:#e7ecef;}"
            "QMessageBox QLabel{color:#e7ecef;background:transparent;}"
            "QMessageBox QPushButton{background:#151d25;color:#edf2f5;border:1px solid #35414c;border-radius:5px;padding:6px 16px;font-weight:700;min-width:72px;}"
            "QMessageBox QPushButton:hover{background:#1b252e;border-color:#ff7a00;}"));
        yesButton->setStyleSheet(QStringLiteral(
            "QPushButton{background:#ff7a00;color:#101419;border:1px solid #ff9828;border-radius:5px;padding:6px 16px;font-weight:700;min-width:72px;}"
            "QPushButton:hover{background:#ff9828;}"
            "QPushButton:pressed{background:#d96500;}"));
    }

    applyNativeTitleBar(&box, dark);
    box.exec();
    return box.clickedButton() == yesButton;
}

class MainWindowTitleBarFix : public QObject
{
public:
    explicit MainWindowTitleBarFix(QObject *parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        QMainWindow *window = qobject_cast<QMainWindow*>(watched);

        if (window && (event->type() == QEvent::Show || event->type() == QEvent::Polish))
        {
            QTimer::singleShot(0, window, [window]() { applyMainWindowTitleBar(window); });

            if (isPrimaryMainWindow(window) && !window->property("ecuConnectionHeaderSyncInstalled").toBool())
            {
                window->setProperty("ecuConnectionHeaderSyncInstalled", true);
                QTimer *syncTimer = new QTimer(window);
                syncTimer->setInterval(200);
                QObject::connect(syncTimer, &QTimer::timeout, window, [window]() {
                    syncConnectionHeader(window);
                });
                syncTimer->start();
                QTimer::singleShot(0, window, [window]() { syncConnectionHeader(window); });
            }
        }

        if (window && event->type() == QEvent::Close && isPrimaryMainWindow(window))
        {
            if (!confirmMainWindowClose(window))
            {
                event->ignore();
                return true;
            }
        }

        return QObject::eventFilter(watched, event);
    }
};

void installMainWindowTitleBarFix()
{
    QApplication *app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (app)
        app->installEventFilter(new MainWindowTitleBarFix(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installMainWindowTitleBarFix)
