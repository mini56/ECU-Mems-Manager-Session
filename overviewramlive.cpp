#include "mainwindow.h"
#include "memsinterface.h"
#include "ecuidentification.h"
#include "i18n.h"

#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEvent>
#include <QLabel>
#include <QMetaObject>
#include <QTabWidget>
#include <QWidget>

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

static quint16 highWordC001(quint16 value)
{
    const quint32 product = quint32(value) * quint32(0xC001u);
    return quint16((product >> 16) & 0xFFFFu);
}

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

static bool enterMode4(mems_info *info)
{
    QByteArray response;
    exchangeByte(info, 0xF5, response, 30);
    if (!readMode(info, 0x14))
        return false;
    if (!exchangeByte(info, 0xC4, response, 30))
        return false;
    return readMode(info, 0x1E);
}

} // namespace

void MEMSInterface::onOverviewLiveModeRequested(bool enabled)
{
    setProperty("overviewLiveRequested", enabled);

    // A disable request is processed only at a normal-session boundary. The
    // injection loop always restores F0 50 before processing queued UI events.
    if (!enabled || property("overviewLiveModeActive").toBool())
        return;
    if (!(m_initComplete && mems_is_connected(&m_memsinfo)) || m_stopPolling || m_shutdownThread)
        return;

    setProperty("overviewLiveModeActive", true);
    QElapsedTimer injectionCadence;
    injectionCadence.start();

    while (property("overviewLiveRequested").toBool() && !m_stopPolling &&
           !m_shutdownThread && mems_is_connected(&m_memsinfo))
    {
        // Keep the original application data path intact. Every existing gauge,
        // the summary tab, diagnostics and normal logging continue to receive
        // exactly the same mems_data structure produced by librosco.
        if (!mems_read(&m_memsinfo, &m_data))
        {
            emit readError();
            m_stopPolling = true;
            break;
        }

        emit readSuccess();
        emit dataReady();

        // Process tab changes and normal commands while the ECU is still in the
        // normal F0 50 session. No queued UI command is processed inside Mode 4.
        QCoreApplication::processEvents();
        if (!property("overviewLiveRequested").toBool() || m_stopPolling || m_shutdownThread)
            break;

        // The new injection value does not need to replace the normal gauge
        // polling. Sample it independently at about 4 Hz.
        if (injectionCadence.elapsed() < 250)
            continue;

        bool sampleOk = ensureNormalSession(&m_memsinfo);
        if (sampleOk)
            sampleOk = enterMode4(&m_memsinfo);

        quint16 transientPair = 0;
        quint16 transientSource = 0;
        quint16 injectionBase = 0;

        if (sampleOk)
        {
            sampleOk = selectMode4Block(&m_memsinfo, 0x02) &&
                       readMode4Word(&m_memsinfo, 0x40, &transientPair);
            // 0x0280 is a state/counter, never a multiplier.
            if (sampleOk && quint8(transientPair & 0x00FFu) != 0)
                sampleOk = readMode4Word(&m_memsinfo, 0x37, &transientSource);
        }

        if (sampleOk)
            sampleOk = selectMode4Block(&m_memsinfo, 0x03) &&
                       readMode4Word(&m_memsinfo, 0x64, &injectionBase);

        // Always return to F0 50 before exposing events or continuing normal
        // librosco polling, even when a RAM read failed.
        const bool normalRestored = restoreNormalSession(&m_memsinfo);
        if (!normalRestored)
        {
            emit overviewInjectionReady(-1.0);
            m_stopPolling = true;
            break;
        }

        if (!sampleOk)
        {
            // Injection becomes unavailable, but the rest of ECU MEMS Manager
            // is left on the normal ROSCO path and continues to work.
            emit overviewInjectionReady(-1.0);
            setProperty("overviewLiveRequested", false);
            break;
        }

        quint32 injectionTicks = quint32(injectionBase);
        if (quint8(transientPair & 0x00FFu) != 0)
            injectionTicks += quint32(highWordC001(transientSource));

        emit overviewInjectionReady(double(injectionTicks) * 0.002);
        injectionCadence.restart();
        QCoreApplication::processEvents();
    }

    setProperty("overviewLiveModeActive", false);
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

        QWidget *injectorSource = window->findChild<QWidget*>(QStringLiteral("m_injector_time"));
        if (!injectorSource)
        {
            injectorSource = new QWidget(window);
            injectorSource->setObjectName(QStringLiteral("m_injector_time"));
            injectorSource->hide();
        }
        injectorSource->setProperty("minimum", 0.0);
        injectorSource->setProperty("maximum", 20.0);
        injectorSource->setProperty("value", QStringLiteral("--"));

        window->setProperty("overviewLiveInstalled", true);
        window->setProperty("ecuExtendedIdentifier", QString());
        MEMSInterface *mems = window->m_mems;

        // Only the injection source is written here. Existing gauges remain
        // exclusively controlled by MainWindow::onDataReady().
        QObject::connect(mems, &MEMSInterface::overviewInjectionReady, window,
            [tabs, injectorSource](double injectionMs)
            {
                if (!tabs || tabs->currentIndex() != 0)
                    return;
                if (injectionMs < 0.0)
                    injectorSource->setProperty("value", QStringLiteral("--"));
                else
                    injectorSource->setProperty("value", injectionMs);
            });

        // Ask D1 once after the normal connection has completed. The request
        // uses the existing queued MainWindow -> MEMSInterface path and is
        // therefore handled only between normal librosco reads.
        QObject::connect(mems, &MEMSInterface::connected, window,
            [window, injectorSource]()
            {
                window->setProperty("ecuExtendedIdentifier", QString());
                injectorSource->setProperty("value", QStringLiteral("--"));
                emit window->requestProtocolCommand(quint8(0xD1));
            });

        QObject::connect(mems, &MEMSInterface::protocolResponse, window,
            [window, mems, tabs, injectorSource](quint8 command, const QByteArray &response)
            {
                if (command != quint8(0xD1))
                    return;

                const QString identifier = EcuIdentification::extendedIdentifierFromReply(response);
                if (identifier.isEmpty())
                {
                    // Unsupported/empty D1: keep the already displayed D0 and
                    // do not enter AANMP002-specific RAM mode.
                    window->setProperty("ecuExtendedIdentifier", QString());
                    injectorSource->setProperty("value", QStringLiteral("--"));
                    return;
                }

                window->setProperty("ecuExtendedIdentifier", identifier);
                if (QLabel *label = window->findChild<QLabel*>(QStringLiteral("m_ecuIdLabel")))
                {
                    QString text = I18n::text(7050);
                    if (!text.endsWith(QLatin1Char(' ')))
                        text += QLatin1Char(' ');
                    text += identifier;
                    const QString reference = EcuIdentification::referenceForExtendedIdentifier(identifier);
                    if (!reference.isEmpty())
                        text += QString::fromUtf8(" — ") + reference;
                    label->setText(text);
                }

                if (tabs && tabs->currentIndex() == 0 && identifier == QStringLiteral("AANMP002"))
                    QMetaObject::invokeMethod(mems, "onOverviewLiveModeRequested",
                                              Qt::QueuedConnection, Q_ARG(bool, true));
            });

        QObject::connect(tabs, &QTabWidget::currentChanged, window,
            [window, mems, injectorSource](int index)
            {
                if (index != 0)
                {
                    injectorSource->setProperty("value", QStringLiteral("--"));
                    QMetaObject::invokeMethod(mems, "onOverviewLiveModeRequested",
                                              Qt::QueuedConnection, Q_ARG(bool, false));
                    return;
                }

                if (window->property("ecuExtendedIdentifier").toString() == QStringLiteral("AANMP002"))
                    QMetaObject::invokeMethod(mems, "onOverviewLiveModeRequested",
                                              Qt::QueuedConnection, Q_ARG(bool, true));
            });

        QObject::connect(mems, &MEMSInterface::disconnected, window,
            [window, injectorSource]()
            {
                window->setProperty("ecuExtendedIdentifier", QString());
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
