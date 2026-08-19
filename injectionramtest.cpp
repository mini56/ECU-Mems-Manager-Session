#include "mainwindow.h"
#include "memsinterface.h"
#include "i18n.h"

#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDialog>
#include <QElapsedTimer>
#include <QEvent>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
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

static quint8 byteAt(const QByteArray &bytes, int index)
{
    return static_cast<quint8>(static_cast<unsigned char>(bytes.at(index)));
}

static QString hexText(const QByteArray &bytes)
{
    if (bytes.isEmpty())
        return QStringLiteral("<aucune réponse>");
    return QString::fromLatin1(bytes.toHex(' ').toUpper());
}

static bool isAllowedTestRequest(const QByteArray &request)
{
    // Closed list.  D1 and 0x80 are allowed ONLY while mode 3 has been
    // explicitly confirmed by the state checks in the test routine.
    return request == testBytes({0xF0}) ||
           request == testBytes({0xF5}) ||
           request == testBytes({0xD1}) ||
           request == testBytes({0x80}) ||
           request == testBytes({0xC4}) ||
           request == testBytes({0xDC, 0x03}) ||
           request == testBytes({0x60}) ||
           request == testBytes({0xDC, 0x00}) ||
           request == testBytes({0x32});
}

static bool exactModeReply(const QByteArray &response, quint8 modeValue)
{
    if (response.size() == 2)
        return byteAt(response, 0) == 0xF0 && byteAt(response, 1) == modeValue;

    // Accept one leading physical/interface echo of the F0 command.
    if (response.size() == 3)
        return byteAt(response, 0) == 0xF0 &&
               byteAt(response, 1) == 0xF0 &&
               byteAt(response, 2) == modeValue;

    return false;
}

static bool decodeKnownModeReply(const QByteArray &response, quint8 *modeValue)
{
    if (!modeValue)
        return false;

    const quint8 knownModes[] = {0x14, 0x1E, 0x50};
    for (quint8 candidate : knownModes)
    {
        if (exactModeReply(response, candidate))
        {
            *modeValue = candidate;
            return true;
        }
    }
    return false;
}

static bool isAANMP002Reply(const QByteArray &response)
{
    const QByteArray id("AANMP002");
    const int first = response.indexOf(id);
    if (first < 1)
        return false;

    // The first ASCII identifier must be introduced by D1.  One additional
    // D1 immediately before it is tolerated as an interface echo.
    if (byteAt(response, first - 1) == 0xD1)
        return true;

    return first >= 2 && byteAt(response, first - 2) == 0xD1;
}

static bool parseExactData80(const QByteArray &response, quint16 *rpm)
{
    if (!rpm)
        return false;

    // AANMP002 / MEMS 1.6: command 0x80 plus a 28-byte frame whose first
    // byte is 0x1C.  Accept exactly that, with at most one leading 0x80 echo.
    int start = -1;
    if (response.size() == 29 &&
        byteAt(response, 0) == 0x80 && byteAt(response, 1) == 0x1C)
    {
        start = 0;
    }
    else if (response.size() == 30 &&
             byteAt(response, 0) == 0x80 &&
             byteAt(response, 1) == 0x80 &&
             byteAt(response, 2) == 0x1C)
    {
        start = 1;
    }
    else
    {
        return false;
    }

    *rpm = (quint16(byteAt(response, start + 2)) << 8)
         | quint16(byteAt(response, start + 3));
    return true;
}

static bool exactDcReply(const QByteArray &response, quint8 block)
{
    const QByteArray direct = testBytes({0xDC, block});
    const QByteArray commandEcho = testBytes({0xDC, 0xDC, block});
    const QByteArray fullEcho = testBytes({0xDC, block, 0xDC, block});
    return response == direct || response == commandEcho || response == fullEcho;
}

