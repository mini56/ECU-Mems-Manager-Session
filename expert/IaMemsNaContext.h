#pragma once
// Read-only adapter for the exact same qualified decoder and diagnostic table.
// No serial I/O, profile guessing, duplicate conversion or legacy rule reuse.
#include "../mems16na_presentation.h"
#include "../rosco.h"
#include "ExpertTypes.h"
#include "IaResponseLogic.h"
#include <QRegularExpression>
#include <QDateTime>

namespace IaMemsNa {
inline ExpertObservation observation(const Mems16Na::SessionEvidence &na,
                                      const mems_data &data,qint64 timestamp)
{
    using namespace Mems16Na;
    ExpertObservation out;
    out.timestampMs=timestamp;
    if(!na.ready())return out;
    const auto m=na.measurements();
    auto &v=out.values;
    auto insert=[&](const char *key,const Value &value) {
        if(value.availability==Availability::Available)
            v.insert(QString::fromLatin1(key),value.calculated);
    };
    // Independent measurements retained in the Diagnostic page.
    v.insert(QStringLiteral("rpm"),data.engine_rpm);
    v.insert(QStringLiteral("coolant_c"),int(data.coolant_temp)-55.0);
    v.insert(QStringLiteral("intake_air_c"),int(data.intake_air_temp)-55.0);
    v.insert(QStringLiteral("map_kpa"),data.map_kpa);
    v.insert(QStringLiteral("battery_v"),data.battery_voltage/10.0);
    v.insert(QStringLiteral("coil_time_ms"),data.coil_time*0.002);
    v.insert(QStringLiteral("ignition_advance_deg"),data.ignition_advance*0.5-24.0);
    v.insert(QStringLiteral("iac_position"),data.iac_position);
    if(!na.identifiedProfile()->alternateTransmission)
        v.insert(QStringLiteral("short_term_trim_pct"),int(data.short_term_fuel_trim)-100.0);
    insert("ambient_c",m.ambient);insert("fuel_c",m.fuel);
    insert("pn_active",m.pn);insert("ac_active",m.ac);
    insert("idle_amplitude_rpm",m.idleAmplitude);insert("idle_signed_rpm",qualifiedIdleSigned(m));
    insert("idle_hot_steps",m.idleHot);insert("learn_voltage_v",m.learnVoltage);
    insert("throttle_voltage_v",m.throttleVoltage);insert("ignition_off",m.ignitionOff);
    insert("injection_time_us",m.injectionTime);insert("idle_base_steps",m.idleBase);
    insert("idle_error_rpm",m.idleError);insert("learn_count",m.learnCount);
    insert("throttle_angle_deg",m.throttleAngle);insert("lambda_mv",m.lambdaMv);
    insert("lambda_fault_active",m.lambdaFault);insert("closed_loop",m.closedLoop);
    for(int i=0;i<4;++i)if(m.throttleReference[i].availability==Availability::Available)
        v.insert(QStringLiteral("tps_ref%1_tps").arg(i+1),m.throttleReference[i].calculated);
    const Group groups[]={Group::Dtc0,Group::Dtc1,Group::Dtc2,Group::Dtc3,Group::Dtc4,Group::Dtc5,Group::Indirect7D10};
    for(int i=0;i<7;++i){
        const auto d=na.dtc(groups[i]);
        const QString k=i<6?QStringLiteral("na_dtc%1").arg(i):QStringLiteral("na_7d10");
        v.insert(k+QStringLiteral("_availability"),int(d.availability));
        if(d.availability!=Availability::Missing)v.insert(k+QStringLiteral("_raw"),d.raw);
        if(d.availability==Availability::Available){
            v.insert(k+QStringLiteral("_fault"),d.faultBits);
            v.insert(k+QStringLiteral("_state"),d.stateBits);
        }
        if(d.availability==Availability::Available||d.availability==Availability::Unverified)
            v.insert(k+QStringLiteral("_unknown"),d.unknownBits);
    }
    return out;
}
inline QString measurementDetails(const Mems16Na::SessionEvidence &na)
{
    using namespace Mems16Na;
    const auto m=na.measurements();
    QStringList out;
    auto add=[&](int key,const QString &value){out<<I18n::text(key)+QStringLiteral(" : ")+value;};
    add(29046,numeric(m.ambient,0,QStringLiteral(" °C")));
    add(29047,numeric(m.fuel,0,QStringLiteral(" °C")));
    add(29022,m.pn.availability==Availability::Available?
        (m.pn.calculated?QStringLiteral("P/N"):I18n::text(29035)):statusText(m.pn));
    add(29023,state(m.ac,29036,29037));
    add(29024,idleCorrection(m));
    add(29025,steps(m.idleHot));
    add(29026,numeric(m.learnVoltage,1,QStringLiteral(" V")));
    add(29027,numeric(m.throttleVoltage,2,QStringLiteral(" V")));
    add(29028,state(m.ignitionOff,29038,29039));
    add(29029,numeric(m.injectionTime,0,QStringLiteral(" µs")));
    out<<I18n::text(29044);
    add(29030,steps(m.idleBase));
    add(29031,numeric(m.idleError,0,QStringLiteral(" RPM")));
    out<<I18n::text(29043);
    for(int i=0;i<4;++i)out<<I18n::text(29032).arg(i+1)+QStringLiteral(" : ")+numeric(m.throttleReference[i],2,QStringLiteral(" TPS"));
    out<<I18n::text(29041);
    if(na.identifiedProfile()&&na.identifiedProfile()->repeatedThrottleReference)out<<I18n::text(29042);
    add(29033,numeric(m.learnCount,0));
    add(6627,numeric(m.throttleAngle,1,QStringLiteral(" °")));
    add(29057,numeric(m.lambdaMv,0,QStringLiteral(" mV")));
    add(29058,state(m.lambdaFault,29038,29039));
    add(29059,state(m.closedLoop,29038,29039));
    out<<I18n::text(29054);
    return out.join(QLatin1Char('\n'));
}
inline int requestedDtc(const QString &question)
{
    const QRegularExpression re(QStringLiteral("\\bdtc\\s*([0-5])\\b"),QRegularExpression::CaseInsensitiveOption);
    const auto match=re.match(question);
    if(match.hasMatch())return match.captured(1).toInt();
    if(QRegularExpression(QStringLiteral("\\b7d\\s*/\\s*(?:0x)?10\\b"),QRegularExpression::CaseInsensitiveOption).match(question).hasMatch())return 6;
    return -1;
}
inline QString groupName(int i){return i<6?QStringLiteral("DTC %1").arg(i):QStringLiteral("7D/10");}
inline QString dtcAnswer(int i,const Mems16Na::SessionEvidence &na,bool referenceOnly)
{
    using namespace Mems16Na;
    const Group g=static_cast<Group>(i);
    if(referenceOnly)return groupName(i)+QStringLiteral("\n")+dtcDocumentation(g);
    const auto *p=na.identifiedProfile();
    const auto d=na.dtc(g);
    return groupName(i)+QStringLiteral("\n")+I18n::text(29049)+QStringLiteral(" : ")+
        QString::fromLatin1(p->firmware)+QStringLiteral("\n")+dtcText(p,g,d)+
        QStringLiteral("\n")+dtcHelp(p,g);
}
inline bool explicitReference(const QString &question)
{
    const QString t=IaResponseLogic::normalize(question);
    if(t.contains(QStringLiteral("1.6"))&&IaResponseLogic::hasAny(t,{"atmospher","naturally aspirated","atmosfer","saugmotor","aspirato"}))return true;
    std::size_t n=0;const auto *p=Mems16Na::catalog(n);
    for(std::size_t i=0;i<n;++i)
        if(question.contains(QString::fromLatin1(p[i].firmware),Qt::CaseInsensitive))return true;
    return false;
}
inline bool relevant(const QString &question)
{
    const auto intent=IaResponseLogic::classify(question);
    if(intent!=IaResponseLogic::Intent::None && intent!=IaResponseLogic::Intent::Captures)return true;
    const QString t=IaResponseLogic::normalize(question);
    if(requestedDtc(question)>=0||IaResponseLogic::hasAny(t,{"dtc","defaut","historique","history","historico","verlauf","valeurs","mesures","measurement","current values","messwerte","medidas","misure","injection","apprentissage","learning","aprendizaje","lern","tps","papillon","throttle","ralenti","idle","lambda","diagnostic","hypothese","oscill","instable"}))return true;
    for(int k:{29022,29023,29024,29025,29026,29027,29028,29029,29030,29031,29033,29046,29047,29057,29058,29059})
        if(t.contains(IaResponseLogic::normalize(I18n::text(k))))return true;
    return false;
}
}
