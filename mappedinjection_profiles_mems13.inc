// Rover MEMS 1.3 injection profiles recovered from Andrew Revill ASM/ROM.
//
// The MEMS 1.3 output scheduler converts the value passed to injector service
// 5 with value * 0x2710 / 0x3415.  This establishes the scheduler input as
// microseconds, i.e. 0.001 ms/raw for the pulse-width values below.
//
// ABEMR002 passes INTMEM 0x00C8 directly to injector service 5.
// KBE6R003 and MGE7R002 use INTMEM 0x00C6 as the normal pulse width.  On the
// specific scheduler state 0x0078 == 0x8000 they apply a biased signed
// correction immediately before firing: KBE6R003 from 0xC100, MGE7R002 from
// 0xC0E2.  The Mode-4 runtime reproduces that exact 16-bit operation only for
// those two firmware IDs.
static bool validatedInjectionProfileForFirmware(const QString &firmware,
                                                 InjectionCalculationProfile *profile)
{
    if (!profile)
        return false;

    if (validatedInjectionProfileForFirmware16(firmware, profile))
        return true;

    const QString id = firmware.trimmed().toUpper();

    if (id == QStringLiteral("ABEMR002"))
    {
        // Direct final pulse-width snapshot, 1 us/raw.
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x00C8u, 0x0000u, 0x0000u},
            InjectionCalculationKind::IndependentAsync,
            InjectionCounterPolicy::NonZeroActive,
            0.001,
            0.0);
        return true;
    }

    if (id == QStringLiteral("KBE6R003"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x00C6u, 0xC100u, 0x0078u},
            InjectionCalculationKind::IndependentAsync,
            InjectionCounterPolicy::NonZeroActive,
            0.001,
            0.001);
        return true;
    }

    if (id == QStringLiteral("MGE7R002"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x00C6u, 0xC0E2u, 0x0078u},
            InjectionCalculationKind::IndependentAsync,
            InjectionCounterPolicy::NonZeroActive,
            0.001,
            0.001);
        return true;
    }

    return false;
}
