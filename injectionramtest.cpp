#include "mainwindow.h"
#include "memsinterface.h"

#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDialog>
#include <QElapsedTimer>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <initializer_list>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/select.h>
#endif

namespace {

static QByteArray testBytes(std::initializer_list<int> values)
{
    QByteArray bytes;
    for (int value : values)
        bytes.append(char(value & 0xFF));
    return bytes;
}

static QString hexText(const QByteArray &bytes)
{
    if (bytes.isEmpty())
        return QStringLiteral("<aucune réponse>");
    return QString::fromLatin1(bytes.toHex(' ').toUpper());
}

static bool hasPair(const QByteArray &response, quint8 first, quint8 second)
{
    for (int i = 0; i + 1 < response.size(); ++i)
    {
        if (quint8(response.at(i)) == first && quint8(response.at(i + 1)) == second)
            return true;
    }
    return false;
}

static bool containsCommand(const QByteArray &response, quint8 command)
{
    for (char c : response)
    {
        if (quint8(c) == command)
            return true;
    }
    return false;
}

static int payloadStart(const QByteArray &response, quint8 command, int payloadBytes)
{
    // Search backwards so a possible adapter echo (CMD CMD payload...)
    // resolves to the ECU reply rather than to the physical echo.
    for (int i = response.size() - payloadBytes - 1; i >= 0; --i)
    {
        if (quint8(response.at(i)) == command)
            return i + 1;
    }
    return -1;
}

static QWidget *realTabPage(QWidget *tab)
{
    if (!tab)
        return nullptr;
    if (QScrollArea *scroll = qobject_cast<QScrollArea*>(tab))
        return scroll->widget();
    return tab;
}

class InjectionRamTestDialog final : public QDialog
{
public:
    explicit InjectionRamTestDialog(QWidget *parent = nullptr)
        : QDialog(parent), m_running(false)
    {
    }

    void setTestRunning(bool running)
    {
        m_running = running;
    }

protected:
    void reject() override
    {
        if (!m_running)
            QDialog::reject();
    }

    void closeEvent(QCloseEvent *event) override
    {
        if (m_running)
        {
            event->ignore();
            return;
        }
        QDialog::closeEvent(event);
    }

private:
    bool m_running;
};

} // namespace

/*
 * Dedicated one-shot read-only RAM test for the AANMP002 MEMS 1.6 firmware.
 *
 * Safety rules deliberately enforced here:
 *  - no arbitrary command entry is exposed;
 *  - no calibration/adaptation/write command is present in this routine;
 *  - the ECU identity must contain AANMP002 before mode 4 is requested;
 *  - a fresh 0x80 frame must report 0 RPM before mode 4 is requested;
 *  - mode 3 must be confirmed before entering mode 4;
 *  - F5 + F0 are always used to leave/verify mode 3 after a mode-4 attempt;
 *  - if mode 3 cannot be confirmed, normal polling is stopped so no further
 *    0x7D/0x80 traffic is sent through the existing service loop.
 */
