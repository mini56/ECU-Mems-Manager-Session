#include "mainwindow.h"
#include "memsinterface.h"

#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEvent>
#include <QMetaObject>
#include <QTabWidget>
#include <QVariant>
#include <QWidget>
#include <QtGlobal>

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/select.h>
#endif

namespace {

static quint8 byteAt(const QByteArray &bytes, int index)
{
    return static_cast<quint8>(static_cast<unsigned char>(bytes.at(index)));
}

static bool exactModeReply(const QByteArray &response, quint8 modeValue)
{
    if (response.size() == 2)
        return byteAt(response, 0) == 0xF0 && byteAt(response, 1) == modeValue;
    if (response.size() == 3)
        return byteAt(response, 0) == 0xF0 && byteAt(response, 1) == 0xF0 &&
               byteAt(response, 2) == modeValue;
    return false;
}

static bool decodeKnownModeReply(const QByteArray &response, quint8 *modeValue)
{
    if (!modeValue)
        return false;
    const quint8 modes[] = {0x14, 0x1E, 0x50};
    for (quint8 mode : modes)
    {
        if (exactModeReply(response, mode))
        {
            *modeValue = mode;
            return true;
        }
    }
    return false;
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
        payload = 1;
    else if (response.size() == 4 && byteAt(response, 0) == command &&
             byteAt(response, 1) == command)
        payload = 2;
    else
        return false;

    *value = quint16(byteAt(response, payload)) |
             (quint16(byteAt(response, payload + 1)) << 8);
    return true;
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

static quint16 highWordC001(quint16 value)
{
    const quint32 product = quint32(value) * quint32(0xC001u);
    return quint16((product >> 16) & 0xFFFFu);
}

// One command byte per write. In particular, DC and its block parameter are
// deliberately sent by two separate calls, as validated on AANMP002.
static bool exchangeByte(mems_info *info, quint8 command, QByteArray &response,
                         int quietLimitMs = 30)
{
    response.clear();
    if (!info || !mems_is_connected(info))
        return false;

#ifdef WIN32
    HANDLE h = info->sd;
    if (h == INVALID_HANDLE_VALUE || h == NULL)
        return false;

    COMMTIMEOUTS oldTimeouts;
    if (!GetCommTimeouts(h, &oldTimeouts))
        return false;

    COMMTIMEOUTS timeouts = oldTimeouts;
    const bool fast = quietLimitMs <= 20;
    timeouts.ReadIntervalTimeout = fast ? 5 : 20;
    timeouts.ReadTotalTimeoutConstant = fast ? 10 : 35;
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

    const unsigned char tx = command;
    DWORD written = 0;
    if (!WriteFile(h, &tx, 1, &written, NULL) || written != 1)
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
    const int fd = info->sd;
    if (fd < 0)
        return false;

    const unsigned char tx = command;
    if (write(fd, &tx, 1) != 1)
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
}

// Exact little-endian Mode-4 word reader copied from the already validated
// dynamic RAM test behaviour: no artificial quiet period after a full reply.
static bool readMode4Word(mems_info *info, quint8 command, quint16 *value)
{
    if (!info || !value || !mems_is_connected(info))
        return false;

    QByteArray response;
#ifdef WIN32
    HANDLE h = info->sd;
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
            !(response.size() == 3 && byteAt(response, 0) == command &&
              byteAt(response, 1) == command))
            break;
    }

    SetCommTimeouts(h, &oldTimeouts);
    return false;
#else
    const int fd = info->sd;
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
            !(response.size() == 3 && byteAt(response, 0) == command &&
              byteAt(response, 1) == command))
            break;
    }
    return false;
#endif
}

static bool selectMode4Block(mems_info *info, quint8 block)
{
    if (block > 0x03)
        return false;

    QByteArray response;
    if (!exchangeByte(info, 0xDC, response, 20) || !exactEchoReply(response, 0xDC))
        return false;
    if (!exchangeByte(info, block, response, 20) || !exactEchoReply(response, block))
        return false;
    return true;
}

static bool readMode(mems_info *info, quint8 expectedMode)
{
    QByteArray response;
    return exchangeByte(info, 0xF0, response, 30) && exactModeReply(response, expectedMode);
}

static bool restoreNormalSession(mems_info *info)
{
    QByteArray response;
    exchangeByte(info, 0xF5, response, 30);
    if (!readMode(info, 0x14))
        return false;

    exchangeByte(info, 0xF4, response, 30);
    return readMode(info, 0x50);
}

