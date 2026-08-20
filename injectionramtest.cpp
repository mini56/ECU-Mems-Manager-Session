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
    // Closed read/session-only whitelist for the targeted AANMP002 test.
    // No clear/reset/adaptation/calibration/actuator command is present.
    return request == testBytes({0xF0}) ||
           request == testBytes({0xF4}) ||
           request == testBytes({0xF5}) ||
           request == testBytes({0xD1}) ||
           request == testBytes({0x80}) ||
           request == testBytes({0xC4}) ||
           request == testBytes({0xDC}) ||
           request == testBytes({0x02}) ||
           request == testBytes({0x03}) ||
           request == testBytes({0x37}) ||
           request == testBytes({0x40}) ||
           request == testBytes({0x64});
}

static bool exactModeReply(const QByteArray &response, quint8 modeValue)
{
    if (response.size() == 2)
        return byteAt(response, 0) == 0xF0 && byteAt(response, 1) == modeValue;

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

    if (byteAt(response, first - 1) == 0xD1)
        return true;

    return first >= 2 && byteAt(response, first - 2) == 0xD1;
}

static bool parseExactData80(const QByteArray &response,
                             quint16 *rpm,
                             quint8 *batteryDecivolts = nullptr)
{
    if (!rpm)
        return false;

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

    if (batteryDecivolts)
        *batteryDecivolts = byteAt(response, start + 9);

    return true;
}

