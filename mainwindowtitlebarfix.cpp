#include <QAbstractButton>
#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QString>
#include <QTimer>

#include "i18n.h"
#include "optionsdialog.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {

#ifdef Q_OS_WIN
static HANDLE g_instanceMutex = nullptr;
static HANDLE g_replaceEvent = nullptr;
static bool g_replacePending = false;
static const wchar_t *kInstanceMutexName = L"Local\\ECU_MEMS_Manager_Single_Instance";
static const wchar_t *kReplaceEventName = L"Local\\ECU_MEMS_Manager_Replace_Request";
#endif

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

#ifdef Q_OS_WIN
struct ExistingInstanceText
{
    QString title;
    QString message;
};

static ExistingInstanceText existingInstanceText(const QString &language)
{
    if (language == QStringLiteral("en"))
        return {QStringLiteral("ECU MEMS Manager already running"),
                QStringLiteral("ECU MEMS Manager is already running. The existing instance will be closed so this new startup can continue.")};
    if (language == QStringLiteral("es"))
        return {QString::fromUtf8("ECU MEMS Manager ya está abierto"),
                QString::fromUtf8("ECU MEMS Manager ya está abierto. La instancia existente se cerrará para que este nuevo inicio pueda continuar.")};
    if (language == QStringLiteral("it"))
        return {QStringLiteral("ECU MEMS Manager è già aperto"),
                QStringLiteral("ECU MEMS Manager è già aperto. L'istanza esistente verrà chiusa per consentire il proseguimento di questo nuovo avvio.")};
    if (language == QStringLiteral("pt"))
        return {QString::fromUtf8("ECU MEMS Manager já está aberto"),
                QString::fromUtf8("ECU MEMS Manager já está aberto. A instância existente será fechada para que esta nova inicialização possa continuar.")};
    if (language == QStringLiteral("de"))
        return {QStringLiteral("ECU MEMS Manager ist bereits geöffnet"),
                QStringLiteral("ECU MEMS Manager ist bereits geöffnet. Die vorhandene Instanz wird geschlossen, damit dieser neue Start fortgesetzt werden kann.")};

    return {QString::fromUtf8("ECU MEMS Manager déjà ouvert"),
            QString::fromUtf8("ECU MEMS Manager est déjà ouvert. L'instance déjà ouverte va être fermée pour continuer ce nouveau démarrage.")};
}

static void showExistingInstanceNotice()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, PROJECTNAME);
    settings.beginGroup(QStringLiteral("Settings"));
    QString language = settings.value(QStringLiteral("Language"), QStringLiteral("fr")).toString().toLower();
    QString theme = settings.value(QStringLiteral("Theme"), QStringLiteral("light")).toString().toLower();
    settings.endGroup();

    if (theme == QStringLiteral("sombre"))
        theme = QStringLiteral("dark");

    const ExistingInstanceText text = existingInstanceText(language);
    QMessageBox box(QMessageBox::Information, text.title, text.message, QMessageBox::Ok, nullptr);
    const bool dark = theme == QStringLiteral("dark");

    if (dark)
    {
        box.setStyleSheet(QStringLiteral(
            "QMessageBox{background:#090e13;color:#e7ecef;}"
            "QMessageBox QLabel{color:#e7ecef;background:transparent;}"
            "QMessageBox QPushButton{background:#ff7a00;color:#101419;border:1px solid #ff9828;border-radius:5px;padding:6px 16px;font-weight:700;min-width:72px;}"
            "QMessageBox QPushButton:hover{background:#ff9828;}"
            "QMessageBox QPushButton:pressed{background:#d96500;}"));
    }

    applyNativeTitleBar(&box, dark);
    box.exec();
}

static bool initializeSingleInstance()
{
    g_instanceMutex = CreateMutexW(nullptr, TRUE, kInstanceMutexName);
    if (!g_instanceMutex)
        return true;

    const bool alreadyRunning = GetLastError() == ERROR_ALREADY_EXISTS;
    g_replaceEvent = CreateEventW(nullptr, FALSE, FALSE, kReplaceEventName);

    if (!alreadyRunning)
        return true;

    showExistingInstanceNotice();

    if (g_replaceEvent)
        SetEvent(g_replaceEvent);

    const DWORD waitResult = WaitForSingleObject(g_instanceMutex, 15000);
    if (waitResult == WAIT_OBJECT_0 || waitResult == WAIT_ABANDONED)
        return true;

    QMessageBox box(QMessageBox::Critical,
                    QStringLiteral("ECU MEMS Manager"),
                    QString::fromUtf8("Impossible de fermer l'instance déjà ouverte. Le nouveau démarrage est annulé."),
                    QMessageBox::Ok, nullptr);
    box.setStyleSheet(QStringLiteral(
        "QMessageBox{background:#090e13;color:#e7ecef;}"
        "QMessageBox QLabel{color:#e7ecef;background:transparent;}"
        "QMessageBox QPushButton{background:#ff7a00;color:#101419;border:1px solid #ff9828;border-radius:5px;padding:6px 16px;font-weight:700;min-width:72px;}"));
    applyNativeTitleBar(&box, true);
    box.exec();
    ExitProcess(0);
    return false;
}

