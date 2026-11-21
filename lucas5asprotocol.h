#ifndef LUCAS5ASPROTOCOL_H
#define LUCAS5ASPROTOCOL_H

// Lucas 5AS normal diagnostic mode: read-only protocol preparation.
// Source: James Portman, rover-mems-documentation,
// Immobiliser/5AS/diagnostics and configuring.md (SHA 79290af8887cdc1f069d4d86e6f613ccc9e4a978).
// This header has NO serial I/O and CANNOT write EEPROM, learn fobs,
// change alarm configuration, or program an immobiliser.
#include <QByteArray>
#include <QString>
#include <QVector>
#include <QtGlobal>

namespace Lucas5AsProtocol {

constexpr quint8 kNormalWakeAddress = 0x1Cu;
constexpr int kSlowBitCount = 10;
constexpr int kSlowBitDurationMs = 200;
constexpr int kNormalBaud = 9600;
constexpr quint8 kReadStatusCommand = 0x83u;
constexpr quint8 kEndSessionCommand = 0xF6u;
constexpr int kMaxDocumentedPacketBytes = 127;

// Ten ISO-style asynchronous bits, start=0, LSB-first data, stop=1.
// The actual USB KKL BREAK timing and 5AS handshake remain subject to
// hardware testing; these helpers do not claim a successful connection.
inline bool slowBitHigh(int index)
{
    if(index == 0) return false;
    if(index == kSlowBitCount - 1) return true;
    if(index < 1 || index > 8) return false;
    return ((kNormalWakeAddress >> (index - 1)) & 1u) != 0u;
}
inline QByteArray normalWakeReply()
{
    return QByteArray::fromHex("558376");
}
inline bool hasNormalWakeReply(const QByteArray &data)
{
    return data.indexOf(normalWakeReply()) >= 0;
}

// Source documents 0x55 0x83 0x76, but does NOT document the subsequent
// challenge/complement timing or mandatory confirmation for every 5AS.
// Do not infer transport success solely from this three-byte prefix.

struct SwitchField {
    bool available = false;
    bool bit7High = false;
};

struct StatusPacket {
    QByteArray raw;
    SwitchField bonnet;
    SwitchField ignition;
    SwitchField doors;
    SwitchField possibleBoot;  // Portman explicitly marks boot(?) uncertain.
    QVector<quint8> fobCountersRaw; // Momentary values, not lifetime counts.
    QByteArray serialLittleEndian;  // Only when bytes 20-22 are available.
    QString unitDescription;        // ASCII from 35 onward when available.
};

inline quint8 octet(const QByteArray &p, int offset)
{
    return static_cast<quint8>(static_cast<unsigned char>(p.at(offset)));
}
inline SwitchField switchAt(const QByteArray &p, int offset)
{
    SwitchField field;
    if(offset >= 0 && offset < p.size()){
        field.available = true;
        field.bit7High = (octet(p,offset)&0x80u) != 0u;
    }
    return field;
}

// The first response byte is the length INCLUDING that length byte.
// No general checksum has been established for this packet in the source.
// Preserve the exact raw bytes; never invent a checksum or field meaning.
inline bool decodeStatus(const QByteArray &packet,StatusPacket *out)
{
    if(!out || packet.size() < 2 || packet.size() > kMaxDocumentedPacketBytes)
        return false;
    if(octet(packet,0) != packet.size())
        return false;
    StatusPacket decoded;
    decoded.raw=packet;
    decoded.bonnet=switchAt(packet,1);
    decoded.ignition=switchAt(packet,6);
    decoded.doors=switchAt(packet,9);
    decoded.possibleBoot=switchAt(packet,11);
    if(packet.size() > 19){
        for(int i=16;i<=19;++i)
            decoded.fobCountersRaw.append(octet(packet,i));
    }
    if(packet.size() > 22)
        decoded.serialLittleEndian=packet.mid(20,3);
    if(packet.size() > 35){
        QByteArray description;
        // The only fully documented example contains 14 ASCII bytes at
        // offsets 35..48. Do not consume unknown trailing payload/check bytes.
        const int end = qMin(packet.size(),49);
        for(int i=35;i<end;++i){
            const quint8 v=octet(packet,i);
            if(v<0x20u || v>0x7Eu)break;
            description.append(char(v));
        }
        decoded.unitDescription=QString::fromLatin1(description);
    }
    *out=decoded;
    return true;
}

// Recover one complete length-framed reply after a limited, explicitly
// recognised KKL command echo (0x83) or a zero lead-in. Do NOT scan arbitrary
// payload bytes looking for a small fake "length": a truncated real packet
// may contain such bytes at indices 16..19 (PLIP counters).
inline bool extractStatus(const QByteArray &received,StatusPacket *out,
                          int *frameOffset=nullptr)
{
    if(!out)return false;
    int offset=0;
    while(offset<received.size() &&
          (octet(received,offset)==0x00u ||
           octet(received,offset)==kReadStatusCommand))
        ++offset;
    if(offset>=received.size())return false;
    const int n=octet(received,offset);
    if(n<2 || n>kMaxDocumentedPacketBytes ||
       received.size()-offset<n)
        return false;
    StatusPacket candidate;
    if(!decodeStatus(received.mid(offset,n),&candidate))
        return false;
    if(frameOffset)*frameOffset=offset;
    *out=candidate;
    return true;
}

} // namespace Lucas5AsProtocol
#endif // LUCAS5ASPROTOCOL_H