void MEMSInterface::onInjectionRamTestRequested()
{
    if (property("injectionRamTestRunning").toBool())
    {
        emit injectionRamTestFinished(false, 0, 0,
                                      QStringLiteral("TEST REFUSÉ — UN TEST EST DÉJÀ EN COURS"),
                                      QStringLiteral("Aucune commande supplémentaire n'a été envoyée."));
        return;
    }

    setProperty("injectionRamTestRunning", true);

    QString log;
    quint16 baseRaw = 0;
    quint16 correctionRaw = 0;

    auto finish = [this, &log](bool success,
                               quint16 baseValue,
                               quint16 correctionValue,
                               const QString &status)
    {
        setProperty("injectionRamTestRunning", false);
        emit injectionRamTestFinished(success, baseValue, correctionValue, status, log);
    };

    if (!(m_initComplete && mems_is_connected(&m_memsinfo)))
    {
        log = QStringLiteral("ECU non connecté.\nAucune commande n'a été envoyée.");
        finish(false, 0, 0, QStringLiteral("TEST REFUSÉ — ECU NON CONNECTÉ"));
        return;
    }

    // First guard: never even begin the protocol pre-flight if the latest
    // normal data frame already says the engine is rotating.
    if (m_data.engine_rpm != 0)
    {
        log = QStringLiteral("Régime moteur détecté : %1 tr/min.\nAucune commande de test n'a été envoyée.")
                  .arg(m_data.engine_rpm);
        finish(false, 0, 0,
               QStringLiteral("TEST REFUSÉ — MOTEUR DÉTECTÉ EN FONCTIONNEMENT"));
        return;
    }

    auto exchange = [this](const QByteArray &request, QByteArray &response) -> bool
    {
        response.clear();
        if (request.isEmpty() || !(m_initComplete && mems_is_connected(&m_memsinfo)))
            return false;

#ifdef WIN32
        HANDLE h = m_memsinfo.sd;
        if (h == INVALID_HANDLE_VALUE || h == NULL)
            return false;

        COMMTIMEOUTS oldTimeouts;
        if (!GetCommTimeouts(h, &oldTimeouts))
            return false;

        COMMTIMEOUTS timeouts = oldTimeouts;
        timeouts.ReadIntervalTimeout = 20;
        timeouts.ReadTotalTimeoutConstant = 35;
        timeouts.ReadTotalTimeoutMultiplier = 0;
        timeouts.WriteTotalTimeoutConstant = 100;
        timeouts.WriteTotalTimeoutMultiplier = 5;

        if (!SetCommTimeouts(h, &timeouts))
            return false;

        // Only discard stale RX bytes. Never purge/cancel transmitted data.
        PurgeComm(h, PURGE_RXCLEAR);

        DWORD written = 0;
        const DWORD requested = static_cast<DWORD>(request.size());
        const BOOL writeOk = WriteFile(h, request.constData(), requested, &written, NULL);
        if (!writeOk || written != requested)
        {
            SetCommTimeouts(h, &oldTimeouts);
            return false;
        }

        QElapsedTimer overall;
        QElapsedTimer quiet;
        overall.start();
        quiet.start();
        bool gotAny = false;

        while (overall.elapsed() < 700 && (!gotAny || quiet.elapsed() < 120))
        {
            unsigned char buffer[64];
            DWORD count = 0;
            if (!ReadFile(h, buffer, sizeof(buffer), &count, NULL))
                break;

            if (count > 0)
            {
                response.append(reinterpret_cast<const char*>(buffer), int(count));
                gotAny = true;
                quiet.restart();
            }
        }

        SetCommTimeouts(h, &oldTimeouts);
        return gotAny;
#else
        const int fd = m_memsinfo.sd;
        if (fd < 0)
            return false;

        const ssize_t requested = static_cast<ssize_t>(request.size());
        if (write(fd, request.constData(), size_t(request.size())) != requested)
            return false;

        QElapsedTimer overall;
        QElapsedTimer quiet;
        overall.start();
        quiet.start();
        bool gotAny = false;

        while (overall.elapsed() < 700 && (!gotAny || quiet.elapsed() < 120))
        {
            fd_set set;
            FD_ZERO(&set);
            FD_SET(fd, &set);
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 30000;

            const int ready = select(fd + 1, &set, NULL, NULL, &timeout);
            if (ready < 0)
                break;
            if (ready == 0)
                continue;

            unsigned char buffer[64];
            const ssize_t count = read(fd, buffer, sizeof(buffer));
            if (count > 0)
            {
                response.append(reinterpret_cast<const char*>(buffer), int(count));
                gotAny = true;
                quiet.restart();
            }
        }

        return gotAny;
#endif
    };

    auto send = [&exchange, &log](const QByteArray &request, QByteArray &response) -> bool
    {
        log += QStringLiteral("TX : %1\n").arg(hexText(request));
        const bool ok = exchange(request, response);
        log += QStringLiteral("RX : %1\n").arg(hexText(response));
        return ok;
    };

    auto restoreMode3 = [&send, &log]() -> bool
    {
        log += QStringLiteral("\n--- Retour sécurisé au mode 3 ---\n");

        QByteArray f5Response;
        send(testBytes({0xF5}), f5Response);

        QByteArray modeResponse;
        const bool modeRead = send(testBytes({0xF0}), modeResponse);
        const bool confirmed = modeRead && hasPair(modeResponse, 0xF0, 0x14);

        log += confirmed
            ? QStringLiteral("MODE 3 CONFIRMÉ (F0 14).\n")
            : QStringLiteral("MODE 3 NON CONFIRMÉ.\n");
        return confirmed;
    };

    auto abortAfterMode4Attempt = [this, &restoreMode3, &finish, &log](const QString &reason)
    {
        log += QStringLiteral("\nARRÊT DU TEST : %1\n").arg(reason);
        if (restoreMode3())
        {
            finish(false, 0, 0, QStringLiteral("TEST ARRÊTÉ — MODE 3 CONFIRMÉ"));
        }
        else
        {
            // Do not let the normal service loop continue to send 0x7D/0x80
            // if we cannot prove that the ECU is back in diagnostic mode 3.
            m_stopPolling = true;
            log += QStringLiteral("Session de communication arrêtée par sécurité.\n");
            finish(false, 0, 0,
                   QStringLiteral("TEST ARRÊTÉ — SESSION DÉCONNECTÉE PAR SÉCURITÉ"));
        }
    };

    log += QStringLiteral("TEST RAM TEMPS INJECTION — AANMP002\n");
    log += QStringLiteral("Moteur à l'arrêt / contact mis / aucune écriture mémoire.\n\n");

    // Pre-flight 1: hard-lock this experimental address test to the exact
    // firmware family that was reverse engineered.
    QByteArray response;
    if (!send(testBytes({0xD1}), response) || !response.contains("AANMP002"))
    {
        log += QStringLiteral("AANMP002 non confirmé. Le mode 4 n'a pas été demandé.\n");
        finish(false, 0, 0,
               QStringLiteral("TEST REFUSÉ — ECU AANMP002 NON CONFIRMÉ"));
        return;
    }
    log += QStringLiteral("ECU AANMP002 confirmé.\n\n");

    // Pre-flight 2: request a fresh normal live-data frame and verify that
    // RPM is exactly zero immediately before any mode change.
    response.clear();
    if (!send(testBytes({0x80}), response))
    {
        finish(false, 0, 0,
               QStringLiteral("TEST REFUSÉ — CONTRÔLE RPM IMPOSSIBLE"));
        return;
    }

    const int data80 = payloadStart(response, 0x80, 3);
    if (data80 < 0 || data80 + 2 >= response.size())
    {
        log += QStringLiteral("Format de trame 0x80 inattendu. Le mode 4 n'a pas été demandé.\n");
        finish(false, 0, 0,
               QStringLiteral("TEST REFUSÉ — CONTRÔLE RPM IMPOSSIBLE"));
        return;
    }

    const quint16 freshRpm = (quint16(quint8(response.at(data80 + 1))) << 8)
                           | quint16(quint8(response.at(data80 + 2)));
    log += QStringLiteral("Régime moteur frais : %1 tr/min.\n").arg(freshRpm);
    if (freshRpm != 0)
    {
        log += QStringLiteral("Le mode 4 n'a pas été demandé.\n");
        finish(false, 0, 0,
               QStringLiteral("TEST REFUSÉ — MOTEUR DÉTECTÉ EN FONCTIONNEMENT"));
        return;
    }

    // Pre-flight 3: C4 is documented only as a mode-3 -> mode-4 command.
    response.clear();
    if (!send(testBytes({0xF0}), response) || !hasPair(response, 0xF0, 0x14))
    {
        m_stopPolling = true;
        log += QStringLiteral("Mode 3 non confirmé avant le test. Aucune commande C4 n'a été envoyée.\n");
        log += QStringLiteral("Session de communication arrêtée par sécurité.\n");
        finish(false, 0, 0,
               QStringLiteral("TEST REFUSÉ — MODE 3 NON CONFIRMÉ — SESSION DÉCONNECTÉE"));
        return;
    }
    log += QStringLiteral("Mode 3 confirmé avant test (F0 14).\n\n");

    // From this point on, any uncertainty triggers F5 + F0 before returning.
    response.clear();
    if (!send(testBytes({0xC4}), response) || !containsCommand(response, 0xC4))
    {
        abortAfterMode4Attempt(QStringLiteral("réponse C4 absente ou inattendue"));
        return;
    }

    response.clear();
    if (!send(testBytes({0xF0}), response) || !hasPair(response, 0xF0, 0x1E))
    {
        abortAfterMode4Attempt(QStringLiteral("mode 4 non confirmé par F0 1E"));
        return;
    }
    log += QStringLiteral("Mode 4 confirmé (F0 1E).\n\n");

    response.clear();
    if (!send(testBytes({0xDC, 0x03}), response) || !hasPair(response, 0xDC, 0x03))
    {
        abortAfterMode4Attempt(QStringLiteral("sélection du bloc 0x03 non confirmée"));
        return;
    }

    response.clear();
    if (!send(testBytes({0x60}), response))
    {
        abortAfterMode4Attempt(QStringLiteral("lecture RAM 0x03C0 sans réponse"));
        return;
    }

    int start = payloadStart(response, 0x60, 2);
    if (start < 0 || start + 1 >= response.size())
    {
        abortAfterMode4Attempt(QStringLiteral("format de lecture RAM 0x03C0 inattendu"));
        return;
    }
    baseRaw = quint16(quint8(response.at(start)))
            | (quint16(quint8(response.at(start + 1))) << 8);
    log += QStringLiteral("RAM 0x03C0 brut : 0x%1  (%2)\n\n")
               .arg(baseRaw, 4, 16, QLatin1Char('0'))
               .arg(baseRaw)
               .toUpper();

    response.clear();
    if (!send(testBytes({0xDC, 0x00}), response) || !hasPair(response, 0xDC, 0x00))
    {
        abortAfterMode4Attempt(QStringLiteral("sélection du bloc 0x00 non confirmée"));
        return;
    }

    response.clear();
    if (!send(testBytes({0x32}), response))
    {
        abortAfterMode4Attempt(QStringLiteral("lecture RAM 0x0064 sans réponse"));
        return;
    }

    start = payloadStart(response, 0x32, 2);
    if (start < 0 || start + 1 >= response.size())
    {
        abortAfterMode4Attempt(QStringLiteral("format de lecture RAM 0x0064 inattendu"));
        return;
    }
    correctionRaw = quint16(quint8(response.at(start)))
                  | (quint16(quint8(response.at(start + 1))) << 8);
    log += QStringLiteral("RAM 0x0064 brut : 0x%1  (%2)\n\n")
               .arg(correctionRaw, 4, 16, QLatin1Char('0'))
               .arg(correctionRaw)
               .toUpper();

    if (!restoreMode3())
    {
        m_stopPolling = true;
        log += QStringLiteral("Session de communication arrêtée par sécurité.\n");
        finish(false, baseRaw, correctionRaw,
               QStringLiteral("TEST ARRÊTÉ — SESSION DÉCONNECTÉE PAR SÉCURITÉ"));
        return;
    }

    finish(true, baseRaw, correctionRaw,
           QStringLiteral("MODE 3 CONFIRMÉ — TEST TERMINÉ"));
}