static bool ensureNormalSession(mems_info *info)
{
    QByteArray response;
    if (!exchangeByte(info, 0xF0, response, 30))
        return false;

    quint8 mode = 0;
    if (!decodeKnownModeReply(response, &mode))
        return false;
    if (mode == 0x50)
        return true;
    return restoreNormalSession(info);
}

static bool confirmAANMP002(mems_info *info)
{
    QByteArray response;
    return exchangeByte(info, 0xD1, response, 150) && isAANMP002Reply(response);
}

static void setGaugeValue(MainWindow *window, const char *name, const QVariant &value)
{
    if (!window)
        return;
    if (QObject *gauge = window->findChild<QObject*>(QString::fromLatin1(name)))
        gauge->setProperty("value", value);
}

} // namespace

void MEMSInterface::onOverviewLiveModeRequested(bool enabled)
{
    setProperty("overviewLiveRequested", enabled);

    if (!enabled || property("overviewLiveModeActive").toBool())
        return;
    if (!(m_initComplete && mems_is_connected(&m_memsinfo)) || m_stopPolling || m_shutdownThread)
        return;

    // This RAM map is validated for AANMP002 only. Other ECUs stay on the
    // existing normal ROSCO polling path.
    if (!ensureNormalSession(&m_memsinfo) || !confirmAANMP002(&m_memsinfo))
    {
        setProperty("overviewLiveRequested", false);
        return;
    }

    QByteArray response;
    exchangeByte(&m_memsinfo, 0xF5, response, 30);
    if (!readMode(&m_memsinfo, 0x14))
    {
        setProperty("overviewLiveRequested", false);
        if (!restoreNormalSession(&m_memsinfo))
            m_stopPolling = true;
        return;
    }

    if (!exchangeByte(&m_memsinfo, 0xC4, response, 30) || !readMode(&m_memsinfo, 0x1E))
    {
        setProperty("overviewLiveRequested", false);
        if (!restoreNormalSession(&m_memsinfo))
            m_stopPolling = true;
        return;
    }

    setProperty("overviewLiveModeActive", true);
    bool readOk = true;

    while (property("overviewLiveRequested").toBool() && !m_stopPolling &&
           !m_shutdownThread && mems_is_connected(&m_memsinfo))
    {
        quint16 rpm = 0, iac = 0, shortTrim = 0, advance = 0;
        quint16 lambda = 0, battery = 0, airTemp = 0, coolant = 0, map = 0, throttle = 0;
        quint16 transientPair = 0, transientSource = 0, injectionBase = 0;

        // One complete current-Aperçu refresh; no priority/cadence split.
        readOk = selectMode4Block(&m_memsinfo, 0x00) &&
                 readMode4Word(&m_memsinfo, 0x2F, &rpm) &&
                 readMode4Word(&m_memsinfo, 0x47, &iac) &&
                 readMode4Word(&m_memsinfo, 0x6B, &shortTrim) &&
                 readMode4Word(&m_memsinfo, 0x6E, &advance);

        if (readOk)
            readOk = selectMode4Block(&m_memsinfo, 0x01) &&
                     readMode4Word(&m_memsinfo, 0x25, &lambda) &&
                     readMode4Word(&m_memsinfo, 0x7A, &battery) &&
                     readMode4Word(&m_memsinfo, 0x7B, &airTemp) &&
                     readMode4Word(&m_memsinfo, 0x7C, &coolant) &&
                     readMode4Word(&m_memsinfo, 0x7D, &map) &&
                     readMode4Word(&m_memsinfo, 0x7E, &throttle);

        if (readOk)
        {
            readOk = selectMode4Block(&m_memsinfo, 0x02) &&
                     readMode4Word(&m_memsinfo, 0x40, &transientPair);
            // 0x0280 is a state/counter, never a multiplier.
            if (readOk && quint8(transientPair & 0x00FFu) != 0)
                readOk = readMode4Word(&m_memsinfo, 0x37, &transientSource);
        }

        if (readOk)
            readOk = selectMode4Block(&m_memsinfo, 0x03) &&
                     readMode4Word(&m_memsinfo, 0x64, &injectionBase);

        if (!readOk)
            break;

        quint32 injectionTicks = quint32(injectionBase);
        if (quint8(transientPair & 0x00FFu) != 0)
            injectionTicks += quint32(highWordC001(transientSource));

        emit overviewLiveDataReady(
            int(rpm),
            int(iac & 0x00FFu),
            int(shortTrim & 0x00FFu),
            int(advance & 0x00FFu),
            int((lambda >> 8) & 0x00FFu),
            int((battery >> 8) & 0x00FFu),
            int((airTemp >> 8) & 0x00FFu),
            int((coolant >> 8) & 0x00FFu),
            int((map >> 8) & 0x00FFu),
            int((throttle >> 8) & 0x00FFu),
            double(injectionTicks) * 0.002);

        // The normal 7D/80 service loop is suspended in Mode 4. Only process
        // queued tab/disconnect requests at a complete-snapshot boundary.
        QCoreApplication::processEvents();
    }

    const bool normalRestored = restoreNormalSession(&m_memsinfo);
    setProperty("overviewLiveModeActive", false);
    if (!readOk)
        setProperty("overviewLiveRequested", false);
    if (!normalRestored)
        m_stopPolling = true;
}

