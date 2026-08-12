#include <QApplication>
#include <QMessageBox>
#include <QSplashScreen>
#include <QProgressBar>
#include <QLabel>
#include <QPainter>
#include <QFont>
#include <QScreen>
#include <QGuiApplication>
#include "splashprogress.h"

#include "mainwindow.h"
#include "database/DatabaseManager.h"
#include "database/DatabaseSeed.h"

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
        g_startupStatus->setText(QStringLiteral("Détection des ports série : %1 %").arg(percent));
    qApp->processEvents();
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
                     QStringLiteral("Initialisation du diagnostic ECU"));
    painter.drawText(QRect(24, 84, 472, 22), Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("Version %1").arg(QStringLiteral(APP_VERSION)));
    painter.end();

    QSplashScreen *splash = new QSplashScreen(pixmap, Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    QProgressBar *progress = new QProgressBar(splash);
    progress->setGeometry(24, 136, 472, 18);
    progress->setRange(0, 100);
    progress->setValue(0);
    QLabel *status = new QLabel(QStringLiteral("Initialisation..."), splash);
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
        g_startupStatus->setText(QStringLiteral("Initialisation de la base de données..."));
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

    if (!DatabaseSeed::populate(database))
    {
        QMessageBox::critical(nullptr, QObject::tr("Erreur base de données"),
                              QObject::tr("La base SQLite est ouverte mais son chargement de connaissances a échoué."));
        g_splashProgressCallback = nullptr;
        splash->close();
        delete splash;
        return 1;
    }

    if (g_startupStatus)
        g_startupStatus->setText(QStringLiteral("Chargement de l'interface..."));
    updateStartupProgress(100);

    MainWindow window(&database);

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
