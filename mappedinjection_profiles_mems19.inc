// Rover MEMS 1.9 injection profiles - correlated batch 1/7.
//
// These firmware schedulers keep the normal pulse contribution at INTMEM 0x004E.
// The scheduler copies that contribution eight times into each injector-bank
// accumulator before firing, therefore baseMsPerRaw is 0.008 ms/raw here.
// Immediately before the injector output routine the firmware adds a 0x8000-
// biased injector compensation word.  The compensation address is 0x014A or
// 0x014C depending on the correlated firmware family below.
//
// The low-level injector test path loads approximately 2000 raw units for the
// documented ~2 ms test pulse, independently confirming 0.001 ms per scheduler
// timer unit.  Both words are read in one Mode-4 sample cycle; no 7D/80 value is
// mixed into this calculation.
static bool validatedInjectionProfileForFirmware(const QString &firmware,
                                                 InjectionCalculationProfile *profile)
{
    if (!profile)
        return false;

    if (validatedInjectionProfileForFirmware13(firmware, profile))
        return true;

    const QString id = firmware.trimmed().toUpper();
    quint16 compensationAddress = 0u;

    if (id == QStringLiteral("KBHBG009") ||
        id == QStringLiteral("KBHBV001") ||
        id == QStringLiteral("KBHBV002") ||
        id == QStringLiteral("KBHHV002") ||
        id == QStringLiteral("KFACG002") ||
        id == QStringLiteral("KFACV000") ||
        id == QStringLiteral("KFACV001") ||
        id == QStringLiteral("KFHBG008") ||
        id == QStringLiteral("KFHBG010") ||
        id == QStringLiteral("KFHBV001") ||
        id == QStringLiteral("KFHBV002") ||
        id == QStringLiteral("KFHBV003") ||
        id == QStringLiteral("KFHBV004") ||
        id == QStringLiteral("KFHBV010") ||
        id == QStringLiteral("KFHHV002") ||
        id == QStringLiteral("KFHHV003") ||
        id == QStringLiteral("KIH6G011") ||
        id == QStringLiteral("KIH6V003"))
    {
        compensationAddress = 0x014Au;
    }
    else if (id == QStringLiteral("KBHBG010") ||
             id == QStringLiteral("KBHHG008") ||
             id == QStringLiteral("KFACG001") ||
             id == QStringLiteral("KFHBG007") ||
             id == QStringLiteral("KFHBG009") ||
             id == QStringLiteral("KIH6G010") ||
             id == QStringLiteral("KIH6G012"))
    {
        compensationAddress = 0x014Cu;
    }
    else
    {
        return false;
    }

    *profile = InjectionCalculationProfile(
        InjectionRamMap{0x004Eu, compensationAddress, 0x0000u},
        InjectionCalculationKind::IndependentAsync,
        InjectionCounterPolicy::NonZeroActive,
        0.008,   // 8 scheduler contributions * 1 us/raw
        0.001);  // biased injector compensation uses 1 us/raw
    return true;
}
