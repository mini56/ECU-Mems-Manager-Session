#include "mems16na_live_trace.h"
#include <iostream>
#include <fstream>
#include <sstream>
using namespace Mems16Na;
#define CHECK(x) do{if(!(x)){std::cerr<<"FAIL "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
int main(int argc,char **argv) {
    CHECK(argc==2);
    std::ifstream f(argv[1]);CHECK(f.good());
    SessionEvidence s;
    s.tx({0xD0});s.rx({0xD0,0x98,0,2,2});s.finish();
    s.tx({0xD1});s.rx({0xD1,'A','A','N','M','P','0','0','2',0x98,0,2,2});s.finish();
    CHECK(s.identifiedProfile());
    std::string line,trace;unsigned n=0;
    bool afterD1=false;
    auto checkPair=[&](){
        if(trace.empty())return true;
        if(!consumeLivePair(s,trace))return false;
        ++n;
        auto m=s.measurements();
        return m.idleHot.calculated==32 && m.throttleReference[1].calculated==31.75 &&
               s.dtc(Group::Dtc0).noFault() && s.dtc(Group::Dtc1).noFault();
    };
    while(std::getline(f,line)){
        if(line=="TX D1"){afterD1=true;continue;}
        if(!afterD1)continue;
        if(line=="TX 80"){CHECK(checkPair());trace.clear();}
        if(line=="TX 80" || !trace.empty()){
            if(line.size()<4)return 1;
            trace+="2026-11-24 00:00:00.000 | "+line.substr(0,2)+" | "+line.substr(3)+"\r\n";
        }
    }
    CHECK(checkPair());CHECK(n==30);
    CHECK(!consumeLivePair(s,trace.substr(0,trace.size()-1)));CHECK(!s.ready()&&s.identifiedProfile());
    CHECK(!consumeLivePair(s,trace+"2026-11-24 00:00:00.000 | TX | 91\n"));CHECK(!s.ready());
    CHECK(!consumeLivePair(s,"2026-11-24 00:00:00.000 | TX | 80\n"));CHECK(!s.ready());
    CHECK(consumeLivePair(s,trace));CHECK(s.ready());
    s.reset();CHECK(!consumeLivePair(s,trace));CHECK(!s.ready());
    std::cout<<"MEMS16NA_LIVE_TRACE_OK: 30 bounded real pairs; invalid, stale and unclassified samples rejected\n";
}