static bool parseExactWordReply(const QByteArray &response, quint8 command, quint16 *value)
{
    if (!value)
        return false;

    int payload = -1;
    if (response.size() == 3 && byteAt(response, 0) == command)
    {
        payload = 1;
    }
    else if (response.size() == 4 &&
             byteAt(response, 0) == command &&
             byteAt(response, 1) == command)
    {
        payload = 2;
    }
    else
    {
        return false;
    }

    *value = quint16(byteAt(response, payload))
           | (quint16(byteAt(response, payload + 1)) << 8);
    return true;
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
 * One-shot RAM read test for AANMP002.
 *
 * No test-specific ECU traffic exists before the user's explicit Run click.
 * D1 and 0x80 are NEVER sent until mode 3 has been positively proven.
 * No DC/offset read is sent until mode 4 has been positively proven.
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
    bool mode3Confirmed = false;
    bool mode4Confirmed = false;
    bool mode4Attempted = false;

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
        log = QStringLiteral("ECU non connecté.\nAucune commande de test n'a été envoyée.");
        finish(false, 0, 0, QStringLiteral("TEST REFUSÉ — ECU NON CONNECTÉ"));
        return;
    }

    // Local guard before the first test command.
    if (m_data.engine_rpm != 0)
    {
        log = QStringLiteral("Régime moteur déjà détecté : %1 tr/min.\n"
                             "Aucune commande de test n'a été envoyée.")
                  .arg(m_data.engine_rpm);
        finish(false, 0, 0,
               QStringLiteral("TEST REFUSÉ — MOTEUR DÉTECTÉ EN FONCTIONNEMENT"));
        return;
    }

    auto exchange = [this](const QByteArray &request, QByteArray &response) -> bool
    {
        response.clear();
        if (request.isEmpty() || !isAllowedTestRequest(request) ||
            !(m_initComplete && mems_is_connected(&m_memsinfo)))
        {
            return false;
        }

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

        // Receive-only purge: no transmitted byte is cancelled.
        if (!PurgeComm(h, PURGE_RXCLEAR))
        {
            SetCommTimeouts(h, &oldTimeouts);
            return false;
        }

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

        while (overall.elapsed() < 900 && (!gotAny || quiet.elapsed() < 150))
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

        while (overall.elapsed() < 900 && (!gotAny || quiet.elapsed() < 150))
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
        if (!isAllowedTestRequest(request))
        {
            response.clear();
            log += QStringLiteral("BLOCAGE INTERNE : commande hors liste blanche refusée.\n");
            return false;
        }

        log += QStringLiteral("TX : %1\n").arg(hexText(request));
        const bool ok = exchange(request, response);
        log += QStringLiteral("RX : %1\n").arg(hexText(response));
        return ok;
    };

    auto sendMode3 = [&](const QByteArray &request, QByteArray &response) -> bool
    {
        if (!mode3Confirmed)
        {
            response.clear();
            log += QStringLiteral("BLOCAGE INTERNE : commande mode 3 refusée sans confirmation F0 14.\n");
            return false;
        }
        return send(request, response);
    };

    auto sendMode4 = [&](const QByteArray &request, QByteArray &response) -> bool
    {
        if (!mode4Confirmed)
        {
            response.clear();
            log += QStringLiteral("BLOCAGE INTERNE : lecture mode 4 refusée sans confirmation F0 1E.\n");
            return false;
        }
        return send(request, response);
    };

    auto restoreMode3 = [&]() -> bool
    {
        if (!mode4Attempted)
            return false;

        log += QStringLiteral("\n--- Retour contrôlé au mode 3 ---\n");
        mode4Confirmed = false;

        QByteArray f5Response;
        send(testBytes({0xF5}), f5Response);

        QByteArray modeResponse;
        const bool modeRead = send(testBytes({0xF0}), modeResponse);
        const bool confirmed = modeRead && exactModeReply(modeResponse, 0x14);

        if (confirmed)
        {
            mode3Confirmed = true;
            mode4Attempted = false;
            log += QStringLiteral("MODE 3 CONFIRMÉ (F0 14).\n");
        }
        else
        {
            mode3Confirmed = false;
            log += QStringLiteral("MODE 3 NON CONFIRMÉ.\n");
        }
        return confirmed;
    };

    auto abortAfterMode4Attempt = [&](const QString &reason)
    {
        log += QStringLiteral("\nARRÊT DU TEST : %1\n").arg(reason);
        if (restoreMode3())
        {
            finish(false, baseRaw, correctionRaw,
                   QStringLiteral("TEST ARRÊTÉ — MODE 3 CONFIRMÉ"));
        }
        else
        {
            m_stopPolling = true;
            log += QStringLiteral("Polling normal arrêté et connexion fermée par sécurité.\n");
            finish(false, baseRaw, correctionRaw,
                   QStringLiteral("TEST ARRÊTÉ — MODE 3 NON CONFIRMÉ — DÉCONNECTER ET COUPER LE CONTACT"));
        }
    };

    log += QStringLiteral("TEST RAM TEMPS INJECTION — AANMP002\n");
    log += QStringLiteral("Moteur à l'arrêt / contact mis.\n");
    log += QStringLiteral("Aucune commande d'effacement, d'adaptation ou d'écriture de calibration.\n\n");

    QByteArray response;

    // CHECK 1 — Determine the current mode with F0 before any mode-sensitive
    // command.  If normal diagnostics is currently in mode 5/6 (F0 50), or
    // if a prior session left mode 4 active (F0 1E), F5 is the documented
    // route back to mode 3.  No D1 or 0x80 is sent until F0 14 is then proven.
    if (!send(testBytes({0xF0}), response))
    {
        m_stopPolling = true;
        log += QStringLiteral("Impossible de lire le mode initial. D1/80/C4 non envoyés.\n");
        finish(false, 0, 0,
               QStringLiteral("TEST REFUSÉ — MODE INITIAL INCONNU — DÉCONNECTER ET COUPER LE CONTACT"));
        return;
    }

    quint8 initialMode = 0;
    if (!decodeKnownModeReply(response, &initialMode))
    {
        m_stopPolling = true;
        log += QStringLiteral("Réponse F0 initiale non reconnue. D1/80/C4 non envoyés.\n");
        finish(false, 0, 0,
               QStringLiteral("TEST REFUSÉ — MODE INITIAL INCONNU — DÉCONNECTER ET COUPER LE CONTACT"));
        return;
    }

    if (initialMode == 0x14)
    {
        mode3Confirmed = true;
        log += QStringLiteral("Mode 3 déjà confirmé (F0 14).\n\n");
    }
    else
    {
        log += QStringLiteral("Mode non-3 détecté (F0 %1). Retour F5 demandé avant toute autre commande.\n")
                   .arg(initialMode, 2, 16, QLatin1Char('0'))
                   .toUpper();

        QByteArray f5Response;
        send(testBytes({0xF5}), f5Response);

        response.clear();
        if (!send(testBytes({0xF0}), response) || !exactModeReply(response, 0x14))
        {
            m_stopPolling = true;
            log += QStringLiteral("Mode 3 non confirmé après F5. D1/80/C4 non envoyés.\n");
            finish(false, 0, 0,
                   QStringLiteral("TEST REFUSÉ — MODE 3 NON CONFIRMÉ — DÉCONNECTER ET COUPER LE CONTACT"));
            return;
        }

        mode3Confirmed = true;
        log += QStringLiteral("Mode 3 confirmé après F5 (F0 14).\n\n");
    }

    // CHECK 2 — Exact ECU identity, while mode 3 is proven.
    response.clear();
    if (!sendMode3(testBytes({0xD1}), response) || !isAANMP002Reply(response))
    {
        log += QStringLiteral("AANMP002 non confirmé. C4 n'a pas été envoyé.\n");
        finish(false, 0, 0,
               QStringLiteral("TEST REFUSÉ — ECU AANMP002 NON CONFIRMÉ"));
        return;
    }
    log += QStringLiteral("ECU AANMP002 confirmé.\n\n");

    // CHECK 3 — Fresh exact-length 0x80 frame, while mode 3 is proven.
    response.clear();
    quint16 freshRpm = 0;
    if (!sendMode3(testBytes({0x80}), response) || !parseExactData80(response, &freshRpm))
    {
        log += QStringLiteral("Trame 0x80 MEMS 1.6 complète non validée. C4 non envoyé.\n");
        finish(false, 0, 0,
               QStringLiteral("TEST REFUSÉ — CONTRÔLE RPM IMPOSSIBLE"));
        return;
    }

    log += QStringLiteral("Régime moteur frais : %1 tr/min.\n").arg(freshRpm);
    if (freshRpm != 0)
    {
        log += QStringLiteral("C4 n'a pas été envoyé.\n");
        finish(false, 0, 0,
               QStringLiteral("TEST REFUSÉ — MOTEUR DÉTECTÉ EN FONCTIONNEMENT"));
        return;
    }

    // CHECK 4 — Reconfirm mode 3 immediately before requesting mode 4.
    response.clear();
    if (!send(testBytes({0xF0}), response) || !exactModeReply(response, 0x14))
    {
        mode3Confirmed = false;
        m_stopPolling = true;
        log += QStringLiteral("Mode 3 perdu juste avant C4. C4 non envoyé.\n"
                              "Polling normal arrêté par sécurité.\n");
        finish(false, 0, 0,
               QStringLiteral("TEST REFUSÉ — MODE 3 PERDU — DÉCONNECTER ET COUPER LE CONTACT"));
        return;
    }
    mode3Confirmed = true;
    log += QStringLiteral("Mode 3 reconfirmé juste avant C4 (F0 14).\n\n");

    // C4 itself is allowed only after the immediately preceding F0 14.
    response.clear();
    if (!sendMode3(testBytes({0xC4}), response))
    {
        mode4Attempted = true; // C4 may have reached the ECU even without a reply.
        mode3Confirmed = false;
        abortAfterMode4Attempt(QStringLiteral("aucune réponse après C4"));
        return;
    }

    mode4Attempted = true;
    mode3Confirmed = false;

    // CHECK 5 — No RAM addressing command until exact mode-4 proof.
    response.clear();
    if (!send(testBytes({0xF0}), response) || !exactModeReply(response, 0x1E))
    {
        abortAfterMode4Attempt(QStringLiteral("mode 4 non confirmé par réponse exacte F0 1E"));
        return;
    }
    mode4Confirmed = true;
    log += QStringLiteral("Mode 4 confirmé (F0 1E).\n\n");

    response.clear();
    if (!sendMode4(testBytes({0xDC, 0x03}), response) || !exactDcReply(response, 0x03))
    {
        abortAfterMode4Attempt(QStringLiteral("sélection du bloc de lecture 0x03 non confirmée"));
        return;
    }

    response.clear();
    if (!sendMode4(testBytes({0x60}), response) ||
        !parseExactWordReply(response, 0x60, &baseRaw))
    {
        abortAfterMode4Attempt(QStringLiteral("lecture RAM 0x03C0 absente ou format inattendu"));
        return;
    }
    log += QStringLiteral("RAM 0x03C0 brut : 0x%1  (%2)\n\n")
               .arg(baseRaw, 4, 16, QLatin1Char('0'))
               .arg(baseRaw)
               .toUpper();

    response.clear();
    if (!sendMode4(testBytes({0xDC, 0x00}), response) || !exactDcReply(response, 0x00))
    {
        abortAfterMode4Attempt(QStringLiteral("sélection du bloc de lecture 0x00 non confirmée"));
        return;
    }

    response.clear();
    if (!sendMode4(testBytes({0x32}), response) ||
        !parseExactWordReply(response, 0x32, &correctionRaw))
    {
        abortAfterMode4Attempt(QStringLiteral("lecture RAM 0x0064 absente ou format inattendu"));
        return;
    }
    log += QStringLiteral("RAM 0x0064 brut : 0x%1  (%2)\n\n")
               .arg(correctionRaw, 4, 16, QLatin1Char('0'))
               .arg(correctionRaw)
               .toUpper();

    if (!restoreMode3())
    {
        m_stopPolling = true;
        log += QStringLiteral("Polling normal arrêté et connexion fermée par sécurité.\n");
        finish(false, baseRaw, correctionRaw,
               QStringLiteral("TEST ARRÊTÉ — MODE 3 NON CONFIRMÉ — DÉCONNECTER ET COUPER LE CONTACT"));
        return;
    }

    finish(true, baseRaw, correctionRaw,
           QStringLiteral("MODE 3 CONFIRMÉ — TEST TERMINÉ"));
}

