// Rover MEMS 1.9 special firmware TBP7F015.
// Unlike the other correlated MEMS 1.9 firmwares, TBP7F015 does not use
// scheduler contribution 0x004E or compensation 0x0148/0x014A/0x014C.
// Its ASM proves an 8-count scheduler contribution at 0x0056; the accumulated
// scheduler is then combined immediately before injector output with the
// 0x8000-biased compensation word at 0x0040.
// 1 scheduler timer unit = 1 us.
static bool validatedInjectionProfileForFirmware(const QString &firmware,
                                                 InjectionCalculationProfile *profile)
{
    if (!profile) return false;
    if (validatedInjectionProfileForFirmwareMems19B7(firmware, profile)) return true;

    const QString id = firmware.trimmed().toUpper();
    if (id != QStringLiteral("TBP7F015")) return false;

    *profile = InjectionCalculationProfile(
        InjectionRamMap{0x0056u, 0x0040u, 0x0000u},
        InjectionCalculationKind::IndependentAsync,
        InjectionCounterPolicy::NonZeroActive,
        0.008,
        0.001);
    return true;
}
