#include "mems218_tests.h"
#include <cassert>
#include <iostream>
using namespace Mems218;
static Mems16Na::SessionEvidence sample(unsigned temp,unsigned rpm,unsigned fault,unsigned running=0){
    Mems16Na::SessionEvidence s;
    s.tx({0xd0});s.rx({0xd0,0x98,0,2,2});s.finish();
    s.tx({0xd1});s.rx({0xd1,'A','A','N','M','P','0','0','2',0x98,0,2,2});s.finish();
    std::vector<std::uint8_t> a(29,0),b(33,0);
    a[0]=0x80;a[1]=28;a[2]=rpm>>8;a[3]=rpm&255;a[4]=temp;a[14]=fault;a[26]=running;
    b[0]=0x7d;b[1]=32;b[2]=1;b[10]=255;b[11]=255;
    s.tx({0x80});s.rx(a);s.finish();s.tx({0x7d});s.rx(b);s.finish();return s;
}
int main(){
    auto cold=sample(75,900,0),warm=sample(80,900,0),stopped=sample(80,0,0);
    assert(gaugeState(false,&cold)==GaugeState::Disconnected);
    assert(gaugeState(true,nullptr)==GaugeState::Unidentified);
    assert(gaugeState(true,&cold)==GaugeState::EngineRunning);
    assert(gaugeState(true,&stopped)==GaugeState::AwaitBench);
    auto crank=sample(80,0,0,1);assert(gaugeState(true,&crank)==GaugeState::EngineRunning);
    auto invalid=stopped;invalid.discardPair();assert(gaugeState(true,&invalid)==GaugeState::AwaitData);
    for(const char *f:{"AANMP002","AANMP000","ABNMF032","ABNMP005","ACNMF024","ACNMF026"})assert(gaugePathDocumented(f));
    assert(!gaugePathDocumented("KBHBG009")&&!gaugePathDocumented("EKDMF008"));
    assert(instrumentSupported(InstrumentAction::GaugeMin,stopped));
    assert(instrumentSafeSample(InstrumentAction::GaugeMin,stopped));
    assert(instrumentSafeSample(InstrumentAction::GaugeMax,stopped));
    assert(!instrumentSafeSample(InstrumentAction::GaugeMax,cold));
    assert(!instrumentSafeSample(InstrumentAction::GaugeMin,crank));
    assert(!instrumentSafeSample(InstrumentAction::GaugeMin,invalid));
    assert(!instrumentSupported(InstrumentAction::Tachometer,stopped));
    assert(instrumentCommand(InstrumentAction::GaugeMin)==0);
    assert(instrumentCommand(InstrumentAction::GaugeMax)==0x10);
    assert(instrumentCommand(InstrumentAction::Tachometer)==0x6b);
    assert(instrumentCommand(InstrumentAction::None)>255);
    Mems16Na::SessionEvidence tach;
    tach.tx({0xd0});tach.rx({0xd0,0x99,1,2,3});tach.finish();
    tach.tx({0xd1});tach.rx({0xd1,'T','F','H','7','G','0','2','9',0x99,1,2,3});tach.finish();
    std::vector<std::uint8_t> a19(28,0),b19(33,0);
    a19[0]=28;b19[0]=33;b19[1]=1;
    tach.setExtended80(a19);tach.setExtended7d(b19);
    assert(instrumentSafeSample(InstrumentAction::Tachometer,tach));
    assert(!instrumentSupported(InstrumentAction::GaugeMin,tach));
    b19[1]=0;tach.setExtended7d(b19);
    assert(!instrumentSafeSample(InstrumentAction::Tachometer,tach));
    b19[1]=1;a19[2]=1;tach.setExtended80(a19);tach.setExtended7d(b19);
    assert(!instrumentSafeSample(InstrumentAction::Tachometer,tach));
    tach.discardPair();assert(!instrumentSafeSample(InstrumentAction::Tachometer,tach));
    CoolantTest t;
    assert(!t.start(coolantSample(true,stopped)));
    assert(t.start(coolantSample(true,cold))&&t.initial==20);
    t.sample(coolantSample(true,warm));assert(t.current==25&&t.warmingObserved());
    t.stop();t.sample(coolantSample(true,cold));assert(t.current==25&&t.state==TestState::Finished);
    assert(t.start(coolantSample(true,cold)));
    auto fault=sample(82,900,1);t.sample(coolantSample(true,fault));
    t.sample(coolantSample(true,warm));assert(t.storedFaultSeen&&!t.warmingObserved());
    t.sample(coolantSample(true,stopped));assert(t.state==TestState::Interrupted);
    assert(!t.start(coolantSample(false,cold)));
    assert(t.start(coolantSample(true,cold)));t.sample(coolantSample(true,invalid));
    assert(t.state==TestState::Interrupted);
    std::cout<<"SESSION_218_GAUGE_INTERLOCK_COOLANT_INDEPENDENT_PASS\n";
}
