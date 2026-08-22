// Additional MEMS 1.6 NA profiles recovered directly from the correlated
// Andrew Revill ASM files which were not part of the 82-entry RAM-map class.
//
// Validation rule for every firmware below:
//  - the synchronous injector timer snapshot is stored to the listed base word;
//  - the listed byte is incremented immediately before a separate async fire;
//  - the listed async-width word is loaded and passed to a routine performing
//    mulu #-3FFFh (0xC001), whose high word is added to the injector timer.
// This is the same IndependentAsync execution path anchored by ACNMP001 /
// AANMP000: 0.001333 ms per synchronous timer raw and 0.001 ms per async raw.
static bool validatedInjectionProfileForFirmware(const QString &firmware,
                                                 InjectionCalculationProfile *profile)
{
    if (!profile)
        return false;

    if (validatedInjectionProfileForFirmwareExisting(firmware, profile))
        return true;

    const QString id = firmware.trimmed().toUpper();

    if (id == QStringLiteral("AANMP001") ||
        id == QStringLiteral("ABNMF030") ||
        id == QStringLiteral("ABNMP000") ||
        id == QStringLiteral("ABNMP002") ||
        id == QStringLiteral("TAP7P004") ||
        id == QStringLiteral("TAP8P003") ||
        id == QStringLiteral("TAP8P005") ||
        id == QStringLiteral("TBP7P000"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x03C8u, 0x0270u, 0x00EBu},
            InjectionCalculationKind::IndependentAsync,
            InjectionCounterPolicy::IncrementEvent,
            0.001333,
            0.001);
        return true;
    }

    if (id == QStringLiteral("KGE6F021") ||
        id == QStringLiteral("KGM6P001") ||
        id == QStringLiteral("TBP7P004"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x03C8u, 0x0270u, 0x00ECu},
            InjectionCalculationKind::IndependentAsync,
            InjectionCounterPolicy::IncrementEvent,
            0.001333,
            0.001);
        return true;
    }

    if (id == QStringLiteral("KBP8P001") ||
        id == QStringLiteral("TAEDP000") ||
        id == QStringLiteral("TAP7P000") ||
        id == QStringLiteral("TAP8P001") ||
        id == QStringLiteral("TBP7P003"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x03C8u, 0x026Eu, 0x00EAu},
            InjectionCalculationKind::IndependentAsync,
            InjectionCounterPolicy::IncrementEvent,
            0.001333,
            0.001);
        return true;
    }

    if (id == QStringLiteral("KBP8P003") ||
        id == QStringLiteral("KBP8P004") ||
        id == QStringLiteral("KCD8P000") ||
        id == QStringLiteral("KCE6P000") ||
        id == QStringLiteral("KCE6P001") ||
        id == QStringLiteral("KCE6P002") ||
        id == QStringLiteral("KDE6P001") ||
        id == QStringLiteral("KDE6P002") ||
        id == QStringLiteral("TAP7P003"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x03C8u, 0x026Eu, 0x00EBu},
            InjectionCalculationKind::IndependentAsync,
            InjectionCounterPolicy::IncrementEvent,
            0.001333,
            0.001);
        return true;
    }

    if (id == QStringLiteral("KBP8P002"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x03C8u, 0x026Cu, 0x00EFu},
            InjectionCalculationKind::IndependentAsync,
            InjectionCounterPolicy::IncrementEvent,
            0.001333,
            0.001);
        return true;
    }

    if (id == QStringLiteral("KBP8P000") ||
        id == QStringLiteral("TAP8P000"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x03CAu, 0x026Cu, 0x00EFu},
            InjectionCalculationKind::IndependentAsync,
            InjectionCounterPolicy::IncrementEvent,
            0.001333,
            0.001);
        return true;
    }

    if (id == QStringLiteral("KAE6P000"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x03C8u, 0x026Au, 0x00EBu},
            InjectionCalculationKind::IndependentAsync,
            InjectionCounterPolicy::IncrementEvent,
            0.001333,
            0.001);
        return true;
    }

    if (id == QStringLiteral("TAP8P002"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x03C8u, 0x0268u, 0x00EBu},
            InjectionCalculationKind::IndependentAsync,
            InjectionCounterPolicy::IncrementEvent,
            0.001333,
            0.001);
        return true;
    }

    return false;
}
