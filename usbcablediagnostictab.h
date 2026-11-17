#pragma once

// Native, optional USB-serial cable diagnostics. No ECU commands are sent.
// Adapted from mini56/Detection-port-usb (VID/PID identification and 15 s monitoring).
#include <QComboBox>
#include <QDateTime>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMap>
#include <QPushButton>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QSet>
#include <QStringList>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QLocale>

class UsbCableDiagnosticTab final : public QWidget
{
public:
    explicit UsbCableDiagnosticTab(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setObjectName(QStringLiteral("usb_cable_diagnostic_tab"));
        const bool french = QLocale().language() == QLocale::French;
        auto txt = [french](const char *fr, const char *en) {
            return QString::fromUtf8(french ? fr : en);
        };
        QVBoxLayout *root = new QVBoxLayout(this);
        root->setContentsMargins(18, 15, 18, 15);
        root->setSpacing(12);

        QLabel *intro = new QLabel(txt(
            "Contrôle facultatif des interfaces USB / série. La connexion ECU reste directe et inchangée.",
            "Optional USB/serial interface check. Normal ECU connection remains unchanged."), this);
        intro->setWordWrap(true);
        root->addWidget(intro);

        QGroupBox *devicesBox = new QGroupBox(txt("Interfaces détectées", "Detected interfaces"), this);
        QVBoxLayout *devicesLayout = new QVBoxLayout(devicesBox);
        table_ = new QTableWidget(0, 4, devicesBox);
        table_->setObjectName(QStringLiteral("usbCableDetectedPorts"));
        table_->setHorizontalHeaderLabels({
            txt("Port", "Port"), txt("Puce / adaptateur", "Chip / adapter"),
            txt("VID / PID", "VID / PID"), txt("Description", "Description")});
        table_->setSelectionBehavior(QAbstractItemView::SelectRows);
        table_->setSelectionMode(QAbstractItemView::SingleSelection);
        table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table_->horizontalHeader()->setStretchLastSection(true);
        table_->verticalHeader()->setVisible(false);
        table_->setMinimumHeight(170);
        devicesLayout->addWidget(table_);
        root->addWidget(devicesBox, 1);

        QHBoxLayout *actions = new QHBoxLayout;
        refresh_ = new QPushButton(txt("Actualiser", "Refresh"), this);
        monitor_ = new QPushButton(txt("Surveiller un branchement (15 s)", "Watch for connection (15 s)"), this);
        test_ = new QPushButton(txt("Tester l'ouverture du port", "Test opening the port"), this);
        actions->addWidget(refresh_);
        actions->addWidget(monitor_);
        actions->addWidget(test_);
        actions->addStretch(1);
        root->addLayout(actions);

        status_ = new QLabel(this);
        status_->setObjectName(QStringLiteral("usbCableStatus"));
        status_->setWordWrap(true);
        root->addWidget(status_);

        QLabel *warning = new QLabel(txt(
            "Ce contrôle identifie l'adaptateur et teste uniquement l'ouverture du port COM. "
            "Il ne vérifie ni le câblage interne ni la communication avec un ECU. "
            "Déconnectez d'abord le calculateur si le port est déjà utilisé.",
            "This check identifies the adapter and only tests opening its COM port. "
            "It does not check the internal cable wiring or communication with an ECU. "
            "Disconnect the ECU first if the port is busy."), this);
        warning->setWordWrap(true);
        root->addWidget(warning);

        tick_.setInterval(300);
        connect(&tick_, &QTimer::timeout, this, [this]() {
            refreshPorts();
            const qint64 remaining = QDateTime::currentMSecsSinceEpoch() - startedAt_;
            if (remaining >= 15000) {
                tick_.stop();
                monitor_->setEnabled(true);
                if (!newPortDetected_)
                    status_->setText(localText("Aucun nouveau port COM détecté pendant 15 secondes.",
                                              "No new COM port detected during 15 seconds."));
            } else if (!newPortDetected_) {
                const int seconds = int((15000 - remaining + 999) / 1000);
                status_->setText(localText("Surveillance : encore %1 s. Branchez le câble.",
                                           "Watching: %1 s remaining. Plug in the cable.").arg(seconds));
            }
        });
        connect(refresh_, &QPushButton::clicked, this, [this]() {
            tick_.stop();
            monitor_->setEnabled(true);
            refreshPorts();
        });
        connect(monitor_, &QPushButton::clicked, this, [this]() {
            refreshPorts();
            initialPorts_.clear();
            for (const auto &port : QSerialPortInfo::availablePorts())
                initialPorts_.insert(port.portName());
            startedAt_ = QDateTime::currentMSecsSinceEpoch();
            newPortDetected_ = false;
            monitor_->setEnabled(false);
            status_->setText(localText("Surveillance en cours : branchez votre câble.",
                                       "Watching: plug in your cable."));
            tick_.start();
        });
        connect(test_, &QPushButton::clicked, this, [this]() {
            const int row = table_->currentRow();
            if (row < 0 || !table_->item(row, 0)) {
                status_->setText(localText("Sélectionnez un port COM dans la liste.",
                                           "Select a COM port from the list."));
                return;
            }
            const QString portName = table_->item(row, 0)->text();
            QSerialPort serial;
            serial.setPortName(portName);
            serial.setBaudRate(QSerialPort::Baud9600);
            serial.setDataBits(QSerialPort::Data8);
            serial.setParity(QSerialPort::NoParity);
            serial.setStopBits(QSerialPort::OneStop);
            serial.setFlowControl(QSerialPort::NoFlowControl);
            if (serial.open(QIODevice::ReadWrite)) {
                serial.close();
                status_->setText(localText("Port %1 : ouverture et fermeture réussies. "
                                           "La communication ECU n'a pas été testée.",
                                           "Port %1: opened and closed successfully. "
                                           "ECU communication was not tested.").arg(portName));
            } else {
                status_->setText(localText("Port %1 : impossible d'ouvrir le port (%2). "
                                           "Il peut déjà être utilisé ou le pilote peut être absent.",
                                           "Port %1: cannot open port (%2). "
                                           "It may be in use or the driver may be missing.")
                                 .arg(portName, serial.errorString()));
            }
        });
        refreshPorts();
    }

private:
    static QString localText(const char *fr, const char *en)
    {
        return QString::fromUtf8(QLocale().language() == QLocale::French ? fr : en);
    }

