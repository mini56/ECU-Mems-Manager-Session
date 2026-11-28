#include "mems217_passive.h"
#include "ecureconnectpolicy.h"
#include <cassert>
#include <iostream>
int main(){
    using namespace Mems217;
    assert(injectionUs(127)==-8&&injectionUs(128)==0&&injectionUs(129)==8&&injectionUs(0)==-1024&&injectionUs(254)==1008);
    assert(idleRpm(127)==-25&&idleRpm(129)==25);
    Mems16Na::SessionEvidence s;
    s.tx({0xD0});s.rx({0xD0,0x98,0,2,2});s.finish();
    s.tx({0xD1});s.rx({0xD1,'A','A','N','M','P','0','0','2',0x98,0,2,2});s.finish();
    assert(injectionProfile(s));
    Observation o;
    auto sample=[&](unsigned t,unsigned mv,unsigned fault,unsigned loop,unsigned dtc){
        std::vector<std::uint8_t> a(29,0),b(33,0);
        a[0]=0x80;a[1]=28;a[2]=3;a[3]=132;a[4]=t;a[14]=dtc;
        b[0]=0x7d;b[1]=32;b[7]=mv/5;b[10]=fault;b[11]=loop;
        s.tx({0x80});s.rx(a);s.finish();s.tx({0x7d});s.rx(b);s.finish();o.sample(s);
    };
    sample(80,100,1,1,0);assert(o.valid&&!o.lambdaObserved);
    sample(81,800,1,1,0);sample(83,100,1,1,0);assert(o.lambdaObserved&&o.coolantObserved);
    sample(83,800,1,0,0);assert(!o.lambdaObserved&&o.crossings==0);
    sample(83,800,0,1,1);assert(o.lambdaFault&&o.coolantFault&&!o.coolantObserved);
    s.discardPair();o.sample(s);assert(!o.valid&&!o.lambdaObserved);
    // Distinct MEMS 1.9 firmware; raw data must be a current complete pair.
    Mems16Na::SessionEvidence n;
    n.tx({0xd0});n.rx({0xd0,0x99,0,9,1});n.finish();
    n.tx({0xd1});n.rx({0xd1,'K','B','H','B','G','0','0','9',0x99,0,9,1});n.finish();
    assert(!n.identifiedProfile()&&injectionProfile(n)&&passiveProfile(n));
    std::vector<std::uint8_t> a19(28,0),b19(32,0);
    a19[0]=28;a19[1]=3;a19[2]=132;a19[3]=100;
    b19[0]=32;b19[6]=160;b19[9]=1;b19[10]=1;b19[11]=129;
    n.setExtended80(a19);n.setExtended7d(b19);o.sample(n);
    assert(o.valid&&o.mv==800&&present(frame7d(n),11)&&injectionUs(frame7d(n).bytes[11])==8);
    n.discardPair();o.sample(n);assert(!o.valid&&!present(frame7d(n),11));
    EcuReconnectPolicy p;using O=EcuReconnectPolicy::Outcome;
    assert(p.startManual()&&p.established());assert(p.cancel());assert(p.closed()==O::None&&p.startManual());
    assert(p.established()&&p.lost()&&p.closed()==O::Retry);
    for(int i=1;i<=3;++i){assert(p.startRetry());assert(p.failed()==(i==3?O::Alert:O::Retry));}
    assert(p.idle()&&p.startManual());
    std::cout<<"SESSION_217_PASSIVE_CONVERSIONS_RECONNECT_OK\n";
}
