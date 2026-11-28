#include "mems16na_live_trace.h"
#include <cstdlib>
#include <cstdio>
using namespace MemsExtendedDtc;
static void require(bool b){if(!b){std::fprintf(stderr,"DTC regression\n");std::exit(1);}}
int main(){
    for(const char *f:{"KBHBG009","KBHBG010","KBHBV002","KFHBG009","KLH3G015","REGI1224","REGI1360","REGI1417","TAH7G018","TAHHG019","TFHSG014"})require(pressurePipeFaultKnown(f));
    for(const char *f:{"AANMP002","ABEMR002","TCF7X005","TBP7F015","SELFG007","UNKNOWN",""})require(!pressurePipeFaultKnown(f));
    require(!pressurePipeFaultKnown(nullptr));
    require(pressurePipeLabelKey("KBHBG009",false)==29253);
    require(pressurePipeLabelKey("AANMP002",true)==29255);
    require(pressurePipeLabelKey("UNKNOWN",false)==29254);
    require(pressurePipeLabelKey(nullptr,false)==29254);
    require(extraFaultMask("KBHBG009")==6);
    require(extraFaultMask("REGI1224")==0x1e);
    require(extraFaultMask("REGI1297")==0x20);
    require(extraFaultMask("TAHHG019")==0x40);
    require(extraFaultMask("AANMP002")==0);
    require(extraFaultMask("UNKNOWN")==0);
    std::uint8_t b[32]={32};
    for(unsigned r=0;r<256;++r){
        b[14]=static_cast<std::uint8_t>(r);
        const auto d=decode("KBHBG009",3,b,32);
        require(d.faults==(r==255?0:r&1));require(d.states==(r==255?0:r&4));
        if(r==4)require(d.status==Status::Decoded&&d.faults==0);
        b[17]=static_cast<std::uint8_t>(r);
        require(decode("TCF7X005",4,b,32).faults==(r==255?0:r&3));
        require(decode("UNKNOWN",4,b,32).faults==0);
        b[22]=static_cast<std::uint8_t>(r);
        require(decode("TCF7X005",5,b,32).status==(r==255?Status::NotUsed:Status::Unverified));
        require(decode("REGI1224",5,b,32).faults==(r==255?0:r&0x1e));
        require(decode("TAHHG019",5,b,32).faults==(r==255?0:r&0x40));
        require(decode("REGI1297",5,b,32).faults==(r==255?0:r&0x20));
        b[5]=static_cast<std::uint8_t>(r);require(decode("KBHBG009",2,b,32).faults==(r==255?0:r&0xB4));
    }
    // Exhaustive raw-byte parity for the independently proven equivalent ROM.
    require(dtc2Known("KBHBV002")&&sync19("KBHBV002")&&extraFaultMask("KBHBV002")==6);
    for(int group=2;group<=6;++group){
        const unsigned pos=offset(group);
        for(unsigned raw=0;raw<256;++raw){
            b[pos]=static_cast<std::uint8_t>(raw);
            const auto x=decode("KBHBG009",group,b,32), y=decode("KBHBV002",group,b,32);
            require(x.status==y.status&&x.raw==y.raw&&x.faults==y.faults&&x.states==y.states&&x.indicators==y.indicators);
        }
    }
    std::uint8_t shortFrame[14]={14};
    for(const char *f:{"ABEMR002","KBE6R003","MGE7R002"}){
        for(unsigned value=0;value<256;++value){
            shortFrame[3]=static_cast<std::uint8_t>(value);
            require(analogue13Available(f,shortFrame,14)==(value<=159));
        }
    }
    shortFrame[3]=42;
    require(!analogue13Available("UNKNOWN",shortFrame,14));
    require(!analogue13Available("KBHBG009",shortFrame,14));
    require(!analogue13Available("ABEMR002",shortFrame,13));
    require(!analogue13Available("ABEMR002",nullptr,14));
    shortFrame[0]=13;require(!analogue13Available("ABEMR002",shortFrame,14));shortFrame[0]=14;
    require(decode("ABEMR002",3,shortFrame,14).status==Status::Missing);
    require(decode("KBHBG009",5,shortFrame,14).status==Status::Missing);
    b[14]=255;require(decode("TBP7F015",3,b,32).status==Status::NotUsed);
    b[17]=0;require(decode("KBHBG009",4,b,32).status==Status::NotUsed);
    b[17]=0x40;require(decode("KBHBG009",4,b,32).status==Status::Unverified);
    require(decode("REGI1224",4,b,32).faults==0);
    b[17]=255;require(decode("SELFG007",4,b,32).status==Status::Unverified);
    auto p=Mems16Na::byFirmware("AANMP002");require(p!=nullptr);
    std::uint8_t a[28]={28};b[5]=0x40;b[14]=255;b[17]=255;b[22]=255;
    auto na=Mems16Na::decodeDtc(p,Mems16Na::Group::Dtc2,{a,28},{b,32});
    require(na.faultBits==0&&na.stateBits==0x40);
    for(auto g:{Mems16Na::Group::Dtc3,Mems16Na::Group::Dtc4,Mems16Na::Group::Dtc5})
        require(Mems16Na::decodeDtc(p,g,{a,28},{b,32}).availability==Mems16Na::Availability::NotUsed);
    Mems16Na::SessionEvidence session;
    session.tx({0xD0});session.rx({0xD0,1,2,3,4});session.finish();
    session.tx({0xD1});session.rx({0xD1,'K','B','H','B','G','0','0','9',1,2,3,4});session.finish();
    require(session.firmwareName()=="KBHBG009");
    const std::string trace="x | TX | 80\nx | RX | 80 03 00 64\nx | TX | 7D\nx | RX | 7D 06 00 00 00 00 04\n";
    require(Mems16Na::consumeExtendedPair(session,trace,false,100,4,0,0,0));
    require(session.extendedDtc(2).faults==4);
    require(session.extendedDtc(3).status==Status::Missing);
    session.discardPair();require(session.extendedDtc(2).status==Status::Missing);
    require(!Mems16Na::consumeExtendedPair(session,trace,false,101,4,0,0,0));
    return 0;
}