static bool exactEchoReply(const QByteArray &response, quint8 value)
{
    if (response.size() == 1)
        return byteAt(response, 0) == value;

    if (response.size() == 2)
        return byteAt(response, 0) == value && byteAt(response, 1) == value;

    return false;
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

static quint16 highWordC001(quint16 value)
{
    const quint32 product = quint32(value) * quint32(0xC001u);
    return quint16((product >> 16) & 0xFFFFu);
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
 * AANMP002 transient injection capture, build #671 target.
 *
 * Phase 1: short idle communication/cadence check on RAM 0x0280/0x0281.
 * Phase 2: longer maximum-cadence capture while the operator varies engine speed.
 *
 * Important change from #670:
 * - no 2000 rpm software guard during the capture;
 * - no live R5E read in the fast loop;
 * - block 0x02 is selected once, then command 0x40 is repeated;
 * - no 600 ms pre-capture delay and no 15 ms inter-sample sleep;
 * - the next 0x40 is sent only after a complete valid reply to the previous one;
 * - on 0x0280 > 0, 0x026E and 0x03C8 are read immediately.
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

    const bool phase2Requested = property("injectionRamPhase2Requested").toBool();
    setProperty("injectionRamPhase2Requested", false);

    if (phase2Requested && !property("injectionRamPhase1Ready").toBool())
    {
        emit injectionRamTestFinished(false, 0, 0,
                                      QStringLiteral("PHASE 2 REFUSÉE — PHASE 1 À REFAIRE"),
                                      QStringLiteral("La phase 1 de contrôle au ralenti n'est pas validée."));
        return;
    }

    setProperty("injectionRamTestRunning", true);

    QString log;
    quint16 resultPrimary = 0;
    quint16 correctionRaw = 0;
    bool normalSessionConfirmed = false;
    bool mode3Confirmed = false;
    bool mode4Confirmed = false;
    bool mode4Attempted = false;

    auto finish = [this, &log](bool success,
                               quint16 primaryValue,
                               quint16 correctionValue,
                               const QString &status)
    {
        setProperty("injectionRamTestRunning", false);
        emit injectionRamTestFinished(success, primaryValue, correctionValue, status, log);
    };

    if (!(m_initComplete && mems_is_connected(&m_memsinfo)))
    {
        setProperty("injectionRamPhase1Ready", false);
        log = QStringLiteral("ECU non connecté.\nAucune commande de test n'a été envoyée.");
        finish(false, 0, 0, QStringLiteral("TEST REFUSÉ — ECU NON CONNECTÉ"));
        return;
    }

    auto exchange = [this](const QByteArray &request, QByteArray &response, int quietLimitMs = 150) -> bool
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
        const bool fastExchange = quietLimitMs <= 20;
        timeouts.ReadIntervalTimeout = fastExchange ? 5 : 20;
        timeouts.ReadTotalTimeoutConstant = fastExchange ? 10 : 35;
        timeouts.ReadTotalTimeoutMultiplier = 0;
        timeouts.WriteTotalTimeoutConstant = 100;
        timeouts.WriteTotalTimeoutMultiplier = 5;

        if (!SetCommTimeouts(h, &timeouts))
            return false;

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

        while (overall.elapsed() < 900 && (!gotAny || quiet.elapsed() < quietLimitMs))
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

        while (overall.elapsed() < 900 && (!gotAny || quiet.elapsed() < quietLimitMs))
        {
            fd_set set;
            FD_ZERO(&set);
            FD_SET(fd, &set);
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = (quietLimitMs <= 20) ? 5000 : 30000;

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

    auto send = [&exchange, &log](const QByteArray &request,
                                  QByteArray &response,
                                  int quietLimitMs = 150) -> bool
    {
        if (!isAllowedTestRequest(request))
        {
            response.clear();
            log += QStringLiteral("BLOCAGE INTERNE : commande hors liste blanche refusée.\n");
            return false;
        }

        log += QStringLiteral("TX : %1\n").arg(hexText(request));
        const bool ok = exchange(request, response, quietLimitMs);
        log += QStringLiteral("RX : %1\n").arg(hexText(response));
        return ok;
    };

    auto sendNormal = [&](const QByteArray &request, QByteArray &response) -> bool
    {
        if (!normalSessionConfirmed)
        {
            response.clear();
            log += QStringLiteral("BLOCAGE INTERNE : commande session normale refusée sans confirmation F0 50.\n");
            return false;
        }
        return send(request, response);
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
        return send(request, response, 20);
    };

    auto selectMode4Block = [&](quint8 block) -> bool
    {
        if (block != 0x02 && block != 0x03)
        {
            log += QStringLiteral("BLOCAGE INTERNE : bloc mode 4 hors liste blanche refusé.\n");
            return false;
        }

        QByteArray dcResponse;
        if (!sendMode4(testBytes({0xDC}), dcResponse) || !exactEchoReply(dcResponse, 0xDC))
            return false;

        QByteArray blockResponse;
        if (!sendMode4(testBytes({block}), blockResponse) || !exactEchoReply(blockResponse, block))
            return false;

        return true;
    };

    // Fast one-word Mode-4 read. There is no artificial quiet period: as soon as
    // the exact 3-byte or doubled-echo 4-byte reply is complete, the call returns.
    auto fastReadMode4Word = [this, &mode4Confirmed](quint8 command, quint16 *value) -> bool
    {
        if (!value || !mode4Confirmed || !isAllowedTestRequest(testBytes({command})) ||
            !(m_initComplete && mems_is_connected(&m_memsinfo)))
        {
            return false;
        }

        QByteArray response;

#ifdef WIN32
        HANDLE h = m_memsinfo.sd;
        if (h == INVALID_HANDLE_VALUE || h == NULL)
            return false;

        COMMTIMEOUTS oldTimeouts;
        if (!GetCommTimeouts(h, &oldTimeouts))
            return false;

        COMMTIMEOUTS timeouts = oldTimeouts;
        timeouts.ReadIntervalTimeout = 1;
        timeouts.ReadTotalTimeoutConstant = 12;
        timeouts.ReadTotalTimeoutMultiplier = 0;
        timeouts.WriteTotalTimeoutConstant = 60;
        timeouts.WriteTotalTimeoutMultiplier = 0;

        if (!SetCommTimeouts(h, &timeouts))
            return false;

        if (!PurgeComm(h, PURGE_RXCLEAR))
        {
            SetCommTimeouts(h, &oldTimeouts);
            return false;
        }

        const unsigned char tx = command;
        DWORD written = 0;
        if (!WriteFile(h, &tx, 1, &written, NULL) || written != 1)
        {
            SetCommTimeouts(h, &oldTimeouts);
            return false;
        }

        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < 120 && response.size() < 4)
        {
            unsigned char rx = 0;
            DWORD count = 0;
            if (!ReadFile(h, &rx, 1, &count, NULL))
                break;
            if (count == 0)
                continue;

            response.append(char(rx));
            if (parseExactWordReply(response, command, value))
            {
                SetCommTimeouts(h, &oldTimeouts);
                return true;
            }

            if (response.size() >= 3 &&
                !(response.size() == 3 && byteAt(response, 0) == command && byteAt(response, 1) == command))
            {
                break;
            }
        }

        SetCommTimeouts(h, &oldTimeouts);
        return false;
#else
        const int fd = m_memsinfo.sd;
        if (fd < 0)
            return false;

        const unsigned char tx = command;
        if (write(fd, &tx, 1) != 1)
            return false;

        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < 120 && response.size() < 4)
        {
            fd_set set;
            FD_ZERO(&set);
            FD_SET(fd, &set);
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 12000;

            const int ready = select(fd + 1, &set, NULL, NULL, &timeout);
            if (ready <= 0)
                continue;

            unsigned char rx = 0;
            if (read(fd, &rx, 1) != 1)
                continue;

            response.append(char(rx));
            if (parseExactWordReply(response, command, value))
                return true;

            if (response.size() >= 3 &&
                !(response.size() == 3 && byteAt(response, 0) == command && byteAt(response, 1) == command))
            {
                break;
            }
        }

        return false;
#endif
    };

    auto restoreNormalSession = [&]() -> bool
    {
        log += QStringLiteral("\n--- Retour contrôlé à la session normale ---\n");
        normalSessionConfirmed = false;
        mode4Confirmed = false;

        if (mode4Attempted)
        {
            QByteArray f5Response;
            send(testBytes({0xF5}), f5Response);

            QByteArray modeResponse;
            if (!send(testBytes({0xF0}), modeResponse) || !exactModeReply(modeResponse, 0x14))
            {
                mode3Confirmed = false;
                log += QStringLiteral("MODE 3 NON CONFIRMÉ après F5.\n");
                return false;
            }

            mode3Confirmed = true;
            mode4Attempted = false;
            log += QStringLiteral("MODE 3 CONFIRMÉ (F0 14).\n");
        }

        if (!mode3Confirmed)
        {
            log += QStringLiteral("Retour session normale refusé : mode 3 non confirmé.\n");
            return false;
        }

        QByteArray f4Response;
        send(testBytes({0xF4}), f4Response);

        QByteArray normalResponse;
        if (!send(testBytes({0xF0}), normalResponse) || !exactModeReply(normalResponse, 0x50))
        {
            mode3Confirmed = false;
            log += QStringLiteral("SESSION NORMALE NON CONFIRMÉE après F4.\n");
            return false;
        }

        normalSessionConfirmed = true;
        mode3Confirmed = false;
        log += QStringLiteral("SESSION NORMALE CONFIRMÉE (F0 50).\n");
        return true;
    };

    auto abortAfterMode4Attempt = [&](const QString &reason)
    {
        log += QStringLiteral("\nARRÊT DU TEST : %1\n").arg(reason);
        setProperty("injectionRamPhase1Ready", false);
        if (restoreNormalSession())
        {
            finish(false, resultPrimary, correctionRaw,
                   QStringLiteral("TEST ARRÊTÉ — SESSION NORMALE CONFIRMÉE"));
        }
        else
        {
            m_stopPolling = true;
            log += QStringLiteral("Polling normal arrêté et connexion fermée par sécurité.\n");
            finish(false, resultPrimary, correctionRaw,
                   QStringLiteral("TEST ARRÊTÉ — SESSION NON CONFIRMÉE — DÉCONNECTER ET COUPER LE CONTACT"));
        }
    };

    log += QStringLiteral("TEST DYNAMIQUE RAM TEMPS INJECTION — AANMP002\n");
    log += phase2Requested
        ? QStringLiteral("BUILD #671 — PHASE 2 : capture rapide à régime variable.\n")
        : QStringLiteral("BUILD #671 — PHASE 1 : contrôle rapide au ralenti.\n");
    log += QStringLiteral("Véhicule immobilisé. Lecture seule. Aucun contrôle logiciel 2000 tr/min pendant la capture.\n");
    log += QStringLiteral("La boucle rapide reste sur le bloc 0x02 et n'envoie la commande 0x40 suivante qu'après une réponse complète valide.\n\n");

    QByteArray response;

    // CHECK 1 — Prove or restore a known normal diagnostic session.
    if (!send(testBytes({0xF0}), response))
    {
        setProperty("injectionRamPhase1Ready", false);
        m_stopPolling = true;
        log += QStringLiteral("Impossible de lire le mode initial. D1/80/C4 non envoyés.\n");
        finish(false, 0, 0,
               QStringLiteral("TEST REFUSÉ — MODE INITIAL INCONNU — DÉCONNECTER ET COUPER LE CONTACT"));
        return;
    }

    quint8 initialMode = 0;
    if (!decodeKnownModeReply(response, &initialMode))
    {
        setProperty("injectionRamPhase1Ready", false);
        m_stopPolling = true;
        log += QStringLiteral("Réponse F0 initiale non reconnue. D1/80/C4 non envoyés.\n");
        finish(false, 0, 0,
               QStringLiteral("TEST REFUSÉ — MODE INITIAL INCONNU — DÉCONNECTER ET COUPER LE CONTACT"));
        return;
    }

    if (initialMode == 0x50)
    {
        normalSessionConfirmed = true;
        log += QStringLiteral("Session normale déjà confirmée (F0 50).\n\n");
    }
    else
    {
        if (initialMode == 0x1E)
        {
            log += QStringLiteral("Mode 4 détecté au départ. Retour F5 demandé avant toute identification.\n");
            QByteArray f5Response;
            send(testBytes({0xF5}), f5Response);

            response.clear();
            if (!send(testBytes({0xF0}), response) || !exactModeReply(response, 0x14))
            {
                setProperty("injectionRamPhase1Ready", false);
                m_stopPolling = true;
                finish(false, 0, 0,
                       QStringLiteral("TEST REFUSÉ — MODE 3 NON CONFIRMÉ — DÉCONNECTER ET COUPER LE CONTACT"));
                return;
            }
            mode3Confirmed = true;
        }
        else if (initialMode == 0x14)
        {
            mode3Confirmed = true;
        }

        QByteArray f4Response;
        send(testBytes({0xF4}), f4Response);

        response.clear();
        if (!send(testBytes({0xF0}), response) || !exactModeReply(response, 0x50))
        {
            setProperty("injectionRamPhase1Ready", false);
            m_stopPolling = true;
            finish(false, 0, 0,
                   QStringLiteral("TEST REFUSÉ — SESSION NORMALE NON CONFIRMÉE — DÉCONNECTER ET COUPER LE CONTACT"));
            return;
        }

        normalSessionConfirmed = true;
        mode3Confirmed = false;
        log += QStringLiteral("Session normale confirmée après F4 (F0 50).\n\n");
    }

    // CHECK 2 — Exact ECU identity.
    response.clear();
    if (!sendNormal(testBytes({0xD1}), response) || !isAANMP002Reply(response))
    {
        setProperty("injectionRamPhase1Ready", false);
        log += QStringLiteral("AANMP002 non confirmé. C4 n'a pas été envoyé.\n");
        finish(false, 0, 0, QStringLiteral("TEST REFUSÉ — ECU AANMP002 NON CONFIRMÉ"));
        return;
    }
    log += QStringLiteral("ECU AANMP002 confirmé.\n\n");

    // Fresh normal RPM is kept only as context. Phase 1 also uses it to make sure
    // the first cadence test really starts near idle; phase 2 has no RPM limit.
    response.clear();
    quint16 freshRpm = 0;
    quint8 batteryDecivolts = 0;
    if (!sendNormal(testBytes({0x80}), response) ||
        !parseExactData80(response, &freshRpm, &batteryDecivolts))
    {
        setProperty("injectionRamPhase1Ready", false);
        finish(false, 0, 0, QStringLiteral("TEST REFUSÉ — CONTRÔLE RPM INITIAL IMPOSSIBLE"));
        return;
    }

    log += QStringLiteral("Régime avant entrée Mode 4 : %1 tr/min.\n").arg(freshRpm);
    log += QStringLiteral("Tension batterie : %1 V.\n")
               .arg(double(batteryDecivolts) / 10.0, 0, 'f', 1);

    if (!phase2Requested && (freshRpm < 500 || freshRpm > 1500))
    {
        setProperty("injectionRamPhase1Ready", false);
        log += QStringLiteral("Phase 1 demandée au ralenti : régime initial hors fenêtre 500–1500 tr/min.\n");
        finish(false, 0, 0, QStringLiteral("PHASE 1 REFUSÉE — METTRE LE MOTEUR AU RALENTI ET REFAIRE"));
        return;
    }

    // Enter mode 3 then mode 4 with the already-proven session guards.
    QByteArray f5Response;
    send(testBytes({0xF5}), f5Response);

    response.clear();
    if (!send(testBytes({0xF0}), response) || !exactModeReply(response, 0x14))
    {
        setProperty("injectionRamPhase1Ready", false);
        normalSessionConfirmed = false;
        mode3Confirmed = false;
        m_stopPolling = true;
        finish(false, 0, 0,
               QStringLiteral("TEST REFUSÉ — MODE 3 NON CONFIRMÉ — DÉCONNECTER ET COUPER LE CONTACT"));
        return;
    }

    normalSessionConfirmed = false;
    mode3Confirmed = true;
    log += QStringLiteral("Mode 3 confirmé (F0 14).\n");

    response.clear();
    if (!sendMode3(testBytes({0xC4}), response))
    {
        mode4Attempted = true;
        mode3Confirmed = false;
        abortAfterMode4Attempt(QStringLiteral("aucune réponse après C4"));
        return;
    }

    mode4Attempted = true;
    mode3Confirmed = false;

    response.clear();
    if (!send(testBytes({0xF0}), response, 20) || !exactModeReply(response, 0x1E))
    {
        abortAfterMode4Attempt(QStringLiteral("mode 4 non confirmé par réponse exacte F0 1E"));
        return;
    }
    mode4Confirmed = true;
    log += QStringLiteral("Mode 4 confirmé (F0 1E).\n");

    // ROM proof: DC stores the block in RAM 0x015B, so select block 02 once.
    if (!selectMode4Block(0x02))
    {
        abortAfterMode4Attempt(QStringLiteral("sélection persistante du bloc 0x02 impossible"));
        return;
    }
    log += QStringLiteral("Bloc 0x02 sélectionné une seule fois. Début de la boucle 0x40 sans pause artificielle.\n\n");

    const qint64 captureDurationMs = phase2Requested ? 8000 : 1500;
    QElapsedTimer captureTimer;
    captureTimer.start();

    int reads = 0;
    quint8 maxTransientCounter = 0;
    quint8 minPreviousX = 0xFF;
    quint8 maxPreviousX = 0;
    quint16 lastPair = 0;
    bool transientSeen = false;
    qint64 transientAtMs = -1;
    quint8 transientCounter = 0;
    quint8 previousX = 0;

    while (captureTimer.elapsed() < captureDurationMs)
    {
        quint16 pair = 0;
        if (!fastReadMode4Word(0x40, &pair))
        {
            log += QStringLiteral("Lecture rapide 0x40 interrompue après %1 lectures à %2 ms : réponse absente ou invalide.\n")
                       .arg(reads)
                       .arg(captureTimer.elapsed());
            abortAfterMode4Attempt(QStringLiteral("communication rapide 0x40 perdue"));
            return;
        }

        ++reads;
        lastPair = pair;
        const quint8 counter = quint8(pair & 0x00FFu);
        const quint8 prev = quint8((pair >> 8) & 0x00FFu);

        if (counter > maxTransientCounter)
            maxTransientCounter = counter;
        if (prev < minPreviousX)
            minPreviousX = prev;
        if (prev > maxPreviousX)
            maxPreviousX = prev;

        if (counter != 0)
        {
            transientSeen = true;
            transientAtMs = captureTimer.elapsed();
            transientCounter = counter;
            previousX = prev;
            break;
        }
    }

    const qint64 elapsedMs = captureTimer.elapsed();
    const double averageMs = reads > 0 ? double(elapsedMs) / double(reads) : 0.0;
    const double readsPerSecond = elapsedMs > 0 ? (double(reads) * 1000.0) / double(elapsedMs) : 0.0;

    log += QStringLiteral("--- STATISTIQUES BOUCLE RAPIDE ---\n");
    log += QStringLiteral("Durée : %1 ms ; lectures 0x40 complètes : %2 ; moyenne : %3 ms/lecture ; cadence : %4 lectures/s.\n")
               .arg(elapsedMs)
               .arg(reads)
               .arg(averageMs, 0, 'f', 2)
               .arg(readsPerSecond, 0, 'f', 1);
    log += QStringLiteral("0280 max=%1 ; plage 0281=%2..%3 ; dernière paire=0x%4.\n")
               .arg(maxTransientCounter)
               .arg(minPreviousX == 0xFF ? 0 : minPreviousX)
               .arg(maxPreviousX)
               .arg(lastPair, 4, 16, QLatin1Char('0'))
               .toUpper();

    if (!phase2Requested)
    {
        // Phase 1 is a communication/cadence gate only. Even if a non-zero counter
        // happens at idle, report it, but do not perform the scientific phase-2 readout.
        if (transientSeen)
        {
            log += QStringLiteral("NOTE PHASE 1 : 0280 est devenu non nul (%1) à %2 ms ; 0281=%3.\n")
                       .arg(transientCounter)
                       .arg(transientAtMs)
                       .arg(previousX);
        }

        if (!restoreNormalSession())
        {
            setProperty("injectionRamPhase1Ready", false);
            m_stopPolling = true;
            finish(false, 0, 0,
                   QStringLiteral("PHASE 1 ARRÊTÉE — SESSION NON CONFIRMÉE — DÉCONNECTER ET COUPER LE CONTACT"));
            return;
        }

        QByteArray final80;
        quint16 finalRpm = 0;
        quint8 finalBattery = 0;
        if (!sendNormal(testBytes({0x80}), final80) ||
            !parseExactData80(final80, &finalRpm, &finalBattery))
        {
            setProperty("injectionRamPhase1Ready", false);
            finish(false, 0, 0,
                   QStringLiteral("PHASE 1 TERMINÉE — SESSION NORMALE CONFIRMÉE — CONTRÔLE FINAL IMPOSSIBLE"));
            return;
        }

        log += QStringLiteral("Régime après phase 1 : %1 tr/min ; batterie %2 V.\n")
                   .arg(finalRpm)
                   .arg(double(finalBattery) / 10.0, 0, 'f', 1);
        setProperty("injectionRamPhase1Ready", true);
        finish(true, 0, 0,
               QStringLiteral("PHASE 1 OK — LANCEZ LA PHASE 2 ET FAITES VARIER LE RÉGIME"));
        return;
    }

    // Phase 2: once 0280 becomes non-zero, stay on block 02 and read 026E
    // immediately, then switch to block 03 only once to read 03C8.
    quint16 r026E = 0;
    quint16 r03C8 = 0;

    if (transientSeen)
    {
        log += QStringLiteral("*** TRANSITOIRE CAPTURÉ à %1 ms : 0280=%2 ; 0281=%3. ***\n")
                   .arg(transientAtMs)
                   .arg(transientCounter)
                   .arg(previousX);

        if (!fastReadMode4Word(0x37, &r026E))
        {
            abortAfterMode4Attempt(QStringLiteral("0280 capturé mais lecture immédiate 0x026E impossible"));
            return;
        }
        log += QStringLiteral("Lecture immédiate 0x026E : %1.\n").arg(r026E);

        if (!selectMode4Block(0x03))
        {
            abortAfterMode4Attempt(QStringLiteral("0280 capturé mais sélection bloc 0x03 impossible"));
            return;
        }

        if (!fastReadMode4Word(0x64, &r03C8))
        {
            abortAfterMode4Attempt(QStringLiteral("0280 capturé mais lecture immédiate 0x03C8 impossible"));
            return;
        }

        resultPrimary = r03C8;
        const quint16 conditionalTicks = highWordC001(r026E);
        const quint32 candidateRaw = quint32(r03C8) + quint32(conditionalTicks);
        const quint16 candidateTicks = candidateRaw > 0xFFFFu ? quint16(0xFFFFu) : quint16(candidateRaw);

        log += QStringLiteral("Lecture immédiate 0x03C8 : %1 ticks = %2 ms.\n")
                   .arg(r03C8)
                   .arg(double(r03C8) * 0.002, 0, 'f', 3);
        log += QStringLiteral("S(0x026E)=%1 ticks ; candidat 0x03C8+S=%2 ticks = %3 ms.\n")
                   .arg(conditionalTicks)
                   .arg(candidateTicks)
                   .arg(double(candidateTicks) * 0.002, 0, 'f', 3);
        log += QStringLiteral("Le candidat reste une valeur de comparaison ; il n'est pas déclaré validé automatiquement.\n");
    }
    else
    {
        log += QStringLiteral("AUCUN TRANSITOIRE CAPTURÉ pendant la phase 2 rapide.\n");
    }

    if (!restoreNormalSession())
    {
        setProperty("injectionRamPhase1Ready", false);
        m_stopPolling = true;
        finish(false, resultPrimary, correctionRaw,
               QStringLiteral("TEST ARRÊTÉ — SESSION NON CONFIRMÉE — DÉCONNECTER ET COUPER LE CONTACT"));
        return;
    }

    QByteArray final80;
    quint16 finalRpm = 0;
    quint8 finalBattery = 0;
    if (!sendNormal(testBytes({0x80}), final80) ||
        !parseExactData80(final80, &finalRpm, &finalBattery))
    {
        setProperty("injectionRamPhase1Ready", false);
        finish(false, resultPrimary, correctionRaw,
               QStringLiteral("PHASE 2 TERMINÉE — SESSION NORMALE CONFIRMÉE — CONTRÔLE RPM FINAL IMPOSSIBLE"));
        return;
    }

    log += QStringLiteral("Régime après retour normal : %1 tr/min ; batterie %2 V.\n")
               .arg(finalRpm)
               .arg(double(finalBattery) / 10.0, 0, 'f', 1);

    setProperty("injectionRamPhase1Ready", false);

    if (transientSeen)
    {
        finish(true, resultPrimary, correctionRaw,
               QStringLiteral("TRANSITOIRE CAPTURÉ — SESSION NORMALE CONFIRMÉE"));
    }
    else
    {
        finish(false, resultPrimary, correctionRaw,
               QStringLiteral("PHASE 2 TERMINÉE — AUCUN TRANSITOIRE CAPTURÉ"));
    }
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
            QStringLiteral("Test transitoire RAM temps injection — AANMP002 — capture rapide en 2 phases"),
            session);
        warning->setObjectName(QStringLiteral("injectionRamTestWarning"));
        warning->setWordWrap(true);
        warning->setStyleSheet(QStringLiteral("font-weight:700; color:#ff9828;"));
        grid->addWidget(warning, row, 0, 1, 4);

        QPushButton *button = new QPushButton(QStringLiteral("Ouvrir le test transitoire temps injection"), session);
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
        if (mems)
        {
            mems->setProperty("injectionRamPhase1Ready", false);
            mems->setProperty("injectionRamPhase2Requested", false);
        }

        InjectionRamTestDialog dialog(window);
        // Keep this exact title: the automatic logger recognises it.
        dialog.setWindowTitle(QStringLiteral("Test dynamique temps injection — AANMP002"));
        dialog.setModal(true);
        dialog.resize(820, 700);

        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        layout->setContentsMargins(14, 14, 14, 14);
        layout->setSpacing(9);

        QLabel *warning = new QLabel(
            QStringLiteral("<b>BUILD #671 — CAPTURE RAPIDE EN 2 PHASES</b><br>"
                           "PHASE 1 : laissez le moteur au ralenti et lancez le contrôle court.<br>"
                           "Si la phase 1 est correcte, la fenêtre vous proposera la PHASE 2. "
                           "Pendant la phase 2, faites varier le régime en accélérant puis en relâchant. "
                           "Il n'est pas nécessaire de maintenir un régime constant."),
            &dialog);
        warning->setWordWrap(true);
        warning->setAlignment(Qt::AlignCenter);
        warning->setStyleSheet(QStringLiteral("font-size:15px; font-weight:700; color:#ff9828;"));
        layout->addWidget(warning);

        QLabel *info = new QLabel(
            QStringLiteral("La boucle rapide sélectionne le bloc 0x02 une seule fois puis répète uniquement 0x40. "
                           "Aucune lecture RPM n'est faite dans cette boucle et aucun garde-fou logiciel 2000 tr/min n'est appliqué. "
                           "Chaque nouvelle commande 0x40 n'est envoyée qu'après réception complète de la précédente. "
                           "Dès que 0x0280 devient non nul, 0x026E puis 0x03C8 sont lus immédiatement."),
            &dialog);
        info->setWordWrap(true);
        layout->addWidget(info);

        QLabel *sequence = new QLabel(
            QStringLiteral("Séquence : session vérifiée — AANMP002 — 80 initial — F5 — F0 14 — C4 — F0 1E — "
                           "DC 02 une seule fois — 40/40/40/... — [si 0280>0 : 37 — DC 03 — 64] — "
                           "F5 — F0 14 — F4 — F0 50 — 80 final"),
            &dialog);
        sequence->setWordWrap(true);
        layout->addWidget(sequence);

        QCheckBox *confirmStopped = new QCheckBox(
            QStringLiteral("Je confirme : véhicule immobilisé pendant tout le test."),
            &dialog);
        QCheckBox *confirmVariable = new QCheckBox(
            QStringLiteral("Je confirme : phase 1 au ralenti, puis régime variable pendant la phase 2."),
            &dialog);
        layout->addWidget(confirmStopped);
        layout->addWidget(confirmVariable);

        QLabel *status = new QLabel(QStringLiteral("PRÊT — PHASE 1 AU RALENTI"), &dialog);
        status->setWordWrap(true);
        status->setStyleSheet(QStringLiteral("font-weight:700;"));
        layout->addWidget(status);

        QPlainTextEdit *output = new QPlainTextEdit(&dialog);
        output->setObjectName(QStringLiteral("injectionRamTestOutput"));
        output->setReadOnly(true);
        output->setPlaceholderText(QStringLiteral("Le journal de chaque phase et la cadence réelle apparaîtront ici."));
        layout->addWidget(output, 1);

        QHBoxLayout *buttons = new QHBoxLayout;
        QPushButton *run = new QPushButton(QStringLiteral("Lancer PHASE 1 au ralenti"), &dialog);
        QPushButton *close = new QPushButton(QStringLiteral("Fermer"), &dialog);
        run->setEnabled(false);
        buttons->addWidget(run);
        buttons->addStretch(1);
        buttons->addWidget(close);
        layout->addLayout(buttons);

        const auto updateRunEnabled = [run, confirmStopped, confirmVariable]()
        {
            run->setEnabled(confirmStopped->isChecked() && confirmVariable->isChecked());
        };
        QObject::connect(confirmStopped, &QCheckBox::toggled, &dialog,
                         [updateRunEnabled](bool) { updateRunEnabled(); });
        QObject::connect(confirmVariable, &QCheckBox::toggled, &dialog,
                         [updateRunEnabled](bool) { updateRunEnabled(); });
        QObject::connect(close, &QPushButton::clicked, &dialog, &QDialog::accept);

        if (mems)
        {
            QObject::connect(mems, &MEMSInterface::injectionRamTestFinished, &dialog,
                             [&dialog, mems, status, output, close, confirmStopped, confirmVariable, run]
                             (bool success,
                              quint16 primaryRaw,
                              quint16 correctionValue,
                              const QString &finalStatus,
                              const QString &testLog)
            {
                Q_UNUSED(primaryRaw)
                Q_UNUSED(correctionValue)

                dialog.setTestRunning(false);
                status->setText(finalStatus);
                status->setStyleSheet(success
                    ? QStringLiteral("font-weight:700; color:#55c979;")
                    : QStringLiteral("font-weight:700; color:#ff9828;"));
                output->setPlainText(testLog);
                close->setEnabled(true);

                const bool phase1Ready = mems && mems->property("injectionRamPhase1Ready").toBool();
                if (phase1Ready && finalStatus.startsWith(QStringLiteral("PHASE 1 OK")))
                {
                    confirmStopped->setEnabled(false);
                    confirmVariable->setEnabled(false);
                    run->setText(QStringLiteral("Lancer PHASE 2 — régime variable"));
                    run->setEnabled(true);
                }
                else
                {
                    confirmStopped->setEnabled(false);
                    confirmVariable->setEnabled(false);
                    run->setEnabled(false);
                }
            });
        }

        QObject::connect(run, &QPushButton::clicked, &dialog,
                         [&dialog, mems, status, output, close, confirmStopped, confirmVariable, run]()
        {
            if (!mems || !confirmStopped->isChecked() || !confirmVariable->isChecked())
                return;

            const bool phase2 = mems->property("injectionRamPhase1Ready").toBool();
            mems->setProperty("injectionRamPhase2Requested", phase2);

            run->setEnabled(false);
            confirmStopped->setEnabled(false);
            confirmVariable->setEnabled(false);
            close->setEnabled(false);
            dialog.setTestRunning(true);

            if (phase2)
            {
                status->setText(QStringLiteral("PHASE 2 EN COURS — FAITES VARIER LE RÉGIME : ACCÉLÉREZ PUIS RELÂCHEZ"));
                output->setPlainText(QStringLiteral("Phase 2 : préparation de la capture rapide. Faites varier le régime pendant la mesure."));
            }
            else
            {
                status->setText(QStringLiteral("PHASE 1 EN COURS — LAISSEZ LE MOTEUR AU RALENTI"));
                output->setPlainText(QStringLiteral("Phase 1 : contrôle de communication et mesure de la cadence au ralenti..."));
            }
            status->setStyleSheet(QStringLiteral("font-weight:700; color:#ff9828;"));

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
