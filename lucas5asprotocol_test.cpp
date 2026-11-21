#include "lucas5asprotocol.h"
#include <QByteArray>
#include <QDebug>
#include <QString>
#include <cstdlib>

static void require(bool ok,const char *description)
{
    if(!ok){
        qCritical().noquote()<<"LUCAS5AS_PROTOCOL_SELFTEST_FAIL"<<description;
        std::exit(1);
    }
}

int main()
{
    using namespace Lucas5AsProtocol;
    require(kNormalWakeAddress==0x1Cu &&
            kNormalBaud==9600 &&
            kReadStatusCommand==0x83u,
            "Lucas 5AS normal-mode constants");
    const bool slowBits[10]={
        false,false,false,true,true,true,false,false,false,true
    };
    require(kSlowBitCount==10 && kSlowBitDurationMs==200,
            "Ten slow-init bits with five-baud 200 ms periods");
    for(int i=0;i<kSlowBitCount;++i)
        require(slowBitHigh(i)==slowBits[i],
                "5AS address 0x1C must be sent LSB-first with start/stop");

    require(normalWakeReply()==QByteArray::fromHex("558376"),
            "Portman normal 5AS wake response");
    require(!hasNormalWakeReply(QByteArray::fromHex("5583")),
            "Fragmented wake must not pass");
    require(!hasNormalWakeReply(QByteArray::fromHex("5583F7")),
            "EEPROM-mode wake must NOT masquerade as normal mode");
    require(hasNormalWakeReply(QByteArray::fromHex("00558376")),
            "Normal wake must survive a leading cable zero");
    require(hasNormalWakeReply(QByteArray::fromHex("001C558376")),
            "Normal wake must survive local 0x1C echo");

    // Synthetic example follows exactly the indices published by Portman.
    // These bytes are NOT a captured response from a vehicle.
    QByteArray raw(49,char(0));
    raw[0]=char(49); // Complete packet length including index zero.
    raw[1]=char(0x80);  // bonnet bit 7
    raw[6]=char(0x80);  // ignition bit 7
    raw[9]=char(0);     // doors bit 7
    raw[11]=char(0x80); // boot(?) remains an uncertain field
    raw[16]=char(1);
    raw[17]=char(3);
    raw[18]=char(5);
    raw[19]=char(7);
    raw[20]=char(0x12);
    raw[21]=char(0x34);
    raw[22]=char(0x56);
    const QByteArray exampleDescription("AP3.005AS-98MY");
    require(exampleDescription.size()==14,"documented ASCII length");
    raw.replace(35,exampleDescription.size(),exampleDescription);

    StatusPacket decoded;
    require(decodeStatus(raw,&decoded),
            "Valid synthetic length-framed status must decode");
    require(decoded.raw==raw,
            "Preserve all 49 raw bytes, including undocumented fields");
    require(decoded.bonnet.available && decoded.bonnet.bit7High,
            "Bonnet switch value at index 1");
    require(decoded.ignition.available && decoded.ignition.bit7High,
            "Ignition switch value at index 6");
    require(decoded.doors.available && !decoded.doors.bit7High,
            "Doors switch value at index 9");
    require(decoded.possibleBoot.available && decoded.possibleBoot.bit7High,
            "Possible boot value at index 11 must keep uncertainty");
    require(decoded.fobCountersRaw.size()==4 &&
            decoded.fobCountersRaw[0]==1 &&
            decoded.fobCountersRaw[3]==7,
            "Four momentary PLIP raw counters at 16..19");
    require(decoded.serialLittleEndian==QByteArray::fromHex("123456"),
            "Serial number bytes preserved in source order");
    require(decoded.unitDescription==QStringLiteral("AP3.005AS-98MY"),
            "14 ASCII identification characters at 35..48");

    QByteArray shortened=raw.left(19);
    shortened[0]=char(shortened.size());
    require(decodeStatus(shortened,&decoded),
            "Source fields must be processed only when present");
    require(decoded.ignition.available && decoded.doors.available,
            "Early switch fields survive shorter frame");
    require(decoded.serialLittleEndian.isEmpty() &&
            decoded.fobCountersRaw.isEmpty() &&
            decoded.unitDescription.isEmpty(),
            "Missing serial, fob and description fields must not be invented");
    require(!decodeStatus(raw.left(18),&decoded),
            "Never decode a truncated packet with mismatching length");
    require(!decodeStatus(QByteArray(),&decoded),
            "Do not accept empty status");
    require(!decodeStatus(raw,nullptr),
            "Do not decode to null output");
    QByteArray bad=raw;
    bad[0]=char(48);
    require(!decodeStatus(bad,&decoded),
            "Declaring the wrong length invalidates a frame");
    require(!decodeStatus(QByteArray(128,char(0)),&decoded),
            "Reject oversized frames");

    const QByteArray echoAndStatus=QByteArray::fromHex("0083")+raw+
                                    QByteArray::fromHex("F6");
    int offset=-1;
    require(extractStatus(echoAndStatus,&decoded,&offset) &&
            offset==2 && decoded.raw==raw,
            "Find complete status after possible 0x83 local echo");
    require(!extractStatus(QByteArray::fromHex("83")+raw.left(20),&decoded),
            "Incomplete received status must never pass");
    require(!extractStatus(QByteArray::fromHex("558376"),&decoded),
            "Handshake alone must never be decoded as a status frame");

    qInfo().noquote()
       <<"LUCAS5AS_PROTOCOL_SELFTEST_PASS 0x1C wake, normal-mode filtering,"
       <<" 0x83 length frame, partial/echo, raw 5AS data and unknown fields";
    return 0;
}