class OverviewLiveInstaller : public QObject
{
public:
    explicit OverviewLiveInstaller(QObject *parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (!watched || (event->type() != QEvent::Show && event->type() != QEvent::Polish))
            return QObject::eventFilter(watched, event);

        MainWindow *window = qobject_cast<MainWindow*>(watched);
        if (!window || window->property("overviewLiveInstalled").toBool() || !window->m_mems)
            return QObject::eventFilter(watched, event);

        QTabWidget *tabs = window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
        if (!tabs)
            return QObject::eventFilter(watched, event);

        // Hidden data source only: OverviewRebuild keeps drawing the exact same
        // card. Its min/max properties recalibrate this one dial to 0..20 ms.
        QWidget *injectorSource = window->findChild<QWidget*>(QStringLiteral("m_injector_time"));
        if (!injectorSource)
        {
            injectorSource = new QWidget(window);
            injectorSource->setObjectName(QStringLiteral("m_injector_time"));
            injectorSource->setProperty("minimum", 0.0);
            injectorSource->setProperty("maximum", 20.0);
            injectorSource->setProperty("value", QStringLiteral("--"));
            injectorSource->hide();
        }

        window->setProperty("overviewLiveInstalled", true);
        MEMSInterface *mems = window->m_mems;

        QObject::connect(mems, &MEMSInterface::overviewLiveDataReady, window,
            [window, tabs, injectorSource](int rpm, int iacRaw, int shortTrimRaw,
                                           int advanceRaw, int lambdaRaw, int batteryRaw,
                                           int airTempRaw, int coolantRaw, int mapRaw,
                                           int throttleRaw, double injectionMs)
            {
                if (!tabs || tabs->currentIndex() != 0)
                    return;

                const int correctedIac = qBound(0, iacRaw, int(IAC_MAXIMUM));
                setGaugeValue(window, "m_revCounter", rpm);
                setGaugeValue(window, "m_waterTempGauge", window->convertTemperature(coolantRaw));
                setGaugeValue(window, "m_mapGauge", mapRaw);
                setGaugeValue(window, "m_throttle_pos", throttleRaw / 2);
                setGaugeValue(window, "m_battery", batteryRaw / 10.0);
                setGaugeValue(window, "m_short_term_correction", shortTrimRaw - 100);
                setGaugeValue(window, "m_lambda_voltage", lambdaRaw * 5);
                setGaugeValue(window, "m_airTempGauge", window->convertTemperature(airTempRaw));
                setGaugeValue(window, "m_idle_position",
                              qRound(double(correctedIac) / double(IAC_MAXIMUM) * 100.0));
                setGaugeValue(window, "m_ignition_advance", (advanceRaw / 2) - 24);
                injectorSource->setProperty("value", injectionMs);
            });

        QObject::connect(tabs, &QTabWidget::currentChanged, window,
            [mems, injectorSource](int index)
            {
                const bool overviewActive = (index == 0);
                if (!overviewActive)
                    injectorSource->setProperty("value", QStringLiteral("--"));
                QMetaObject::invokeMethod(mems, "onOverviewLiveModeRequested",
                                          Qt::QueuedConnection, Q_ARG(bool, overviewActive));
            });

        QObject::connect(mems, &MEMSInterface::connected, window,
            [mems, tabs]()
            {
                if (tabs && tabs->currentIndex() == 0)
                    QMetaObject::invokeMethod(mems, "onOverviewLiveModeRequested",
                                              Qt::QueuedConnection, Q_ARG(bool, true));
            });

        QObject::connect(mems, &MEMSInterface::disconnected, window,
            [injectorSource]()
            {
                injectorSource->setProperty("value", QStringLiteral("--"));
            });

        return QObject::eventFilter(watched, event);
    }
};

static void installOverviewLiveInstaller()
{
    QApplication *app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (app)
        app->installEventFilter(new OverviewLiveInstaller(app));
}

Q_COREAPP_STARTUP_FUNCTION(installOverviewLiveInstaller)
