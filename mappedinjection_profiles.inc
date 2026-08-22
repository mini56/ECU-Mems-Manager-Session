enum class InjectionCalculationKind
{
    Unverified = 0,
    AdditiveC001,
    IndependentAsync
};

struct InjectionCalculationProfile
{
    InjectionRamMap map;
    InjectionCalculationKind kind;
    double baseMsPerRaw;
    double transientMsPerRaw;

    InjectionCalculationProfile()
        : map{0, 0, 0}, kind(InjectionCalculationKind::Unverified),
          baseMsPerRaw(0.0), transientMsPerRaw(0.0)
    {
    }

    InjectionCalculationProfile(const InjectionRamMap &mapping,
                                InjectionCalculationKind calculation,
                                double baseScale,
                                double transientScale)
        : map(mapping), kind(calculation),
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
            0.002,
            0.002);
        return true;
    }

    // ACNMP001 / MEMS 1.6.
    // Andrew Revill PETA3 definitions:
    // s_pulsewidth     0x03C8 W 0.001333 ms/raw
    // t_asyn_pw        0x0270 W 0.001000 ms/raw
    // t_asyn_fire_cnt  0x00EC B
    // The ASM increments the asynchronous-fire counter, then loads 0x0270
    // and calls a separate injector routine. It is therefore a separate
    // asynchronous pulse, not an additive term on the synchronous pulse.
    if (id == QStringLiteral("ACNMP001"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x03C8u, 0x0270u, 0x00ECu},
            InjectionCalculationKind::IndependentAsync,
            0.001333,
            0.001);
        return true;
    }

    // ACNMP003 / MEMS 1.6. PETA3 exposes the same pulse-width semantics,
    // with the asynchronous-fire counter relocated to 0x0285.
    if (id == QStringLiteral("ACNMP003"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x03C8u, 0x0270u, 0x0285u},
            InjectionCalculationKind::IndependentAsync,
            0.001333,
            0.001);
        return true;
    }

    // MEMS 1.3 ABEMR002 / KBE6R003 / MGE7R002 are intentionally not
    // enabled here yet. Their public Andrew Revill archive has ASM/ROM data
    // but no PETA transfer definitions, and their injection calculation must
    // be correlated independently before any RAM address is sent to an ECU.
    return false;
}

static bool calculateValidatedInjectionSample(const InjectionCalculationProfile &profile,
                                              quint16 baseRaw,
                                              quint16 transientRaw,
                                              quint8 counter,
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
        if (counter != 0)
            *transientMs = double(highWordC001(transientRaw)) * profile.transientMsPerRaw;
        *finalMs = *baseMs + *transientMs;
        return true;
    }

    if (profile.kind == InjectionCalculationKind::IndependentAsync)
    {
        // This is a distinct asynchronous injector firing. Do not sum two
        // physically separate pulse widths and present the result as one pulse.
        if (counter != 0)
            *transientMs = double(transientRaw) * profile.transientMsPerRaw;
        return true;
    }

    return false;
}
