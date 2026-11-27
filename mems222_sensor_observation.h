#pragma once
// Observation of already-received ordinary frames only. No new ECU request.
// No access to current fault/substitution RAM flags, no physical switch claim.
#include "mems220_measurements.h"
namespace Mems222 {
struct Sample {
    bool valid=false, stationary=false, airValid=false, switchValid=false;
    bool tpsStored=false, airStored=false, mapStored=false, idle=false;
    unsigned tps=0, map=0;
    int air=0;
};
inline Sample sample(const Mems16Na::SessionEvidence &s){
    using namespace Mems16Na;
    Sample r;
    const bool na=s.identifiedProfile()&&s.firmwareName()=="AANMP002";
    const bool nineteen=Mems220::profile19(s);
    const auto a=Mems217::frame80(s),b=Mems217::frame7d(s);
    if((!na&&!nineteen)||!valid(a,28)||!valid(b,na?32:33))return r;
    r.valid=true;r.stationary=word(a,1)==0;
    r.airValid=Mems219::temperatureAvailable(a.bytes[5]);
    r.air=Mems219::temperatureC(a.bytes[5]);
    r.tps=a.bytes[9];r.map=a.bytes[7];
    r.switchValid=a.bytes[10]<=1;r.idle=a.bytes[10]==0;
    r.airStored=(a.bytes[13]&2)!=0;
    r.tpsStored=(a.bytes[14]&0x80)!=0;
    r.mapStored=(a.bytes[14]&0x20)!=0;
    return r;
}
struct Range {
    unsigned count=0;
    int first=0,last=0,low=0,high=0;
    bool stored=false;
    void clear(){*this=Range();}
    void add(int v,bool fault){
        if(!count){first=low=high=v;}
        last=v;if(v<low)low=v;if(v>high)high=v;
        stored=stored||fault;if(count<1000000u)++count;
    }
    bool changed()const{return count>=2&&high>low;}
};
struct Observation {
    Range air,tps,map;
    bool valid=false,switchKnown=false,lastIdle=false;
    unsigned transitions=0;
    void reset(){*this=Observation();}
    void observe(const Sample &s){
        if(!s.valid){reset();return;}
        valid=true;
        if(s.airValid)air.add(s.air,s.airStored);else air.clear();
        // Only observe throttle progression and idle-state transitions with
        // the engine stopped. Restarting the engine discards this observation.
        if(!s.stationary){tps.clear();switchKnown=false;transitions=0;}
        else {
            tps.add(int(s.tps),s.tpsStored);
            if(s.switchValid){
                if(switchKnown&&lastIdle!=s.idle&&transitions<1000000u)++transitions;
                lastIdle=s.idle;switchKnown=true;
            }else {switchKnown=false;transitions=0;}
        }
        map.add(int(s.map),s.mapStored);
    }
    bool switchResponse()const{return switchKnown&&transitions>=2&&tps.changed()&&!tps.stored;}
};
}
