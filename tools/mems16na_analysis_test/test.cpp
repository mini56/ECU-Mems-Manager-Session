#include "mems16na_analysis.h"
#include <QApplication>
#include <iostream>
using namespace Mems16Na;
using namespace Mems16Na::Analysis;
#define CHECK(x) do{if(!(x)){std::cerr<<"FAIL "<<__LINE__<<" "<<#x<<"\n";return 1;}}while(0)
int main(int argc,char **argv) {
    QApplication app(argc,argv);
    CHECK(I18n::load(QStringLiteral("fr")));
    auto id=identity(QStringLiteral("AANMP002"),QStringLiteral("98000202"));
    CHECK(id.identifiedProfile());
    CHECK(!identity(QStringLiteral("AANMP002"),QStringLiteral("90000202")).identifiedProfile());
    QByteArray a(28,0),b(32,0);a[0]=28;b[0]=32;
    a[4]=char(255);a[6]=char(255);a[8]=char(138);a[9]=char(128);
    a[16]=32;a[17]=char(134);b[5]=0x40;b[6]=char(168);
    b[11]=char(129);b[14]=char(255);b[16]=char(255);b[17]=char(255);b[22]=char(255);
    b[20]=0x7f;b[21]=char(255);
    b[23]=27;b[24]=0;b[25]=31;b[26]=char(192);b[27]=39;b[28]=char(192);b[29]=55;b[30]=char(128);
    const QString stamp=QStringLiteral("2026-11-24T21:00:00.123+01:00");
    auto s=replay(id,QStringLiteral("1"),QString::fromLatin1(a.toHex()),QString::fromLatin1(b.toHex()));
    CHECK(s.ready());
    auto r=row(s,stamp);auto k=keys();
    CHECK(r.numbers.size()==k.size()&&r.texts.size()==k.size());
    auto col=[&](const char *name){return k.indexOf(QString::fromLatin1(name));};
    CHECK(r.numbers.at(col("80x08_battery_voltage"))==13.8);
    CHECK(r.numbers.at(col("na_lambda_mv_value"))==840);
    CHECK(r.numbers.at(col("na_idle_error_rpm_value"))==-1);
    CHECK(r.numbers.at(col("na_injection_time_us_value"))==8);
    CHECK(r.numbers.at(col("na_tps_ref4_tps_value"))==55.5);
    CHECK(qIsNaN(r.numbers.at(col("na_ambient_c_value"))));
    CHECK(r.texts.at(col("na_ambient_c_value"))==I18n::text(29001));
    CHECK(r.texts.at(col("na_dtc2_raw")).contains(I18n::text(29019)));
    CHECK(r.numbers.at(col("na_dtc2_fault_bits"))==0);
    CHECK(r.numbers.at(col("na_dtc2_state_bits"))==64);
    CHECK(qIsNaN(r.numbers.at(col("na_dtc3_fault_bits"))));
    auto invalid=row(replay(id,QStringLiteral("1"),QStringLiteral("1C"),QString::fromLatin1(b.toHex())),stamp);
    CHECK(invalid.numbers.at(col("na_sample_valid"))==0);
    CHECK(qIsNaN(invalid.numbers.at(col("na_lambda_mv_value"))));
    for(const QString &key:k)if(key.startsWith(QStringLiteral("na_")))CHECK(!label(key).isEmpty());
    CHECK(label(QStringLiteral("na_tps_ref4_tps_value")).contains(QStringLiteral("TPS")));
    // A localized label already containing its unit must not duplicate it.
    CHECK(label(QStringLiteral("na_learn_voltage_v_value")).count(QStringLiteral("(V)"))==1);
    CHECK(label(QStringLiteral("na_injection_time_us_value")).count(QStringLiteral("(µs)"))==1);
    CHECK(label(QStringLiteral("na_tps_ref4_tps_value")).endsWith(QStringLiteral(" (TPS)")));
    CHECK(label(QStringLiteral("na_idle_hot_steps_value")).endsWith(QStringLiteral(" (")+I18n::text(29065)+QLatin1Char(')')));
    CHECK(label(QStringLiteral("na_ambient_c_value")).endsWith(QStringLiteral(" (°C)")));
    std::cout<<"MEMS16NA_ANALYSIS_QT_OK: V2 replay, identity, 13.8 V, 840 mV, TPS, gaps, DTC state, labels\n";
}
