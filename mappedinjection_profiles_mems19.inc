// Rover MEMS 1.9 injection profiles - correlated batches 1-2/7 (50 firmware).
//
// Normal family: INTMEM 0x004E is accumulated eight times, then a 0x8000-biased
// injector compensation word is applied. 1 scheduler timer unit = 1 us.
// KLH4 family: same source/correction principle, but the final HSO width is
// doubled by the firmware; baseMsPerRaw=0.016 marks that proven scheduler variant.
// All words are acquired in one Mode-4 sample cycle.
static bool validatedInjectionProfileForFirmware(const QString &firmware,
                                                 InjectionCalculationProfile *profile)
{
    if (!profile)
        return false;

    if (validatedInjectionProfileForFirmware13(firmware, profile))
        return true;

    const QString id = firmware.trimmed().toUpper();
    quint16 compensationAddress = 0u;
    double baseScale = 0.0;

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
        id == QStringLiteral("KIH6V003") ||
        id == QStringLiteral("KIH6V004") ||
        id == QStringLiteral("KIHBG000") ||
        id == QStringLiteral("KIHBV000") ||
        id == QStringLiteral("KJH6G009") ||
        id == QStringLiteral("KJH6G012") ||
        id == QStringLiteral("KJH6V000") ||
        id == QStringLiteral("KJH6V001") ||
        id == QStringLiteral("KJH6V002") ||
        id == QStringLiteral("KJH6V003") ||
        id == QStringLiteral("KJH6V004") ||
        id == QStringLiteral("KJH6V005"))
    {
        compensationAddress = 0x014Au;
        baseScale = 0.008;
    }
    else if (id == QStringLiteral("KBHBG010") ||
        id == QStringLiteral("KBHHG008") ||
        id == QStringLiteral("KFACG001") ||
        id == QStringLiteral("KFHBG007") ||
        id == QStringLiteral("KFHBG009") ||
        id == QStringLiteral("KIH6G010") ||
        id == QStringLiteral("KIH6G012") ||
        id == QStringLiteral("KJH6G008") ||
        id == QStringLiteral("KJH6G011") ||
        id == QStringLiteral("KLH3G015") ||
        id == QStringLiteral("KLH3G017") ||
        id == QStringLiteral("KLH3G018") ||
        id == QStringLiteral("KLH3G019") ||
        id == QStringLiteral("KLH3G020") ||
        id == QStringLiteral("KLH3V004") ||
        id == QStringLiteral("KLH3V005") ||
        id == QStringLiteral("KLH3V006"))
    {
        compensationAddress = 0x014Cu;
        baseScale = 0.008;
    }
    else if (id == QStringLiteral("KLH4G017") ||
        id == QStringLiteral("KLH4G020"))
    {
        compensationAddress = 0x014Au;
        baseScale = 0.016;
    }
    else if (id == QStringLiteral("KLH4G018") ||
        id == QStringLiteral("KLH4G019"))
    {
        compensationAddress = 0x014Cu;
        baseScale = 0.016;
    }
    else
    {
        return false;
    }

    *profile = InjectionCalculationProfile(
        InjectionRamMap{0x004Eu, compensationAddress, 0x0000u},
        InjectionCalculationKind::IndependentAsync,
        InjectionCounterPolicy::NonZeroActive,
        baseScale,
        0.001);
    return true;
}
