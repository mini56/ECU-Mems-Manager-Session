#include "mems219_units.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace Mems219;
int main(){
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
