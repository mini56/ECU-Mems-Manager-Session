#include "mems16na_log_schema.h"
#include <iostream>
#include <fstream>
#include <set>
using namespace Mems16Na;
#define CHECK(x) do{if(!(x)){std::cerr<<"FAIL "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
using Bytes=std::vector<std::uint8_t>;
int main(int argc,char**argv){
    CHECK(argc==2);std::ifstream f(argv[1]);CHECK(f.good());
    SessionEvidence s;std::string line;int c=-1;unsigned pairs=0;
    auto close=[&](){
        s.finish();if(c!=0x7d||!s.ready())return true;
        const auto row=LogV2::fields(s,"2026-11-24T13:13:38.867+01:00");
        auto get=[&](const std::string& key){for(const auto &x:row)if(x.name==key)return x.value;return std::string("ABSENT");};
        std::set<std::string> keys;for(const auto& x:row)keys.insert(x.name);
        if(keys.size()!=row.size() || LogV2::header()!="#time"+LogV2::csv(row,true))return false;
        if(get("na_frame80_hex").size()!=56 || get("na_frame7d_hex").size()!=64 ||
           get("na_ambient_raw")!="255"||get("na_ambient_c_value")!=""||get("na_ambient_availability")!="not_used"||
           get("na_idle_hot_steps_value")!="32"||get("na_learn_voltage_v_value")!="13.4"||
           get("na_tps_ref2_tps_value")!="31.75"||get("na_dtc0_fault_bits")!="0"||
           get("na_dtc3_fault_bits")!=""||get("na_dtc3_availability")!="not_used")return false;
        ++pairs;return true;
    };
    while(std::getline(f,line)){
        std::istringstream in(line);std::string dir,token;Bytes bytes;in>>dir;
        while(in>>token)bytes.push_back(static_cast<std::uint8_t>(std::stoul(token,nullptr,16)));
        if(dir=="TX"){CHECK(close());CHECK(bytes.size()==1);c=bytes[0];s.tx(bytes);}
        else if(dir=="RX")s.rx(bytes);else CHECK(false);
    }
    CHECK(close());CHECK(pairs==30);CHECK(LogV2::hex(s.identity().data(),4)=="98000202");
    s.discardPair();
    const auto row=LogV2::fields(s,"2026-11-24T13:13:39.000+01:00");
    for(const auto &x:row){
        if(x.name=="na_sample_valid")CHECK(x.value=="0");
        else if(x.name=="na_timestamp")CHECK(!x.value.empty());
        else if(x.name.find("_availability")!=std::string::npos)CHECK(x.value=="missing");
        else CHECK(x.value.empty());
    }
    CHECK(LogV2::header()=="#time"+LogV2::csv(row,true));
    std::cout<<"MEMS16NA_LOG_V2_OK: 30 actual samples, exact raw frames, unique stable columns, missing values never zero\n";
}
