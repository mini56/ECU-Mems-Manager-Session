#pragma once
// Passive observation of existing ROSCO communications; no ECU I/O.
// V0.43 ch.47: never select a profile from numeric ID alone.
#include "mems16na_decoder.h"
#include "memsextendeddtc.h"
#include <array>
#include <vector>
#include <string>
#include <algorithm>

namespace Mems16Na {
class SessionEvidence {
public:
    void reset() {
        identity_.fill(0); haveIdentity_=false; profile_=nullptr; firmware_.clear(); extended7d_.clear();
        have80_=false; havePair_=false; extended7d_.clear(); command_=-1; reply_.clear(); overflow_=false;
    }
    // Only feed records from the current connection, with the original TX/RX direction.
    // A new TX closes the previous transaction. No serial command is generated.
    void tx(const std::vector<std::uint8_t>& bytes) {
        finish();
        command_=bytes.size()==1 ? int(bytes[0]) : -1;
        reply_.clear(); overflow_=false;
        if(command_!=0x7D) have80_=false;
        if(command_==0xD0 || command_==0xD1) {havePair_=false; firmware_.clear(); extended7d_.clear();}
        if(command_==0xD0) { haveIdentity_=false; profile_=nullptr; }
        if(command_==0xD1) profile_=nullptr;
    }
    void rx(const std::vector<std::uint8_t>& bytes) {
        if(command_<0 || overflow_) return;
        if(bytes.size()>64 || reply_.size()+bytes.size()>64) {
            overflow_=true; reply_.clear(); return;
        }
        reply_.insert(reply_.end(),bytes.begin(),bytes.end());
    }
    // At a known completed transaction boundary only (next TX or completed replay).
    void finish() {
        if(command_<0) return;
        const int c=command_; command_=-1;
        const bool echo=!overflow_ && !reply_.empty() && reply_[0]==c;
        if(c==0xD0) {
            haveIdentity_=echo && reply_.size()==5;
            profile_=nullptr;
            if(haveIdentity_) std::copy(reply_.begin()+1,reply_.end(),identity_.begin());
        } else if(c==0xD1) {
            profile_=nullptr;
            // Real SESSION 206 capture: echoed D1, then three identical 12-byte
            // records (8 ASCII firmware bytes and 4 binary D0 identity bytes).
            // Accept 1..3 complete identical records; reject extra or conflicting bytes.
            if(echo && haveIdentity_ && reply_.size()>=13 && reply_.size()<=37 &&
               (reply_.size()-1)%12==0) {
                bool consistent=true;
                for(std::size_t i=1;i<reply_.size();i+=12) {
                    if(!std::equal(reply_.begin()+1,reply_.begin()+13,reply_.begin()+i) ||
                       !std::equal(identity_.begin(),identity_.end(),reply_.begin()+i+8)) {
                        consistent=false; break;
                    }
                }
                if(consistent) {
                    const std::string name(reply_.begin()+1,reply_.begin()+9);
                    // Reject embedded NUL and non-ASCII catalogue names.
                    bool ascii=true;
                    for(char x:name) if(!((x>='A'&&x<='Z')||(x>='0'&&x<='9'))) ascii=false;
                    if(ascii) {
                        const std::uint32_t id=(std::uint32_t(identity_[0])<<16)|
                            (std::uint32_t(identity_[1])<<8)|identity_[2];
                        // Full exact name in the NA-only catalogue establishes family.
                        firmware_=name;
                        profile_=select(true,name.c_str(),true,id);
                    }
                }
            }
        } else if(c==0x80) {
            have80_=echo && reply_.size()==29 && reply_[1]==28;
            havePair_=false;
            if(have80_) std::copy(reply_.begin()+1,reply_.end(),a_.begin());
        } else if(c==0x7D) {
            const bool ok=echo && reply_.size()==33 && reply_[1]==32;
            havePair_=profile_ && have80_ && ok;
            have80_=false;
            if(havePair_) std::copy(reply_.begin()+1,reply_.end(),b_.begin());
        } else {
            have80_=false; havePair_=false;
        }
        reply_.clear(); overflow_=false;
    }
    // Invalidate the last sample without losing the verified firmware identity.
    void discardPair() {
        have80_=false; havePair_=false; extended7d_.clear(); command_=-1; reply_.clear(); overflow_=false;
    }
    const std::array<std::uint8_t,4>& identity() const { return identity_; }
    const Profile* identifiedProfile() const { return profile_; }
    const std::string& firmwareName() const {return firmware_;}
    void setExtended7d(const std::vector<std::uint8_t>& b) {extended7d_=b;}
    const std::vector<std::uint8_t>& extended7d() const {return extended7d_;}
    MemsExtendedDtc::Result extendedDtc(int group) const {
        return MemsExtendedDtc::decode(firmware_.c_str(),group,extended7d_.data(),extended7d_.size());
    }
    bool ready() const { return profile_ && havePair_; }
    Frame frame80() const { return ready()?Frame{a_.data(),a_.size()}:Frame{nullptr,0}; }
    Frame frame7d() const { return ready()?Frame{b_.data(),b_.size()}:Frame{nullptr,0}; }
    Measurements measurements() const { return decode(ready()?profile_:nullptr,frame80(),frame7d()); }
    DtcResult dtc(Group g) const { return decodeDtc(ready()?profile_:nullptr,g,frame80(),frame7d()); }
private:
    std::array<std::uint8_t,4> identity_{};
    bool haveIdentity_=false;
    const Profile* profile_=nullptr;
    std::string firmware_;
    std::vector<std::uint8_t> extended7d_;
    std::array<std::uint8_t,28> a_{};
    std::array<std::uint8_t,32> b_{};
    bool have80_=false,havePair_=false;
    int command_=-1;
    std::vector<std::uint8_t> reply_;
    bool overflow_=false;
};
}
