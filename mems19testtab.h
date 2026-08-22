#ifndef MEMS19TESTTAB_H
#define MEMS19TESTTAB_H

#include <QByteArray>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include "i18n.h"
#include "memsinterface.h"

class Mems19TestTab final : public QWidget
{
public:
    explicit Mems19TestTab(MEMSInterface *mems, QWidget *parent = nullptr)
        : QWidget(parent), m_mems(mems)
    {
        setObjectName(QStringLiteral("mems19_test_tab"));
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        QVBoxLayout *root = new QVBoxLayout(this);
        root->setContentsMargins(12, 10, 12, 10);
        root->setSpacing(10);

        QLabel *title = new QLabel(I18n::text(7931), this);
        QFont titleFont = title->font();
        titleFont.setBold(true);
        titleFont.setPointSizeF(titleFont.pointSizeF() + 2.0);
        title->setFont(titleFont);
        root->addWidget(title);

        QLabel *description = new QLabel(I18n::text(7932), this);
        description->setWordWrap(true);
        root->addWidget(description);

        QLabel *safety = new QLabel(I18n::text(7933), this);
        safety->setWordWrap(true);
        QFont safetyFont = safety->font();
        safetyFont.setBold(true);
        safety->setFont(safetyFont);
        root->addWidget(safety);

        QGroupBox *statusBox = new QGroupBox(I18n::text(7931), this);
        QGridLayout *status = new QGridLayout(statusBox);
        status->setColumnStretch(0, 0);
        status->setColumnStretch(1, 1);
        int row = 0;
        m_connection = addStatusRow(status, row++, I18n::text(7934));
        m_d0 = addStatusRow(status, row++, I18n::text(7935));
        m_d1 = addStatusRow(status, row++, I18n::text(7936));
        m_d2 = addStatusRow(status, row++, I18n::text(7937));
        m_f0 = addStatusRow(status, row++, I18n::text(7938));
        m_reads = addStatusRow(status, row++, I18n::text(7939));
        m_mode = addStatusRow(status, row++, I18n::text(7940));
        m_injection = addStatusRow(status, row++, I18n::text(7941));
        m_logPathLabel = addStatusRow(status, row++, I18n::text(7942));
        root->addWidget(statusBox);

        QHBoxLayout *buttons = new QHBoxLayout();
        QPushButton *newSessionButton = new QPushButton(I18n::text(7946), this);
        QPushButton *openFolderButton = new QPushButton(I18n::text(7947), this);
        buttons->addWidget(newSessionButton);
        buttons->addWidget(openFolderButton);
        buttons->addStretch(1);
        root->addLayout(buttons);

        QLabel *eventsTitle = new QLabel(I18n::text(7948), this);
        QFont eventsFont = eventsTitle->font();
        eventsFont.setBold(true);
        eventsTitle->setFont(eventsFont);
        root->addWidget(eventsTitle);

        m_events = new QPlainTextEdit(this);
        m_events->setReadOnly(true);
        m_events->setMaximumBlockCount(5000);
        m_events->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        root->addWidget(m_events, 1);

        m_uiRefresh.start();
        resetStatus();

        connect(newSessionButton, &QPushButton::clicked, this, [this]() {
            closeSessionFiles();
            m_readCount = 0;
            m_dataRowsSinceFlush = 0;
            m_firmware.clear();
            resetStatus();
            ensureSessionFiles();
            appendEvent(QStringLiteral("SESSION"), QStringLiteral("manual-new-session"), true);
        });
        connect(openFolderButton, &QPushButton::clicked, this, [this]() {
            ensureSessionFiles();
            if (!m_eventPath.isEmpty())
                QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_eventPath).absolutePath()));
        });

        if (!m_mems)
            return;

        connect(m_mems, &MEMSInterface::serialInterfaceDetected, this,
                [this](const QString &portName, const QString &adapterFamily, const QString &protocolName) {
            const bool mems19 = protocolName.contains(QStringLiteral("MEMS 1.9"), Qt::CaseInsensitive);
            setState(m_connection, I18n::text(7953).arg(portName, adapterFamily, protocolName), mems19);
            appendEvent(QStringLiteral("INTERFACE"),
                        QStringLiteral("port=%1 adapter=%2 protocol=%3").arg(portName, adapterFamily, protocolName), true);
        });

        connect(m_mems, &MEMSInterface::connected, this, [this]() {
            m_connected = true;
            if (m_readMode.isEmpty() || m_readMode == QStringLiteral("disconnected"))
                m_readMode = QStringLiteral("diagnostic");
            setState(m_mode, m_readMode, true);
            appendEvent(QStringLiteral("CONNECTION"), QStringLiteral("connected"), true);
        });

        connect(m_mems, &MEMSInterface::disconnected, this, [this]() {
            m_connected = false;
            m_readMode = QStringLiteral("disconnected");
            setState(m_mode, I18n::text(7944), false);
            appendEvent(QStringLiteral("CONNECTION"), QStringLiteral("disconnected"), true);
            flushAll();
        });

        connect(m_mems, &MEMSInterface::failedToConnect, this, [this](const QString &device) {
            appendEvent(QStringLiteral("CONNECTION"), QStringLiteral("failed device=%1").arg(device), true);
        });

        connect(m_mems, &MEMSInterface::readError, this, [this]() {
            appendEvent(QStringLiteral("READ"), QStringLiteral("read-error"), true);
        });

        connect(m_mems, &MEMSInterface::errorSendingCommand, this, [this]() {
            appendEvent(QStringLiteral("COMMAND"), QStringLiteral("send-error"), true);
        });

        connect(m_mems, &MEMSInterface::gotEcuId, this, [this](uint8_t *id) {
            if (!id) return;
            const QByteArray raw(reinterpret_cast<const char*>(id), 4);
            const QString hex = QString::fromLatin1(raw.toHex(' ').toUpper());
            setState(m_d0, hex, true);
            appendEvent(QStringLiteral("D0"), QStringLiteral("ecu-id=%1").arg(hex), true);
        });

        connect(m_mems, &MEMSInterface::protocolResponse, this,
                [this](quint8 command, const QByteArray &response) { handleProtocolResponse(command, response); });

        connect(m_mems, &MEMSInterface::dataReady, this, [this]() { captureDataSample(); });

        connect(m_mems, &MEMSInterface::readModeChanged, this, [this](const QString &mode) {
            m_readMode = mode;
            setState(m_mode, mode, mode == QStringLiteral("diagnostic"));
            appendEvent(QStringLiteral("MODE"), QStringLiteral("read-mode=%1").arg(mode), true);
        });

        connect(m_mems, &MEMSInterface::injectionLiveSample, this,
                [this](double finalMs, double baseMs, double transientMs,
                       quint16 baseRaw, quint16 transientRaw, quint8 counter) {
            setState(m_injection, I18n::text(7950)
                .arg(finalMs, 0, 'f', 3)
                .arg(baseMs, 0, 'f', 3)
                .arg(transientMs, 0, 'f', 3)
                .arg(counter), true);
            ensureSessionFiles();
            if (m_injectionFile.isOpen())
            {
                m_injectionStream << now() << ',' << m_readMode << ',' << m_firmware << ','
                                  << QString::number(finalMs, 'f', 6) << ','
                                  << QString::number(baseMs, 'f', 6) << ','
                                  << QString::number(transientMs, 'f', 6) << ','
                                  << baseRaw << ',' << transientRaw << ',' << int(counter) << '\n';
                m_injectionStream.flush();
            }
        });
    }

    ~Mems19TestTab() override
    {
        closeSessionFiles();
    }

