#include "legacyhotidle.h"
#include "mems219_units.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace Mems219;
int main(){
    // Historical user convention: current hot-idle adjustment, never hard-code -3.
    assert(LegacyHotIdle::raw(0x80,0)==32768);
    assert(LegacyHotIdle::corrected(32768,32)==3);
    assert(LegacyHotIdle::corrected(32768,35)==0);
    assert(LegacyHotIdle::corrected(32768,38)==-3);
    for(int raw=0;raw<=65535;++raw){
        assert(LegacyHotIdle::raw(raw>>8,raw&255)==raw);
        for(int hot=0;hot<=255;++hot)
            assert(LegacyHotIdle::corrected(raw,hot)==raw-32768-hot+35);
    }
    assert(temperatureC(55)==0&&temperatureC(35)==-20);
    assert(std::abs(celsiusToFahrenheit(90)-194)<1e-9);
    assert(std::abs(deltaFahrenheit(5)-9)<1e-9);
    assert(!temperatureAvailable(0)&&!temperatureAvailable(255)&&temperatureAvailable(145));
    assert(std::abs(batteryV(138)-13.8)<1e-9&&lambdaMv(180)==900);
    assert(std::abs(coilMs(1250)-2.5)<1e-9);
    assert(ignitionAdjustmentDeg(47)==-0.5&&ignitionAdjustmentDeg(51)==1.5);
    assert(std::abs(legacyThrottleDeg(13)-7.8)<1e-9);
    assert(Mems217::injectionUs(127)==-8&&Mems217::injectionUs(129)==8);
    Mems16Na::SessionEvidence s;assert(!injectionAvailable(s)&&!lambdaQualified(s));
    assert(correctionReading(s,false).kind==CorrectionKind::Missing);
    for(unsigned v=0;v<256;++v){
        std::vector<std::uint8_t> b(14);b[0]=14;b[11]=static_cast<std::uint8_t>(v);b[12]=static_cast<std::uint8_t>(v);
        s.setExtended7d(b);
        auto longRaw=correctionReading(s,false),shortRaw=correctionReading(s,true);
        assert(longRaw.kind==CorrectionKind::Historical&&longRaw.value==int(v)-128);
        assert(shortRaw.kind==CorrectionKind::Historical&&shortRaw.value==int(v)-100);
        assert(!injectionAvailable(s)&&!lambdaQualified(s));
        b.resize(11);b[0]=11;s.setExtended7d(b);
        assert(correctionReading(s,false).kind==CorrectionKind::Missing&&correctionReading(s,true).kind==CorrectionKind::Missing);
    }
    s.discardPair();
    s.tx({0xd0});s.rx({0xd0,0x90,0,9,1});s.finish();
    s.tx({0xd1});s.rx({0xd1,'K','B','H','B','G','0','0','9',0x90,0,9,1});s.finish();
    std::vector<std::uint8_t> proven(33);proven[0]=33;proven[11]=129;proven[12]=103;s.setExtended7d(proven);
    assert(correctionReading(s,false).kind==CorrectionKind::Qualified&&correctionReading(s,false).value==8);
    assert(correctionReading(s,true).kind==CorrectionKind::Qualified&&correctionReading(s,true).value==3);
    s.discardPair();assert(correctionReading(s,false).kind==CorrectionKind::Missing);
    GaugeSimulation g;
    assert(g.request(GaugeIntent::Minimum,false)==-1);
    assert(g.request(GaugeIntent::Minimum,true)==0x00);
    assert(g.request(GaugeIntent::Maximum,true)==-1);
    assert(g.request(GaugeIntent::MotorStarted,false)==0xf4);
    assert(g.request(GaugeIntent::Maximum,true)==-1);
    g.restorationObserved(true);
    assert(g.request(GaugeIntent::Maximum,true)==0x10);
    assert(g.request(GaugeIntent::Stop,true)==0xf4);
    g.restorationObserved(false);
    assert(g.request(GaugeIntent::Minimum,true)==-1);
    GaugeSimulation lost;
    assert(lost.request(GaugeIntent::Maximum,true)==0x10);
    assert(lost.request(GaugeIntent::LostLink,false)==-1);
    assert(lost.request(GaugeIntent::Minimum,true)==-1);
    std::cout<<"SESSION_219_UNITS_AND_OFFLINE_GAUGE_SEQUENCE_PASS\n";
}