    static QString chipName(const QSerialPortInfo &port)
    {
        if (port.hasVendorIdentifier() && port.hasProductIdentifier()) {
            const quint32 key = (quint32(port.vendorIdentifier()) << 16) | port.productIdentifier();
            switch (key) {
            case 0x04036001: return QStringLiteral("FTDI FT232");
            case 0x04036015: return QStringLiteral("FTDI FT230X");
            case 0x04036014: return QStringLiteral("FTDI FT232H");
            case 0x04036010: return QStringLiteral("FTDI FT2232");
            case 0x067B2303: return QStringLiteral("Prolific PL2303");
            case 0x1A867523: return QStringLiteral("CH340");
            case 0x1A865523: return QStringLiteral("CH341");
            case 0x10C4EA60: return QStringLiteral("Silicon Labs CP210x");
            default: break;
            }
        }
        const QString desc = (port.description() + QLatin1Char(' ') + port.manufacturer()).toLower();
        if (desc.contains(QStringLiteral("bluetooth"))) return QStringLiteral("Bluetooth SPP");
        if (desc.contains(QStringLiteral("ftdi")) || desc.contains(QStringLiteral("ft232"))) return QStringLiteral("FTDI");
        if (desc.contains(QStringLiteral("prolific")) || desc.contains(QStringLiteral("pl2303"))) return QStringLiteral("Prolific PL2303");
        if (desc.contains(QStringLiteral("ch340")) || desc.contains(QStringLiteral("ch341")) || desc.contains(QStringLiteral("wch"))) return QStringLiteral("CH340 / CH341");
        if (desc.contains(QStringLiteral("cp210")) || desc.contains(QStringLiteral("silicon labs"))) return QStringLiteral("Silicon Labs CP210x");
        return localText("Modèle inconnu", "Unknown model");
    }

    void refreshPorts()
    {
        const QString selected = (table_->currentRow() >= 0 && table_->item(table_->currentRow(), 0))
            ? table_->item(table_->currentRow(), 0)->text() : QString();
        const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
        table_->setRowCount(ports.size());
        int selectedRow = -1;
        for (int row = 0; row < ports.size(); ++row) {
            const auto &port = ports.at(row);
            QString vidPid = QStringLiteral("—");
            if (port.hasVendorIdentifier() && port.hasProductIdentifier())
                vidPid = QStringLiteral("%1:%2")
                    .arg(port.vendorIdentifier(), 4, 16, QLatin1Char('0'))
                    .arg(port.productIdentifier(), 4, 16, QLatin1Char('0')).toUpper();
            const QStringList values = { port.portName(), chipName(port), vidPid, port.description() };
            for (int col = 0; col < values.size(); ++col)
                table_->setItem(row, col, new QTableWidgetItem(values.at(col)));
            if (port.portName() == selected) selectedRow = row;
            if (tick_.isActive() && !initialPorts_.contains(port.portName()) && !newPortDetected_) {
                newPortDetected_ = true;
                status_->setText(localText("Nouveau port détecté : %1 — %2",
                                           "New port detected: %1 — %2")
                                 .arg(port.portName(), chipName(port)));
                selectedRow = row;
                tick_.stop();
                monitor_->setEnabled(true);
            }
        }
        if (selectedRow >= 0) table_->selectRow(selectedRow);
        else table_->clearSelection();
        if (!tick_.isActive() && !newPortDetected_ && startedAt_ == 0)
            status_->setText(localText("Sélectionnez une interface ou lancez une surveillance de 15 s.",
                                       "Select an interface or start a 15-second watch."));
    }

    QTableWidget *table_ = nullptr;
    QPushButton *refresh_ = nullptr;
    QPushButton *monitor_ = nullptr;
    QPushButton *test_ = nullptr;
    QLabel *status_ = nullptr;
    QTimer tick_;
    QSet<QString> initialPorts_;
    qint64 startedAt_ = 0;
    bool newPortDetected_ = false;
};
