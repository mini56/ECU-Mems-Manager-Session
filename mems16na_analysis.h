#pragma once
// Read-only presentation of the V2 measurement schema, shared by live and replay.
#include "mems16na_log_schema.h"
#include "mems16na_presentation.h"
#include <QByteArray>
#include <QDateTime>
#include <QMap>
#include <QVector>
#include <QtMath>
#include <limits>

namespace Mems16Na { namespace Analysis {
struct Channel { const char *stem; const char *value; int label; const char *unit; };
inline const Channel *channels(std::size_t &n) {
    static const Channel c[]={
        {"ambient","ambient_c",29046,"°C"},{"fuel","fuel_c",29047,"°C"},
        {"pn","pn_active",29022,""},{"ac","ac_request",29023,""},
        {"idle_amplitude","idle_amplitude_rpm",29056,"RPM"},
        {"idle_signed","idle_signed_rpm",29024,"RPM"},
        {"idle_hot","idle_hot_steps",29025,"steps"},
        {"learn_voltage","learn_voltage_v",29026,"V"},
        {"throttle_voltage","throttle_voltage_v",29027,"V"},
        {"ignition_off","ignition_off",29028,""},
        {"injection_time","injection_time_us",29029,"µs"},
        {"idle_base","idle_base_steps",29030,"steps"},
        {"idle_error","idle_error_rpm",29031,"RPM"},
        {"tps_ref1","tps_ref1_tps",29032,"TPS"},{"tps_ref2","tps_ref2_tps",29032,"TPS"},
        {"tps_ref3","tps_ref3_tps",29032,"TPS"},{"tps_ref4","tps_ref4_tps",29032,"TPS"},
        {"learn_count","learn_count",29033,""},{"throttle_angle","throttle_angle_deg",6627,"°"},
        {"lambda","lambda_mv",29057,"mV"},{"lambda_fault","lambda_fault",29058,""},
        {"closed_loop","closed_loop",29059,""}
    };
    n=sizeof(c)/sizeof(c[0]);return c;
}
inline QString channelLabel(const Channel &c) {
    QString t=I18n::text(c.label);
    if(c.label==29032)t=t.arg(QString::fromLatin1(c.stem).right(1));
    return t;
}
inline QString dtcName(int i) {return i<6?QStringLiteral("DTC %1").arg(i):QStringLiteral("7D/10");}
inline QString dtcStem(int i) {return i<6?QStringLiteral("na_dtc%1").arg(i):QStringLiteral("na_7d10");}
inline Group dtcGroup(int i) {
    static const Group g[]={Group::Dtc0,Group::Dtc1,Group::Dtc2,Group::Dtc3,Group::Dtc4,Group::Dtc5,Group::Indirect7D10};
    return g[i];
}
inline QString label(const QString &key) {
    if(key==QStringLiteral("na_timestamp"))return I18n::text(29060);
    if(key==QStringLiteral("na_sample_valid"))return I18n::text(29061);
    if(key==QStringLiteral("na_frame80_hex"))return I18n::text(29062).arg(QStringLiteral("80"));
    if(key==QStringLiteral("na_frame7d_hex"))return I18n::text(29062).arg(QStringLiteral("7D"));
    std::size_t n=0;const auto *c=channels(n);
    for(std::size_t i=0;i<n;++i) {
        const QString prefix=QStringLiteral("na_")+QString::fromLatin1(c[i].stem);
        const QString name=channelLabel(c[i]);
        if(key==prefix+QStringLiteral("_raw"))return I18n::text(29063).arg(name);
        if(key==prefix+QStringLiteral("_availability"))return I18n::text(29064).arg(name);
        if(key==QStringLiteral("na_")+QString::fromLatin1(c[i].value)+QStringLiteral("_value")) {
            QString unit=QString::fromUtf8(c[i].unit);
            if(unit==QStringLiteral("steps"))unit=I18n::text(29065);
            if(!unit.isEmpty()&&!name.contains(QStringLiteral("("+unit+")")))return name+QStringLiteral(" (")+unit+QLatin1Char(')');
            return name;
        }
    }
    for(int i=0;i<7;++i) {
        const QString stem=dtcStem(i),name=dtcName(i);
        if(key==stem+QStringLiteral("_raw"))return I18n::text(29063).arg(name);
        if(key==stem+QStringLiteral("_availability"))return I18n::text(29064).arg(name);
        if(key==stem+QStringLiteral("_fault_bits"))return I18n::text(29066).arg(name);
        if(key==stem+QStringLiteral("_state_bits"))return I18n::text(29067).arg(name);
        if(key==stem+QStringLiteral("_unknown_bits"))return I18n::text(29068).arg(name);
    }
    return QString();
}
inline QString status(const QString &v) {
    if(v==QStringLiteral("available"))return I18n::text(29069);
    if(v==QStringLiteral("not_used"))return I18n::text(29001);
    if(v==QStringLiteral("unverified"))return I18n::text(29002);
    return I18n::text(29048);
}
inline QStringList keys() {
    QStringList result;
    for(const auto &f:LogV2::fields(SessionEvidence{}, ""))result<<QString::fromStdString(f.name);
    return result;
}
inline double gap(){return std::numeric_limits<double>::quiet_NaN();}
struct Row { QVector<double> numbers; QStringList texts; };
inline Row row(const SessionEvidence &s,const QString &timestamp) {
    Row out;
    const auto fields=LogV2::fields(s,timestamp.toStdString());
    QMap<QString,QString> data;
    for(const auto &f:fields)data.insert(QString::fromStdString(f.name),QString::fromStdString(f.value));
    for(const auto &f:fields) {
        const QString key=QString::fromStdString(f.name);
        const QString value=QString::fromStdString(f.value);
        bool ok=false;const double number=value.toDouble(&ok);
        out.numbers<<((ok&&qIsFinite(number))?number:gap());
        QString text=value;
        if(key.endsWith(QStringLiteral("_availability")))text=status(value);
        else if(value.isEmpty()) {
            text=I18n::text(29000);
            std::size_t n=0;const auto *c=channels(n);
            for(std::size_t i=0;i<n;++i)if(key==QStringLiteral("na_")+QString::fromLatin1(c[i].value)+QStringLiteral("_value"))
                text=status(data.value(QStringLiteral("na_")+QString::fromLatin1(c[i].stem)+QStringLiteral("_availability")));
        }
        // Keep the raw byte/mask as the plotted value and show all circuits in Columns.
        for(int i=0;i<7;++i)if(key==dtcStem(i)+QStringLiteral("_raw"))
            text=(value.isEmpty()?QString():value+QStringLiteral(" — "))+dtcText(s.identifiedProfile(),dtcGroup(i),s.dtc(dtcGroup(i)));
        out.texts<<text;
    }
    return out;
}
inline QByteArray exactHex(const QString &text,int bytes) {
    if(text.size()!=bytes*2)return {};
    const QByteArray ascii=text.toLatin1();
    const QByteArray raw=QByteArray::fromHex(ascii);
    if(raw.size()!=bytes||raw.toHex().toUpper()!=ascii.toUpper())return {};
    return raw;
}
inline std::vector<std::uint8_t> bytes(const QByteArray &b) {return {b.begin(),b.end()};}
inline void observe(SessionEvidence &s,std::uint8_t cmd,const QByteArray &payload) {
    s.tx({cmd});auto v=bytes(payload);v.insert(v.begin(),cmd);s.rx(v);s.finish();
}
inline SessionEvidence identity(const QString &firmware,const QString &ecuId) {
    SessionEvidence s;
    const QByteArray id=exactHex(ecuId,4),name=firmware.toLatin1();
    if(id.size()!=4||name.size()!=8)return s;
    // Header stores the verified D0 identity and D1 name from the original
    // logger. This reconstructs local evidence, not an ECU transaction.
    observe(s,0xd0,id);observe(s,0xd1,name+id);
    return s;
}
inline SessionEvidence replay(const SessionEvidence &identified,const QString &sampleValid,
                              const QString &frame80,const QString &frame7d) {
    SessionEvidence s=identified;s.discardPair();
    if(!s.identifiedProfile()||sampleValid!=QStringLiteral("1"))return s;
    const QByteArray a=exactHex(frame80,28),b=exactHex(frame7d,32);
    if(a.size()!=28||b.size()!=32)return s;
    observe(s,0x80,a);observe(s,0x7d,b);
    return s;
}
} }
