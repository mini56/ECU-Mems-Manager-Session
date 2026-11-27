#include "mems220_measurements.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace Mems16Na;
static SessionEvidence fixture(unsigned id0=0x90){
    SessionEvidence s;
    s.tx({0xd0});s.rx({0xd0,static_cast<std::uint8_t>(id0),0,9,1});s.finish();
    s.tx({0xd1});s.rx({0xd1,'K','B','H','B','G','0','0','9',static_cast<std::uint8_t>(id0),0,9,1});s.finish();
    std::vector<std::uint8_t> a(28),b(33);
    a[0]=28;a[16]=32;a[17]=143;b[0]=33;b[19]=126;b[20]=0x7f;b[21]=0xff;b[31]=7;
    for(unsigned i=0;i<4;++i){b[23+2*i]=42;b[24+2*i]=128;}
    s.setExtended80(a);s.setExtended7d(b);return s;
}
int main(){
    auto s=fixture();auto m=Mems220::decode(s);
    assert(m.profile&&m.pair&&m.hotPosition.calculated==32);
    assert(std::abs(m.learnVoltage.calculated-14.3)<1e-9);
    assert(m.idleCorrection.calculated==-50&&m.idleError.calculated==-1);
    assert(m.throttleReference.calculated==42.5&&m.learnCount.calculated==7);
    auto wrong=fixture(0x99);assert(!Mems220::decode(wrong).profile);
    auto b=s.extended7d();b[29]++;s.setExtended7d(b);
    m=Mems220::decode(s);assert(m.pair&&m.throttleReference.availability==Availability::Unverified);
    b.pop_back();s.setExtended7d(b);assert(!Mems220::decode(s).pair);
    s.discardPair();m=Mems220::decode(s);assert(m.profile&&!m.pair&&m.idleError.availability==Availability::Missing);
    s.reset();assert(!Mems220::decode(s).profile);
    std::cout<<"SESSION_220_KBHBG009_ORDINARY_FRAMES_PASS\n";
}
