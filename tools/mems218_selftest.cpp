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
    tach.tx({0xd0});tach.rx({0xd0,0x90,0,0x29,3});tach.finish();
    tach.tx({0xd1});tach.rx({0xd1,'T','F','H','7','G','0','2','9',0x90,0,0x29,3});tach.finish();
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
    struct ProfileCase {const char *name; unsigned id; bool gauge;};
    const ProfileCase compatible[]={
        {"AANMP000",0x980000u,true},
        {"AANMP001",0x980001u,true},
        {"AANMP002",0x980002u,true},
        {"ABNMF030",0x900030u,true},
        {"ABNMF032",0x900032u,true},
        {"ABNMP002",0x990002u,true},
        {"ABNMP005",0x990005u,true},
        {"ABNMP006",0x990006u,true},
        {"ACNMF024",0x900024u,true},
        {"ACNMF026",0x900026u,true},
        {"ACNMP001",0x9A0001u,true},
        {"ACNMP003",0x9A0003u,true},
        {"ACNMP004",0x9A0004u,true},
        {"ACNMP005",0x9A0005u,true},
        {"EKDMF009",0x900009u,true},
        {"TAHHG019",0x900019u,false},
        {"TBH7G019",0x900019u,false},
        {"TFH7G028",0x900028u,false},
        {"TFH7G029",0x900029u,false},
        {"TAH7G018",0x900018u,false},
        {"REGI1371",0x901371u,false},
        {"TAHHV006",0xAD0006u,false},
        {"TFH7G025",0x900025u,false},
        {"TFHSG011",0x900011u,false},
        {"TFHSV000",0x510000u,false},
        {"TFHSV006",0x510006u,false},
        {"REGI1369",0x901369u,false},
        {"TFH7G026",0x900026u,false},
        {"TFH7G027",0x900027u,false},
        {"TFHSG012",0x900012u,false},
        {"TFHSG013",0x900013u,false},
        {"TFHSG014",0x900014u,false},
        {"TAHHG013",0x900013u,false},
        {"TAHHV003",0xAD0003u,false},
        {"TAHHV004",0xAD0004u,false},
        {"TAHHV005",0xAD0005u,false},
        {"TBH7G016",0x900016u,false},
        {"TBH7G017",0x900017u,false},
        {"TFHSV001",0x510001u,false},
        {"TFHSV002",0x510002u,false},
        {"TFHSV003",0x510003u,false},
        {"TFHSV004",0x510004u,false},
        {"TFHSV005",0x510005u,false},
        {"TAH7V002",0xA90002u,false},
        {"TAH7V003",0xA90003u,false},
        {"TAH7V004",0xA90004u,false},
        {"TAH8V001",0xA80001u,false},
        {"TBH7V003",0xA70003u,false},
        {"TBH7V004",0xA70004u,false},
        {"TBH7V005",0xA70005u,false},
        {"TBH7V006",0xA70006u,false},
        {"TBH7V007",0xA70007u,false},
        {"TFH7V004",0x440004u,false},
        {"TFH7V005",0x440005u,false},
        {"TFH7V006",0x440006u,false},
        {"TFH7V007",0x440007u,false}
    };
    static_assert(sizeof(compatible)/sizeof(compatible[0])==56,"All qualified profiles must be tested");
    for(const auto &p:compatible){
        const auto hi=static_cast<std::uint8_t>(p.id>>16);
        const auto mid=static_cast<std::uint8_t>(p.id>>8);
        const auto lo=static_cast<std::uint8_t>(p.id);
        Mems16Na::SessionEvidence e;
        e.tx({0xd0});e.rx({0xd0,hi,mid,lo,1});e.finish();
        std::vector<std::uint8_t> identity={0xd1};
        for(const char *c=p.name;*c;++c)identity.push_back(static_cast<std::uint8_t>(*c));
        identity.insert(identity.end(),{hi,mid,lo,1});
        e.tx({0xd1});e.rx(identity);e.finish();
        const auto action=p.gauge?InstrumentAction::GaugeMax:InstrumentAction::Tachometer;
        assert(instrumentSupported(action,e));
        assert(!instrumentSupported(p.gauge?InstrumentAction::Tachometer:InstrumentAction::GaugeMin,e));
        assert(!instrumentSafeSample(action,e));
        std::vector<std::uint8_t> a(28,0),b(32,0);
        a[0]=28;b[0]=32;b[1]=1;
        const auto setSample=[&](){
            if(p.gauge){
                std::vector<std::uint8_t> aa={0x80},bb={0x7d};
                aa.insert(aa.end(),a.begin(),a.end());bb.insert(bb.end(),b.begin(),b.end());
                e.tx({0x80});e.rx(aa);e.finish();e.tx({0x7d});e.rx(bb);e.finish();
            }else{e.setExtended80(a);e.setExtended7d(b);}
        };
        setSample();assert(instrumentSafeSample(action,e));
        a[25]=1;setSample();assert(!instrumentSafeSample(action,e));
        a[25]=0;a[2]=1;setSample();assert(!instrumentSafeSample(action,e));
        a[2]=0;b[1]=0;setSample();assert(!instrumentSafeSample(action,e));
        e.discardPair();assert(!instrumentSafeSample(action,e));
    }
    // A name alone, or a numerical identity alone, never authorizes a test.
    Mems16Na::SessionEvidence wrong;
    wrong.tx({0xd0});wrong.rx({0xd0,0x90,0,0x28,1});wrong.finish();
    wrong.tx({0xd1});wrong.rx({0xd1,'T','F','H','7','G','0','2','9',0x90,0,0x28,1});wrong.finish();
    assert(!instrumentSupported(InstrumentAction::Tachometer,wrong));
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
