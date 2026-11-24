#pragma once
#include "mems16na_session_evidence.h"
#include <sstream>
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
