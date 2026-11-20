#include <QCoreApplication>
#include <QDebug>
#include "mems3protocol.h"

// Synthetic protocol vectors: this test does not claim to have an actual ECU.
int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv);
    using namespace Mems3Protocol;
    auto check=[](bool ok,const char *label)->bool {
        if(!ok)qCritical()<<"FAIL MEMS 3:"<<label;
        return ok;
    };
    bool ok=true;
    ok&=check(frame(QByteArray::fromHex("1a9a"))==
              QByteArray::fromHex("b813f7021a9ade"),"8E1 MEMS3 wake frame");
    ok&=check(frame(QByteArray::fromHex("140000"))==
              QByteArray::fromHex("b813f7031400004b"),"clear faults frame");
    QByteArray payload;
    ok&=check(unpack(QByteArray::fromHex("000000025a9ac2"),&payload) &&
              payload==QByteArray::fromHex("5a9a"),"response framing and XOR");
    ok&=check(!unpack(QByteArray::fromHex("000000025a9ac3"),&payload),
              "incorrect XOR rejected");
    ok&=check(!unpack(QByteArray::fromHex("000000025a9a"),&payload),
              "truncated reply rejected");
    ok&=check(!unpack(QByteArray::fromHex("000000ff"),&payload),
              "absurd frame length rejected");
    QVariantMap values;
    ok&=check(decodeMeasurement(0x00,QByteArray::fromHex("61000b7200000b7200000b72"),&values)
              && values.value("coolant_c").toDouble()==20 &&
                 values.value("oil_c").toDouble()==20 &&
                 values.value("intake_c").toDouble()==20,
              "coolant oil air 20C");
    ok&=check(decodeMeasurement(0x06,QByteArray::fromHex("610627100000000013880bb8"),&values)
              && values.value("map_kpa").toDouble()==100 &&
                 values.value("throttle_mv").toInt()==5000 &&
                 values.value("rpm").toInt()==3000,"MAP TPS RPM");
    ok&=check(decodeMeasurement(0x0A,QByteArray::fromHex("6100271001f4"),&values)==false,
              "reject wrong response ID");
    ok&=check(decodeMeasurement(0x0A,QByteArray::fromHex("610a271001f4"),&values)
              && values.value("lambda_feedback_percent").toDouble()==100 &&
                 values.value("lambda_mv").toInt()==500,"lambda");
    ok&=check(decodeMeasurement(0x0B,QByteArray::fromHex("610b00fa0104"),&values)
              && values.value("coil1_us").toInt()==250 &&
                 values.value("coil2_us").toInt()==260,"coil times");
    ok&=check(decodeMeasurement(0x21,QByteArray::fromHex("6121ffff"),&values)
              && values.value("rpm_deviation_raw").toInt()==65535,
              "unknown sign preserves raw RPM deviation");
    ok&=check(!decodeMeasurement(0x06,QByteArray::fromHex("61062710"),&values),
              "short measurement rejected");
    QStringList faults=decodeFaults(QByteArray::fromHex("5800123261011820"));
    ok&=check(faults.size()==2 && faults[0].contains("1232") &&
              faults[1].contains("0118"),"MEMS3 16-bit DTC codes and state");
    ok&=check(Mems2JProtocol::generateKey(0x1234)==0x8247,"shared seed/key vector");
    if(ok)qInfo()<<"PASS MEMS 3 XOR, K-Line responses, measurements and DTC catalog";
    return ok?0:1;
}
