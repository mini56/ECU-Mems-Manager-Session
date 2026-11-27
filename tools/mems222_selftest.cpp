#include "mems222_sensor_observation.h"
#include <cassert>
#include <iostream>
using namespace Mems222;
int main(){
    Mems16Na::SessionEvidence s;
    s.tx({0xd0});s.rx({0xd0,0x90,0,9,1});s.finish();
    s.tx({0xd1});s.rx({0xd1,'K','B','H','B','G','0','0','9',0x90,0,9,1});s.finish();
    std::vector<std::uint8_t> a(28),b(33);a[0]=28;b[0]=33;
    a[5]=75;a[7]=100;a[9]=20;a[10]=0;
    s.setExtended80(a);s.setExtended7d(b);
    Observation o;auto x=sample(s);assert(x.valid&&x.stationary&&x.air==20);
    o.observe(x);assert(!o.switchResponse()&&!o.tps.changed());
    a[9]=120;a[10]=1;a[7]=90;a[5]=76;s.setExtended80(a);o.observe(sample(s));
    a[9]=20;a[10]=0;s.setExtended80(a);o.observe(sample(s));
    assert(o.switchResponse()&&o.air.changed()&&o.map.changed());
    a[14]=0x80;s.setExtended80(a);o.observe(sample(s));assert(!o.switchResponse()&&o.tps.stored);
    a[1]=3;s.setExtended80(a);o.observe(sample(s));assert(!o.switchKnown&&o.tps.count==0);
    a[5]=255;s.setExtended80(a);o.observe(sample(s));assert(o.air.count==0);
    s.discardPair();o.observe(sample(s));assert(!o.valid&&o.map.count==0);
    Mems16Na::SessionEvidence na;
    na.tx({0xd0});na.rx({0xd0,0x98,0,2,2});na.finish();
    na.tx({0xd1});na.rx({0xd1,'A','A','N','M','P','0','0','2',0x98,0,2,2});na.finish();
    std::vector<std::uint8_t> x80(29),x7d(33);
    x80[0]=0x80;x80[1]=28;x80[6]=85;x80[8]=99;x80[10]=32;
    x7d[0]=0x7d;x7d[1]=32;
    na.tx({0x80});na.rx(x80);na.finish();na.tx({0x7d});na.rx(x7d);na.finish();
    assert(sample(na).valid&&sample(na).air==30&&sample(na).tps==32);
    std::cout<<"SESSION_222_PASSIVE_SENSOR_OBSERVATIONS_OK\n";
}
