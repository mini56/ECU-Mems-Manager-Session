#pragma once
// Profile-specific DTC interpretation, from research v1.15 chapters 138, 158–166.
// No serial I/O, Qt, or assumptions about a missing byte.
#include <cstdint>
#include <cstddef>
#include <cstring>
namespace MemsExtendedDtc {
enum class Status { Missing, NotUsed, Unverified, Decoded };
struct Result {
    Status status;
    unsigned raw, faults, states, indicators;
};
inline bool eq(const char *a,const char *b) {return a&&std::strcmp(a,b)==0;}
inline bool turbo(const char *f) {return eq(f,"TCF7X005")||eq(f,"TCF8X005")||eq(f,"TFF8X000")||eq(f,"TFFSX000");}
inline bool short13(const char *f) {return eq(f,"ABEMR002")||eq(f,"KBE6R003")||eq(f,"MGE7R002");}
inline bool fan(const char *f) {return eq(f,"REGI1224")||eq(f,"KLH3G015")||eq(f,"KLH3G018")||eq(f,"KLHCG002")||eq(f,"REGI1360")||eq(f,"REGI1365")||eq(f,"REGI1374")||eq(f,"REGI1384")||eq(f,"KLH3G020");}
inline bool hdc(const char *f) {return eq(f,"REGI1297")||eq(f,"REGI1330")||eq(f,"REGI1341")||eq(f,"KLH4G020")||eq(f,"KLQ4G007");}
inline bool cruise(const char *f) {return eq(f,"TAHHG013")||eq(f,"TAHHG019")||eq(f,"TAHHV006");}
inline bool sync19(const char *f) {return eq(f,"KBHBG009")||eq(f,"REGI1224")||eq(f,"REGI1360")||eq(f,"TAHHG019")||eq(f,"TFHSG014");}
inline bool constant19(const char *f) {return eq(f,"TBP7F015")||eq(f,"SELFG007");}
inline bool da19(const char *f) {return eq(f,"KLH3G015")||eq(f,"TAH7G018")||eq(f,"REGI1224")||eq(f,"TFHSG014");}
// DTC0 bit 04: ROM table and matching Rover symbol m_pbu_fault_log.
inline bool pressurePipeFaultKnown(const char *f) {return eq(f,"KBHBG009")||eq(f,"KBHBG010")||eq(f,"KBHBV002")||eq(f,"KFHBG009")||eq(f,"KLH3G015")||eq(f,"REGI1224")||eq(f,"REGI1360")||eq(f,"REGI1417")||eq(f,"TAH7G018")||eq(f,"TAHHG019")||eq(f,"TFHSG014");}
// Label only: distinguish absent interpretation from a documented unused bit.
inline int pressurePipeLabelKey(const char *f,bool documentedUnused) {
    return pressurePipeFaultKnown(f)?29253:documentedUnused?29255:29254;
}
inline bool dtc2Known(const char *f) {return eq(f,"KBHBG009");}
inline unsigned offset(int group) {
    switch(group){case 2:return 5;case 3:return 14;case 4:return 17;case 5:return 22;case 6:return 16;default:return 256;}
}
inline Result decode(const char *f,int group,const std::uint8_t *b,std::size_t n) {
    const unsigned pos=offset(group);
    if(!b||!n||b[0]!=n||pos>=n)return {Status::Missing,0,0,0,0};
    const unsigned r=b[pos];
    if(!f||!*f)return {Status::Unverified,r,0,0,r};
    if(group==4&&eq(f,"KBHBG009"))return {r==0?Status::NotUsed:Status::Unverified,r,0,0,r};
    if((group>=3&&group<=5&&eq(f,"TBP7F015"))||(group==3&&eq(f,"SELFG007"))||(group==5&&turbo(f))||
       (group==6&&eq(f,"KBHBG009")))
        return {r==255?Status::NotUsed:Status::Unverified,r,0,0,r==255?0u:r};
    if(group>=3&&group<=5&&short13(f))return {Status::Unverified,r,0,0,r};
    unsigned faults=0,states=0,technical=0,possible=0;
    bool complete=false;
    if(group==2&&dtc2Known(f)){faults=0xB4;possible=0xF4;}
    if(group==3&&sync19(f)){faults=1;states=4;possible=5;complete=true;}
    if(group==3&&turbo(f)){technical=3;possible=3;}
    if(group==4&&turbo(f)){faults=3;possible=3;complete=true;}
    if(group==4&&da19(f)){technical=0x40;possible=0x40;}
    if(group==5&&eq(f,"KBHBG009")){faults=6;possible=6;complete=true;}
    if(group==5&&fan(f)){faults=0x18;possible=0x1E; if(eq(f,"REGI1224")){faults|=6;complete=true;}}
    if(group==5&&hdc(f)){faults=0x20;possible=0x26;}
    if(group==5&&cruise(f)){faults=0x40;possible=0x46;}
    if(!possible || (r==255 && possible!=255))return {Status::Unverified,r,0,0,r};
    const unsigned unknown=r&~(faults|states);
    return {complete&&!(r&~possible)?Status::Decoded:Status::Unverified,r,r&faults,r&states,unknown|(r&technical)};
}
}
