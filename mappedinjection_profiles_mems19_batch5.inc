// Rover MEMS 1.9 correlated batch 5: firmware 101-125.
// Same validated normal x8 scheduler as the earlier MEMS 1.9 families.
// 0x004E is accumulated eight times; the 0x8000-biased compensation address
// is correlated per firmware. 1 scheduler timer unit = 1 us.
static bool validatedInjectionProfileForFirmware(const QString &firmware,
                                                 InjectionCalculationProfile *profile)
{
    if (!profile) return false;
    if (validatedInjectionProfileForFirmwareMems19B4(firmware, profile)) return true;

    struct Entry { const char *id; quint16 compensation; double baseScale; };
    static const Entry entries[] = {
        {"KTHBG013", 0x014Au, 0.008},
        {"KTHBG014", 0x014Cu, 0.008},
        {"KTHBG015", 0x014Cu, 0.008},
        {"KTHBV000", 0x014Au, 0.008},
        {"KTHBV001", 0x014Au, 0.008},
        {"KTHBV002", 0x014Au, 0.008},
        {"KTHBV003", 0x014Au, 0.008},
        {"KTHBV004", 0x014Au, 0.008},
        {"KTHBV005", 0x014Au, 0.008},
        {"KTHHG000", 0x014Au, 0.008},
        {"KTHHV000", 0x014Au, 0.008},
        {"KUH3G000", 0x014Cu, 0.008},
        {"KUH3G001", 0x014Cu, 0.008},
        {"KUH3G002", 0x014Cu, 0.008},
        {"KUH3G003", 0x014Cu, 0.008},
        {"KUHHG000", 0x014Au, 0.008},
        {"KUHHG001", 0x014Cu, 0.008},
        {"TAH7G018", 0x014Cu, 0.008},
        {"TAH7V000", 0x0148u, 0.008},
        {"TAH7V001", 0x014Cu, 0.008},
        {"TAH7V002", 0x014Cu, 0.008},
        {"TAH7V003", 0x014Cu, 0.008},
        {"TAH7V004", 0x014Cu, 0.008},
        {"TAH8V000", 0x014Cu, 0.008},
        {"TAH8V001", 0x014Cu, 0.008},
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
