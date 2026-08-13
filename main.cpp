#include <QApplication>
#include <QLocale>
#include <QSettings>
#include <QTranslator>
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
        g_startupStatus->setText(QObject::tr("Détection des ports série : %1 %").arg(percent));
    qApp->processEvents();
}

QString chooseInitialLanguage()
{
    QDialog dialog;
    dialog.setWindowTitle(QStringLiteral("ECU MEMS Manager - Language"));
    dialog.setModal(true);
    dialog.setMinimumWidth(430);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QLabel *title = new QLabel(
        QStringLiteral("<b>Choisissez votre langue / Choose your language</b>"), &dialog);
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    QLabel *info = new QLabel(
        QStringLiteral("La langue pourra être modifiée ensuite dans Options.\n"
                       "The language can be changed later in Options."), &dialog);
    info->setAlignment(Qt::AlignCenter);
    layout->addWidget(info);

    QGridLayout *grid = new QGridLayout();
    struct LanguageChoice { const char *code; const char *name; const char *icon; };
    const LanguageChoice choices[] = {
        {"fr", "Français",  ":/flags/fr.png"},
        {"en", "English",   ":/flags/en.png"},
        {"es", "Español",   ":/flags/es.png"},
        {"it", "Italiano",  ":/flags/it.png"},
        {"pt", "Português", ":/flags/pt.png"},
        {"de", "Deutsch",   ":/flags/de.png"}
    };

    QString selected;
    for (int i = 0; i < 6; ++i)
    {
        QPushButton *button = new QPushButton(
            QIcon(QString::fromLatin1(choices[i].icon)),
            QString::fromUtf8(choices[i].name), &dialog);
        button->setIconSize(QSize(48, 32));
        button->setMinimumSize(180, 52);
        const QString code = QString::fromLatin1(choices[i].code);
        QObject::connect(button, &QPushButton::clicked, &dialog, [&dialog, &selected, code]() {
            selected = code;
            dialog.accept();
        });
        grid->addWidget(button, i / 2, i % 2);
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
                     QStringLiteral("ECU MEMS Manager"));
    // Cadre noir fin autour de l'écran de démarrage.
    painter.setPen(QPen(Qt::black, 1));
    painter.drawRect(pixmap.rect().adjusted(1, 1, -2, -2));

    QFont subFont = painter.font();
    subFont.setBold(false);
    subFont.setPointSize(10);
    painter.setFont(subFont);
    painter.drawText(QRect(24, 62, 472, 24), Qt::AlignLeft | Qt::AlignVCenter,
                     QObject::tr("Initialisation du diagnostic ECU"));
    painter.drawText(QRect(24, 84, 472, 22), Qt::AlignLeft | Qt::AlignVCenter,
                     QObject::tr("Version %1").arg(QStringLiteral(APP_VERSION)));
    painter.end();

    QSplashScreen *splash = new QSplashScreen(pixmap, Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    QProgressBar *progress = new QProgressBar(splash);
    progress->setGeometry(24, 136, 472, 18);
    progress->setRange(0, 100);
    progress->setValue(0);
    QLabel *status = new QLabel(QObject::tr("Initialisation..."), splash);
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

    // Au tout premier démarrage, demander la langue avant de créer les widgets.
    QSettings languageSettings(QSettings::IniFormat, QSettings::UserScope, PROJECTNAME);
    languageSettings.beginGroup("Settings");
    const bool languageConfigured = languageSettings.value("LanguageConfigured", false).toBool();
    QString language = languageSettings.value("Language", "fr").toString();
    languageSettings.endGroup();

    // Mode TEST9 : afficher temporairement le sélecteur à chaque démarrage
    // afin de valider facilement les six langues. Le comportement final
    // reviendra à un affichage uniquement au premier lancement.
    Q_UNUSED(languageConfigured);
    language = chooseInitialLanguage();
    languageSettings.beginGroup("Settings");
    languageSettings.setValue("Language", language);
    languageSettings.setValue("LanguageConfigured", true);
    languageSettings.endGroup();
    languageSettings.sync();

    QTranslator translator;
    const QString translationPath = QCoreApplication::applicationDirPath()
                                  + "/translations/ECUMemsManager_" + language + ".qm";
    if (translator.load(translationPath))
        app.installTranslator(&translator);

    // Vérifie le raccourci Bureau demandé dans Options.
    DesktopShortcut::ensureIfEnabled();

    QSplashScreen *splash = createStartupSplash();
    splash->show();
    app.processEvents();
    g_splashProgressCallback = updateStartupProgress;
    updateStartupProgress(0);

    /*
     * Base SQLite locale.
     *
     * La base est créée automatiquement dans :
     *
     *    <dossier du programme>/database/
     *
     * Aucun accès Internet n'est nécessaire.
     */
    DatabaseManager database;
    if (g_startupStatus)
        g_startupStatus->setText(QObject::tr("Initialisation de la base de données..."));
    app.processEvents();

    if (!database.open())
    {
        QMessageBox::critical(
            nullptr,
            QObject::tr("Erreur base de données"),
            QObject::tr(
                "Impossible d'ouvrir la base de données SQLite.\n\n"
                "ECU Mems Manager ne peut pas démarrer."
            )
        );

        g_splashProgressCallback = nullptr;
        splash->close();
        delete splash;
        return 1;
    }

    if (g_startupStatus)
        g_startupStatus->setText(QObject::tr("Chargement de l'interface..."));
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
