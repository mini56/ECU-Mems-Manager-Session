static quint8 byteAt(const QByteArray &bytes, int index)
{
    return static_cast<quint8>(static_cast<unsigned char>(bytes.at(index)));
}

static bool exactModeReply(const QByteArray &response, quint8 modeValue)
{
    if (response.size() == 2)
        return byteAt(response, 0) == 0xF0 && byteAt(response, 1) == modeValue;
    if (response.size() == 3)
        return byteAt(response, 0) == 0xF0 && byteAt(response, 1) == 0xF0 &&
               byteAt(response, 2) == modeValue;
    return false;
}

static bool decodeKnownModeReply(const QByteArray &response, quint8 *modeValue)
{
    if (!modeValue)
        return false;
    const quint8 modes[] = {0x14, 0x1E, 0x50};
    for (quint8 mode : modes)
    {
        if (exactModeReply(response, mode))
        {
            *modeValue = mode;
            return true;
        }
    }
    return false;
}

static bool exactEchoReply(const QByteArray &response, quint8 value)
{
    if (response.size() == 1)
        return byteAt(response, 0) == value;
    if (response.size() == 2)
        return byteAt(response, 0) == value && byteAt(response, 1) == value;
    return false;
}

static bool parseExactWordReply(const QByteArray &response, quint8 command, quint16 *value)
{
    if (!value)
        return false;

    int payload = -1;
    if (response.size() == 3 && byteAt(response, 0) == command)
        payload = 1;
    else if (response.size() == 4 && byteAt(response, 0) == command &&
             byteAt(response, 1) == command)
        payload = 2;
    else
        return false;

    *value = quint16(byteAt(response, payload)) |
             (quint16(byteAt(response, payload + 1)) << 8);
    return true;
}

static quint16 highWordC001(quint16 value)
{
    const quint32 product = quint32(value) * quint32(0xC001u);
    return quint16((product >> 16) & 0xFFFFu);
}

static bool exchangeByte(mems_info *info, quint8 command, QByteArray &response,
                         int quietLimitMs = 30)
{
    response.clear();
    if (!info || !mems_is_connected(info))
        return false;

#ifdef WIN32
    HANDLE h = info->sd;
    if (h == INVALID_HANDLE_VALUE || h == NULL)
        return false;

    COMMTIMEOUTS oldTimeouts;
    if (!GetCommTimeouts(h, &oldTimeouts))
        return false;

    COMMTIMEOUTS timeouts = oldTimeouts;
    const bool fast = quietLimitMs <= 20;
    timeouts.ReadIntervalTimeout = fast ? 5 : 20;
    timeouts.ReadTotalTimeoutConstant = fast ? 10 : 35;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 100;
    timeouts.WriteTotalTimeoutMultiplier = 5;
    if (!SetCommTimeouts(h, &timeouts))
        return false;

    if (!PurgeComm(h, PURGE_RXCLEAR))
    {
        SetCommTimeouts(h, &oldTimeouts);
        return false;
    }

    const unsigned char tx = command;
    DWORD written = 0;
    if (!WriteFile(h, &tx, 1, &written, NULL) || written != 1)
    {
        SetCommTimeouts(h, &oldTimeouts);
        return false;
    }

    QElapsedTimer overall;
    QElapsedTimer quiet;
    overall.start();
    quiet.start();
    bool gotAny = false;
    while (overall.elapsed() < 900 && (!gotAny || quiet.elapsed() < quietLimitMs))
    {
        unsigned char buffer[64];
        DWORD count = 0;
        if (!ReadFile(h, buffer, sizeof(buffer), &count, NULL))
            break;
        if (count > 0)
        {
            response.append(reinterpret_cast<const char*>(buffer), int(count));
            gotAny = true;
            quiet.restart();
        }
    }

    SetCommTimeouts(h, &oldTimeouts);
    return gotAny;
#else
    const int fd = info->sd;
    if (fd < 0)
        return false;

    const unsigned char tx = command;
    if (write(fd, &tx, 1) != 1)
        return false;

    QElapsedTimer overall;
    QElapsedTimer quiet;
    overall.start();
    quiet.start();
    bool gotAny = false;
    while (overall.elapsed() < 900 && (!gotAny || quiet.elapsed() < quietLimitMs))
    {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(fd, &set);
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = (quietLimitMs <= 20) ? 5000 : 30000;

        const int ready = select(fd + 1, &set, NULL, NULL, &timeout);
        if (ready < 0)
            break;
        if (ready == 0)
            continue;

        unsigned char buffer[64];
        const ssize_t count = read(fd, buffer, sizeof(buffer));
        if (count > 0)
        {
            response.append(reinterpret_cast<const char*>(buffer), int(count));
            gotAny = true;
            quiet.restart();
        }
    }
    return gotAny;
#endif
}

