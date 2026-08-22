    // Safety check for the address-specific Injection RAM reader only.
    // A correlated RAM address is not sufficient: the calculation semantics
    // and scaling for this exact firmware must also have been validated.
    InjectionCalculationProfile profile;
    if(!validatedInjectionProfileForFirmware(firmwareIdentifier,&profile)){onMappedInjectionRequestState(false);return;}
    const InjectionRamMap &map=profile.map;
    const QString firmwareId=firmwareIdentifier.trimmed().toUpper();
    const bool mems13ConditionalRead=(firmwareId==QStringLiteral("KBE6R003")||firmwareId==QStringLiteral("MGE7R002"));
    // MEMS 1.9 correlated profiles use the stable 0x004E scheduler contribution
    // plus one 0x8000-biased injector compensation word.  The zero counter is
    // intentional: both words belong to the same normal scheduled pulse and are
    // always acquired in this single Mode-4 sample cycle.
    const bool mems19ScheduledRead=(map.baseAddress==0x004Eu&&map.transientSourceAddress!=0u&&map.transientCounterAddress==0u&&profile.baseMsPerRaw==0.008);
    const bool directFinalRead=(!mems19ScheduledRead&&map.transientSourceAddress==0u&&map.transientCounterAddress==0u);

    setProperty("mappedInjectionModeActive",true);emit readModeChanged(QStringLiteral("transition"));
    bool entered=false;bool ok=ensureNormalSession(&m_memsinfo);
    if(ok&&m_mappedInjectionRequested.loadAcquire()!=0)ok=enterMode4(&m_memsinfo);
    if(ok&&m_mappedInjectionRequested.loadAcquire()!=0){entered=true;emit readModeChanged(QStringLiteral("injection"));}

    int failures=0;
    bool havePreviousCounter=false;
    quint8 previousCounter=0;
    while(entered&&m_mappedInjectionRequested.loadAcquire()!=0&&!m_stopPolling&&!m_shutdownThread&&mems_is_connected(&m_memsinfo))
    {
        quint8 counter=0;quint16 transientRaw=0,baseRaw=0;quint8 selected=0xFF;
        const auto ensureBlock=[&](quint8 block)->bool{if(selected==block)return true;if(!selectMode4Block(&m_memsinfo,block))return false;selected=block;return true;};
        bool sampleOk=true;
        bool transientActive=false;
        if(mems13ConditionalRead)
        {
            // KBE6R003/MGE7R002 only: the firmware tests the complete 16-bit
            // state word at 0x0078 against 0x8000 before applying the biased
            // signed correction.  Keep every read inside this same Mode-4
            // sample cycle; no diagnostic 7D/80 value is mixed into it.
            quint16 stateRaw=0;
            sampleOk=ensureBlock(quint8(map.transientCounterAddress>>8))&&readMappedWordInBlock(&m_memsinfo,map.transientCounterAddress,selected,&stateRaw);
            transientActive=sampleOk&&stateRaw==0x8000u;
            counter=transientActive?quint8(1):quint8(0);
            if(sampleOk&&transientActive)sampleOk=ensureBlock(quint8(map.transientSourceAddress>>8))&&readMappedWordInBlock(&m_memsinfo,map.transientSourceAddress,selected,&transientRaw);
        }
        else if(mems19ScheduledRead)
        {
            // MEMS 1.9 normal pulse: read the injector compensation word first,
            // then the scheduler contribution. Both reads stay in this Mode-4
            // sample. No normal 7D/80 polling value is used here.
            sampleOk=ensureBlock(quint8(map.transientSourceAddress>>8))&&readMappedWordInBlock(&m_memsinfo,map.transientSourceAddress,selected,&transientRaw);
        }
        else if(!directFinalRead)
        {
            sampleOk=ensureBlock(quint8(map.transientCounterAddress>>8))&&readMappedByteInBlock(&m_memsinfo,map.transientCounterAddress,selected,&counter);
            transientActive=sampleOk&&injectionTransientActive(profile,counter,havePreviousCounter,previousCounter);
            if(sampleOk&&transientActive)sampleOk=ensureBlock(quint8(map.transientSourceAddress>>8))&&readMappedWordInBlock(&m_memsinfo,map.transientSourceAddress,selected,&transientRaw);
        }
        if(sampleOk)sampleOk=ensureBlock(quint8(map.baseAddress>>8))&&readMappedWordInBlock(&m_memsinfo,map.baseAddress,selected,&baseRaw);
        if(!sampleOk){if(++failures>=3)break;QThread::msleep(20);continue;}failures=0;

        if(!directFinalRead&&!mems13ConditionalRead&&!mems19ScheduledRead){previousCounter=counter;havePreviousCounter=true;}
        double finalMs=0.0,baseMs=0.0,transientMs=0.0;
        if(mems13ConditionalRead)
        {
            // Exact 80196 operation used immediately before injector service 5:
            //   final16 = base + correction + 0x8000 (16-bit wrap)
            //   if bit 15 remains set, clamp the pulse to zero.
            // The correction word is a 0x8000-biased signed quantity.
            quint16 finalRaw=baseRaw;
            if(transientActive)
            {
                quint16 adjusted=quint16(quint32(baseRaw)+quint32(transientRaw)+0x8000u);
                if((adjusted&0x8000u)!=0u)adjusted=0u;
                finalRaw=adjusted;
            }
            baseMs=double(baseRaw)*profile.baseMsPerRaw;
            finalMs=double(finalRaw)*profile.baseMsPerRaw;
            transientMs=transientActive?(finalMs-baseMs):0.0;
        }
        else if(mems19ScheduledRead)
        {
            // The MEMS 1.9 scheduler loads 0x004E eight times into each bank,
            // then adds (compensation - 0x8000) immediately before the HSO
            // injector output. The output timer and diagnostic injector test
            // both use 1 us/raw, so the final result is converted at 0.001 ms.
            qint64 finalRaw=qint64(baseRaw)*8+qint64(transientRaw)-qint64(0x8000u);
            if(finalRaw<0)finalRaw=0;
            if(finalRaw>0xFFFF)finalRaw=0xFFFF;
            baseMs=double(baseRaw)*profile.baseMsPerRaw;
            finalMs=double(finalRaw)*profile.transientMsPerRaw;
            // The second word is injector dead-time/compensation, not an
            // asynchronous enrichment pulse. Do not misreport it as transient.
            transientMs=0.0;
        }
        else if(!calculateValidatedInjectionSample(profile,baseRaw,transientRaw,transientActive,&finalMs,&baseMs,&transientMs))break;
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
