#include "singleinstance.h"
#include "nativewindowstyle.h"
#include "mainwindow.h"

#include <QApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSplashScreen>
#include <QString>
#include <QTimer>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {

#ifdef Q_OS_WIN
static HANDLE g_instanceMutex = nullptr;
static HANDLE g_replaceEvent = nullptr;
static bool g_replacePending = false;
static bool g_duplicateInstance = false;
static const wchar_t *kInstanceMutexName = L"Local\\ECU_MEMS_Manager_Single_Instance";
static const wchar_t *kReplaceEventName = L"Local\\ECU_MEMS_Manager_Replace_Request";
#endif
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
    box.setWindowFlag(Qt::WindowStaysOnTopHint, true);
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

static void initializeSingleInstance()
{
    g_instanceMutex = CreateMutexW(nullptr, TRUE, kInstanceMutexName);
    if (!g_instanceMutex)
        return;

    g_duplicateInstance = GetLastError() == ERROR_ALREADY_EXISTS;
    g_replaceEvent = CreateEventW(nullptr, FALSE, FALSE, kReplaceEventName);
}

static void handleDuplicateStartup()
{
    if (!g_duplicateInstance)
        return;

    QSplashScreen *startupSplash = nullptr;
    const QWidgetList topLevels = QApplication::topLevelWidgets();
    for (QWidget *widget : topLevels)
    {
        startupSplash = qobject_cast<QSplashScreen*>(widget);
        if (startupSplash)
            break;
    }

    if (startupSplash)
        startupSplash->hide();

    showExistingInstanceNotice();

    if (g_replaceEvent)
        SetEvent(g_replaceEvent);

    const DWORD waitResult = WaitForSingleObject(g_instanceMutex, 15000);
    if (waitResult == WAIT_OBJECT_0 || waitResult == WAIT_ABANDONED)
    {
        g_duplicateInstance = false;
        if (startupSplash)
            startupSplash->show();
        return;
    }

    QMessageBox box(QMessageBox::Critical,
                    QStringLiteral("ECU MEMS Manager"),
                    QString::fromUtf8("Impossible de fermer l'instance déjà ouverte. Le nouveau démarrage est annulé."),
                    QMessageBox::Ok, nullptr);
    box.setWindowFlag(Qt::WindowStaysOnTopHint, true);
    box.setStyleSheet(QStringLiteral(
        "QMessageBox{background:#090e13;color:#e7ecef;}"
        "QMessageBox QLabel{color:#e7ecef;background:transparent;}"
        "QMessageBox QPushButton{background:#ff7a00;color:#101419;border:1px solid #ff9828;border-radius:5px;padding:6px 16px;font-weight:700;min-width:72px;}"));
    applyNativeTitleBar(&box, true);
    box.exec();
    ExitProcess(0);
}

static void processReplacementRequest()
{
    if (g_replaceEvent && WaitForSingleObject(g_replaceEvent, 0) == WAIT_OBJECT_0)
        g_replacePending = true;

    if (!g_replacePending)
        return;

    MainWindow *mainWindow = nullptr;
    const QWidgetList topLevels = QApplication::topLevelWidgets();
    for (QWidget *widget : topLevels)
    {
        MainWindow *candidate = qobject_cast<MainWindow*>(widget);
        if (candidate)
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

} // namespace

void installSingleInstanceManager(QApplication *app)
{
#ifdef Q_OS_WIN
    if (!app)
        return;
    initializeSingleInstance();
    QTimer *replacementTimer = new QTimer(app);
    replacementTimer->setInterval(100);
    QObject::connect(replacementTimer, &QTimer::timeout, app, []() {
        processReplacementRequest();
    });
    replacementTimer->start();
    if (g_duplicateInstance)
        QTimer::singleShot(0, app, []() { handleDuplicateStartup(); });
#else
    Q_UNUSED(app);
#endif
}