static bool readMode4Word(mems_info *info, quint8 command, quint16 *value)
{
    if (!info || !value || !mems_is_connected(info))
        return false;

    QByteArray response;
#ifdef WIN32
    HANDLE h = info->sd;
    if (h == INVALID_HANDLE_VALUE || h == NULL)
        return false;

    COMMTIMEOUTS oldTimeouts;
    if (!GetCommTimeouts(h, &oldTimeouts))
        return false;

    COMMTIMEOUTS timeouts = oldTimeouts;
    timeouts.ReadIntervalTimeout = 1;
    timeouts.ReadTotalTimeoutConstant = 12;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 60;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    if (!SetCommTimeouts(h, &timeouts))
        return false;

    if (!PurgeComm(h, PURGE_RXCLEAR))
    {
        SetCommTimeouts(h, &oldTimeouts);
        return false;
    }

    const unsigned char tx = command;
    DWORD written = 0;
    if (!WriteFile(h, &tx, 1, &written, NULL) || written != 1)
    {
        SetCommTimeouts(h, &oldTimeouts);
        return false;
    }

    const int expected = 3;
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 120 && response.size() < 4)
    {
        unsigned char rx = 0;
        DWORD count = 0;
        if (!ReadFile(h, &rx, 1, &count, NULL))
            break;
        if (count == 0)
            continue;

        response.append(char(rx));
        if (parseExactWordReply(response, command, value))
        {
            SetCommTimeouts(h, &oldTimeouts);
            return true;
        }
        if (response.size() >= expected &&
            !(response.size() == 3 && byteAt(response, 0) == command &&
              byteAt(response, 1) == command))
            break;
    }

    SetCommTimeouts(h, &oldTimeouts);
    return false;
#else
    const int fd = info->sd;
    if (fd < 0)
        return false;

    const unsigned char tx = command;
    if (write(fd, &tx, 1) != 1)
        return false;

    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 120 && response.size() < 4)
    {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(fd, &set);
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 12000;

        const int ready = select(fd + 1, &set, NULL, NULL, &timeout);
        if (ready <= 0)
            continue;

        unsigned char rx = 0;
        if (read(fd, &rx, 1) != 1)
            continue;

        response.append(char(rx));
        if (parseExactWordReply(response, command, value))
            return true;
        if (response.size() >= 3 &&
            !(response.size() == 3 && byteAt(response, 0) == command &&
              byteAt(response, 1) == command))
            break;
    }
    return false;
#endif
}

static bool selectMode4Block(mems_info *info, quint8 block)
{
    QByteArray response;
    if (!exchangeByte(info, 0xDC, response, 20) || !exactEchoReply(response, 0xDC))
        return false;
    if (!exchangeByte(info, block, response, 20) || !exactEchoReply(response, block))
        return false;
    return true;
}

static bool readMode(mems_info *info, quint8 expectedMode)
{
    QByteArray response;
    return exchangeByte(info, 0xF0, response, 30) && exactModeReply(response, expectedMode);
}

static bool restoreNormalSession(mems_info *info)
{
    QByteArray response;
    exchangeByte(info, 0xF5, response, 30);
    if (!readMode(info, 0x14))
        return false;

    exchangeByte(info, 0xF4, response, 30);
    return readMode(info, 0x50);
}

static bool ensureNormalSession(mems_info *info)
{
    QByteArray response;
    if (!exchangeByte(info, 0xF0, response, 30))
        return false;

    quint8 mode = 0;
    if (!decodeKnownModeReply(response, &mode))
        return false;
    if (mode == 0x50)
        return true;
    return restoreNormalSession(info);
}

static bool enterMode4(mems_info *info)
{
    QByteArray response;
    exchangeByte(info, 0xF5, response, 30);
    if (!readMode(info, 0x14))
        return false;
    if (!exchangeByte(info, 0xC4, response, 30))
        return false;
    return readMode(info, 0x1E);
}

static bool readMappedWordInBlock(mems_info *info, quint16 address, quint8 selectedBlock,
                                  quint16 *value)
{
    if (!value || (address & 0x0001u) != 0 || quint8(address >> 8) != selectedBlock)
        return false;
    const quint8 command = quint8((address & 0x00FFu) >> 1);
    return readMode4Word(info, command, value);
}

static bool readMappedByteInBlock(mems_info *info, quint16 address, quint8 selectedBlock,
                                  quint8 *value)
{
    if (!value || quint8(address >> 8) != selectedBlock)
        return false;

    const quint16 alignedAddress = quint16(address & 0xFFFEu);
    quint16 word = 0;
    if (!readMappedWordInBlock(info, alignedAddress, selectedBlock, &word))
        return false;

    *value = (address & 0x0001u)
        ? quint8((word >> 8) & 0x00FFu)
        : quint8(word & 0x00FFu);
    return true;
}
