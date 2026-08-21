struct InjectionRamMap
{
    quint16 baseAddress;
    quint16 transientSourceAddress;
    quint16 transientCounterAddress;
};

static bool injectionRamMapForFirmware(const QString &firmware, InjectionRamMap *map)
{
    if (!map)
        return false;

    // Correlations extracted from Andrew Revill MEMSTools / Rover MEMS 1.6 NA.
    // Only firmware for which base duration, transient source and transient
    // counter have all been correlated are enabled here.
    static const QHash<QString, InjectionRamMap> maps = {
        {QStringLiteral("AANMP002"), {0x03C8u, 0x026Eu, 0x0280u}},
        {QStringLiteral("ABNMF032"), {0x03C8u, 0x0270u, 0x0283u}},
        {QStringLiteral("ABNMP003"), {0x03C8u, 0x026Eu, 0x0280u}},
        {QStringLiteral("ABNMP005"), {0x03C8u, 0x0270u, 0x0283u}},
        {QStringLiteral("ABNMP006"), {0x03C8u, 0x0270u, 0x0283u}},
        {QStringLiteral("ACNMF024"), {0x03C8u, 0x026Eu, 0x0282u}},
        {QStringLiteral("ACNMF026"), {0x03C8u, 0x0270u, 0x0285u}},
        {QStringLiteral("ACNMP003"), {0x03C8u, 0x0270u, 0x0285u}},
        {QStringLiteral("ACNMP004"), {0x03C8u, 0x0270u, 0x0285u}},
        {QStringLiteral("ACNMP005"), {0x03C8u, 0x0270u, 0x0285u}},
        {QStringLiteral("KBACF000"), {0x03CCu, 0x026Eu, 0x027Eu}},
        {QStringLiteral("KBACF001"), {0x03CCu, 0x026Eu, 0x0280u}},
        {QStringLiteral("KBACP000"), {0x03CCu, 0x026Eu, 0x027Eu}},
        {QStringLiteral("KBACP001"), {0x03CCu, 0x026Eu, 0x027Eu}},
        {QStringLiteral("KBM6P005"), {0x03CCu, 0x0262u, 0x0268u}},
        {QStringLiteral("KBM6P006"), {0x03CCu, 0x0262u, 0x0268u}},
        {QStringLiteral("KBP8F053"), {0x03CCu, 0x0262u, 0x0269u}},
        {QStringLiteral("KBP8F062"), {0x03CCu, 0x0266u, 0x026Cu}},
        {QStringLiteral("KBP8F063"), {0x03CCu, 0x026Eu, 0x0281u}},
        {QStringLiteral("KBP8F064"), {0x03CCu, 0x0266u, 0x026Cu}},
        {QStringLiteral("KBP8F065"), {0x03CCu, 0x026Eu, 0x0281u}},
        {QStringLiteral("KBP8F066"), {0x03CCu, 0x026Eu, 0x0281u}},
        {QStringLiteral("KBP8P005"), {0x03CCu, 0x026Eu, 0x027Eu}},
        {QStringLiteral("KBP8P007"), {0x03CCu, 0x0262u, 0x0268u}},
        {QStringLiteral("KBP8P008"), {0x03CCu, 0x0262u, 0x0269u}},
        {QStringLiteral("KBP8P009"), {0x03CCu, 0x0262u, 0x0268u}},
        {QStringLiteral("KBP8P010"), {0x03CCu, 0x0262u, 0x0269u}},
        {QStringLiteral("KBP8P011"), {0x03CCu, 0x0262u, 0x0268u}},
        {QStringLiteral("KBP8P012"), {0x03CCu, 0x0262u, 0x0268u}},
        {QStringLiteral("KBP8P013"), {0x03CCu, 0x026Eu, 0x027Fu}},
        {QStringLiteral("KBP8P014"), {0x03CCu, 0x0262u, 0x0269u}},
        {QStringLiteral("KBP8P015"), {0x03CCu, 0x0262u, 0x0269u}},
        {QStringLiteral("KBP8P016"), {0x03CCu, 0x0266u, 0x026Cu}},
        {QStringLiteral("KBP8P017"), {0x03CCu, 0x026Eu, 0x0281u}},
        {QStringLiteral("KCE6F014"), {0x03C8u, 0x026Eu, 0x0281u}},
        {QStringLiteral("KCE6F018"), {0x03C8u, 0x026Eu, 0x0281u}},
        {QStringLiteral("KCE6P003"), {0x03C8u, 0x026Eu, 0x027Eu}},
        {QStringLiteral("KCE6P004"), {0x03C8u, 0x026Eu, 0x0281u}},
        {QStringLiteral("KCE8F000"), {0x03C8u, 0x026Eu, 0x027Fu}},
        {QStringLiteral("KCE8F005"), {0x03C8u, 0x026Eu, 0x027Fu}},
        {QStringLiteral("KCE8F006"), {0x03C8u, 0x026Eu, 0x027Fu}},
        {QStringLiteral("KCE8F007"), {0x03C8u, 0x026Eu, 0x0282u}},
        {QStringLiteral("KCE8F008"), {0x03C8u, 0x026Eu, 0x0282u}},
        {QStringLiteral("KCE8F009"), {0x03C8u, 0x026Eu, 0x0282u}},
        {QStringLiteral("KCE8P000"), {0x03C8u, 0x026Eu, 0x027Eu}},
        {QStringLiteral("KCE8P001"), {0x03C8u, 0x026Eu, 0x027Fu}},
        {QStringLiteral("KCE8P003"), {0x03C8u, 0x026Eu, 0x027Fu}},
        {QStringLiteral("KCE8P004"), {0x03C8u, 0x026Eu, 0x0282u}},
        {QStringLiteral("KDE6F015"), {0x03D8u, 0x0272u, 0x0284u}},
        {QStringLiteral("KDE6F022"), {0x03D8u, 0x0274u, 0x0289u}},
        {QStringLiteral("KDE6P004"), {0x03C8u, 0x026Eu, 0x027Eu}},
        {QStringLiteral("KDE6P005"), {0x03C8u, 0x026Eu, 0x0283u}},
        {QStringLiteral("KGM6F001"), {0x03C8u, 0x026Eu, 0x0281u}},
        {QStringLiteral("KGM6F004"), {0x03C8u, 0x0272u, 0x0286u}},
        {QStringLiteral("KGM6F007"), {0x03C8u, 0x0272u, 0x0287u}},
        {QStringLiteral("KGM6F008"), {0x03C8u, 0x0272u, 0x0287u}},
        {QStringLiteral("KGM6P002"), {0x03C8u, 0x026Eu, 0x0281u}},
        {QStringLiteral("KGM6P003"), {0x03C8u, 0x0272u, 0x0286u}},
        {QStringLiteral("KGM6P004"), {0x03C8u, 0x0272u, 0x0287u}},
        {QStringLiteral("KGM6P005"), {0x03C8u, 0x0272u, 0x0287u}},
        {QStringLiteral("TAEDP001"), {0x03CCu, 0x026Eu, 0x027Fu}},
        {QStringLiteral("TAP7F017"), {0x03C8u, 0x026Eu, 0x027Fu}},
        {QStringLiteral("TAP7P005"), {0x03C8u, 0x026Eu, 0x027Fu}},
        {QStringLiteral("TAP7P006"), {0x03C8u, 0x026Eu, 0x027Fu}},
        {QStringLiteral("TAP8F019"), {0x03C8u, 0x026Eu, 0x027Eu}},
        {QStringLiteral("TAP8F022"), {0x03C8u, 0x026Eu, 0x027Eu}},
        {QStringLiteral("TAP8F023"), {0x03C8u, 0x026Eu, 0x027Eu}},
        {QStringLiteral("TAP8F027"), {0x03C8u, 0x026Eu, 0x027Eu}},
        {QStringLiteral("TAP8P006"), {0x03C8u, 0x026Eu, 0x027Fu}},
        {QStringLiteral("TAP8P007"), {0x03C8u, 0x026Eu, 0x027Fu}},
        {QStringLiteral("TAPDF008"), {0x03CCu, 0x026Eu, 0x027Fu}},
        {QStringLiteral("TAPDF010"), {0x03CCu, 0x0270u, 0x0283u}},
        {QStringLiteral("TAPDF011"), {0x03CCu, 0x0270u, 0x0283u}},
        {QStringLiteral("TAPDF012"), {0x03CCu, 0x0270u, 0x0283u}},
        {QStringLiteral("TAPDF013"), {0x03CCu, 0x0270u, 0x0283u}},
        {QStringLiteral("TAPDP000"), {0x03CCu, 0x026Eu, 0x027Fu}},
        {QStringLiteral("TAPDP001"), {0x03CCu, 0x026Eu, 0x027Fu}},
        {QStringLiteral("TAPDP002"), {0x03CCu, 0x026Eu, 0x027Fu}},
        {QStringLiteral("TAPDP003"), {0x03CCu, 0x0270u, 0x0283u}},
        {QStringLiteral("TBP7F016"), {0x03C8u, 0x026Eu, 0x0281u}},
        {QStringLiteral("TBP7P005"), {0x03C8u, 0x026Eu, 0x0281u}},
        {QStringLiteral("TBP7P006"), {0x03C8u, 0x026Eu, 0x0281u}},
    };

    const auto it = maps.constFind(firmware.trimmed().toUpper());
    if (it == maps.constEnd())
        return false;
    *map = it.value();
    return true;
}
