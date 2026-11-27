#pragma once
// Read-only decode of ordinary, coherent 80/7D frames. No serial I/O.
// KBHBG009: XF3 ID 90 00 09; ROM table 3C54/58, 3CFE, 3D02,
// 3D0A/0E/12/16 and 3D1A. Do not extrapolate to other 1.9 variants.
#include "mems219_units.h"
namespace Mems220 {
inline bool profile19(const Mems16Na::SessionEvidence &s){
    const auto &id=s.identity();
    return s.firmwareName()=="KBHBG009"&&id[0]==0x90&&id[1]==0&&id[2]==9;
}
struct Measurements {
    bool profile=false, pair=false;
    Mems16Na::Value hotPosition=Mems16Na::missing(), learnVoltage=Mems16Na::missing(),
        idleCorrection=Mems16Na::missing(), idleError=Mems16Na::missing(),
        throttleReference=Mems16Na::missing(), learnCount=Mems16Na::missing();
};
inline Measurements decode(const Mems16Na::SessionEvidence &s){
    using namespace Mems16Na;
    Measurements m;
    m.profile=profile19(s);
    if(!m.profile)return m;
    const auto a=Mems217::frame80(s),b=Mems217::frame7d(s);
    if(!valid(a,28)||!valid(b,33))return m;
    m.pair=true;
    m.hotPosition=available(a.bytes[16],a.bytes[16]);
    m.learnVoltage=available(a.bytes[17],a.bytes[17]*0.1);
    m.idleCorrection=available(b.bytes[19],Mems217::idleRpm(b.bytes[19]));
    const unsigned error=word(b,20);
    m.idleError=available(error,int(error)-32768);
    const unsigned ref=word(b,23);
    bool repeated=true;
    for(unsigned i=1;i<4;++i)repeated=repeated&&word(b,23+2*i)==ref;
    m.throttleReference=repeated?available(ref,ref/256.0):unverified(ref);
    m.learnCount=available(b.bytes[31],b.bytes[31]);
    return m;
}
}
