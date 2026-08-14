#include <QApplication>
#include <QLocale>
#include <QSettings>
#include <QMessageBox>
#include <QSplashScreen>
#include <QProgressBar>
#include <QLabel>
#include <QPainter>
#include <QFont>
#include <QScreen>
#include <QGuiApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QIcon>
#include "splashprogress.h"

#include "mainwindow.h"
#include "desktopshortcut.h"
#include "database/DatabaseManager.h"
#include "i18n.h"

namespace
{
QProgressBar *g_startupProgress = nullptr;
QLabel *g_startupStatus = nullptr;
QSplashScreen *g_startupSplash = nullptr;

void updateStartupProgress(int percent)
{
    if (!g_startupProgress || !g_startupSplash)
        return;
    percent = qBound(0, percent, 100);
    g_startupProgress->setValue(percent);
    if (g_startupStatus)
        g_startupStatus->setText(I18n::text(1).arg(percent)); // Serial port detection: %1 %
    qApp->processEvents();
}

QString chooseInitialLanguage()
{
    QDialog dialog;
    dialog.setWindowTitle(I18n::text(2)); // ECU MEMS Manager - Language
    dialog.setModal(true);
    dialog.setMinimumWidth(620);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QLabel *title = new QLabel(I18n::text(3), &dialog); // <b>Choose your language</b>
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    QLabel *info = new QLabel(I18n::text(4), &dialog); // The language can be changed later in Options.
    info->setAlignment(Qt::AlignCenter);
    layout->addWidget(info);

    QGridLayout *grid = new QGridLayout();
    struct LanguageChoice { const char *code; const char *nativeName; const char *icon; };
    const LanguageChoice choices[] = {
        {"fr", "Français",  ":/flags/fr.png"},
        {"en", "English",   ":/flags/en.png"},
        {"es", "Español",   ":/flags/es.png"},
        {"it", "Italiano",  ":/flags/it.png"},
        {"pt", "Português", ":/flags/pt.png"},
        {"de", "Deutsch",   ":/flags/de.png"}
    };

    QString selected;
    const int choiceCount = int(sizeof(choices) / sizeof(choices[0]));
    for (int i = 0; i < choiceCount; ++i)
    {
        QPushButton *button = new QPushButton(
            QIcon(QString::fromLatin1(choices[i].icon)),
            QString::fromUtf8(choices[i].nativeName), &dialog);
        button->setIconSize(QSize(48, 32));
        button->setMinimumSize(180, 52);
        const QString code = QString::fromLatin1(choices[i].code);
        QObject::connect(button, &QPushButton::clicked, &dialog, [&dialog, &selected, code]() {
            selected = code;
            dialog.accept();
        });
        grid->addWidget(button, i / 3, i % 3);
    }
    layout->addLayout(grid);

    if (dialog.exec() != QDialog::Accepted || selected.isEmpty())
        return QStringLiteral("fr");
    return selected;
}

QSplashScreen *createStartupSplash()
{
    QPixmap pixmap(520, 190);
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);
    painter.setPen(Qt::black);
    QFont titleFont = painter.font();
    titleFont.setBold(true);
    titleFont.setPointSize(18);
    painter.setFont(titleFont);
    painter.drawText(QRect(24, 22, 472, 40), Qt::AlignLeft | Qt::AlignVCenter,
                     I18n::text(7)); // ECU MEMS Manager
    painter.setPen(QPen(Qt::black, 1));
    painter.drawRect(pixmap.rect().adjusted(1, 1, -2, -2));

    QFont subFont = painter.font();
    subFont.setBold(false);
    subFont.setPointSize(10);
    painter.setFont(subFont);
    painter.drawText(QRect(24, 62, 472, 24), Qt::AlignLeft | Qt::AlignVCenter,
                     I18n::text(8)); // Initializing ECU diagnostics
    painter.drawText(QRect(24, 84, 472, 22), Qt::AlignLeft | Qt::AlignVCenter,
                     I18n::text(9).arg(QStringLiteral(APP_VERSION))); // Version %1
    painter.end();

    QSplashScreen *splash = new QSplashScreen(pixmap, Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    QProgressBar *progress = new QProgressBar(splash);
    progress->setGeometry(24, 136, 472, 18);
    progress->setRange(0, 100);
    progress->setValue(0);
    QLabel *status = new QLabel(I18n::text(10), splash); // Initializing...
    status->setGeometry(24, 110, 472, 22);
    g_startupProgress = progress;
    g_startupStatus = status;
    g_startupSplash = splash;
    return splash;
}
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("ECU Mems Manager");
    QApplication::setApplicationVersion(QStringLiteral(APP_VERSION));
    QApplication::setOrganizationName("ECU Mems Manager");

    QSettings languageSettings(QSettings::IniFormat, QSettings::UserScope, PROJECTNAME);
    languageSettings.beginGroup("Settings");
    const bool languageConfigured = languageSettings.value("LanguageConfigured", false).toBool();
    QString language = languageSettings.value("Language", "fr").toString();
    languageSettings.endGroup();

    I18n::load(QStringLiteral("en"));
    Q_UNUSED(languageConfigured);
    language = chooseInitialLanguage();
    languageSettings.beginGroup("Settings");
    languageSettings.setValue("Language", language);
    languageSettings.setValue("LanguageConfigured", true);
    languageSettings.endGroup();
    languageSettings.sync();
    I18n::load(language);
    I18n::install(&app);

    DesktopShortcut::ensureIfEnabled();

    QSplashScreen *splash = createStartupSplash();
    splash->show();
    app.processEvents();
    g_splashProgressCallback = updateStartupProgress;
    updateStartupProgress(0);

    DatabaseManager database;
    if (g_startupStatus)
        g_startupStatus->setText(I18n::text(11)); // Initializing database...
    app.processEvents();

    if (!database.open())
    {
        QMessageBox::critical(nullptr, I18n::text(12), I18n::text(13)); // Database error / Unable to open SQLite database
        g_splashProgressCallback = nullptr;
        splash->close();
        delete splash;
        return 1;
    }

    if (g_startupStatus)
        g_startupStatus->setText(I18n::text(14)); // Loading interface...
    updateStartupProgress(100);

    MainWindow window;
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen)
    {
        const QRect available = screen->availableGeometry();
        const int width = qMin(1300, qMax(760, available.width() - 40));
        const int height = qMin(690, qMax(480, available.height() - 80));
        window.resize(width, height);
        window.move(available.center() - QPoint(width / 2, height / 2));
    }

    app.processEvents();
    splash->finish(&window);
    delete splash;
    g_startupSplash = nullptr;
    g_startupProgress = nullptr;
    g_startupStatus = nullptr;
    g_splashProgressCallback = nullptr;

    window.show();
    const int result = app.exec();
    database.close();
    return result;
}