private:
    QLabel *addStatusRow(QGridLayout *layout, int row, const QString &name)
    {
        QLabel *key = new QLabel(name, this);
        QFont keyFont = key->font();
        keyFont.setBold(true);
        key->setFont(keyFont);
        QLabel *value = new QLabel(I18n::text(7943), this);
        value->setTextInteractionFlags(Qt::TextSelectableByMouse);
        value->setWordWrap(true);
        layout->addWidget(key, row, 0, Qt::AlignTop);
        layout->addWidget(value, row, 1);
        return value;
    }

    void setState(QLabel *label, const QString &text, bool good)
    {
        if (!label) return;
        label->setText(text);
        label->setStyleSheet(good
            ? QStringLiteral("font-weight:600;color:#58d27a;")
            : QStringLiteral("font-weight:600;"));
    }

    void resetStatus()
    {
        m_connected = m_mems && m_mems->isConnected();
        m_readMode = m_connected ? QStringLiteral("diagnostic") : QStringLiteral("disconnected");
        setState(m_connection, m_connected ? I18n::text(7945) : I18n::text(7944), false);
        setState(m_d0, I18n::text(7943), false);
        setState(m_d1, I18n::text(7943), false);
        setState(m_d2, I18n::text(7943), false);
        setState(m_f0, I18n::text(7943), false);
        setState(m_reads, QStringLiteral("0"), false);
        setState(m_mode, m_readMode, false);
        setState(m_injection, I18n::text(7943), false);
        setState(m_logPathLabel, m_eventPath.isEmpty() ? I18n::text(7951) : I18n::text(7952).arg(m_eventPath), false);
    }

    QString now() const
    {
        return QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    }

    QString sessionDirectory() const
    {
        QString root = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        if (root.isEmpty()) root = QDir::homePath();
        return QDir(root).filePath(QStringLiteral("ECU Mems Manager/MEMS19-tests"));
    }

    bool ensureSessionFiles()
    {
        if (m_eventFile.isOpen() && m_dataFile.isOpen() && m_injectionFile.isOpen())
            return true;

        const QString dirPath = sessionDirectory();
        if (!QDir().mkpath(dirPath))
            return false;

        const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss-zzz"));
        const QString base = QDir(dirPath).filePath(QStringLiteral("MEMS19_test_%1").arg(stamp));
        m_eventPath = base + QStringLiteral("_events.log");
        m_dataPath = base + QStringLiteral("_data.csv");
        m_injectionPath = base + QStringLiteral("_injection.csv");

        m_eventFile.setFileName(m_eventPath);
        m_dataFile.setFileName(m_dataPath);
        m_injectionFile.setFileName(m_injectionPath);
        if (!m_eventFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text) ||
            !m_dataFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text) ||
            !m_injectionFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        {
            closeSessionFiles();
            return false;
        }

        m_eventStream.setDevice(&m_eventFile);
        m_dataStream.setDevice(&m_dataFile);
        m_injectionStream.setDevice(&m_injectionFile);
        m_eventStream.setCodec("UTF-8");
        m_dataStream.setCodec("UTF-8");
        m_injectionStream.setCodec("UTF-8");

        m_eventStream << "# ECU Mems Manager - MEMS 1.9 validation\n"
                      << "# version=" << APP_VERSION << " build=" << APP_BUILD_NUMBER
                      << " commit=" << APP_COMMIT_SHA << '\n'
                      << "# started=" << now() << '\n';
        m_eventStream.flush();

        m_dataStream << "timestamp,engine_rpm,coolant_raw,intake_raw,map_kpa,battery_raw,throttle_raw,idle_switch,iac_position,idle_error,ignition_advance,coil_time,lambda_voltage,closed_loop,long_term_fuel_trim,short_term_fuel_trim,dtc0,dtc1,dtc2,dtc3,dtc4,dtc5,battery_v,coolant_c,intake_c,coil_ms,ignition_deg\n";
        m_dataStream.flush();
        m_injectionStream << "timestamp,read_mode,firmware,final_ms,base_ms,transient_ms,base_raw,transient_raw,counter\n";
        m_injectionStream.flush();

        setState(m_logPathLabel, I18n::text(7952).arg(m_eventPath), true);
        return true;
    }

    void closeSessionFiles()
    {
        flushAll();
        if (m_eventFile.isOpen()) m_eventFile.close();
        if (m_dataFile.isOpen()) m_dataFile.close();
        if (m_injectionFile.isOpen()) m_injectionFile.close();
        m_eventStream.setDevice(nullptr);
        m_dataStream.setDevice(nullptr);
        m_injectionStream.setDevice(nullptr);
        m_eventPath.clear();
        m_dataPath.clear();
        m_injectionPath.clear();
    }

    void flushAll()
    {
        if (m_eventFile.isOpen()) m_eventStream.flush();
        if (m_dataFile.isOpen()) m_dataStream.flush();
        if (m_injectionFile.isOpen()) m_injectionStream.flush();
    }

    void appendEvent(const QString &type, const QString &detail, bool flush)
    {
        ensureSessionFiles();
        const QString line = QStringLiteral("%1\t%2\t%3").arg(now(), type, detail);
        if (m_eventFile.isOpen())
        {
            m_eventStream << line << '\n';
            if (flush) m_eventStream.flush();
        }
        if (m_events) m_events->appendPlainText(line);
    }

    void captureDataSample()
    {
        if (!m_mems) return;
        mems_data *d = m_mems->getData();
        if (!d) return;
        ensureSessionFiles();
        ++m_readCount;

        if (m_dataFile.isOpen())
        {
            m_dataStream << now() << ',' << d->engine_rpm << ','
                << int(d->coolant_temp) << ',' << int(d->intake_air_temp) << ',' << int(d->map_kpa) << ','
                << int(d->battery_voltage) << ',' << int(d->throttle_pot) << ',' << int(d->idle_switch) << ','
                << int(d->iac_position) << ',' << d->idle_error << ',' << int(d->ignition_advance) << ',' << d->coil_time << ','
                << int(d->lambda_voltage) << ',' << int(d->closed_loop) << ',' << int(d->long_term_fuel_trim) << ','
                << int(d->short_term_fuel_trim) << ',' << int(d->dtc0) << ',' << int(d->dtc1) << ',' << int(d->dtc2) << ','
                << int(d->dtc3) << ',' << int(d->dtc4) << ',' << int(d->dtc5) << ','
                << QString::number(d->battery_voltage / 10.0, 'f', 2) << ','
                << (int(d->coolant_temp) - 55) << ',' << (int(d->intake_air_temp) - 55) << ','
                << QString::number(d->coil_time * 0.002, 'f', 3) << ','
                << QString::number((d->ignition_advance * 0.5) - 24.0, 'f', 2) << '\n';
            if (++m_dataRowsSinceFlush >= 16)
            {
                m_dataStream.flush();
                m_dataRowsSinceFlush = 0;
            }
        }

        if (m_uiRefresh.elapsed() >= 250)
        {
            setState(m_reads, I18n::text(7949)
                .arg(m_readCount)
                .arg(d->engine_rpm)
                .arg(d->battery_voltage / 10.0, 0, 'f', 1)
                .arg(int(d->map_kpa)), true);
            m_uiRefresh.restart();
        }
    }

    static QString firmwareFromReply(const QByteArray &response)
    {
        QString printable;
        printable.reserve(response.size());
        for (char c : response)
        {
            const unsigned char u = static_cast<unsigned char>(c);
            if ((u >= '0' && u <= '9') || (u >= 'A' && u <= 'Z') || (u >= 'a' && u <= 'z'))
                printable.append(QChar(u).toUpper());
            else
                printable.append(QLatin1Char(' '));
        }
        QRegularExpressionMatch match = QRegularExpression(QStringLiteral("([A-Z0-9]{8})")).match(printable);
        return match.hasMatch() ? match.captured(1) : QString();
    }

    void handleProtocolResponse(quint8 command, const QByteArray &response)
    {
        const QString cmd = QStringLiteral("%1").arg(command, 2, 16, QLatin1Char('0')).toUpper();
        const QString hex = response.isEmpty() ? QStringLiteral("<empty>") : QString::fromLatin1(response.toHex(' ').toUpper());
        appendEvent(QStringLiteral("RX"), QStringLiteral("cmd=0x%1 bytes=%2").arg(cmd, hex), true);

        if (command == 0xD0)
            setState(m_d0, hex, !response.isEmpty());
        else if (command == 0xD1)
        {
            m_firmware = firmwareFromReply(response);
            setState(m_d1, m_firmware.isEmpty() ? hex : QStringLiteral("%1  [%2]").arg(m_firmware, hex), !response.isEmpty());
        }
        else if (command == 0xD2)
            setState(m_d2, hex, !response.isEmpty());
        else if (command == 0xF0)
        {
            QString decoded;
            for (char c : response)
            {
                const quint8 v = quint8(c);
                if (v == 0x14u) decoded = QStringLiteral("mode3");
                else if (v == 0x1Eu) decoded = QStringLiteral("injection");
                else if (v == 0x50u) decoded = QStringLiteral("diagnostic");
            }
            if (!decoded.isEmpty()) m_readMode = decoded;
            setState(m_f0, decoded.isEmpty() ? hex : QStringLiteral("%1  [%2]").arg(decoded, hex), !response.isEmpty());
            if (!decoded.isEmpty()) setState(m_mode, decoded, decoded == QStringLiteral("diagnostic"));
        }
    }

    MEMSInterface *m_mems = nullptr;
    QLabel *m_connection = nullptr;
    QLabel *m_d0 = nullptr;
    QLabel *m_d1 = nullptr;
    QLabel *m_d2 = nullptr;
    QLabel *m_f0 = nullptr;
    QLabel *m_reads = nullptr;
    QLabel *m_mode = nullptr;
    QLabel *m_injection = nullptr;
    QLabel *m_logPathLabel = nullptr;
    QPlainTextEdit *m_events = nullptr;

    QFile m_eventFile;
    QFile m_dataFile;
    QFile m_injectionFile;
    QTextStream m_eventStream;
    QTextStream m_dataStream;
    QTextStream m_injectionStream;
    QString m_eventPath;
    QString m_dataPath;
    QString m_injectionPath;
    QString m_firmware;
    QString m_readMode;
    quint64 m_readCount = 0;
    int m_dataRowsSinceFlush = 0;
    bool m_connected = false;
    QElapsedTimer m_uiRefresh;
};

#endif // MEMS19TESTTAB_H
