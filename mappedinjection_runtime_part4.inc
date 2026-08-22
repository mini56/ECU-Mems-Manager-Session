    // Safety check for the address-specific Injection RAM reader only.
    // A correlated RAM address is not sufficient: the calculation semantics
    // and scaling for this exact firmware must also have been validated.
    InjectionCalculationProfile profile;
    if(!validatedInjectionProfileForFirmware(firmwareIdentifier,&profile)){onMappedInjectionRequestState(false);return;}
    const InjectionRamMap &map=profile.map;

    setProperty("mappedInjectionModeActive",true);emit readModeChanged(QStringLiteral("transition"));
    bool entered=false;bool ok=ensureNormalSession(&m_memsinfo);
    if(ok&&m_mappedInjectionRequested.loadAcquire()!=0)ok=enterMode4(&m_memsinfo);
    if(ok&&m_mappedInjectionRequested.loadAcquire()!=0){entered=true;emit readModeChanged(QStringLiteral("injection"));}

    int failures=0;
    while(entered&&m_mappedInjectionRequested.loadAcquire()!=0&&!m_stopPolling&&!m_shutdownThread&&mems_is_connected(&m_memsinfo))
    {
        quint8 counter=0;quint16 transientRaw=0,baseRaw=0;quint8 selected=0xFF;
        const auto ensureBlock=[&](quint8 block)->bool{if(selected==block)return true;if(!selectMode4Block(&m_memsinfo,block))return false;selected=block;return true;};
        bool sampleOk=ensureBlock(quint8(map.transientCounterAddress>>8))&&readMappedByteInBlock(&m_memsinfo,map.transientCounterAddress,selected,&counter);
        if(sampleOk&&counter!=0)sampleOk=ensureBlock(quint8(map.transientSourceAddress>>8))&&readMappedWordInBlock(&m_memsinfo,map.transientSourceAddress,selected,&transientRaw);
        if(sampleOk)sampleOk=ensureBlock(quint8(map.baseAddress>>8))&&readMappedWordInBlock(&m_memsinfo,map.baseAddress,selected,&baseRaw);
        if(!sampleOk){if(++failures>=3)break;QThread::msleep(20);continue;}failures=0;

        double finalMs=0.0,baseMs=0.0,transientMs=0.0;
        if(!calculateValidatedInjectionSample(profile,baseRaw,transientRaw,counter,&finalMs,&baseMs,&transientMs))break;
        emit injectionLiveSample(finalMs,baseMs,transientMs,baseRaw,transientRaw,counter);
        QThread::msleep(8);
    }

    emit readModeChanged(QStringLiteral("transition"));
    const bool restored=entered?restoreNormalSession(&m_memsinfo):ensureNormalSession(&m_memsinfo);
    onMappedInjectionRequestState(false);setProperty("mappedInjectionModeActive",false);
    if(restored)emit readModeChanged(QStringLiteral("diagnostic"));
    else {emit readError();m_stopPolling=true;}
}

Q_COREAPP_STARTUP_FUNCTION(installMappedInjectionUi)
