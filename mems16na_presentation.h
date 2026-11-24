#pragma once
#include "mems16na_session_evidence.h"
#include "i18n.h"
#include <QStringList>
namespace Mems16Na {
inline int circuitKey(Circuit c) {
    switch(c) {
    case Circuit::CoolantSensor:return 29006;
    case Circuit::IntakeSensor:return 29007;
    case Circuit::FuelTemperatureSensor:return 29008;
    case Circuit::FuelPump:return 29009;
    case Circuit::PtcHeater:return 29010;
    case Circuit::AirConditioning:return 29011;
    case Circuit::CoolingFans:return 29012;
    case Circuit::AuxiliaryPtc:return 29013;
    case Circuit::Canister:return 29014;
    case Circuit::MapSensor:return 29015;
    case Circuit::ThrottlePot:return 29016;
    case Circuit::LambdaHeater:return 29017;
    case Circuit::EngineSync:return 29018;
    case Circuit::ThrottleSwitch:return 29019;
    case Circuit::UnidentifiedFault:return 29005;
    default:return 29004;
    }
}
inline QString maskText(unsigned mask) {
    return QStringLiteral("0x%1").arg(mask,2,16,QLatin1Char('0')).toUpper();
}
inline QString describe(Circuit c,unsigned mask) {
    QString t=I18n::text(circuitKey(c));
    if(c==Circuit::AuxiliaryPtc)t+=QStringLiteral(" — ")+I18n::text(29020);
    return maskText(mask)+QStringLiteral(" : ")+t;
}
inline QString dtcHelp(const Profile *p,Group g) {
    if(!p)return I18n::text(29002);
    const auto d=definition(p,g);
    if(d.constant)return I18n::text(29001);
    QStringList lines;
    for(unsigned mask=1;mask<=128;mask<<=1)
        if((d.faultMask|d.stateMask)&mask)lines<<describe(circuit(p,g,mask),mask);
    lines<<I18n::text(29021);
    return lines.join(QLatin1Char('\n'));
}
inline QString dtcText(const Profile *p,Group g,const DtcResult &d) {
    if(d.availability==Availability::Missing)return I18n::text(29048);
    if(d.availability==Availability::NotUsed)return I18n::text(29001);
    QStringList lines;
    if(d.noFault())lines<<I18n::text(29003);
    for(unsigned mask=1;mask<=128;mask<<=1)
        if(d.faultBits&mask)lines<<describe(circuit(p,g,mask),mask);
    for(unsigned mask=1;mask<=128;mask<<=1)
        if(d.unknownBits&mask)lines<<describe(Circuit::UnidentifiedIndicator,mask);
    for(unsigned mask=1;mask<=128;mask<<=1)
        if(d.stateBits&mask)lines<<describe(circuit(p,g,mask),mask);
    if(lines.isEmpty())lines<<I18n::text(29002);
    return lines.join(QStringLiteral(" ; "));
}
inline QString statusText(const Value &v) {
    if(v.availability==Availability::NotUsed)return I18n::text(29000);
    if(v.availability==Availability::Missing)return I18n::text(29048);
    return I18n::text(29002);
}
inline QString rawText(const Value &v) {
    return v.availability==Availability::Missing?QStringLiteral("--"):QString::number(v.raw);
}
inline QString numeric(const Value &v,int precision,const QString &unit=QString()) {
    if(v.availability!=Availability::Available)return statusText(v);
    return QString::number(v.calculated,'f',precision)+unit;
}
inline QString steps(const Value &v) {
    return v.availability==Availability::Available?I18n::text(29034).arg(v.calculated):statusText(v);
}
inline QString state(const Value &v,int yes,int no) {
    return v.availability==Availability::Available?I18n::text(v.calculated?yes:no):statusText(v);
}
inline QString idleCorrection(const Measurements &m) {
    if(m.idleAmplitude.availability!=Availability::Available)return statusText(m.idleAmplitude);
    if(m.signedIdleConsistent)return numeric(m.idleSigned,2,QStringLiteral(" RPM"));
    return I18n::text(29040).arg(QString::number(m.idleAmplitude.calculated,'f',2));
}
inline QString rawDtc(const DtcResult &d) {
    return d.availability==Availability::Missing?QStringLiteral("--"):QString::number(d.raw);
}
inline QString throttleWord(const Value &v) {
    if(v.availability==Availability::Missing)return QStringLiteral("--");
    return QStringLiteral("%1 %2").arg(v.raw>>8,2,16,QLatin1Char('0')).arg(v.raw&255,2,16,QLatin1Char('0')).toUpper();
}
}
