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
#include <QStringList>
#include "splashprogress.h"

#include "mainwindow.h"
#include "desktopshortcut.h"
#include "database/DatabaseManager.h"
#include "database/MemsReferenceDatabase.h"
#include "database/MemsGlobalSearchIndex.h"
#include "database/MemsReferencePackageRefresh.h"
#include "i18n.h"
#include "navigationorderpatch.h"

namespace
{
enum class StartupStepState
{
    Pending,
    Active,
    Done,
    Error
};

QProgressBar *g_startupProgress = nullptr;
QLabel *g_profileStep = nullptr;
QLabel *g_databaseStep = nullptr;
QLabel *g_indexStep = nullptr;
QLabel *g_portsStep = nullptr;
QLabel *g_interfaceStep = nullptr;
QSplashScreen *g_startupSplash = nullptr;

void setStartupProgress(int percent)
{
    if (!g_startupProgress)
        return;
    g_startupProgress->setValue(qBound(0, percent, 100));
    if (qApp)
        qApp->processEvents();
}

void setStep(QLabel *label, StartupStepState state, const QString &text)
{
    if (!label)
        return;

    QString marker;
    QString markerColor;
    QString textColor = QStringLiteral("#E7ECF2");

    switch (state)
    {
    case StartupStepState::Pending:
        marker = QStringLiteral("○");
        markerColor = QStringLiteral("#66717D");
        textColor = QStringLiteral("#8E99A5");
        break;
    case StartupStepState::Active:
        marker = QStringLiteral("●");
        markerColor = QStringLiteral("#5AA2FF");
        break;
    case StartupStepState::Done:
        marker = QString::fromUtf8("✓");
        markerColor = QStringLiteral("#48C77B");
        break;
    case StartupStepState::Error:
        marker = QStringLiteral("!");
        markerColor = QStringLiteral("#FF6B6B");
        textColor = QStringLiteral("#FFD7D7");
        break;
    }

    label->setText(QStringLiteral(
        "<span style=\"color:%1;font-weight:700;\">%2</span> "
        "<span style=\"color:%3;\">%4</span>")
        .arg(markerColor, marker, textColor, text.toHtmlEscaped()));
    label->setTextFormat(Qt::RichText);
    if (qApp)
        qApp->processEvents();
}

void updateStartupProgress(int percent)
{
    if (!g_startupProgress || !g_startupSplash)
        return;

    percent = qBound(0, percent, 100);
    setStartupProgress(70 + ((percent * 22) / 100));

    if (percent >= 100)
    {
        setStep(g_portsStep, StartupStepState::Done, I18n::text(24));
        setStep(g_interfaceStep, StartupStepState::Active, I18n::text(25));
    }
    else
    {
        setStep(g_portsStep, StartupStepState::Active, I18n::text(23).arg(percent));
    }
}

QString chooseInitialLanguage()
{
    QDialog dialog;
    dialog.setWindowTitle(I18n::text(2));
    dialog.setModal(true);
    dialog.setMinimumWidth(620);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QLabel *title = new QLabel(I18n::text(3), &dialog);
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    QLabel *info = new QLabel(I18n::text(4), &dialog);
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
    QPixmap pixmap(640, 360);
    pixmap.fill(QColor(QStringLiteral("#111820")));

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(QColor(QStringLiteral("#3A4652")), 1));
    painter.drawRoundedRect(pixmap.rect().adjusted(1, 1, -2, -2), 8, 8);

    painter.setPen(QColor(QStringLiteral("#F4F7FA")));
    QFont titleFont = painter.font();
    titleFont.setBold(true);
    titleFont.setPointSize(19);
    painter.setFont(titleFont);
    painter.drawText(QRect(28, 24, 584, 38), Qt::AlignLeft | Qt::AlignVCenter,
                     I18n::text(7));

    painter.setPen(QColor(QStringLiteral("#AAB4BF")));
    QFont subFont = painter.font();
    subFont.setBold(false);
    subFont.setPointSize(10);
    painter.setFont(subFont);
    painter.drawText(QRect(28, 65, 584, 22), Qt::AlignLeft | Qt::AlignVCenter,
                     I18n::text(8));
    painter.drawText(QRect(28, 88, 584, 22), Qt::AlignLeft | Qt::AlignVCenter,
                     I18n::text(9).arg(QStringLiteral(APP_VERSION)));
    painter.end();

    QSplashScreen *splash = new QSplashScreen(
        pixmap, Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);

    const auto makeStepLabel = [splash](int y) {
        QLabel *label = new QLabel(splash);
        label->setGeometry(30, y, 580, 25);
        QFont font = label->font();
        font.setPointSize(10);
        label->setFont(font);
        return label;
    };

    g_profileStep = makeStepLabel(126);
    g_databaseStep = makeStepLabel(160);
    g_indexStep = makeStepLabel(194);
    g_portsStep = makeStepLabel(228);
    g_interfaceStep = makeStepLabel(262);

