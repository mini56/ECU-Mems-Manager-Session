#include "mems16na_decoder.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>
using namespace Mems16Na;
#define CHECK(x) do { if (!(x)) { std::cerr << "FAIL " << __LINE__ << ": " << #x << '\n'; return 1; } } while (0)
int main() {
    std::size_t n=0; const auto rows=catalog(n);
    std::set<std::string> names; std::set<unsigned> ids;
    unsigned fuel=0,repeat=0,ptc=0,vent=0,aux4=0,aux8=0,swap=0,alt=0;
    for(std::size_t i=0;i<n;++i) {
        const auto &p=rows[i];
        names.insert(p.firmware); ids.insert(p.petaId);
        fuel+=p.fuelTemperature; repeat+=p.repeatedThrottleReference;
        ptc+=p.dtc1Bit04==Circuit::PtcHeater;
        vent+=p.dtc1Bit04==Circuit::CoolingFans;
        aux4+=p.dtc1Bit04==Circuit::AuxiliaryPtc;
        aux8+=p.dtc1Bit08==Circuit::AuxiliaryPtc;
        swap+=p.dtc1Bit08==Circuit::PtcHeater; alt+=p.alternateTransmission;
        CHECK(select(true,p.firmware,true,p.petaId)==rows+i);
        CHECK(!select(false,p.firmware));
        CHECK(!select(true,p.firmware,true,p.petaId^1));
        CHECK(definition(&p,Group::Dtc1).faultMask==0xBE);
        CHECK(definition(&p,Group::Dtc2).stateMask==0x40);
        CHECK(p.dtc0Mask==(p.fuelTemperature?0x23:0x03));
    }
    CHECK(n==123 && names.size()==123 && ids.size()==107);
    CHECK(fuel==36 && repeat==16 && ptc==51 && vent==13 && aux4==56 && aux8==2 && swap==1 && alt==3);
    CHECK(candidateCount(0x900000)>=3);
    CHECK(!select(true,nullptr,true,0x900000));
    CHECK(!select(true,nullptr,true,0x980002));
    CHECK(!select(true,"TCF7X005") && !select(true,"ABNMP000"));
    auto p=select(true,"AANMP002",true,0x980002);
    CHECK(p);
    std::uint8_t a[28]={28},b[32]={32};
    Frame A{a,sizeof a}, B{b,sizeof b};
    a[4]=a[6]=255; a[9]=38; a[11]=0; a[12]=1; a[15]=8;
    a[16]=32; a[17]=134; a[27]=0;
    b[2]=18; b[6]=87; b[9]=1; b[10]=4; b[11]=127; b[15]=107; b[19]=126;
    b[20]=0x7F; b[21]=0xFF;
    b[23]=0x1B; b[24]=0; b[25]=0x1F; b[26]=0xC0;
    b[27]=0x27; b[28]=0xC0; b[29]=0x37; b[30]=0x80; b[31]=1;
    auto m=decode(p,A,B);
    CHECK(m.ambient.availability==Availability::NotUsed && m.ambient.raw==255 && m.fuel.availability==Availability::NotUsed);
    CHECK(m.pn.calculated==1 && m.ac.calculated==0 && m.ignitionOff.calculated==0);
    CHECK(m.idleAmplitude.calculated==50 && m.idleSigned.calculated==-50 && m.signedIdleConsistent);
    CHECK(qualifiedIdleSigned(m).calculated==-50);
    a[15]=9;
    auto mismatch=qualifiedIdleSigned(decode(p,A,B));
    CHECK(mismatch.availability==Availability::Unverified && mismatch.raw==126);
    a[15]=8;
    CHECK(qualifiedIdleSigned(decode(p,A,{nullptr,0})).availability==Availability::Missing);
    CHECK(m.idleHot.calculated==32 && std::abs(m.learnVoltage.calculated-13.4)<1e-9);
    CHECK(std::abs(m.throttleVoltage.calculated-38*5.0/255)<1e-9);
    CHECK(m.throttleAngle.calculated==9 && m.injectionTime.calculated==-8 && m.idleBase.calculated==107);
    CHECK(m.idleError.raw==32767 && m.idleError.calculated==-1);
    CHECK(m.throttleReference[0].calculated==27 && m.throttleReference[1].calculated==31.75);
    CHECK(m.throttleReference[2].calculated==39.75 && m.throttleReference[3].calculated==55.5 && m.learnCount.calculated==1);
    CHECK(m.lambdaMv.calculated==435 && m.lambdaFault.calculated==0 && m.closedLoop.calculated==1);
    a[11]=32; a[12]=0; a[27]=1; b[2]=19; b[11]=128; b[20]=128; b[21]=0;
    m=decode(p,A,B);
    CHECK(m.pn.calculated==0 && m.ac.calculated==1 && m.ignitionOff.calculated==1);
    CHECK(m.throttleAngle.calculated==9.5 && m.injectionTime.calculated==0 && m.idleError.calculated==0);
    b[11]=129; b[21]=1; b[9]=0; b[10]=0;
    m=decode(p,A,B); CHECK(m.injectionTime.calculated==8 && m.idleError.calculated==1 && m.lambdaFault.calculated==1 && m.closedLoop.calculated==0);
    CHECK(decode(p,{a,27},B).idleHot.availability==Availability::Missing);
    CHECK(decode(p,A,{b,14}).throttleAngle.availability==Availability::Missing);
    CHECK(decode(p,A,{b,33}).throttleAngle.availability==Availability::Missing);
    b[0]=14; CHECK(decode(p,A,B).throttleAngle.availability==Availability::Missing); b[0]=32;
    CHECK(decode(nullptr,A,B).idleHot.availability==Availability::Missing);
    CHECK(decodeDtc(p,Group::Dtc0,A,B).noFault());
    a[13]=3; CHECK(decodeDtc(p,Group::Dtc0,A,B).faultBits==3);
    a[13]=0x20; CHECK(decodeDtc(p,Group::Dtc0,A,B).unknownBits==0x20 && !decodeDtc(p,Group::Dtc0,A,B).noFault());
    auto frt=byFirmware("TAP8P007"); CHECK(frt && frt->fuelTemperature);
    CHECK(decodeDtc(frt,Group::Dtc0,A,B).faultBits==0x20);
    CHECK(decode(frt,A,B).fuel.availability==Availability::Available);
    a[14]=0xBE; CHECK(decodeDtc(p,Group::Dtc1,A,B).faultBits==0xBE);
    a[14]=0xA0; CHECK(decodeDtc(p,Group::Dtc1,A,B).faultBits==0xA0);
    CHECK(circuit(p,Group::Dtc1,0x20)==Circuit::MapSensor && circuit(p,Group::Dtc1,0x80)==Circuit::ThrottlePot);
    CHECK(circuit(p,Group::Dtc1,4)==Circuit::PtcHeater && circuit(p,Group::Dtc1,8)==Circuit::AirConditioning);
    auto ek=byFirmware("EKDMF002"); CHECK(circuit(ek,Group::Dtc1,4)==Circuit::AirConditioning && circuit(ek,Group::Dtc1,8)==Circuit::PtcHeater);
    CHECK(circuit(byFirmware("EKDMF008"),Group::Dtc1,4)==Circuit::CoolingFans);
    CHECK(circuit(byFirmware("EKDMF000"),Group::Dtc1,8)==Circuit::AuxiliaryPtc);
    CHECK(circuit(byFirmware("KBP8P005"),Group::Dtc1,4)==Circuit::AuxiliaryPtc);
    b[5]=0x40; auto d=decodeDtc(p,Group::Dtc2,A,B);
    CHECK(d.noFault() && d.faultBits==0 && d.stateBits==0x40);
    b[5]=0x54; d=decodeDtc(p,Group::Dtc2,A,B);
    CHECK(!d.noFault() && d.faultBits==0x14 && d.stateBits==0x40);
    b[5]=0x44; d=decodeDtc(p,Group::Dtc2,A,B);
    CHECK(d.faultBits==4 && d.stateBits==0x40);
    b[5]=1; CHECK(decodeDtc(p,Group::Dtc2,A,B).unknownBits==1 && !decodeDtc(p,Group::Dtc2,A,B).noFault());
    b[14]=b[16]=b[17]=b[22]=255;
    for(auto g:{Group::Dtc3,Group::Dtc4,Group::Dtc5,Group::Indirect7D10}) {
        d=decodeDtc(p,g,A,B); CHECK(d.availability==Availability::NotUsed && !d.noFault() && !d.faultBits);
    }
    b[14]=0; CHECK(decodeDtc(p,Group::Dtc3,A,B).availability==Availability::Unverified);
    CHECK(!decodeDtc(nullptr,Group::Dtc1,A,B).noFault());
    CHECK(decodeDtc(p,Group::Dtc2,A,{b,14}).availability==Availability::Missing);
    auto repeated=byFirmware("EKDMF008"); CHECK(repeated && repeated->repeatedThrottleReference);
    for(unsigned i=0;i<4;++i) {b[23+2*i]=0x1B;b[24+2*i]=0;}
    m=decode(repeated,A,B); for(auto v:m.throttleReference) CHECK(v.calculated==27);
    CHECK(m.pn.availability==(a[11]==repeated->pnSourceMask||a[11]==0?Availability::Available:Availability::Unverified));
    std::cout<<"MEMS16NA_SELFTEST_OK: 123 profiles, 107 PETA IDs, measurements and DTC\n";
}
