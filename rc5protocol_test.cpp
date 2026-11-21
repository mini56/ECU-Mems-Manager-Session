#include "rc5protocol.h"
#include <QCoreApplication>
#include <QDebug>
#include <QSet>

int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv);
    bool ok=true;
    const auto check=[&ok](bool condition,const char *name){
        if(!condition){
            qCritical().noquote()<<"RC5_PROTOCOL_SELFTEST_FAIL"<<name;
            ok=false;
        }
    };

    check(Rc5Protocol::request(0x00)==QByteArray::fromHex("82007d"),
          "Ping command must match RoverMEMS");
    check(Rc5Protocol::request(0x33)==QByteArray::fromHex("82334a"),
          "Fault read command must match RoverMEMS");
    check(Rc5Protocol::request(0xc3)==QByteArray::fromHex("82c3ba"),
          "Clear must be available only as user request");
    for(const QByteArray &packet:{
            QByteArray::fromHex("82007d"),
            QByteArray::fromHex("c2003d"),
            QByteArray::fromHex("82334a"),
            QByteArray::fromHex("82c3ba"),
            QByteArray::fromHex("c2c37a"),
            QByteArray::fromHex("c2330a")
        }){
        check(Rc5Protocol::validPacket(packet),"Valid RC5 checksummed packet");
    }
    check(!Rc5Protocol::validPacket(QByteArray::fromHex("c2003e")),
          "Corrupt additive checksum must be rejected");
    check(!Rc5Protocol::validPacket(QByteArray::fromHex("c200")),
          "Truncated frame must be rejected");
    check(!Rc5Protocol::validPacket(QByteArray::fromHex("55063b")),
          "Wake bytes are not a regular RC5 packet");
    check(Rc5Protocol::packetLength(0xc6)==7,
          "RC5 variable length must include header and checksum");

    QVariantList faults;
    check(Rc5Protocol::decodeFaultPacket(QByteArray::fromHex("c2330a"),&faults)
          && faults.isEmpty(),"Zero-fault response");

    // C4 33 15 0A E9: driver airbag short to positive, 16-bit code.
    check(Rc5Protocol::decodeFaultPacket(QByteArray::fromHex("c433150ae9"),&faults)
          && faults.size()==1 &&
          faults.at(0).toMap().value("code").toInt()==0x150A &&
          faults.at(0).toMap().value("descriptionKey").toInt()==16840 &&
          !faults.at(0).toMap().value("historical").toBool(),
          "Single present 16-bit airbag fault");

    // C6 33 15 0C 25 0C B4: a present/historic pair of the SAME type.
    check(Rc5Protocol::decodeFaultPacket(QByteArray::fromHex("c633150c250cb4"),&faults)
          && faults.size()==2 &&
          faults.at(0).toMap().value("code").toInt()==0x150C &&
          faults.at(1).toMap().value("code").toInt()==0x250C &&
          faults.at(0).toMap().value("descriptionKey").toInt()==16842 &&
          faults.at(1).toMap().value("descriptionKey").toInt()==16842 &&
          !faults.at(0).toMap().value("historical").toBool() &&
          faults.at(1).toMap().value("historical").toBool(),
          "Present/historic DTC decoding without consuming trailing checksum");

    check(!Rc5Protocol::decodeFaultPacket(QByteArray::fromHex("c3330009"),&faults),
          "Reject a fault packet containing a dangling data byte");
    check(!Rc5Protocol::decodeFaultPacket(QByteArray::fromHex("82334a"),&faults),
          "Never decode local cable echo as ECU faults");
    check(!Rc5Protocol::decodeFaultPacket(QByteArray::fromHex("c433150ae8"),&faults),
          "Reject malformed fault checksum");
    check(!Rc5Protocol::decodeFaultPacket(QByteArray::fromHex("c433150a"),&faults),
          "Reject incomplete DTC response");

    check(Rc5Protocol::decodeFaultPacket(QByteArray::fromHex("c433dead7d"),&faults)
          && faults.size()==1 &&
          faults.at(0).toMap().value("descriptionKey").toInt()==16880 &&
          !faults.at(0).toMap().value("known").toBool(),
          "Unknown DTC must retain its raw 16-bit code");

    const quint16 standardCodes[]={
        0x150A,0x150B,0x150C,0x150D,0x150E,
        0x1512,0x1513,0x1514,0x1515,0x1516,
        0x151A,0x151B,0x151C,0x151D,0x151E,
        0x1524,0x1525,0x1526,
        0x152C,0x152D,0x152E,0x160C,0x160D,0x160E
    };
    QSet<int> descriptions;
    for(quint16 code:standardCodes){
        const int key=Rc5Protocol::descriptionKey(code);
        descriptions.insert(key);
        check(key>=16840 && key<=16863,"Known DTC description key in catalog");
        check(key==Rc5Protocol::descriptionKey(quint16(code+0x1000u)),
              "Historical DTC must use same description");
    }
    check(descriptions.size()==24,"24 distinct faults and 24 historic variants");

    if(ok)qInfo().noquote()
       <<"RC5_PROTOCOL_SELFTEST_PASS ping, wake classification, additive checksum,"
       <<" present/historic DTCs, 24 descriptions, clear and malformed frames";
    return ok?0:1;
}
