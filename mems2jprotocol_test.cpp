#include <QCoreApplication>
#include <QDebug>
#include "mems2jprotocol.h"

// No hardware required. These are synthetic vectors, not captured ECU replies.
int main(int argc, char **argv)
{
    QCoreApplication app(argc,argv);
    auto require=[](bool ok, const char *name) -> bool {
        if (!ok) qCritical() << "FAIL MEMS 2J:" << name;
        return ok;
    };
    using namespace Mems2JProtocol;
    bool ok=true;
    ok &= require(frame(QByteArray::fromHex("2109"))==QByteArray::fromHex("0221092c"),"RPM request");
    ok &= require(frame(QByteArray::fromHex("2119"))==QByteArray::fromHex("0221193c"),"fault request");
    ok &= require(frame(QByteArray::fromHex("2101"))==QByteArray::fromHex("02210124"),"coolant request");
    QByteArray reply;
    ok &= require(unpack(QByteArray::fromHex("0461090bb831"),&reply) &&
                  reply==QByteArray::fromHex("61090bb8"),"length/checksum");
    ok &= require(!unpack(QByteArray::fromHex("0461090bb830"),&reply),"checksum rejection");
    ok &= require(!unpack(QByteArray::fromHex("0461090b"),&reply),"truncated frame");
    ok &= require(unpack(QByteArray::fromHex("037f2131d4"),&reply) &&
                  reply==QByteArray::fromHex("7f2131"),"negative response");
    QVariantMap v;
    ok &= require(decodeMeasurement(0x09,QByteArray::fromHex("61090bb8"),&v) &&
                  v.value("rpm").toDouble()==3000.0,"RPM 3000");
    ok &= require(decodeMeasurement(0x01,QByteArray::fromHex("61010b74"),&v) &&
                  v.value("coolant_c").toDouble()==20.0,"coolant 20 C");
    ok &= require(decodeMeasurement(0x07,QByteArray::fromHex("61072710"),&v) &&
                  v.value("map_kpa").toDouble()==100.0,"MAP 100 kPa");
    ok &= require(decodeMeasurement(0x08,QByteArray::fromHex("61080e74"),&v) &&
                  v.value("throttle_deg").toDouble()==37.0,"TPS 37 degrees");
    ok &= require(decodeMeasurement(0x10,QByteArray::fromHex("611000003039"),&v) &&
                  v.value("battery_v").toDouble()==12.345,"battery position 4:5");
    ok &= require(decodeMeasurement(0x21,QByteArray::fromHex("6121ffff"),&v) &&
                  v.value("rpm_error").toInt()==-1,"signed FFFF");
    ok &= require(decodeMeasurement(0x21,QByteArray::fromHex("61218000"),&v) &&
                  v.value("rpm_error").toInt()==-32768,"signed 8000");
    ok &= require(!decodeMeasurement(0x10,QByteArray::fromHex("61103039"),&v),
                  "reject short optional payload");
    ok &= require(decodeMeasurement(0x00,QByteArray::fromHex("6100aabb"),&v) &&
                  v.value("DATA_00_RAW").toString()==QStringLiteral("61 00 AA BB"),
                  "preserve undecoded 21 00 bytes");
    ok &= require(decodeMeasurement(0x06,QByteArray::fromHex("61060102"),&v) &&
                  v.value("DATA_06_RAW").toString()==QStringLiteral("61 06 01 02"),
                  "preserve undecoded 21 06 bytes");
    ok &= require(frame(QByteArray::fromHex("10a0"))==QByteArray::fromHex("0210a0b2"),
                  "start diagnostic packet");
    ok &= require(frame(QByteArray::fromHex("2701"))==QByteArray::fromHex("0227012a"),
                  "seed request packet");
    ok &= require(generateKey(0x1234)==0x8247,"seed key 1234");
    ok &= require(generateKey(0xABCD)==0x85AF,"seed key ABCD");
    QByteArray faults(29,char(0));
    faults[0]=char(0x61); faults[1]=char(0x19);
    faults[12]=char(0x04);
    faults[25]=char(0x08);
    const QStringList parsed=decodeFaults(faults);
    ok &= require(parsed.size()==2 &&
                  parsed.at(0).contains(QStringLiteral("ambigu")) &&
                  parsed.at(1).contains(QStringLiteral("historique")),
                  "Mini MPi fault flags and one ambiguous bit");
    if(ok) qInfo() << "PASS MEMS 2J frames, authentication, measurements and Mini fault flags";
    return ok?0:1;
}
