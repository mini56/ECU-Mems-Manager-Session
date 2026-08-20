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
#include <QThread>
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
           request == testBytes({0x00}) ||
           request == testBytes({0x02}) ||
           request == testBytes({0x03}) ||
           request == testBytes({0x2F}) ||
           request == testBytes({0x32}) ||
           request == testBytes({0x33}) ||
           request == testBytes({0x3D}) ||
           request == testBytes({0x6D}) ||
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
 * Targeted transient injection test for AANMP002.
 *
 * Guards retained from the proven tests:
 * - connected ECU only;
 * - exact AANMP002 identification in F0 50;
 * - authoritative fresh 0x80 RPM between 500 and 2000 rpm;
 * - exact F0 14 before C4;
 * - exact F0 1E before any RAM read;
 * - live Mode-4 R5E RPM guard during the transient capture;
 * - closed read-only command whitelist;
 * - controlled return F5 -> F0 14 -> F4 -> F0 50;
 * - final fresh 0x80 RPM check after returning to normal mode;
 * - stop polling on any unproven final session.
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
        log = QStringLiteral("ECU non connecté.\nAucune commande de test n'a été envoyée.");
        finish(false, 0, 0, QStringLiteral("TEST REFUSÉ — ECU NON CONNECTÉ"));
        return;
    }

    // Early cached guard only. The fresh 0x80 check below is authoritative.
    if (m_data.engine_rpm > 2000)
    {
        log = QStringLiteral("Régime moteur déjà trop élevé : %1 tr/min.\n"
                             "Aucune commande de test n'a été envoyée.")
                  .arg(m_data.engine_rpm);
        finish(false, 0, 0, QStringLiteral("TEST REFUSÉ — RÉGIME TROP ÉLEVÉ"));
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
        if (block != 0x00 && block != 0x02 && block != 0x03)
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

    auto readMode4Word = [&](quint8 command,
                             quint16 address,
                             const QString &label,
                             quint16 *value) -> bool
    {
        if (!value)
            return false;

        QByteArray wordResponse;
        if (!sendMode4(testBytes({command}), wordResponse) ||
            !parseExactWordReply(wordResponse, command, value))
        {
            log += QStringLiteral("Lecture %1 / RAM 0x%2 absente ou format inattendu.\n")
                       .arg(label)
                       .arg(address, 4, 16, QLatin1Char('0'))
                       .toUpper();
            return false;
        }

        log += QStringLiteral("RAM 0x%1 — %2 : 0x%3  (%4)\n")
                   .arg(address, 4, 16, QLatin1Char('0'))
                   .arg(label)
                   .arg(*value, 4, 16, QLatin1Char('0'))
                   .arg(*value)
                   .toUpper();
        return true;
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
    log += QStringLiteral("Variante transitoire — capture répétée de l'enrichissement d'accélération.\n");
    log += QStringLiteral("Plage autorisée : 500 à 2000 tr/min. Véhicule immobilisé.\n");
    log += QStringLiteral("Pendant la capture : UNE impulsion brève d'accélérateur, puis relâcher, sans dépasser 2000 tr/min.\n");
    log += QStringLiteral("Lecture seule : R5E/0x005E, 0x0064, 0x0066, 0x026E, 0x0280/0x0281 et 0x03C8.\n");
    log += QStringLiteral("Aucune commande d'effacement, d'adaptation, d'actionneur ou d'écriture de calibration.\n\n");

    QByteArray response;

    // CHECK 1 — Prove or restore a known normal diagnostic session.
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
                m_stopPolling = true;
                log += QStringLiteral("Mode 3 non confirmé après F5. D1/80/C4 non envoyés.\n");
                finish(false, 0, 0,
                       QStringLiteral("TEST REFUSÉ — MODE 3 NON CONFIRMÉ — DÉCONNECTER ET COUPER LE CONTACT"));
                return;
            }
            mode3Confirmed = true;
        }
        else if (initialMode == 0x14)
        {
            mode3Confirmed = true;
            log += QStringLiteral("Mode 3 détecté au départ (F0 14).\n");
        }

        QByteArray f4Response;
        send(testBytes({0xF4}), f4Response);

        response.clear();
        if (!send(testBytes({0xF0}), response) || !exactModeReply(response, 0x50))
        {
            m_stopPolling = true;
            log += QStringLiteral("Session normale F0 50 non confirmée après F4. D1/80/C4 non envoyés.\n");
            finish(false, 0, 0,
                   QStringLiteral("TEST REFUSÉ — SESSION NORMALE NON CONFIRMÉE — DÉCONNECTER ET COUPER LE CONTACT"));
            return;
        }

        normalSessionConfirmed = true;
        mode3Confirmed = false;
        log += QStringLiteral("Session normale confirmée après F4 (F0 50).\n\n");
    }

    // CHECK 2 — Exact ECU identity before any mode change.
    response.clear();
    if (!sendNormal(testBytes({0xD1}), response) || !isAANMP002Reply(response))
    {
        log += QStringLiteral("AANMP002 non confirmé dans la session normale. C4 n'a pas été envoyé.\n");
        finish(false, 0, 0, QStringLiteral("TEST REFUSÉ — ECU AANMP002 NON CONFIRMÉ"));
        return;
    }
    log += QStringLiteral("ECU AANMP002 confirmé.\n\n");

    // CHECK 3 — Fresh authoritative RPM and battery frame immediately before F5/C4.
    response.clear();
    quint16 freshRpm = 0;
    quint8 batteryDecivolts = 0;
    if (!sendNormal(testBytes({0x80}), response) ||
        !parseExactData80(response, &freshRpm, &batteryDecivolts))
    {
        log += QStringLiteral("Trame 0x80 MEMS 1.6 complète non validée. C4 non envoyé.\n");
        finish(false, 0, 0, QStringLiteral("TEST REFUSÉ — CONTRÔLE RPM IMPOSSIBLE"));
        return;
    }

    log += QStringLiteral("Régime moteur frais avant capture : %1 tr/min.\n").arg(freshRpm);
    log += QStringLiteral("Tension batterie fraîche : %1 V.\n")
               .arg(double(batteryDecivolts) / 10.0, 0, 'f', 1);

    if (freshRpm < 500 || freshRpm > 2000)
    {
        log += QStringLiteral("C4 n'a pas été envoyé : régime hors plage autorisée 500–2000 tr/min.\n");
        finish(false, 0, 0, QStringLiteral("TEST REFUSÉ — RÉGIME HORS PLAGE"));
        return;
    }

    // CHECK 4 — Enter mode 3 and prove F0 14 immediately before C4.
    QByteArray f5Response;
    send(testBytes({0xF5}), f5Response);

    response.clear();
    if (!send(testBytes({0xF0}), response) || !exactModeReply(response, 0x14))
    {
        normalSessionConfirmed = false;
        mode3Confirmed = false;
        m_stopPolling = true;
        log += QStringLiteral("Mode 3 non confirmé après F5. C4 non envoyé.\n"
                              "Polling normal arrêté par sécurité.\n");
        finish(false, 0, 0,
               QStringLiteral("TEST REFUSÉ — MODE 3 NON CONFIRMÉ — DÉCONNECTER ET COUPER LE CONTACT"));
        return;
    }

    normalSessionConfirmed = false;
    mode3Confirmed = true;
    log += QStringLiteral("Mode 3 confirmé juste avant C4 (F0 14).\n\n");

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

    // CHECK 5 — Exact mode-4 proof before RAM addressing.
    response.clear();
    if (!send(testBytes({0xF0}), response, 20) || !exactModeReply(response, 0x1E))
    {
        abortAfterMode4Attempt(QStringLiteral("mode 4 non confirmé par réponse exacte F0 1E"));
        return;
    }
    mode4Confirmed = true;
    log += QStringLiteral("Mode 4 confirmé (F0 1E).\n\n");

    QThread::msleep(600);

    const int maxSamples = 24;
    const int keepAfterFirstTransient = 5;
    bool transientSeen = false;
    int firstTransientSample = -1;
    int samplesAfterFirstTransient = 0;
    quint8 maxTransientCounter = 0;
    quint16 last03C8 = 0;
    quint16 last0064 = 0;
    quint16 last0066 = 0;
    quint16 last026E = 0;
    quint16 lastMode4Rpm = 0;

    log += QStringLiteral("--- CAPTURE TRANSITOIRE — %1 ÉCHANTILLONS MAXIMUM ---\n")
               .arg(maxSamples);
    log += QStringLiteral("Le registre R5E/0x005E est lu à chaque échantillon comme garde-fou régime en Mode 4.\n\n");

    for (int sample = 1; sample <= maxSamples; ++sample)
    {
        quint16 mode4Rpm = 0;
        quint16 r0064 = 0;
        quint16 r0066 = 0;
        quint16 r026E = 0;
        quint16 r0280Pair = 0;
        quint16 r03C8 = 0;

        if (!selectMode4Block(0x00) ||
            !readMode4Word(0x2F, 0x005E, QStringLiteral("R5E régime Mode 4"), &mode4Rpm) ||
            !readMode4Word(0x32, 0x0064, QStringLiteral("compensation tension"), &r0064) ||
            !readMode4Word(0x33, 0x0066, QStringLiteral("R66 persistant"), &r0066))
        {
            abortAfterMode4Attempt(QStringLiteral("lecture bloc 0x00 impossible pendant capture transitoire"));
            return;
        }

        lastMode4Rpm = mode4Rpm;
        last0064 = r0064;
        last0066 = r0066;
        correctionRaw = r0064;

        if (mode4Rpm < 500 || mode4Rpm > 2000)
        {
            log += QStringLiteral("GARDE-FOU RÉGIME : R5E = %1 tr/min, hors plage 500–2000. Capture interrompue immédiatement.\n")
                       .arg(mode4Rpm);
            abortAfterMode4Attempt(QStringLiteral("garde-fou régime Mode 4 déclenché"));
            return;
        }

        if (!selectMode4Block(0x02) ||
            !readMode4Word(0x37, 0x026E, QStringLiteral("source transitoire"), &r026E) ||
            !readMode4Word(0x40, 0x0280, QStringLiteral("paire 0x0280/0x0281"), &r0280Pair))
        {
            abortAfterMode4Attempt(QStringLiteral("lecture bloc 0x02 impossible pendant capture transitoire"));
            return;
        }

        if (!selectMode4Block(0x03) ||
            !readMode4Word(0x64, 0x03C8, QStringLiteral("temps injection normal"), &r03C8))
        {
            abortAfterMode4Attempt(QStringLiteral("lecture 0x03C8 impossible pendant capture transitoire"));
            return;
        }

        last026E = r026E;
        last03C8 = r03C8;
        resultPrimary = r03C8;

        const quint8 transientCounter = quint8(r0280Pair & 0x00FFu);
        const quint8 previousX = quint8((r0280Pair >> 8) & 0x00FFu);
        const quint16 conditionalTicks = highWordC001(r026E);
        const quint32 candidateRaw = quint32(r03C8) + quint32(conditionalTicks);
        const quint16 candidateTicks = candidateRaw > 0xFFFFu ? quint16(0xFFFFu) : quint16(candidateRaw);

        if (transientCounter > maxTransientCounter)
            maxTransientCounter = transientCounter;

        const qint32 correctionTicks = qint32(r0064) - 0x8000;
        const qint32 expected03C8 = qint32(r0066) + correctionTicks;
        const qint32 delta03C8 = qint32(r03C8) - expected03C8;

        log += QStringLiteral("ÉCHANTILLON %1/%2 : RPM=%3 ; 0280=%4 ; 0281=%5 ; 026E=%6 ; "
                              "03C8=%7 ticks=%8 ms ; S(026E)=%9 ticks ; candidat 03C8+S=%10 ticks=%11 ms ; "
                              "R66=%12 ; corr=%13 ; écart03C8=%14 ticks\n")
                   .arg(sample)
                   .arg(maxSamples)
                   .arg(mode4Rpm)
                   .arg(transientCounter)
                   .arg(previousX)
                   .arg(r026E)
                   .arg(r03C8)
                   .arg(double(r03C8) * 0.002, 0, 'f', 3)
                   .arg(conditionalTicks)
                   .arg(candidateTicks)
                   .arg(double(candidateTicks) * 0.002, 0, 'f', 3)
                   .arg(r0066)
                   .arg(correctionTicks)
                   .arg(delta03C8);

        if (transientCounter != 0)
        {
            if (!transientSeen)
            {
                transientSeen = true;
                firstTransientSample = sample;
                samplesAfterFirstTransient = 0;
                log += QStringLiteral("*** TRANSITOIRE DÉTECTÉ : compteur RAM 0x0280 devient non nul à l'échantillon %1. ***\n")
                           .arg(sample);
            }
            else
            {
                ++samplesAfterFirstTransient;
            }
        }
        else if (transientSeen)
        {
            ++samplesAfterFirstTransient;
        }

        if (transientSeen && samplesAfterFirstTransient >= keepAfterFirstTransient)
        {
            log += QStringLiteral("Capture poursuivie %1 échantillons après la première détection ; arrêt de la série.\n")
                       .arg(keepAfterFirstTransient);
            break;
        }

        QThread::msleep(15);
    }

    log += QStringLiteral("\n--- SYNTHÈSE CAPTURE TRANSITOIRE ---\n");
    log += QStringLiteral("Dernier régime Mode 4 R5E : %1 tr/min.\n").arg(lastMode4Rpm);
    log += QStringLiteral("Dernier 0x0064 : 0x%1 ; dernier R66 : %2 ; dernier 0x026E : %3.\n")
               .arg(last0064, 4, 16, QLatin1Char('0'))
               .arg(last0066)
               .arg(last026E)
               .toUpper();
    log += QStringLiteral("Dernier 0x03C8 : %1 ticks = %2 ms.\n")
               .arg(last03C8)
               .arg(double(last03C8) * 0.002, 0, 'f', 3);
    log += QStringLiteral("Compteur transitoire maximal observé : %1.\n").arg(maxTransientCounter);

    if (transientSeen)
    {
        log += QStringLiteral("TRANSITOIRE CAPTURÉ : première détection à l'échantillon %1.\n")
                   .arg(firstTransientSample);
        log += QStringLiteral("Les valeurs candidat 0x03C8 + S(0x026E) sont enregistrées pour comparaison ROM ; elles ne sont pas encore déclarées comme temps d'injection transitoire validé.\n");
    }
    else
    {
        log += QStringLiteral("AUCUN TRANSITOIRE CAPTURÉ : RAM 0x0280 est restée à zéro pendant toute la série.\n");
        log += QStringLiteral("Le test peut être refait avec une impulsion d'accélérateur un peu plus nette, toujours sans dépasser 2000 tr/min.\n");
    }

    if (!restoreNormalSession())
    {
        m_stopPolling = true;
        log += QStringLiteral("Polling normal arrêté et connexion fermée par sécurité.\n");
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
        log += QStringLiteral("Contrôle final 0x80 impossible malgré retour F0 50 confirmé.\n");
        finish(false, resultPrimary, correctionRaw,
               QStringLiteral("TEST TERMINÉ — SESSION NORMALE CONFIRMÉE — CONTRÔLE RPM FINAL IMPOSSIBLE"));
        return;
    }

    log += QStringLiteral("Régime moteur frais après retour normal : %1 tr/min.\n").arg(finalRpm);
    log += QStringLiteral("Tension batterie finale : %1 V.\n")
               .arg(double(finalBattery) / 10.0, 0, 'f', 1);

    if (finalRpm > 2000)
    {
        log += QStringLiteral("GARDE-FOU FINAL : régime supérieur à 2000 tr/min après la capture.\n");
        finish(false, resultPrimary, correctionRaw,
               QStringLiteral("TEST TERMINÉ — RÉGIME FINAL TROP ÉLEVÉ"));
        return;
    }

    if (transientSeen)
    {
        finish(true, resultPrimary, correctionRaw,
               QStringLiteral("TRANSITOIRE CAPTURÉ — SESSION NORMALE CONFIRMÉE"));
    }
    else
    {
        finish(false, resultPrimary, correctionRaw,
               QStringLiteral("TEST TERMINÉ — AUCUN TRANSITOIRE CAPTURÉ — REFAIRE"));
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
            QStringLiteral("Test transitoire RAM temps injection — AANMP002 — MAX 2000 TR/MIN"),
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

        InjectionRamTestDialog dialog(window);
        // Keep this exact title: the automatic logger recognises it.
        dialog.setWindowTitle(QStringLiteral("Test dynamique temps injection — AANMP002"));
        dialog.setModal(true);
        dialog.resize(820, 700);

        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        layout->setContentsMargins(14, 14, 14, 14);
        layout->setSpacing(9);

        QLabel *warning = new QLabel(
            QStringLiteral("<b>TEST TRANSITOIRE — 500 À 2000 TR/MIN</b><br>"
                           "Véhicule immobilisé. Après avoir lancé le test, attendez environ 2 secondes, "
                           "donnez UNE brève impulsion d'accélérateur vers 1600–1900 tr/min puis relâchez. "
                           "NE DÉPASSEZ JAMAIS 2000 TR/MIN."),
            &dialog);
        warning->setWordWrap(true);
        warning->setAlignment(Qt::AlignCenter);
        warning->setStyleSheet(QStringLiteral("font-size:15px; font-weight:700; color:#ff9828;"));
        layout->addWidget(warning);

        QLabel *info = new QLabel(
            QStringLiteral("Cette variante cherche volontairement à faire apparaître RAM 0x0280 > 0. "
                           "Elle effectue jusqu'à 24 captures rapides de R5E/0x005E, 0x0064, 0x0066, "
                           "0x026E, 0x0280/0x0281 et 0x03C8. "
                           "R5E est contrôlé à chaque échantillon et le test s'arrête immédiatement "
                           "si le régime sort de la plage 500–2000 tr/min. Aucune écriture RAM n'est effectuée."),
            &dialog);
        info->setWordWrap(true);
        layout->addWidget(info);

        QLabel *sequence = new QLabel(
            QStringLiteral("Séquence verrouillée : F0 — [retour F0 50 si nécessaire] — D1 — 80 — "
                           "F5 — F0 14 — C4 — F0 1E — "
                           "[captures : DC 00 / 2F,32,33 — DC 02 / 37,40 — DC 03 / 64] — "
                           "F5 — F0 14 — F4 — F0 50 — 80 final"),
            &dialog);
        sequence->setWordWrap(true);
        layout->addWidget(sequence);

        QCheckBox *confirmStopped = new QCheckBox(
            QStringLiteral("Je confirme : véhicule immobilisé pendant tout le test."),
            &dialog);
        QCheckBox *confirmTransient = new QCheckBox(
            QStringLiteral("Je confirme : une seule impulsion brève, puis relâcher, sans dépasser 2000 tr/min."),
            &dialog);
        layout->addWidget(confirmStopped);
        layout->addWidget(confirmTransient);

        QLabel *status = new QLabel(QStringLiteral("PRÊT — AUCUNE COMMANDE DE TEST ENVOYÉE"), &dialog);
        status->setWordWrap(true);
        status->setStyleSheet(QStringLiteral("font-weight:700;"));
        layout->addWidget(status);

        QPlainTextEdit *output = new QPlainTextEdit(&dialog);
        output->setObjectName(QStringLiteral("injectionRamTestOutput"));
        output->setReadOnly(true);
        output->setPlaceholderText(QStringLiteral("Le journal TX/RX et les échantillons transitoires apparaîtront ici."));
        layout->addWidget(output, 1);

        QHBoxLayout *buttons = new QHBoxLayout;
        QPushButton *run = new QPushButton(QStringLiteral("Lancer la capture transitoire"), &dialog);
        QPushButton *close = new QPushButton(QStringLiteral("Fermer"), &dialog);
        run->setEnabled(false);
        buttons->addWidget(run);
        buttons->addStretch(1);
        buttons->addWidget(close);
        layout->addLayout(buttons);

        const auto updateRunEnabled = [run, confirmStopped, confirmTransient]()
        {
            run->setEnabled(confirmStopped->isChecked() && confirmTransient->isChecked());
        };
        QObject::connect(confirmStopped, &QCheckBox::toggled, &dialog,
                         [updateRunEnabled](bool) { updateRunEnabled(); });
        QObject::connect(confirmTransient, &QCheckBox::toggled, &dialog,
                         [updateRunEnabled](bool) { updateRunEnabled(); });
        QObject::connect(close, &QPushButton::clicked, &dialog, &QDialog::accept);

        if (mems)
        {
            QObject::connect(mems, &MEMSInterface::injectionRamTestFinished, &dialog,
                             [&dialog, status, output, close, confirmStopped, confirmTransient, run]
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
                confirmStopped->setEnabled(false);
                confirmTransient->setEnabled(false);
                run->setEnabled(false);
            });
        }

        QObject::connect(run, &QPushButton::clicked, &dialog,
                         [&dialog, mems, status, output, close, confirmStopped, confirmTransient, run]()
        {
            if (!confirmStopped->isChecked() || !confirmTransient->isChecked())
                return;

            run->setEnabled(false);
            confirmStopped->setEnabled(false);
            confirmTransient->setEnabled(false);
            close->setEnabled(false);
            dialog.setTestRunning(true);
            status->setText(QStringLiteral("TEST EN COURS — ATTENDEZ ~2 S PUIS UNE IMPULSION BRÈVE — RESTER SOUS 2000 TR/MIN"));
            status->setStyleSheet(QStringLiteral("font-weight:700; color:#ff9828;"));
            output->setPlainText(QStringLiteral("Préparation de la capture transitoire... Attendez environ 2 secondes puis donnez UNE impulsion brève et relâchez."));

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
