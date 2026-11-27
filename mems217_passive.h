#pragma once
#include <string>
#include <cstdint>
#include "mems16na_session_evidence.h"
namespace Mems217 {
inline bool injectionProfile(const Mems16Na::SessionEvidence &s) {
    // NA decoder already establishes the wire mapping; 1.9 only proven wire profiles.
    return s.identifiedProfile() || s.firmwareName()=="KBHBG009" || s.firmwareName()=="KBHBV001";
}
inline Mems16Na::Frame frame80(const Mems16Na::SessionEvidence &s) {
    if(s.identifiedProfile())return s.frame80();
    const auto &b=s.extended80(); return {b.data(),b.size()};
}
inline Mems16Na::Frame frame7d(const Mems16Na::SessionEvidence &s) {
    if(s.identifiedProfile())return s.frame7d();
    const auto &b=s.extended7d(); return {b.data(),b.size()};
}
inline bool present(Mems16Na::Frame f,unsigned i) {
    return f.bytes && f.size>i && f.bytes[0]==f.size;
}
inline int injectionUs(unsigned raw){return (int(raw)-128)*8;}
inline int idleRpm(unsigned raw){return (int(raw)-128)*25;}
inline bool passiveProfile(const std::string &f){return f=="AANMP002"||f=="KBHBG009";}
struct Observation {
    bool valid=false, lambdaFault=false, lambdaObserved=false, coolantFault=false, coolantObserved=false, closedLoop=false;
    unsigned mv=0, crossings=0, coolantRaw=0, low=0, high=0;
    int side=0;
    void reset(){*this=Observation();}
    void sample(const Mems16Na::SessionEvidence &s) {
        auto a=frame80(s), b=frame7d(s);
        if(!passiveProfile(s.firmwareName()) || !present(a,14) || !present(b,10) ||
           Mems16Na::word(a,1)==0 || b.bytes[9]>1 || b.bytes[10]>1){reset();return;}
        const unsigned t=a.bytes[3];
        if(t==0||t>=250){reset();return;}
        const bool first=!valid;
        valid=true; coolantRaw=t; mv=unsigned(b.bytes[6])*5;
        coolantFault=(a.bytes[13]&1)!=0; // stored circuit fault, not proof of current substitution
        if(first||coolantFault){low=high=t;coolantObserved=false;}
        else {if(t<low)low=t;if(t>high)high=t;coolantObserved=high-low>=3;}
        closedLoop=b.bytes[10]==1;
        lambdaFault=b.bytes[9]==0;
        if(lambdaFault||!closedLoop){crossings=0;side=0;lambdaObserved=false;return;}
        // Deliberately conservative observation bands, not a Rover pass/fail spec.
        const int next=mv<=300?-1:mv>=600?1:0;
        if(next){if(side&&side!=next&&crossings<1000000)++crossings;side=next;}
        lambdaObserved=crossings>=2;
    }
};
}