class InjectionRamTestInstaller
{
public:
    static void schedule()
    {
        tryInstall(0);
    }

private:
    static void tryInstall(int attempt)
    {
        QTimer::singleShot(150, qApp, [attempt]()
        {
            for (QWidget *top : QApplication::topLevelWidgets())
            {
                MainWindow *window = qobject_cast<MainWindow*>(top);
                if (window && install(window))
                    return;
            }

            if (attempt < 40)
                tryInstall(attempt + 1);
        });
    }

    static bool install(MainWindow *window)
    {
        if (!window)
            return false;

        QTabWidget *tabs = window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
        if (!tabs)
            return false;

        QWidget *roscoPage = nullptr;
        for (int i = 0; i < tabs->count(); ++i)
        {
            QWidget *page = realTabPage(tabs->widget(i));
            if (page && page->property("strictRoscoBuilt").toBool())
            {
                roscoPage = page;
                break;
            }
        }

        // Wait until the existing ROSCO visual rebuild has finished so this
        // safety control cannot be lost/reparented by another startup patch.
        if (!roscoPage)
            return false;

        if (roscoPage->findChild<QPushButton*>(QStringLiteral("injectionRamTestButton")))
            return true;

        const QList<QGroupBox*> groups = roscoPage->findChildren<QGroupBox*>();
        if (groups.isEmpty())
            return false;

        QGroupBox *session = groups.first();
        QGridLayout *grid = qobject_cast<QGridLayout*>(session->layout());
        if (!grid)
            return false;

        const int row = grid->rowCount();
        QLabel *warning = new QLabel(
            QStringLiteral("Test expérimental RAM temps injection — AANMP002 — MOTEUR À L'ARRÊT"),
            session);
        warning->setObjectName(QStringLiteral("injectionRamTestWarning"));
        warning->setWordWrap(true);
        warning->setStyleSheet(QStringLiteral("font-weight:700; color:#ff9828;"));
        grid->addWidget(warning, row, 0, 1, 4);

        QPushButton *button = new QPushButton(QStringLiteral("Ouvrir le test RAM temps injection"), session);
        button->setObjectName(QStringLiteral("injectionRamTestButton"));
        button->setMinimumHeight(30);
        grid->addWidget(button, row + 1, 0, 1, 4);

        QObject::connect(button, &QPushButton::clicked, window, [window]()
        {
            openDialog(window);
        });

        return true;
    }

