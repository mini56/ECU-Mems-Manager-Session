#pragma once
// Read-only qualification and observation, independent of serial transport.
#include "mems217_passive.h"
namespace Mems218 {
enum class GaugeState { Disconnected, Unidentified, Unsupported, AwaitData, EngineRunning, AwaitBench };
inline bool gaugePathDocumented(const std::string &f) {
    return f=="AANMP002"||f=="AANMP000"||f=="ABNMF032"||
           f=="ABNMP005"||f=="ACNMF024"||f=="ACNMF026";
}
inline GaugeState gaugeState(bool connected,const Mems16Na::SessionEvidence *s) {
    if(!connected)return GaugeState::Disconnected;
    if(!s||s->firmwareName().empty())return GaugeState::Unidentified;
    const auto a=Mems217::frame80(*s);
    if(Mems217::present(a,2)&&Mems16Na::word(a,1)>0)return GaugeState::EngineRunning;
    if(!s->identifiedProfile()||!gaugePathDocumented(s->firmwareName()))return GaugeState::Unsupported;
    if(!Mems217::present(a,25))return GaugeState::AwaitData;
    if(a.bytes[25]!=0)return GaugeState::EngineRunning;
    // Static proof is not physical qualification. No enabled state or transport
    // exists until bench restoration and the actual vehicle wiring are checked.
    return GaugeState::AwaitBench;
}
// Active instrument tests: only the exact firmware whose output path and
// independent firmware watchdog/restoration have both been traced.
// All other profiles stay unsupported, even if the same opcode is present.
enum class InstrumentAction : int { None=0, GaugeMin=1, GaugeMax=2, Tachometer=3 };
inline bool instrumentSupported(InstrumentAction action,const Mems16Na::SessionEvidence &s) {
    if(action==InstrumentAction::GaugeMin||action==InstrumentAction::GaugeMax)
        return s.firmwareName()=="AANMP002" && s.identifiedProfile()!=nullptr;
    if(action==InstrumentAction::Tachometer)
        return s.firmwareName()=="TFH7G029" && s.identifiedProfile()==nullptr;
    return false;
}
inline bool instrumentSafeSample(InstrumentAction action,const Mems16Na::SessionEvidence &s) {
    if(!instrumentSupported(action,s))return false;
    const auto a=Mems217::frame80(s),b=Mems217::frame7d(s);
    if(!Mems217::present(a,25)||!Mems217::present(b,1))return false;
    // Use the actual engine speed, not the requested tachometer indication.
    if(Mems16Na::word(a,1)!=0||a.bytes[25]!=0||b.bytes[1]!=1)return false;
    if(action!=InstrumentAction::Tachometer &&
       (!Mems217::present(a,27)||a.bytes[27]!=0))return false;
    return true;
}
inline unsigned instrumentCommand(InstrumentAction action) {
    switch(action){
        case InstrumentAction::GaugeMin:return 0x00u;
        case InstrumentAction::GaugeMax:return 0x10u;
        case InstrumentAction::Tachometer:return 0x6Bu;
        default:return 0x100u;
    }
}
constexpr unsigned instrumentDurationMs=1500;
constexpr unsigned instrumentMaximumSampleMs=400;
constexpr unsigned instrumentRestoreCommand=0xF4u;

enum class CoolantAvailability { Disconnected, Unsupported, AwaitData, EngineStopped, Ready };
struct CoolantSample {
    CoolantAvailability availability=CoolantAvailability::AwaitData;
    int celsius=0;
    bool storedFault=false;
};
inline CoolantSample coolantSample(bool connected,const Mems16Na::SessionEvidence &s) {
    CoolantSample out;
    if(!connected){out.availability=CoolantAvailability::Disconnected;return out;}
    if(!Mems217::passiveProfile(s.firmwareName())){
        out.availability=CoolantAvailability::Unsupported;return out;
    }
    const auto a=Mems217::frame80(s);
    // Unlike the SESSION 217 joint observer, coolant does not depend on
    // valid lambda flags, loop state or the measured lambda voltage.
    if(!Mems217::present(a,14)||!Mems217::present(Mems217::frame7d(s),5))return out;
    if(Mems16Na::word(a,1)==0){out.availability=CoolantAvailability::EngineStopped;return out;}
    if(a.bytes[3]==0||a.bytes[3]>=250)return out;
    out.availability=CoolantAvailability::Ready;
    out.celsius=int(a.bytes[3])-55;
    out.storedFault=(a.bytes[13]&1)!=0;
    return out;
}
enum class TestState { Idle, Running, Finished, Interrupted };
struct CoolantTest {
    TestState state=TestState::Idle;
    int initial=0,current=0,minimum=0,maximum=0;
    bool storedFaultSeen=false;
    unsigned samples=0;
    void reset(){*this=CoolantTest();}
    bool start(const CoolantSample &s){
        if(state==TestState::Running||s.availability!=CoolantAvailability::Ready)return false;
        reset();state=TestState::Running;
        initial=current=minimum=maximum=s.celsius;storedFaultSeen=s.storedFault;samples=1;
        return true;
    }
    void interrupt(){if(state==TestState::Running)state=TestState::Interrupted;}
    void stop(){if(state==TestState::Running)state=TestState::Finished;}
    void sample(const CoolantSample &s){
        if(state!=TestState::Running)return;
        if(s.availability!=CoolantAvailability::Ready){interrupt();return;}
        current=s.celsius;if(current<minimum)minimum=current;if(current>maximum)maximum=current;
        storedFaultSeen=storedFaultSeen||s.storedFault;
        if(samples<100000000u)++samples;
    }
    // Indicative 3-degree warming, not a manufacturer test threshold.
    bool warmingObserved()const{return samples>=2&&!storedFaultSeen&&maximum-initial>=3;}
};
}
