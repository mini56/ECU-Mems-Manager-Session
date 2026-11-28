#pragma once
#include "mems218_tests.h"
namespace Mems219 {
inline double celsiusToFahrenheit(double c){return c*1.8+32.0;}
inline double deltaFahrenheit(double c){return c*1.8;}
inline int temperatureC(unsigned raw){return int(raw)-55;}
inline double batteryV(unsigned raw){return raw/10.0;}
inline unsigned lambdaMv(unsigned raw){return raw*5u;}
inline double coilMs(unsigned raw){return raw*0.002;}
inline double ignitionAdjustmentDeg(unsigned raw){return (int(raw)-48)*0.5;}
inline double legacyThrottleDeg(unsigned raw){return raw*0.6;}
inline bool temperatureAvailable(unsigned raw){return raw>0&&raw<250;}
inline bool lambdaQualified(const Mems16Na::SessionEvidence &s){
    return Mems217::injectionProfile(s)&&(!s.identifiedProfile()||!s.identifiedProfile()->alternateTransmission)
        &&Mems217::present(Mems217::frame7d(s),12);
}
inline bool injectionAvailable(const Mems16Na::SessionEvidence &s){
    return Mems217::injectionProfile(s)&&Mems217::present(Mems217::frame7d(s),11);
}
// Historic offsets are information only: do not use as physical units or diagnostic thresholds.
enum class CorrectionKind { Missing, Unqualified, Historical, Qualified };
struct CorrectionReading {
    CorrectionKind kind;
    int value;
};
inline CorrectionReading correctionReading(const Mems16Na::SessionEvidence &s,bool lambda){
    const auto b=Mems217::frame7d(s);
    if(!Mems217::present(b,lambda?12:11))return {CorrectionKind::Missing,0};
    if(lambda&&s.identifiedProfile()&&s.identifiedProfile()->alternateTransmission)
        return {CorrectionKind::Unqualified,0};
    const unsigned raw=b.bytes[lambda?12:11];
    if(lambda?lambdaQualified(s):injectionAvailable(s))
        return {CorrectionKind::Qualified,lambda?int(raw)-100:Mems217::injectionUs(raw)};
    return {CorrectionKind::Historical,int(raw)-(lambda?100:128)};
}
// Offline test plan only: no serial transport or physical qualification.
enum class GaugeIntent { Minimum, Maximum, Stop, MotorStarted, LostLink };
enum class GaugePhase { Idle, Forcing, RestorePending, ReturnUnconfirmed };
struct GaugeSimulation {
    GaugePhase phase=GaugePhase::Idle;
    int request(GaugeIntent action,bool qualifiedStationary){
        if(action==GaugeIntent::LostLink){
            if(phase!=GaugePhase::Idle)phase=GaugePhase::ReturnUnconfirmed;
            return -1;
        }
        if(action==GaugeIntent::Stop||action==GaugeIntent::MotorStarted){
            if(phase!=GaugePhase::Forcing)return -1;
            phase=GaugePhase::RestorePending;return 0xf4;
        }
        if(!qualifiedStationary||phase!=GaugePhase::Idle)return -1;
        phase=GaugePhase::Forcing;
        return action==GaugeIntent::Minimum?0x00:0x10;
    }
    void restorationObserved(bool physicalReturn){
        if(phase==GaugePhase::RestorePending)
            phase=physicalReturn?GaugePhase::Idle:GaugePhase::ReturnUnconfirmed;
    }
};
}
