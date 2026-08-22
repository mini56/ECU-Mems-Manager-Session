// Rover MEMS 1.9 correlated batch 6: firmware 126-150 (TBP7F015 deferred).
// Same validated normal x8 scheduler as the earlier MEMS 1.9 families.
// 0x004E is accumulated eight times; some IDA listings expose it as 0x004C+2.
// The 0x8000-biased compensation address is verified per firmware from the
// correlated disassembly. 1 scheduler timer unit = 1 us.
static bool validatedInjectionProfileForFirmware(const QString &firmware,
                                                 InjectionCalculationProfile *profile)
{
    if (!profile) return false;
    if (validatedInjectionProfileForFirmwareMems19B5(firmware, profile)) return true;

    struct Entry { const char *id; quint16 compensation; double baseScale; };
    static const Entry entries[] = {
        {"TAHHG013", 0x014Cu, 0.008},
        {"TAHHG019", 0x014Cu, 0.008},
        {"TAHHV003", 0x014Cu, 0.008},
        {"TAHHV004", 0x014Cu, 0.008},
        {"TAHHV005", 0x014Cu, 0.008},
        {"TAHHV006", 0x014Cu, 0.008},
        {"TBH7G016", 0x014Cu, 0.008},
        {"TBH7G017", 0x014Cu, 0.008},
        {"TBH7G019", 0x014Cu, 0.008},
        {"TBH7V000", 0x0148u, 0.008},
        {"TBH7V002", 0x014Cu, 0.008},
        {"TBH7V003", 0x014Cu, 0.008},
        {"TBH7V004", 0x014Cu, 0.008},
        {"TBH7V005", 0x014Cu, 0.008},
        {"TBH7V006", 0x014Cu, 0.008},
        {"TBH7V007", 0x014Cu, 0.008},
        {"TFH7G025", 0x014Cu, 0.008},
        {"TFH7G026", 0x014Cu, 0.008},
        {"TFH7G027", 0x014Cu, 0.008},
        {"TFH7G028", 0x014Cu, 0.008},
        {"TFH7G029", 0x014Cu, 0.008},
        {"TFH7G031", 0x014Cu, 0.008},
        {"TFH7V000", 0x0148u, 0.008},
        {"TFH7V004", 0x014Cu, 0.008},
        {"TFH7V005", 0x014Cu, 0.008},
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
