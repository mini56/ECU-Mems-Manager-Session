// Rover MEMS 1.6 Turbo injection profiles.
//
// TCF7X005, TCF8X005, TFF8X000 and TFFSX000 use a different fuel scheduler
// from the naturally aspirated MEMS 1.6 families. Their final calculated
// injector pulse width is held directly at RAM 0x0630. The firmware copies
// that word into the per-channel injector width state; no separate asynchronous
// pulse-width/counter path equivalent to the NA t_asyn_pw mechanism is present.
//
// Scale validation: all four firmwares compare the 0x0630 value against the
// result of 0x01C9C380 (30,000,000) / engine RPM. That quantity is the available
// half-revolution time in microseconds, so 0x0630 is expressed at 1 us/raw,
// i.e. 0.001 ms/raw.
static bool validatedInjectionProfileForFirmware(const QString &firmware,
                                                 InjectionCalculationProfile *profile)
{
    if (!profile)
        return false;

    if (validatedInjectionProfileForFirmwareNa(firmware, profile))
        return true;

    const QString id = firmware.trimmed().toUpper();
    if (id == QStringLiteral("TCF7X005") ||
        id == QStringLiteral("TCF8X005") ||
        id == QStringLiteral("TFF8X000") ||
        id == QStringLiteral("TFFSX000"))
    {
        // A zero transient source/counter marks a direct-final profile. The
        // Mode-4 reader therefore reads only 0x0630 and does not manufacture a
        // transient event or counter for these sequential-injection firmwares.
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x0630u, 0x0000u, 0x0000u},
            InjectionCalculationKind::IndependentAsync,
            InjectionCounterPolicy::NonZeroActive,
            0.001,
            0.0);
        return true;
    }

    return false;
}