static void processReplacementRequest()
{
    if (g_replaceEvent && WaitForSingleObject(g_replaceEvent, 0) == WAIT_OBJECT_0)
        g_replacePending = true;

    if (!g_replacePending)
        return;

    QMainWindow *mainWindow = nullptr;
    const QWidgetList topLevels = QApplication::topLevelWidgets();
    for (QWidget *widget : topLevels)
    {
        QMainWindow *candidate = qobject_cast<QMainWindow*>(widget);
        if (isPrimaryMainWindow(candidate))
        {
            mainWindow = candidate;
            break;
        }
    }

    if (!mainWindow)
        return;

    g_replacePending = false;

    QWidget *modal = QApplication::activeModalWidget();
    if (modal && modal != mainWindow)
        modal->close();

    mainWindow->setProperty("ecuSkipExitConfirmation", true);
    mainWindow->close();
}
#endif

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
                QStringLiteral("Deseja realmente sair de ECU MEMS Manager?"),
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

static bool matchesConnectionErrorTemplate(const QString &text, const QString &pattern)
{
    const int marker = pattern.indexOf(QStringLiteral("%1"));
    if (marker < 0)
        return text == pattern;

    return text.startsWith(pattern.left(marker)) &&
           text.endsWith(pattern.mid(marker + 2));
}

static bool isConnectionErrorBox(QMessageBox *box)
{
    if (!box || box->icon() != QMessageBox::Warning)
        return false;

    QMainWindow *parentWindow = qobject_cast<QMainWindow*>(box->parentWidget());
    if (!isPrimaryMainWindow(parentWindow))
        return false;

    const QString text = box->text();
    if (text == I18n::text(7057) + I18n::text(7058))
        return true;

    return matchesConnectionErrorTemplate(text, I18n::text(7060));
}

static void styleConnectionErrorBox(QMessageBox *box)
{
    if (!isConnectionErrorBox(box))
        return;

    const bool dark = qApp && qApp->property("ecuDarkTheme").toBool();
    if (dark)
    {
        box->setStyleSheet(QStringLiteral(
            "QMessageBox{background:#090e13;color:#e7ecef;}"
            "QMessageBox QLabel{color:#e7ecef;background:transparent;}"
            "QMessageBox QPushButton{background:#151d25;color:#edf2f5;border:1px solid #35414c;border-radius:5px;padding:6px 16px;font-weight:700;min-width:72px;}"
            "QMessageBox QPushButton:hover{background:#1b252e;border-color:#ff7a00;}"));

        if (QAbstractButton *okButton = box->button(QMessageBox::Ok))
        {
            okButton->setStyleSheet(QStringLiteral(
                "QPushButton{background:#ff7a00;color:#101419;border:1px solid #ff9828;border-radius:5px;padding:6px 16px;font-weight:700;min-width:72px;}"
                "QPushButton:hover{background:#ff9828;}"
                "QPushButton:pressed{background:#d96500;}"));
        }
    }

    applyNativeTitleBar(box, dark);
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

        if (QMessageBox *box = qobject_cast<QMessageBox*>(watched))
        {
            if ((event->type() == QEvent::Show || event->type() == QEvent::Polish) && isConnectionErrorBox(box))
                QTimer::singleShot(0, box, [box]() { styleConnectionErrorBox(box); });
        }

        if (window && event->type() == QEvent::Close && isPrimaryMainWindow(window))
        {
            if (window->property("ecuSkipExitConfirmation").toBool())
                return QObject::eventFilter(watched, event);

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
    if (!app)
        return;

#ifdef Q_OS_WIN
    if (!initializeSingleInstance())
        return;
#endif

    app->installEventFilter(new MainWindowTitleBarFix(app));

#ifdef Q_OS_WIN
    QTimer *replaceTimer = new QTimer(app);
    replaceTimer->setInterval(100);
    QObject::connect(replaceTimer, &QTimer::timeout, app, []() {
        processReplacementRequest();
    });
    replaceTimer->start();
#endif
}

}

Q_COREAPP_STARTUP_FUNCTION(installMainWindowTitleBarFix)
