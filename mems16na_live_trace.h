#pragma once
#include "mems16na_session_evidence.h"
#include <sstream>
#include <utility>
namespace Mems16Na {
// Consume exactly the TX/RX records bounded by one successful existing mems_read.
// A partial/malformed journal never becomes a valid sample.
inline bool consumeLivePair(SessionEvidence &s,const std::string &trace) {
    s.discardPair();
    if(!s.identifiedProfile() || trace.empty() || trace.size()>8192 || trace.back()!='\n') return false;
    SessionEvidence next=s;
    std::istringstream in(trace);
    std::string line;
    unsigned phase=0;
    while(std::getline(in,line)) {
        if(!line.empty() && line.back()=='\r')line.pop_back();
        if(line.empty())continue;
        const auto first=line.find(" | ");
        if(first==std::string::npos)return false;
        const auto second=line.find(" | ",first+3);
        if(second==std::string::npos)return false;
        const auto direction=line.substr(first+3,second-first-3);
        if(direction=="INFO")continue;
        std::istringstream words(line.substr(second+3));
        std::string token;std::vector<std::uint8_t> bytes;
        while(words>>token) {
            if(token.size()!=2 || token.find_first_not_of("0123456789ABCDEFabcdef")!=std::string::npos)return false;
            bytes.push_back(static_cast<std::uint8_t>(std::stoul(token,nullptr,16)));
        }
        if(bytes.empty())return false;
        if(direction=="TX") {
            if(bytes.size()!=1 || phase>=2 || bytes[0]!=(phase==0?0x80:0x7D))return false;
            ++phase; next.tx(bytes);
        } else if(direction=="RX" && phase>0)next.rx(bytes);
        else return false;
    }
    if(phase!=2)return false;
    next.finish();
    if(!next.ready())return false;
    s=next;
    return true;
}
}

namespace Mems16Na {
// Separate observer for non-NA frames. Uses the actual announced length, including
// optional K-line adapter echo; never reads a padded DLL tail as ECU data.
inline bool consumeExtendedPair(SessionEvidence &s,const std::string &trace,bool localEcho,
                                unsigned rpm,unsigned dtc2,unsigned dtc3,unsigned dtc4,unsigned dtc5) {
    s.setExtended7d({}); s.setExtended80({});
    // A complete ordinary frame may be observed even when D1 is unavailable.
    // Its raw bytes do not establish a firmware profile or a physical conversion.
    if(s.identifiedProfile()||trace.empty()||trace.size()>8192||trace.back()!='\n')return false;
    std::istringstream in(trace);std::string line;
    std::vector<std::uint8_t> rx[2];unsigned phase=0;
    while(std::getline(in,line)){
        if(!line.empty()&&line.back()=='\r')line.pop_back();
        if(line.empty())continue;
        auto a=line.find(" | ");if(a==std::string::npos)return false;
        auto b=line.find(" | ",a+3);if(b==std::string::npos)return false;
        const auto direction=line.substr(a+3,b-a-3);if(direction=="INFO")continue;
        std::istringstream words(line.substr(b+3));std::string token;std::vector<std::uint8_t> bytes;
        while(words>>token){if(token.size()!=2||token.find_first_not_of("0123456789ABCDEFabcdef")!=std::string::npos)return false;bytes.push_back(static_cast<std::uint8_t>(std::stoul(token,nullptr,16)));}
        if(bytes.empty())return false;
        if(direction=="TX"){if(phase>=2||bytes.size()!=1||bytes[0]!=(phase==0?0x80:0x7D))return false;++phase;}
        else if(direction=="RX"&&phase>0)rx[phase-1].insert(rx[phase-1].end(),bytes.begin(),bytes.end());
        else return false;
    }
    if(phase!=2)return false;
    const unsigned prefix=localEcho?2:1;
    for(unsigned i=0;i<2;++i){
        const unsigned cmd=i?0x7D:0x80;
        if(rx[i].size()<=prefix||rx[i][0]!=cmd||(localEcho&&rx[i][1]!=cmd)||rx[i][prefix]!=rx[i].size()-prefix)return false;
        rx[i].erase(rx[i].begin(),rx[i].begin()+prefix);
    }
    if(rx[0].size()<3||word(Frame{rx[0].data(),rx[0].size()},1)!=rpm||rx[1].size()<6||rx[1][5]!=dtc2)return false;
    const std::pair<unsigned,unsigned> checks[]={{14,dtc3},{17,dtc4},{22,dtc5}};
    for(const auto &v:checks)
        if(v.first<rx[1].size()&&rx[1][v.first]!=v.second)return false;
    s.setExtended80(rx[0]); s.setExtended7d(rx[1]);return true;
}
}
