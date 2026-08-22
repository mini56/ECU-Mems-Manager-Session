// Rover MEMS 1.9 injection profiles - 75 correlated firmware enabled.
// 0x004E is the stable normal scheduler contribution. The compensation word
// is 0x8000-biased and is correlated per firmware below. baseScale 0.008 is
// the normal x8 scheduler; 0.016 marks the proven final-doubled KLH4/KLQ4 path.
// 1 scheduler timer unit = 1 us. All reads stay in one Mode-4 sample cycle.
static bool validatedInjectionProfileForFirmware(const QString &firmware,
                                                 InjectionCalculationProfile *profile)
{
    if (!profile) return false;
    if (validatedInjectionProfileForFirmware13(firmware, profile)) return true;
    struct Entry { const char *id; quint16 compensation; double baseScale; };
    static const Entry entries[] = {
        {"KBHBG009", 0x014Au, 0.008},
        {"KBHBG010", 0x014Cu, 0.008},
        {"KBHBV001", 0x014Au, 0.008},
        {"KBHBV002", 0x014Au, 0.008},
        {"KBHHG008", 0x014Cu, 0.008},
        {"KBHHV002", 0x014Au, 0.008},
        {"KFACG001", 0x014Cu, 0.008},
        {"KFACG002", 0x014Au, 0.008},
        {"KFACV000", 0x014Au, 0.008},
        {"KFACV001", 0x014Au, 0.008},
        {"KFHBG007", 0x014Cu, 0.008},
        {"KFHBG008", 0x014Au, 0.008},
        {"KFHBG009", 0x014Cu, 0.008},
        {"KFHBG010", 0x014Au, 0.008},
        {"KFHBV001", 0x014Au, 0.008},
        {"KFHBV002", 0x014Au, 0.008},
        {"KFHBV003", 0x014Au, 0.008},
        {"KFHBV004", 0x014Au, 0.008},
        {"KFHBV010", 0x014Au, 0.008},
        {"KFHHV002", 0x014Au, 0.008},
        {"KFHHV003", 0x014Au, 0.008},
        {"KIH6G010", 0x014Cu, 0.008},
        {"KIH6G011", 0x014Au, 0.008},
        {"KIH6G012", 0x014Cu, 0.008},
        {"KIH6V003", 0x014Au, 0.008},
        {"KIH6V004", 0x014Au, 0.008},
        {"KIHBG000", 0x014Au, 0.008},
        {"KIHBV000", 0x014Au, 0.008},
        {"KJH6G008", 0x014Cu, 0.008},
        {"KJH6G009", 0x014Au, 0.008},
        {"KJH6G011", 0x014Cu, 0.008},
        {"KJH6G012", 0x014Au, 0.008},
        {"KJH6V000", 0x014Au, 0.008},
        {"KJH6V001", 0x014Au, 0.008},
        {"KJH6V002", 0x014Au, 0.008},
        {"KJH6V003", 0x014Au, 0.008},
        {"KJH6V004", 0x014Au, 0.008},
        {"KJH6V005", 0x014Au, 0.008},
        {"KLH3G015", 0x014Cu, 0.008},
        {"KLH3G017", 0x014Cu, 0.008},
        {"KLH3G018", 0x014Cu, 0.008},
        {"KLH3G019", 0x014Cu, 0.008},
        {"KLH3G020", 0x014Cu, 0.008},
        {"KLH3V004", 0x014Cu, 0.008},
        {"KLH3V005", 0x014Cu, 0.008},
        {"KLH3V006", 0x014Cu, 0.008},
        {"KLH4G017", 0x014Au, 0.016},
        {"KLH4G018", 0x014Cu, 0.016},
        {"KLH4G019", 0x014Cu, 0.016},
        {"KLH4G020", 0x014Au, 0.016},
        {"KLH4V006", 0x014Au, 0.016},
        {"KLH4V007", 0x014Au, 0.016},
        {"KLH4V008", 0x014Au, 0.016},
        {"KLH4V009", 0x014Au, 0.016},
        {"KLH4V010", 0x014Au, 0.016},
        {"KLHBG000", 0x014Cu, 0.008},
        {"KLHBG001", 0x014Au, 0.008},
        {"KLHBG002", 0x014Cu, 0.008},
        {"KLHBV000", 0x014Au, 0.008},
        {"KLHBV001", 0x014Au, 0.008},
        {"KLHBV002", 0x014Au, 0.008},
        {"KLHCG000", 0x014Cu, 0.008},
        {"KLHCG001", 0x014Cu, 0.008},
        {"KLHCG002", 0x014Cu, 0.008},
        {"KLHCV000", 0x014Cu, 0.008},
        {"KLHCV001", 0x014Cu, 0.008},
        {"KLQ4G000", 0x014Au, 0.016},
        {"KLQ4G001", 0x014Cu, 0.016},
        {"KLQ4G002", 0x014Au, 0.016},
        {"KLQ4G003", 0x014Cu, 0.016},
        {"KLQ4G004", 0x014Au, 0.016},
        {"KLQ4G005", 0x014Cu, 0.016},
        {"KLQ4G006", 0x014Cu, 0.016},
        {"KLQ4G007", 0x014Au, 0.016},
        {"KLQ4V002", 0x014Au, 0.016},
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
