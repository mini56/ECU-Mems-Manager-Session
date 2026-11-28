#include "mems222_sensor_observation.h"
#include <cassert>
#include <iostream>
using namespace Mems16Na;
static void tx(SessionEvidence &s,unsigned command,const std::vector<std::uint8_t>& reply){
    s.tx({static_cast<std::uint8_t>(command)});s.rx(reply);s.finish();
}
int main(){
    std::size_t n=0;const auto *profiles=catalog(n);assert(n==123);
    for(std::size_t i=0;i<n;++i){
        const auto &p=profiles[i];SessionEvidence s;
        const std::vector<std::uint8_t> id={static_cast<std::uint8_t>(p.petaId>>16),static_cast<std::uint8_t>(p.petaId>>8),static_cast<std::uint8_t>(p.petaId),2};
        std::vector<std::uint8_t> d0={0xd0},d1={0xd1};
        d0.insert(d0.end(),id.begin(),id.end());
        const std::string name(p.firmware);
        d1.insert(d1.end(),name.begin(),name.end());d1.insert(d1.end(),id.begin(),id.end());
        tx(s,0xd0,d0);tx(s,0xd1,d1);assert(s.identifiedProfile()==&p);
        std::vector<std::uint8_t> a(29,0),b(33,0);
        a[0]=0x80;a[1]=28;a[2]=3;a[3]=132;a[4]=80;a[6]=85;a[8]=99;a[10]=32;
        a[12]=p.pnSourceMask;a[13]=0;a[28]=1;
        b[0]=0x7d;b[1]=32;b[7]=20;b[10]=1;b[11]=4;
        auto send=[&](){tx(s,0x80,a);tx(s,0x7d,b);};
        send();assert(s.ready()&&Mems217::passiveProfile(s));
        const auto m=s.measurements();
        assert(m.pn.availability==Availability::Available&&m.pn.calculated==0);
        assert(m.ac.availability==Availability::Available&&m.ac.calculated==1);
        assert(m.ignitionOff.availability==Availability::Available&&m.ignitionOff.calculated==1);
        Mems217::Observation o;o.sample(s);assert(o.valid&&o.lambdaValid&&o.coolantValid);
        Mems218::CoolantTest t;assert(t.start(Mems218::coolantSample(true,s)));
        a[4]=84;b[7]=160;send();o.sample(s);t.sample(Mems218::coolantSample(true,s));
        b[7]=20;send();o.sample(s);assert(o.lambdaObserved&&o.coolantObserved&&t.warmingObserved());
        b[10]=255;send();o.sample(s);assert(!o.lambdaValid&&o.coolantValid);
        a[2]=a[3]=0;a[11]=0;send();Mems222::Observation obs;obs.observe(Mems222::sample(s));
        a[10]=100;a[11]=16;a[6]=86;a[8]=90;send();obs.observe(Mems222::sample(s));
        a[10]=32;a[11]=0;send();obs.observe(Mems222::sample(s));
        assert(obs.valid&&obs.air.changed()&&obs.map.changed()&&obs.tps.changed()&&obs.switchResponse());
        s.discardPair();o.sample(s);assert(!o.valid&&!Mems222::sample(s).valid);
    }
    SessionEvidence unknown;assert(!Mems217::passiveProfile(unknown));
    std::cout<<"SESSION_231_NA_123_PASSIVE_PROFILES_PASS\n";
}