    QProgressBar *progress = new QProgressBar(splash);
    progress->setGeometry(30, 310, 580, 20);
    progress->setRange(0, 100);
    progress->setValue(0);
    progress->setTextVisible(true);
    progress->setStyleSheet(QStringLiteral(
        "QProgressBar {"
        "  border: 1px solid #3A4652;"
        "  border-radius: 5px;"
        "  background: #0C1218;"
        "  color: #E7ECF2;"
        "  text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "  background: #2D7FF9;"
        "  border-radius: 4px;"
        "}"));

    g_startupProgress = progress;
    g_startupSplash = splash;

    setStep(g_profileStep, StartupStepState::Pending, I18n::text(15));
    setStep(g_databaseStep, StartupStepState::Pending, I18n::text(31));
    setStep(g_indexStep, StartupStepState::Pending, I18n::text(32));
    setStep(g_portsStep, StartupStepState::Pending, I18n::text(29));
    setStep(g_interfaceStep, StartupStepState::Pending, I18n::text(30));

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
    QString language = languageSettings.value("Language", "fr").toString().toLower();
    languageSettings.endGroup();

    const QStringList supportedLanguages = {
        QStringLiteral("fr"), QStringLiteral("en"), QStringLiteral("es"),
        QStringLiteral("it"), QStringLiteral("pt"), QStringLiteral("de")
    };
    if (!supportedLanguages.contains(language))
        language = QStringLiteral("fr");

    if (!languageConfigured)
    {
        I18n::load(QStringLiteral("en"));
        language = chooseInitialLanguage();
        languageSettings.beginGroup("Settings");
        languageSettings.setValue("Language", language);
        languageSettings.setValue("LanguageConfigured", true);
        languageSettings.endGroup();
        languageSettings.sync();
    }

    I18n::load(language);
    I18n::install(&app);

    DesktopShortcut::ensureIfEnabled();

    QSplashScreen *splash = createStartupSplash();
    splash->show();
    app.processEvents();

    setStep(g_profileStep, StartupStepState::Active, I18n::text(15));
    setStartupProgress(5);
    setStep(g_profileStep, StartupStepState::Done, I18n::text(16));
    setStartupProgress(12);

    MemsReferencePackageAction referenceAction = memsReferencePackageAction();
    if (referenceAction == MemsReferencePackageAction::Install)
        setStep(g_databaseStep, StartupStepState::Active, I18n::text(18));
    else if (referenceAction == MemsReferencePackageAction::Update)
        setStep(g_databaseStep, StartupStepState::Active, I18n::text(19));
    else
        setStep(g_databaseStep, StartupStepState::Active, I18n::text(17));
    setStartupProgress(20);

    if (!refreshMemsReferencePackage(&referenceAction))
    {
        setStep(g_databaseStep, StartupStepState::Error, I18n::text(27));
        QMessageBox::critical(nullptr, I18n::text(12), I18n::text(27));
        splash->close();
        delete splash;
        return 1;
    }

    MemsReferenceDatabase referenceDatabase;
    if (!referenceDatabase.open())
    {
        setStep(g_databaseStep, StartupStepState::Error, I18n::text(27));
        QMessageBox::critical(nullptr, I18n::text(12), I18n::text(27));
        splash->close();
        delete splash;
        return 1;
    }
    referenceDatabase.close();

    DatabaseManager database;
    if (!database.open())
    {
        setStep(g_databaseStep, StartupStepState::Error, I18n::text(13));
        QMessageBox::critical(nullptr, I18n::text(12), I18n::text(13));
        splash->close();
        delete splash;
        return 1;
    }

    setStep(g_databaseStep, StartupStepState::Done, I18n::text(20));
    setStartupProgress(42);

    setStep(g_indexStep, StartupStepState::Active, I18n::text(21));
    setStartupProgress(50);
    QString indexError;
    if (!MemsGlobalSearchIndex::ensureBuilt(&indexError))
    {
        Q_UNUSED(indexError);
        setStep(g_indexStep, StartupStepState::Error, I18n::text(28));
        QMessageBox::critical(nullptr, I18n::text(12), I18n::text(28));
        database.close();
        splash->close();
        delete splash;
        return 1;
    }
    setStep(g_indexStep, StartupStepState::Done, I18n::text(22));
    setStartupProgress(68);

    setStep(g_portsStep, StartupStepState::Active, I18n::text(23).arg(0));
    setStep(g_interfaceStep, StartupStepState::Pending, I18n::text(30));
    setStartupProgress(70);
    g_splashProgressCallback = updateStartupProgress;

    MainWindow window;
    if (QObject *injectorGauge = window.findChild<QObject*>(QStringLiteral("m_injector_time")))
    {
        injectorGauge->setProperty("minimum", 0.0);
        injectorGauge->setProperty("maximum", 20.0);
    }

    setStep(g_portsStep, StartupStepState::Done, I18n::text(24));
    setStep(g_interfaceStep, StartupStepState::Done, I18n::text(26));
    setStartupProgress(100);

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
    g_profileStep = nullptr;
    g_databaseStep = nullptr;
    g_indexStep = nullptr;
    g_portsStep = nullptr;
    g_interfaceStep = nullptr;
    g_splashProgressCallback = nullptr;

    window.showMaximized();
    installFinalNavigation(&app, &window);
    const int result = app.exec();
    database.close();
    return result;
}
