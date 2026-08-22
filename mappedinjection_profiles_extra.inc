// Additional MEMS 1.6 NA profiles recovered directly from Andrew Revill ASM
// files outside the original 82-entry correlated RAM-map class. This includes
// both correlated outliers and the ten NA/Uncorrelated firmware disassemblies.
//
// IndependentAsync profiles below use the execution path anchored by
// ACNMP001/AANMP000: the synchronous pulse snapshot is kept separately from
// the asynchronous width, whose fire counter increments for each async event.
// AdditiveC001 profiles are enabled only where the ASM shows the mapped source
// passing through mulu #-3FFFh (0xC001) and its high word being added to the
// active injector timer accumulator.
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

    // Rover MEMS 1.6 NA / Uncorrelated.
    // EKDMF000 uses the same independent asynchronous-fire structure as the
    // correlated KBP8P000/KBP8P002 family. The final synchronous snapshot is
    // stored at 0x0308; the async word is 0x0266 and fire counter is 0x00EE.
    if (id == QStringLiteral("EKDMF000"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x0308u, 0x0266u, 0x00EEu},
            InjectionCalculationKind::IndependentAsync,
            InjectionCounterPolicy::IncrementEvent,
            0.001333,
            0.001);
        return true;
    }

    // EKDMF002/EKDMF003 retain the EKDMF000 async path but their synchronous
    // snapshot is explicitly stored by the firmware at external RAM 0x1208.
    if (id == QStringLiteral("EKDMF002") ||
        id == QStringLiteral("EKDMF003"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x1208u, 0x0266u, 0x00EEu},
            InjectionCalculationKind::IndependentAsync,
            InjectionCounterPolicy::IncrementEvent,
            0.001333,
            0.001);
        return true;
    }

    // EKDMF004 is structurally closest to KBP8P000: independent async firing,
    // with synchronous snapshot 0x03CA, async width 0x026C and counter 0x00EF.
    if (id == QStringLiteral("EKDMF004"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x03CAu, 0x026Cu, 0x00EFu},
            InjectionCalculationKind::IndependentAsync,
            InjectionCounterPolicy::IncrementEvent,
            0.001333,
            0.001);
        return true;
    }

    // EKDMF005/EKDMF006 match the KAE6P000 independent-async structure with
    // the same 0x03C8 snapshot and 0x00EB event counter, source relocated to
    // 0x026A in both uncorrelated firmwares.
    if (id == QStringLiteral("EKDMF005") ||
        id == QStringLiteral("EKDMF006"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x03C8u, 0x026Au, 0x00EBu},
            InjectionCalculationKind::IndependentAsync,
            InjectionCounterPolicy::IncrementEvent,
            0.001333,
            0.001);
        return true;
    }

    // EKDMF007 is a direct structural match for the validated KBP8P005
    // additive path, including source 0x026E and state/counter 0x027E; only
    // the synchronous snapshot is relocated to 0x03CC in this firmware.
    if (id == QStringLiteral("EKDMF007"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x03CCu, 0x026Eu, 0x027Eu},
            InjectionCalculationKind::AdditiveC001,
            InjectionCounterPolicy::NonZeroActive,
            0.002,
            0.002);
        return true;
    }

    // EKDMF008 uses a dual injector-timer scheduler. The common synchronous
    // pulse width is calculated through C001 and stored at 0x0140 before being
    // expanded into the two channel widths at 0x0146/0x0148. Its transient
    // correction is stored at 0x0262, passed through the C001 high-word helper
    // and added to both injector timer accumulators; 0x0287 is incremented on
    // application and cleared when the transient path is inactive.
    if (id == QStringLiteral("EKDMF008"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x0140u, 0x0262u, 0x0287u},
            InjectionCalculationKind::AdditiveC001,
            InjectionCounterPolicy::NonZeroActive,
            0.002,
            0.002);
        return true;
    }

    // EKDMF009 has the exact 0x03C8 / 0x0270 / 0x0283 variable layout and the
    // same independent-async fire path as the validated ABNMF032 family.
    if (id == QStringLiteral("EKDMF009"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x03C8u, 0x0270u, 0x0283u},
            InjectionCalculationKind::IndependentAsync,
            InjectionCounterPolicy::IncrementEvent,
            0.001333,
            0.001);
        return true;
    }

    // OCLIP000 follows the validated KBM6P005/KBP8P007 additive scheduler,
    // with source 0x0262 and C001 high-word addition. Its state/counter is
    // 0x0265 and the synchronous snapshot is at 0x03C8.
    if (id == QStringLiteral("OCLIP000"))
    {
        *profile = InjectionCalculationProfile(
            InjectionRamMap{0x03C8u, 0x0262u, 0x0265u},
            InjectionCalculationKind::AdditiveC001,
            InjectionCounterPolicy::NonZeroActive,
            0.002,
            0.002);
        return true;
    }

    return false;
}
