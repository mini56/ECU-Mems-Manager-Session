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
// Internal raw formatter remains available to non-UI consumers.
inline QString maskText(unsigned mask) {
    return QStringLiteral("0x%1").arg(mask,2,16,QLatin1Char('0')).toUpper();
}
// Display labels only; the decoder and raw data remain unchanged.
inline QString describe(Circuit c,unsigned) {
    return I18n::text(circuitKey(c));
}
inline QString dtcHelp(const Profile *p,Group g) {
    if(!p)return I18n::text(29002);
    const auto d=definition(p,g);
    if(d.constant)return I18n::text(29001);
    QStringList names;
    bool auxiliary=false;
    for(unsigned mask=1;mask<=128;mask<<=1)
        if((d.faultMask|d.stateMask)&mask){
            const auto c=circuit(p,g,mask);
            names<<describe(c,mask);
            auxiliary|=c==Circuit::AuxiliaryPtc;
        }
    names.removeDuplicates();
    names.sort(Qt::CaseInsensitive);
    if(auxiliary)names<<I18n::text(29020);
    names<<I18n::text(29021);
    return names.join(QLatin1Char('\n'));
}
// A reference is not a selected ECU profile. List all documented circuits,
// without exposing the mapping or the order of its underlying fields.
inline QString dtcDocumentation(Group g) {
    QStringList lines{I18n::text(29074)};
    std::size_t n=0;const auto *profiles=catalog(n);
    if(n&&definition(profiles,g).constant) {
        lines<<I18n::text(29075);
        return lines.join(QLatin1Char('\n'));
    }
    QStringList names;
    bool auxiliary=false;
    for(std::size_t i=0;i<n;++i){
        const auto def=definition(&profiles[i],g);
        for(unsigned mask=1;mask<=128;mask<<=1){
            if(!((def.faultMask|def.stateMask)&mask))continue;
            const auto c=circuit(&profiles[i],g,mask);
            names<<describe(c,mask);
            auxiliary|=c==Circuit::AuxiliaryPtc;
        }
    }
    names.removeDuplicates();
    names.sort(Qt::CaseInsensitive);
    lines<<names;
    if(auxiliary)lines<<I18n::text(29020);
    lines<<I18n::text(29021);
    return lines.join(QLatin1Char('\n'));
}
inline bool documentedUnusedDtc(Group g) {
    std::size_t n=0;const auto *profiles=catalog(n);
    if(!n)return false;
    for(std::size_t i=0;i<n;++i)if(!definition(&profiles[i],g).constant)return false;
    return true;
}
inline QString documentation(const QString &detail) {
    const QString scope=I18n::text(29074);
    return detail.startsWith(scope)?detail:scope+QStringLiteral("\n")+detail;
}
inline QString dtcText(const Profile *p,Group g,const DtcResult &d) {
    if(d.availability==Availability::Missing)return I18n::text(29048);
    if(d.availability==Availability::NotUsed)return I18n::text(29001);
    if(d.availability==Availability::Unverified && !d.unknownBits)return I18n::text(29002);
    QStringList lines;
    if(d.noFault())lines<<I18n::text(29003);
    for(unsigned mask=1;mask<=128;mask<<=1)
        if(d.faultBits&mask)lines<<describe(circuit(p,g,mask),mask);
    for(unsigned mask=1;mask<=128;mask<<=1)
        if(d.unknownBits&mask)lines<<describe(Circuit::UnidentifiedIndicator,mask);
    for(unsigned mask=1;mask<=128;mask<<=1)
        if(d.stateBits&mask)lines<<describe(circuit(p,g,mask),mask);
    if(lines.isEmpty())lines<<I18n::text(29002);
    lines.removeDuplicates();
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
    if(m.signedIdleConsistent)return numeric(qualifiedIdleSigned(m),2,QStringLiteral(" RPM"));
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