    static void openDialog(MainWindow *window)
    {
        if (!window)
            return;

        MEMSInterface *mems = window->m_mems;

        InjectionRamTestDialog dialog(window);
        dialog.setWindowTitle(QStringLiteral("Test RAM temps injection — AANMP002"));
        dialog.setModal(true);
        dialog.resize(720, 560);

        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        layout->setContentsMargins(14, 14, 14, 14);
        layout->setSpacing(9);

        QLabel *warning = new QLabel(
            QStringLiteral("<b>MOTEUR À L'ARRÊT — CONTACT MIS</b><br>"
                           "Ne démarrez pas le moteur pendant le test."),
            &dialog);
        warning->setWordWrap(true);
        warning->setAlignment(Qt::AlignCenter);
        warning->setStyleSheet(QStringLiteral("font-size:15px; font-weight:700; color:#ff9828;"));
        layout->addWidget(warning);

        QLabel *info = new QLabel(
            QStringLiteral("Test verrouillé sur l'identifiant <b>AANMP002</b>. "
                           "Avant le passage en mode 4, le programme vérifie l'identité ECU, "
                           "lit une trame 0x80 fraîche et exige exactement 0 tr/min, puis confirme le mode 3. "
                           "La séquence ne contient aucune commande d'écriture mémoire, de calibration, "
                           "d'effacement de défauts ou d'effacement d'adaptations."),
            &dialog);
        info->setWordWrap(true);
        layout->addWidget(info);

        QLabel *sequence = new QLabel(
            QStringLiteral("Séquence de test : D1 / 80 / F0 — C4 / F0 — DC 03 / 60 — DC 00 / 32 — F5 / F0"),
            &dialog);
        sequence->setWordWrap(true);
        layout->addWidget(sequence);

        QCheckBox *confirm = new QCheckBox(
            QStringLiteral("Je confirme : moteur à l'arrêt, contact mis."),
            &dialog);
        layout->addWidget(confirm);

        QLabel *status = new QLabel(QStringLiteral("PRÊT — AUCUNE COMMANDE ENVOYÉE"), &dialog);
        status->setWordWrap(true);
        status->setStyleSheet(QStringLiteral("font-weight:700;"));
        layout->addWidget(status);

        QPlainTextEdit *output = new QPlainTextEdit(&dialog);
        output->setReadOnly(true);
        output->setPlaceholderText(QStringLiteral("Le journal TX/RX du test apparaîtra ici."));
        layout->addWidget(output, 1);

        QHBoxLayout *buttons = new QHBoxLayout;
        QPushButton *run = new QPushButton(QStringLiteral("Lancer le test une fois"), &dialog);
        QPushButton *close = new QPushButton(QStringLiteral("Fermer"), &dialog);
        run->setEnabled(false);
        buttons->addWidget(run);
        buttons->addStretch(1);
        buttons->addWidget(close);
        layout->addLayout(buttons);

        QObject::connect(confirm, &QCheckBox::toggled, run, [run](bool checked)
        {
            run->setEnabled(checked);
        });

        QObject::connect(close, &QPushButton::clicked, &dialog, &QDialog::accept);

        if (mems)
        {
            QObject::connect(mems, &MEMSInterface::injectionRamTestFinished, &dialog,
                             [&dialog, status, output, close, confirm, run]
                             (bool success,
                              quint16 baseRaw,
                              quint16 correctionRaw,
                              const QString &finalStatus,
                              const QString &log)
            {
                Q_UNUSED(baseRaw)
                Q_UNUSED(correctionRaw)

                dialog.setTestRunning(false);
                status->setText(finalStatus);
                status->setStyleSheet(success
                    ? QStringLiteral("font-weight:700; color:#55c979;")
                    : QStringLiteral("font-weight:700; color:#ff9828;"));
                output->setPlainText(log);
                close->setEnabled(true);
                confirm->setEnabled(false);
                run->setEnabled(false); // one shot per dialog opening
            });
        }

        QObject::connect(run, &QPushButton::clicked, &dialog,
                         [&dialog, mems, status, output, close, confirm, run]()
        {
            run->setEnabled(false);
            confirm->setEnabled(false);
            close->setEnabled(false);
            dialog.setTestRunning(true);
            status->setText(QStringLiteral("TEST EN COURS — NE PAS DÉMARRER LE MOTEUR"));
            status->setStyleSheet(QStringLiteral("font-weight:700; color:#ff9828;"));
            output->setPlainText(QStringLiteral("Contrôles de sécurité en cours..."));

            if (!mems)
            {
                dialog.setTestRunning(false);
                status->setText(QStringLiteral("TEST REFUSÉ — INTERFACE ECU INDISPONIBLE"));
                close->setEnabled(true);
                return;
            }

            QMetaObject::invokeMethod(mems,
                                      "onInjectionRamTestRequested",
                                      Qt::QueuedConnection);
        });

        dialog.exec();
    }
};

static void scheduleInjectionRamTestUi()
{
    InjectionRamTestInstaller::schedule();
}

Q_COREAPP_STARTUP_FUNCTION(scheduleInjectionRamTestUi)
