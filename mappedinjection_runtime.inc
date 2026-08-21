void MEMSInterface::onMappedInjectionModeRequested(bool enabled, QString firmwareIdentifier)
{
    setProperty("mappedInjectionRequested", enabled);

    if (!enabled || property("mappedInjectionModeActive").toBool())
        return;

    InjectionRamMap map;
    if (!injectionRamMapForFirmware(firmwareIdentifier, &map) ||
        firmwareIdentifier.trimmed().toUpper() == QStringLiteral("AANMP002"))
    {
        setProperty("mappedInjectionRequested", false);
        return;
    }

    if (!(m_initComplete && mems_is_connected(&m_memsinfo)) || m_stopPolling || m_shutdownThread)
        return;

    setProperty("mappedInjectionModeActive", true);
    QElapsedTimer injectionCadence;
    injectionCadence.start();

    while (property("mappedInjectionRequested").toBool() && !m_stopPolling &&
           !m_shutdownThread && mems_is_connected(&m_memsinfo))
    {
        // Preserve the normal librosco path for every existing indicator.
        if (!mems_read(&m_memsinfo, &m_data))
        {
            emit readError();
            m_stopPolling = true;
            break;
        }

        emit readSuccess();
        emit dataReady();

        QCoreApplication::processEvents();
        if (!property("mappedInjectionRequested").toBool() || m_stopPolling || m_shutdownThread)
            break;

        if (injectionCadence.elapsed() < 250)
            continue;

        bool sampleOk = ensureNormalSession(&m_memsinfo);
        if (sampleOk)
            sampleOk = enterMode4(&m_memsinfo);

        quint8 transientCounter = 0;
        quint16 transientSource = 0;
        quint16 injectionBase = 0;

        const quint8 transientBlock = quint8(map.transientSourceAddress >> 8);
        const quint8 counterBlock = quint8(map.transientCounterAddress >> 8);
        if (sampleOk && transientBlock == counterBlock)
            sampleOk = selectMode4Block(&m_memsinfo, transientBlock);
        else if (sampleOk)
            sampleOk = false;

        if (sampleOk)
            sampleOk = readMappedByteInBlock(&m_memsinfo, map.transientCounterAddress,
                                             counterBlock, &transientCounter);

        if (sampleOk && transientCounter != 0)
            sampleOk = readMappedWordInBlock(&m_memsinfo, map.transientSourceAddress,
                                             transientBlock, &transientSource);

        const quint8 baseBlock = quint8(map.baseAddress >> 8);
        if (sampleOk)
            sampleOk = selectMode4Block(&m_memsinfo, baseBlock) &&
                       readMappedWordInBlock(&m_memsinfo, map.baseAddress,
                                             baseBlock, &injectionBase);

        // Never leave an ECU in Mode 4, including after a failed mapped read.
        const bool normalRestored = restoreNormalSession(&m_memsinfo);
        if (!normalRestored)
        {
            emit overviewInjectionReady(-1.0);
            m_stopPolling = true;
            break;
        }

        if (!sampleOk)
        {
            emit overviewInjectionReady(-1.0);
            setProperty("mappedInjectionRequested", false);
            break;
        }

        quint32 injectionTicks = quint32(injectionBase);
        if (transientCounter != 0)
            injectionTicks += quint32(highWordC001(transientSource));

        emit overviewInjectionReady(double(injectionTicks) * 0.002);
        injectionCadence.restart();
        QCoreApplication::processEvents();
    }

    setProperty("mappedInjectionModeActive", false);
}

class MappedInjectionInstaller : public QObject
{
public:
    explicit MappedInjectionInstaller(QObject *parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (!watched || (event->type() != QEvent::Show && event->type() != QEvent::Polish))
            return QObject::eventFilter(watched, event);

        MainWindow *window = qobject_cast<MainWindow*>(watched);
        if (!window || window->property("mappedInjectionInstalled").toBool())
            return QObject::eventFilter(watched, event);

        MEMSInterface *mems = window->findChild<MEMSInterface*>();
        QTabWidget *tabs = window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
        if (!mems || !tabs)
            return QObject::eventFilter(watched, event);

        window->setProperty("mappedInjectionInstalled", true);
        window->setProperty("mappedInjectionFirmware", QString());

        QObject::connect(mems, &MEMSInterface::protocolResponse, window,
            [window, mems, tabs](quint8 command, const QByteArray &response)
            {
                if (command != quint8(0xD1))
                    return;

                const QString identifier = EcuIdentification::extendedIdentifierFromReply(response);
                InjectionRamMap map;
                if (identifier == QStringLiteral("AANMP002") ||
                    !injectionRamMapForFirmware(identifier, &map))
                {
                    window->setProperty("mappedInjectionFirmware", QString());
                    return;
                }

                window->setProperty("mappedInjectionFirmware", identifier);
                if (tabs->currentIndex() == 0)
                    QMetaObject::invokeMethod(mems, "onMappedInjectionModeRequested",
                                              Qt::QueuedConnection,
                                              Q_ARG(bool, true),
                                              Q_ARG(QString, identifier));
            });

        QObject::connect(tabs, &QTabWidget::currentChanged, window,
            [window, mems](int index)
            {
                if (index != 0)
                {
                    QMetaObject::invokeMethod(mems, "onMappedInjectionModeRequested",
                                              Qt::QueuedConnection,
                                              Q_ARG(bool, false),
                                              Q_ARG(QString, QString()));
                    return;
                }

                const QString firmware = window->property("mappedInjectionFirmware").toString();
                if (!firmware.isEmpty())
                    QMetaObject::invokeMethod(mems, "onMappedInjectionModeRequested",
                                              Qt::QueuedConnection,
                                              Q_ARG(bool, true),
                                              Q_ARG(QString, firmware));
            });

        QObject::connect(mems, &MEMSInterface::disconnected, window,
            [window, mems]()
            {
                window->setProperty("mappedInjectionFirmware", QString());
                QMetaObject::invokeMethod(mems, "onMappedInjectionModeRequested",
                                          Qt::QueuedConnection,
                                          Q_ARG(bool, false),
                                          Q_ARG(QString, QString()));
            });

        return QObject::eventFilter(watched, event);
    }
};

static void installMappedInjectionInstaller()
{
    QApplication *app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (app)
        app->installEventFilter(new MappedInjectionInstaller(app));
}

Q_COREAPP_STARTUP_FUNCTION(installMappedInjectionInstaller)
