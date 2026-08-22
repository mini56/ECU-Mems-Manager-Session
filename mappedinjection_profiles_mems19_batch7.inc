// Rover MEMS 1.9 correlated batch 7: firmware 151-163 (TBP7F015 deferred).
// Final normal correlated MEMS 1.9 group.
// Each firmware stores the scheduler word at 0x004E, loads an 8-count
// accumulation, and applies the 0x8000-biased compensation at 0x014C.
// 1 scheduler timer unit = 1 us.
static bool validatedInjectionProfileForFirmware(const QString &firmware,
                                                 InjectionCalculationProfile *profile)
{
    if (!profile) return false;
    if (validatedInjectionProfileForFirmwareMems19B6(firmware, profile)) return true;

    struct Entry { const char *id; quint16 compensation; double baseScale; };
    static const Entry entries[] = {
        {"TFH7V006", 0x014Cu, 0.008},
        {"TFH7V007", 0x014Cu, 0.008},
        {"TFHSG011", 0x014Cu, 0.008},
        {"TFHSG012", 0x014Cu, 0.008},
        {"TFHSG013", 0x014Cu, 0.008},
        {"TFHSG014", 0x014Cu, 0.008},
        {"TFHSV000", 0x014Cu, 0.008},
        {"TFHSV001", 0x014Cu, 0.008},
        {"TFHSV002", 0x014Cu, 0.008},
        {"TFHSV003", 0x014Cu, 0.008},
        {"TFHSV004", 0x014Cu, 0.008},
        {"TFHSV005", 0x014Cu, 0.008},
        {"TFHSV006", 0x014Cu, 0.008},
    };

    const QString id = firmware.trimmed().toUpper();
    for (const Entry &entry : entries)
    {
        if (id != QLatin1String(entry.id)) continue;
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x004Eu, entry.compensation, 0x0000u},
            InjectionCalculationKind::IndependentAsync,
            InjectionCounterPolicy::NonZeroActive,
            entry.baseScale,
            0.001);
        return true;
    }
    return false;
}
