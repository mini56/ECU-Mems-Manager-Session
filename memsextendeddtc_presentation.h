#pragma once
#include "mems16na_session_evidence.h"
#include "i18n.h"
#include <QStringList>
namespace MemsExtendedDtc {
inline QString rawText(const Result &r) {return r.status==Status::Missing?QStringLiteral("--"):QString::number(r.raw);}
inline QString mask(unsigned b) {return QStringLiteral("0x")+QString::number(b,16).rightJustified(2,QLatin1Char('0')).toUpper();}
inline int labelKey(const char *f,int g,unsigned bit) {
    if(g==2&&dtc2Known(f)){if(bit==4)return 29017;if(bit==0x10)return 29018;if(bit==0x20)return 29251;if(bit==0x80)return 29252;}
    if(g==2&&short13(f)){if(bit==1)return 2972401;if(bit==2)return 2972402;}
    if(g==3&&sync19(f)){if(bit==1)return 29018;if(bit==4)return 29240;}
    if(g==3&&turbo(f)&&bit==2)return 29241;
    if(g==4&&turbo(f)){if(bit==1)return 29242;if(bit==2)return 29243;}
    if(g==4&&da19(f)&&bit==0x40)return 29244;
    if(g==5){if((equivalent009(f)||eq(f,"REGI1224"))&&bit==2)return 29245;
        if((equivalent009(f)||eq(f,"REGI1224"))&&bit==4)return 29246;
        if(fan(f)&&bit==8)return 29247;if(fan(f)&&bit==0x10)return 29248;
        if(hdc(f)&&bit==0x20)return 29249;if(cruise(f)&&bit==0x40)return 29250;}
    return 29004;
}
inline Result diagnosticDtc2(const Mems16Na::SessionEvidence *s,unsigned legacy) {
    if(!s)return diagnosticDtc2(nullptr,nullptr,0,legacy);
    const auto &b=s->extended7d();
    return diagnosticDtc2(s->firmwareName().c_str(),b.data(),b.size(),legacy);
}
inline bool diagnosticDtc2FrameAvailable(const Mems16Na::SessionEvidence *s){
    return s&&s->extendedDtc(2).status!=Status::Missing;
}
inline QString diagnosticDtc2Raw(const Mems16Na::SessionEvidence *s,unsigned legacy){
    // Restore the original missing-data display on identified, qualified profiles.
    // Keep historical DLL raw readings on other profiles, explicitly labelled below.
    if(s&&(dtc2Known(s->firmwareName().c_str())||short13(s->firmwareName().c_str()))
       &&!diagnosticDtc2FrameAvailable(s))return QStringLiteral("--");
    return QString::number(diagnosticDtc2(s,legacy).raw);
}
inline QString diagnosticDtc2Text(const Mems16Na::SessionEvidence *s,unsigned legacy) {
    const auto r=diagnosticDtc2(s,legacy);
    QStringList out;
    if(!diagnosticDtc2FrameAvailable(s))out<<I18n::text(29048);
    if(r.status==Status::Unverified)out<<I18n::text(29002);
    if(r.status==Status::Decoded&&!r.faults&&!r.indicators)out<<I18n::text(29003);
    const char *f=s?s->firmwareName().c_str():nullptr;
    for(unsigned bit=1;bit<=128;bit<<=1)if(r.faults&bit)
        out<<I18n::text(labelKey(f,2,bit))+QStringLiteral(" (")+mask(bit)+QStringLiteral(")");
    // Unknown bits are preserved as raw evidence, not attributed to a circuit.
    if(r.indicators&&diagnosticDtc2FrameAvailable(s))out<<I18n::text(2972310)+QStringLiteral(" (")+mask(r.indicators)+QStringLiteral(")");
    return out.join(QStringLiteral(" ; "));
}
inline QString text(const Mems16Na::SessionEvidence *s,int group) {
    if(!s||s->firmwareName().empty())return I18n::text(29002);
    const auto r=s->extendedDtc(group);const auto f=s->firmwareName().c_str();
    if(r.status==Status::Missing)return I18n::text(29048);
    if(r.status==Status::NotUsed)return I18n::text(29001);
    QStringList out;
    if(r.status==Status::Decoded&&!r.faults&&!r.indicators)out<<I18n::text(29003);
    if(r.status==Status::Unverified)out<<I18n::text(29002);
    // 0xFF outside the documented dynamic mask is inconsistent, not eight faults.
    if(r.status==Status::Unverified&&r.raw==255&&!r.faults&&!r.states)return out.join(QStringLiteral(" ; "));
    for(unsigned bit=1;bit<=128;bit<<=1)if((r.faults|r.states|r.indicators)&bit){
        out<<I18n::text(labelKey(f,group,bit))+QStringLiteral(" (")+mask(bit)+QStringLiteral(")");
    }
    return out.join(QStringLiteral(" ; "));
}
inline QString raw(const Mems16Na::SessionEvidence *s,int group,unsigned historical) {
    if(!s||s->firmwareName().empty())return QString::number(historical);
    return rawText(s->extendedDtc(group));
}
}
