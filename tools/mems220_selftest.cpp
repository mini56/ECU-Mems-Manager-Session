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
    auto a=s.extended80();auto states=s.extended7d();
    a[11]=128;a[12]=0;a[27]=0;states[1]=64;states[32]=8;
    s.setExtended80(a);s.setExtended7d(states);m=Mems220::decode(s);
    assert(m.dn.calculated==1&&m.ac.calculated==1&&m.ignition80.calculated==1&&m.ignition7d.calculated==1&&m.fanRequest.calculated==1);
    a[27]=1;states[32]=7;s.setExtended80(a);s.setExtended7d(states);m=Mems220::decode(s);
    assert(m.ignition80.availability==Availability::Unverified&&m.ignition7d.availability==Availability::Unverified&&m.fanRequest.availability==Availability::Unverified);
    a[11]=0;a[12]=1;states[1]=0;states[32]=0;s.setExtended80(a);s.setExtended7d(states);m=Mems220::decode(s);
    assert(m.dn.calculated==0&&m.ac.calculated==0&&m.ignition7d.calculated==0&&m.fanRequest.calculated==0);
    auto wrong=fixture(0x99);assert(!Mems220::decode(wrong).profile);
    assert(!Mems217::injectionProfile(wrong)&&!Mems217::passiveProfile(wrong));
    assert(!Mems219::injectionAvailable(wrong)&&!Mems219::lambdaQualified(wrong));
    auto b=s.extended7d();b[29]++;s.setExtended7d(b);
    m=Mems220::decode(s);assert(m.pair&&m.throttleReference.availability==Availability::Unverified);
    b.pop_back();s.setExtended7d(b);assert(!Mems220::decode(s).pair);
    s.discardPair();m=Mems220::decode(s);assert(m.profile&&!m.pair&&m.idleError.availability==Availability::Missing);
    s.reset();assert(!Mems220::decode(s).profile);
    // Exact independent identity of the equivalent firmware.
    auto v=fixture();
    v.tx({0xd0});v.rx({0xd0,0xc2,0,2,1});v.finish();
    v.tx({0xd1});v.rx({0xd1,'K','B','H','B','V','0','0','2',0xc2,0,2,1});v.finish();
    auto orig=fixture();
    v.setExtended80(orig.extended80());v.setExtended7d(orig.extended7d());
    m=Mems220::decode(v);
    assert(m.profile&&m.pair&&m.hotPosition.calculated==32&&m.learnCount.calculated==7);
    assert(m.idleCorrection.calculated==-50&&m.idleError.calculated==-1&&m.throttleReference.calculated==42.5);
    assert(Mems217::passiveProfile(v)&&Mems217::injectionProfile(v));
    auto va=v.extended80();auto vb=v.extended7d();
    va[1]=3;va[2]=132;va[3]=100;va[5]=75;vb[6]=160;vb[9]=1;vb[10]=1;vb[11]=129;vb[12]=100;
    v.setExtended80(va);v.setExtended7d(vb);
    Mems217::Observation o;o.sample(v);
    assert(o.valid&&o.mv==800&&Mems219::injectionAvailable(v)&&Mems219::lambdaQualified(v));
    assert(Mems218::coolantSample(true,v).availability==Mems218::CoolantAvailability::Ready);
    v.discardPair();assert(!Mems220::decode(v).pair&&!Mems219::injectionAvailable(v));
    v.tx({0xd0});v.rx({0xd0,0x90,0,2,1});v.finish();
    v.tx({0xd1});v.rx({0xd1,'K','B','H','B','V','0','0','2',0x90,0,2,1});v.finish();
    assert(!Mems220::profile19(v)&&!Mems217::passiveProfile(v)&&!Mems217::injectionProfile(v));
    std::cout<<"SESSION_220_KBHBG009_ORDINARY_FRAMES_PASS\n";
}
