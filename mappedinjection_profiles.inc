enum class InjectionCalculationKind
{
    Unverified = 0,
    AdditiveC001,
    IndependentAsync
};

enum class InjectionCounterPolicy
{
    NonZeroActive = 0,
    IncrementEvent
};

struct InjectionCalculationProfile
{
    InjectionRamMap map;
    InjectionCalculationKind kind;
    InjectionCounterPolicy counterPolicy;
    double baseMsPerRaw;
    double transientMsPerRaw;

    InjectionCalculationProfile()
        : map{0, 0, 0}, kind(InjectionCalculationKind::Unverified),
          counterPolicy(InjectionCounterPolicy::NonZeroActive),
          baseMsPerRaw(0.0), transientMsPerRaw(0.0)
    {
    }

    InjectionCalculationProfile(const InjectionRamMap &mapping,
                                InjectionCalculationKind calculation,
                                InjectionCounterPolicy counter,
                                double baseScale,
                                double transientScale)
        : map(mapping), kind(calculation), counterPolicy(counter),
          baseMsPerRaw(baseScale), transientMsPerRaw(transientScale)
    {
    }
};

static bool validatedInjectionProfileForFirmware(const QString &firmware,
                                                 InjectionCalculationProfile *profile)
{
    if (!profile)
        return false;

    const QString id = firmware.trimmed().toUpper();

    // AANMP002 / MEMS 1.6 Mini SPi.
    // Validated from the ROM execution path and real ECU Mode-4 captures:
    // final = base(0x03C8) + HI16(transient(0x026E) * 0xC001)
    // while counter 0x0280 indicates the transient contribution is active.
    // Timer granularity is 0.002 ms per tick.
    if (id == QStringLiteral("AANMP002"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x03C8u, 0x026Eu, 0x0280u},
            InjectionCalculationKind::AdditiveC001,
            InjectionCounterPolicy::NonZeroActive,
            0.002,
            0.002);
        return true;
    }

    // ACNMP001 / MEMS 1.6.
    // Andrew Revill PETA3 definitions:
    // s_pulsewidth     0x03C8 W 0.001333 ms/raw
    // t_asyn_pw        0x0270 W 0.001000 ms/raw
    // t_asyn_fire_cnt  0x00EC B
    // The ASM increments the fire counter for each separate asynchronous pulse.
    if (id == QStringLiteral("ACNMP001"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x03C8u, 0x0270u, 0x00ECu},
            InjectionCalculationKind::IndependentAsync,
            InjectionCounterPolicy::IncrementEvent,
            0.001333,
            0.001);
        return true;
    }

    // Mini Auto immobilised MEMS 1.6 family.
    // ACNMP003 is the direct PETA3 anchor:
    //   s_pulsewidth    0x03C8 W 0.001333 ms/raw
    //   t_asyn_pw       0x0270 W 0.001000 ms/raw
    //   t_asyn_fire_cnt 0x0285 B
    // Andrew Revill's 1.6 correlation data maps ACNMF026, ACNMP004 and
    // ACNMP005 to the same three RAM variables. They therefore share this
    // calculation profile; no address or scale is inferred by proximity alone.
    if (id == QStringLiteral("ACNMP003") ||
        id == QStringLiteral("ACNMF026") ||
        id == QStringLiteral("ACNMP004") ||
        id == QStringLiteral("ACNMP005"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x03C8u, 0x0270u, 0x0285u},
            InjectionCalculationKind::IndependentAsync,
            InjectionCounterPolicy::IncrementEvent,
            0.001333,
            0.001);
        return true;
    }

    // AANMP000 / MEMS 1.6. Andrew Revill ASM correlates the same synchronous
    // snapshot at 0x03C8, asynchronous width at 0x0270 and the same C001/timer
    // path as ACNMP001. The low byte at 0x00EB is incremented per async fire.
    if (id == QStringLiteral("AANMP000"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x03C8u, 0x0270u, 0x00EBu},
            InjectionCalculationKind::IndependentAsync,
            InjectionCounterPolicy::IncrementEvent,
            0.001333,
            0.001);
        return true;
    }

    // KGM6P000 / MEMS 1.6. Andrew Revill ASM shows the same C001/timer path,
    // synchronous snapshot at 0x03C8 and async width at 0x0270. Counter 0x00EC
    // is reset by the firmware and incremented for each asynchronous fire.
    if (id == QStringLiteral("KGM6P000"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x03C8u, 0x0270u, 0x00ECu},
            InjectionCalculationKind::IndependentAsync,
            InjectionCounterPolicy::IncrementEvent,
            0.001333,
            0.001);
        return true;
    }

    // MEMS 1.3 ABEMR002 / KBE6R003 / MGE7R002 remain intentionally disabled
    // until their injection calculation, RAM addresses and timing scale have
    // been independently correlated from their Andrew Revill ASM/ROM data.
    return false;
}

static bool injectionTransientActive(const InjectionCalculationProfile &profile,
                                     quint8 counter,
                                     bool havePreviousCounter,
                                     quint8 previousCounter)
{
    if (profile.counterPolicy == InjectionCounterPolicy::NonZeroActive)
        return counter != 0;

    if (profile.counterPolicy == InjectionCounterPolicy::IncrementEvent)
    {
        // Establish a baseline on the first sample. A cumulative event counter
        // being non-zero does not mean an asynchronous pulse is active now.
        // Ignore a transition to zero so a firmware counter reset is not
        // mistaken for a new injection event.
        return havePreviousCounter && counter != 0 && counter != previousCounter;
    }

    return false;
}

static bool calculateValidatedInjectionSample(const InjectionCalculationProfile &profile,
                                              quint16 baseRaw,
                                              quint16 transientRaw,
                                              bool transientActive,
                                              double *finalMs,
                                              double *baseMs,
                                              double *transientMs)
{
    if (!finalMs || !baseMs || !transientMs)
        return false;

    *baseMs = double(baseRaw) * profile.baseMsPerRaw;
    *transientMs = 0.0;
    *finalMs = *baseMs;

    if (profile.kind == InjectionCalculationKind::AdditiveC001)
    {
        if (transientActive)
            *transientMs = double(highWordC001(transientRaw)) * profile.transientMsPerRaw;
        *finalMs = *baseMs + *transientMs;
        return true;
    }

    if (profile.kind == InjectionCalculationKind::IndependentAsync)
    {
        // This is a distinct asynchronous injector firing. Its pulse width is
        // displayed separately and is never summed with the synchronous pulse.
        if (transientActive)
            *transientMs = double(transientRaw) * profile.transientMsPerRaw;
        return true;
    }

    return false;
}
