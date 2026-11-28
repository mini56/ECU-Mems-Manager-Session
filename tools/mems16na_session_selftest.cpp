#include "mems16na_session_evidence.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
using namespace Mems16Na;
#define CHECK(x) do { if(!(x)){std::cerr<<"FAIL "<<__LINE__<<": "<<#x<<'\n';return 1;} }while(0)
using Bytes=std::vector<std::uint8_t>;
static Bytes ident(){return {0xD0,0x98,0,2,2};}
static Bytes named(const std::string& name="AANMP002",unsigned repeats=3){
    Bytes b{0xD1};
    for(unsigned i=0;i<repeats;++i){b.insert(b.end(),name.begin(),name.end());b.insert(b.end(),{0x98,0,2,2});}
    return b;
}
static void transaction(SessionEvidence &s,unsigned c,const Bytes& b){
    s.tx({static_cast<std::uint8_t>(c)});s.rx(b);s.finish();
}
static Bytes packet(unsigned c,unsigned length){
    Bytes b(length+1,0); b[0]=static_cast<std::uint8_t>(c); b[1]=static_cast<std::uint8_t>(length);return b;
}
int main(int argc,char**argv){
    // SESSION 230: every known NA firmware, including duplicate PETA IDs.
    std::size_t profileCount=0;
    const auto *profiles=catalog(profileCount);
    CHECK(profileCount==123);
    for(std::size_t i=0;i<profileCount;++i) {
        const auto &p=profiles[i];
        SessionEvidence each;
        const Bytes id{static_cast<std::uint8_t>(p.petaId>>16),
                       static_cast<std::uint8_t>(p.petaId>>8),
                       static_cast<std::uint8_t>(p.petaId),2};
        Bytes d0{0xD0};d0.insert(d0.end(),id.begin(),id.end());
        Bytes d1{0xD1};
        const std::string name(p.firmware);
        d1.insert(d1.end(),name.begin(),name.end());
        d1.insert(d1.end(),id.begin(),id.end());
        each.observeCoolantFault(true,500);
        CHECK(!each.coolantFaultAtFirstSample()&&!each.coolantFaultSeenAt());
        transaction(each,0xD0,d0);
        transaction(each,0xD1,d1);
        CHECK(each.identifiedProfile()==&p);
        auto a=packet(0x80,28),b=packet(0x7D,32);
        a[14]=1; // response echo precedes frame byte 13 (DTC0)
        transaction(each,0x80,a);transaction(each,0x7D,b);
        CHECK(each.ready());
        const auto dtc=each.dtc(Group::Dtc0);
        CHECK(dtc.availability==Availability::Available && dtc.faultBits==1);
        CHECK(circuit(&p,Group::Dtc0,1)==Circuit::CoolantSensor);
        each.observeCoolantFault(true,1000);
        CHECK(each.coolantFaultAtFirstSample()&&!each.coolantFaultSeenAt());
        each.observeCoolantFault(true,2000);
        CHECK(!each.coolantFaultSeenAt());
        each.observeCoolantFault(false,3000);
        each.observeCoolantFault(true,4000);
        each.observeCoolantFault(false,5000);
        each.observeCoolantFault(true,6000);
        CHECK(each.coolantFaultSeenAt()==4000);
        each.reset();
        CHECK(!each.coolantFaultSeenAt()&&!each.coolantFaultAtFirstSample());
        transaction(each,0xD0,d0);transaction(each,0xD1,d1);
        each.observeCoolantFault(false,7000);
        each.observeCoolantFault(true,8000);
        CHECK(!each.coolantFaultAtFirstSample()&&each.coolantFaultSeenAt()==8000);
        transaction(each,0xD0,d0);
        CHECK(!each.coolantFaultSeenAt()&&!each.coolantFaultAtFirstSample());
        d1.back()=3; // mismatched D0/D1 calibration identity
        transaction(each,0xD1,d1);
        CHECK(!each.identifiedProfile());
        each.observeCoolantFault(true,9000);
        CHECK(!each.coolantFaultAtFirstSample()&&!each.coolantFaultSeenAt());
    }
    std::cout<<"MEMS16NA_COOLANT_HISTORY_123_PROFILES_PASS\n";
    SessionEvidence s;
    transaction(s,0xD0,ident());
    CHECK(!s.identifiedProfile());
    transaction(s,0xD1,named());
    CHECK(s.identifiedProfile() && std::string(s.identifiedProfile()->firmware)=="AANMP002");
    CHECK(!s.ready());
    s.observeCoolantFault(true,1000);
    CHECK(s.coolantFaultAtFirstSample() && !s.coolantFaultSeenAt());
    s.observeCoolantFault(true,2000);
    CHECK(!s.coolantFaultSeenAt());
    s.observeCoolantFault(false,3000);
    s.observeCoolantFault(true,4000);
    s.observeCoolantFault(false,5000);
    s.observeCoolantFault(true,6000);
    CHECK(s.coolantFaultSeenAt()==4000);
    s.reset();
    CHECK(!s.coolantFaultSeenAt() && !s.coolantFaultAtFirstSample());
    transaction(s,0xD0,ident()); transaction(s,0xD1,named());
    s.observeCoolantFault(false,7000); s.observeCoolantFault(true,8000);
    CHECK(!s.coolantFaultAtFirstSample() && s.coolantFaultSeenAt()==8000);
    auto a=packet(0x80,28),b=packet(0x7D,32);
    transaction(s,0x80,a);transaction(s,0x7D,b);CHECK(s.ready());
    CHECK(s.dtc(Group::Dtc0).noFault());
    transaction(s,0x80,a);transaction(s,0x7D,packet(0x7D,14));CHECK(!s.ready());
    transaction(s,0x80,a);transaction(s,0x7D,packet(0x7D,33));CHECK(!s.ready());
    auto broken=b; broken[1]=14;
    transaction(s,0x80,a);transaction(s,0x7D,broken);CHECK(!s.ready());
    transaction(s,0x80,a);transaction(s,0x91,{0x91,1});transaction(s,0x7D,b);CHECK(!s.ready());
    auto corrupt=named();corrupt.back()=3;transaction(s,0xD1,corrupt);CHECK(!s.identifiedProfile());
    transaction(s,0xD1,named("AANMP000"));CHECK(!s.identifiedProfile());
    transaction(s,0xD1,named("TCF7X005"));CHECK(!s.identifiedProfile());
    auto nul=named();nul[3]=0;transaction(s,0xD1,nul);CHECK(!s.identifiedProfile());
    auto extra=named();extra.push_back(0);transaction(s,0xD1,extra);CHECK(!s.identifiedProfile());
    transaction(s,0xD1,named());CHECK(s.identifiedProfile());
    s.reset();CHECK(!s.identifiedProfile()&&!s.ready());
    transaction(s,0xD1,named());CHECK(!s.identifiedProfile());
    transaction(s,0xD0,{0xD0,0x90,0,0,2});CHECK(!s.identifiedProfile());
    transaction(s,0xD1,named());CHECK(!s.identifiedProfile());
    CHECK(argc==2);
    std::ifstream file(argv[1]);CHECK(file.good());
    s.reset();std::string line;int command=-1;Bytes reply;unsigned pairs=0,unidentified=0;
    // Capture transactions are finalized at the next TX or end of replay.
    auto close=[&](){
        if(command<0)return true;
        s.finish();
        if(command==0x7D){
            if(!s.identifiedProfile()){++unidentified;return !s.ready();}
            if(!s.ready())return false;
            ++pairs;const auto m=s.measurements();
            if(m.ambient.availability!=Availability::NotUsed || m.fuel.availability!=Availability::NotUsed ||
               m.idleHot.calculated!=32 || std::abs(m.learnVoltage.calculated-13.4)>1e-9 ||
               m.throttleReference[0].calculated!=27 || m.throttleReference[1].calculated!=31.75 ||
               m.throttleReference[2].calculated!=39.75 || m.throttleReference[3].calculated!=55.5 ||
               m.learnCount.calculated!=1 || m.idleError.calculated!=int(m.idleError.raw)-32768 ||
               !s.dtc(Group::Dtc0).noFault() || !s.dtc(Group::Dtc1).noFault() ||
               !s.dtc(Group::Dtc2).noFault())return false;
        }
        return true;
    };
    while(std::getline(file,line)){
        std::istringstream in(line);std::string dir,hex;in>>dir;Bytes bytes;
        while(in>>hex){unsigned v=std::stoul(hex,nullptr,16);CHECK(v<=255);bytes.push_back(static_cast<std::uint8_t>(v));}
        if(dir=="TX"){CHECK(close());CHECK(bytes.size()==1);command=bytes[0];s.tx(bytes);}
        else if(dir=="RX")s.rx(bytes);
        else CHECK(false);
    }
    CHECK(close());CHECK(pairs==30 && unidentified==1);
    std::cout<<"MEMS16NA_SESSION_OK: real capture 31 pairs, 30 identified after D1, 1 unclassified; invalid frames and identities rejected\n";
}
