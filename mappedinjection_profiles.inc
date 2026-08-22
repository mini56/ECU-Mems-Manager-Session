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

    // Mini SPi MEMS 1.6 family using the validated AANMP002 injection path.
    // AANMP002 is validated from ROM execution and real ECU Mode-4 captures.
    // Andrew Revill's RAM-variable correlation maps ABNMP003 to the same
    // 0x03C8 / 0x026E / 0x0280 variable classes, so it uses the same profile:
    // final = base + HI16(transient * 0xC001), at 0.002 ms per timer tick.
    if (id == QStringLiteral("AANMP002") ||
        id == QStringLiteral("ABNMP003"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x03C8u, 0x026Eu, 0x0280u},
            InjectionCalculationKind::AdditiveC001,
            InjectionCounterPolicy::NonZeroActive,
            0.002,
            0.002);
        return true;
    }

    // ACNMF024 / Mini Auto MEMS 1.6.
    // Andrew Revill's full-triplet correlation keeps the same 0x03C8 duration
    // and 0x026E conditional-source classes as the validated Mini SPi path;
    // the transient state/counter is relocated to 0x0282.
    if (id == QStringLiteral("ACNMF024"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x03C8u, 0x026Eu, 0x0282u},
            InjectionCalculationKind::AdditiveC001,
            InjectionCounterPolicy::NonZeroActive,
            0.002,
            0.002);
        return true;
    }

    // MEMS 1.6 family with the AANMP002 additive injection path and the
    // transient state/counter relocated to 0x0281. For every firmware below,
    // Andrew Revill's correlated map gives 0x03C8 / 0x026E / 0x0281 and the
    // ASM directly loads 0x026E after checking 0x0281, then calls the routine
    // performing mulu #-3FFFh (0xC001) and adds the high word to the injector
    // timer accumulator. This is the same validated 0.002 ms/tick path.
    if (id == QStringLiteral("KCE6F014") ||
        id == QStringLiteral("KCE6F018") ||
        id == QStringLiteral("KCE6P004") ||
        id == QStringLiteral("KGM6F001") ||
        id == QStringLiteral("KGM6P002") ||
        id == QStringLiteral("TBP7F016") ||
        id == QStringLiteral("TBP7P005") ||
        id == QStringLiteral("TBP7P006"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x03C8u, 0x026Eu, 0x0281u},
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

    // Mini Cooper late MEMS 1.6 family.
    // Andrew Revill's correlated RAM-variable classes identify the same
    // s_pulsewidth and t_asyn_pw variables as the PETA3 anchor above, with
    // t_asyn_fire_cnt relocated to 0x0283. His correlator explicitly carries
    // PETA definitions by variable class across firmwares, so the scale is
    // inherited from the class definition rather than guessed from the ID.
    if (id == QStringLiteral("ABNMF032") ||
        id == QStringLiteral("ABNMP005") ||
        id == QStringLiteral("ABNMP006"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x03C8u, 0x0270u, 0x0283u},
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