class InjectionRamTestInstaller : public QObject
{
public:
    explicit InjectionRamTestInstaller(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event && event->type() == QEvent::Show)
        {
            if (MainWindow *window = qobject_cast<MainWindow*>(watched))
                installButton(window);
        }
        return QObject::eventFilter(watched, event);
    }

private:
    static bool installButton(MainWindow *window)
    {
        if (!window)
            return false;

        QTabWidget *tabs = window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
        if (!tabs)
            return false;

        QWidget *roscoPage = nullptr;
        QGroupBox *session = nullptr;

        // No timer: at MainWindow Show, the ECU/ROSCO page and its session
        // group already exist.  The later visual rebuild reparents this same
        // group, so the button remains attached to it.
        for (int i = 0; i < tabs->count() && !session; ++i)
        {
            QWidget *page = realTabPage(tabs->widget(i));
            if (!page)
                continue;

            const QList<QGroupBox*> groups = page->findChildren<QGroupBox*>();
            for (QGroupBox *group : groups)
            {
                if (group && group->title().trimmed() == I18n::text(7000).trimmed())
                {
                    roscoPage = page;
                    session = group;
                    break;
                }
            }
        }

        if (!roscoPage || !session)
            return false;

        if (session->findChild<QPushButton*>(QStringLiteral("injectionRamTestButton")))
            return true;

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
        dialog.resize(760, 620);

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
            QStringLiteral("Aucune commande spécifique à ce test n'est envoyée en ouvrant cette fenêtre. "
                           "Le test ne démarre qu'après les DEUX confirmations ci-dessous et un clic sur "
                           "« Lancer le test une fois ». F0 est toujours la première commande du test. "
                           "D1 et 0x80 restent bloqués tant que F0 14 n'a pas confirmé le mode 3."),
            &dialog);
        info->setWordWrap(true);
        layout->addWidget(info);

        QLabel *sequence = new QLabel(
            QStringLiteral("Séquence verrouillée : F0 — [F5 / F0 si nécessaire] — D1 — 80 — F0 — "
                           "C4 — F0 — DC 03 / 60 — DC 00 / 32 — F5 — F0"),
            &dialog);
        sequence->setWordWrap(true);
        layout->addWidget(sequence);

        QCheckBox *confirmStopped = new QCheckBox(
            QStringLiteral("Je confirme : moteur à l'arrêt, contact mis."),
            &dialog);
        QCheckBox *confirmNoStart = new QCheckBox(
            QStringLiteral("Je confirme : je ne démarrerai pas le moteur avant la fin du test."),
            &dialog);
        layout->addWidget(confirmStopped);
        layout->addWidget(confirmNoStart);

        QLabel *status = new QLabel(QStringLiteral("PRÊT — AUCUNE COMMANDE DE TEST ENVOYÉE"), &dialog);
        status->setWordWrap(true);
        status->setStyleSheet(QStringLiteral("font-weight:700;"));
        layout->addWidget(status);

        QPlainTextEdit *output = new QPlainTextEdit(&dialog);
        output->setObjectName(QStringLiteral("injectionRamTestOutput"));
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

        const auto updateRunEnabled = [run, confirmStopped, confirmNoStart]()
        {
            run->setEnabled(confirmStopped->isChecked() && confirmNoStart->isChecked());
        };
        QObject::connect(confirmStopped, &QCheckBox::toggled, &dialog,
                         [updateRunEnabled](bool) { updateRunEnabled(); });
        QObject::connect(confirmNoStart, &QCheckBox::toggled, &dialog,
                         [updateRunEnabled](bool) { updateRunEnabled(); });
        QObject::connect(close, &QPushButton::clicked, &dialog, &QDialog::accept);

        if (mems)
        {
            QObject::connect(mems, &MEMSInterface::injectionRamTestFinished, &dialog,
                             [&dialog, status, output, close, confirmStopped, confirmNoStart, run]
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
                confirmStopped->setEnabled(false);
                confirmNoStart->setEnabled(false);
                run->setEnabled(false);
            });
        }

        QObject::connect(run, &QPushButton::clicked, &dialog,
                         [&dialog, mems, status, output, close, confirmStopped, confirmNoStart, run]()
        {
            // This is the only UI path that queues the ECU test slot.
            if (!confirmStopped->isChecked() || !confirmNoStart->isChecked())
                return;

            run->setEnabled(false);
            confirmStopped->setEnabled(false);
            confirmNoStart->setEnabled(false);
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

static void installInjectionRamTestUi()
{
    QApplication *app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (app)
        app->installEventFilter(new InjectionRamTestInstaller(app));
}

Q_COREAPP_STARTUP_FUNCTION(installInjectionRamTestUi)
