#ifndef MEMS19PROTOCOL_H
#define MEMS19PROTOCOL_H

#include <QByteArray>
#include <QtGlobal>

// Pure MEMS 1.9 ISO 9141 wake-up helpers. No serial I/O, ECU lookup, or UI.
// Can be tested against synthetic replies without a physical calculator.
namespace Mems19Protocol
{
constexpr quint8 kAddress = 0x16u;
constexpr int kSlowBitCount = 10;
constexpr int kSlowBitDurationMs = 200;

// Start bit (0), eight LSB-first data bits, and stop bit (1).
inline bool slowBitHigh(int index)
{
    if (index == 0)
        return false;
    if (index == kSlowBitCount - 1)
        return true;
    if (index < 1 || index >= kSlowBitCount - 1)
        return false;
    return ((kAddress >> (index - 1)) & 1u) != 0u;
}

// Accept arbitrary ISO 9141 key bytes after 0x55; do not hardcode 0x76 0x83.
// An incomplete stream is not a successful handshake.
inline bool keyComplement(const QByteArray &stream, quint8 *complement)
{
    if (!complement)
        return false;
    for (int i = 0; i + 2 < stream.size(); ++i)
    {
        if (static_cast<quint8>(stream.at(i)) != 0x55u)
            continue;
        *complement = static_cast<quint8>(~static_cast<quint8>(stream.at(i + 2)));
        return true;
    }
    return false;
}

/* The K-Line cable can echo the transmitted command in addition to the ECU
 * response. Strip only this optional cable-local echo; never strip payload. */
inline QByteArray normalizedReply(const QByteArray &received, quint8 command,
                                  bool cableEcho)
{
    if (cableEcho && received.size() > 1 &&
        static_cast<quint8>(received.at(0)) == command &&
        static_cast<quint8>(received.at(1)) == command)
        return received.mid(1);
    return received;
}

inline bool hasAddressConfirmation(const QByteArray &stream)
{
    const quint8 expected = static_cast<quint8>(~kAddress); // 0xE9
    for (const char byte : stream)
    {
        if (static_cast<quint8>(byte) == expected)
            return true;
    }
    return false;
}
}

#endif // MEMS19PROTOCOL_H
