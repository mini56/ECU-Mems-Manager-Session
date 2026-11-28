#pragma once
// Manager-only interpretation of validated MEMS 1.6 NA frames.
// No I/O, DLL calls, ECU commands, Qt or translated strings.
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace Mems16Na {
enum class Circuit {
    None, CoolantSensor, IntakeSensor, FuelTemperatureSensor, FuelPump,
    PtcHeater, AirConditioning, CoolingFans, AuxiliaryPtc, Canister,
    MapSensor, ThrottlePot, LambdaHeater, EngineSync, ThrottleSwitch,
    UnidentifiedFault, UnidentifiedIndicator
};
struct Profile {
    const char *firmware;
    std::uint32_t petaId; // three bytes, never a universal unique ID
    bool fuelTemperature;
    bool repeatedThrottleReference;
    std::uint8_t dtc0Mask;
    std::uint8_t pnSourceMask;
    std::uint8_t ignitionSourceMask;
    Circuit dtc1Bit04;
    Circuit dtc1Bit08;
    bool alternateTransmission;
};
inline const Profile *catalog(std::size_t &size) {
    static const Profile rows[] = {
#include "mems16na_profiles.inc"
    };
    size = sizeof(rows) / sizeof(rows[0]);
    return rows;
}
inline const Profile *byFirmware(const char *name) {
    if (!name || !*name) return nullptr;
    std::size_t n = 0;
    const Profile *rows = catalog(n);
    for (std::size_t i=0;i<n;++i)
        if (std::strcmp(rows[i].firmware,name)==0) return rows+i;
    return nullptr;
}
inline std::size_t candidateCount(std::uint32_t id) {
    std::size_t n=0, count=0;
    const Profile *rows=catalog(n);
    for (std::size_t i=0;i<n;++i) if (rows[i].petaId==id) ++count;
    return count;
}
// Caller must independently establish NA and supply the full firmware name.
// A three-byte identifier (even unique in this catalog) is insufficient.
// When a PETA identifier is available, disagreement invalidates selection.
inline const Profile *select(bool confirmedNa, const char *name,
                             bool hasPetaId=false, std::uint32_t id=0) {
    if (!confirmedNa) return nullptr;
    const Profile *p=byFirmware(name);
    return p && (!hasPetaId || p->petaId==id) ? p : nullptr;
}
struct Frame {
    const std::uint8_t *bytes;
    std::size_t size;
};
inline bool valid(Frame f, std::size_t expected) {
    return f.bytes && f.size==expected && f.bytes[0]==expected;
}
inline unsigned word(Frame f, unsigned offset) {
    return (unsigned(f.bytes[offset])<<8) | unsigned(f.bytes[offset+1]);
}
enum class Availability { Missing, Available, NotUsed, Unverified };
struct Value {
    Availability availability;
    unsigned raw;
    double calculated;
};
inline Value missing() { return {Availability::Missing,0,0}; }
inline Value available(unsigned r,double v) { return {Availability::Available,r,v}; }
inline Value unverified(unsigned r) { return {Availability::Unverified,r,0}; }
inline Value constantFF(unsigned r) {
    return {r==255 ? Availability::NotUsed : Availability::Unverified,r,0};
}
// No numeric/boolean display for Missing, NotUsed or Unverified.
struct Measurements {
    Value ambient, fuel, pn, ac, idleAmplitude, idleSigned, idleHot, learnVoltage,
          throttleVoltage, ignitionOff, injectionTime, idleBase, idleError,
          throttleReference[4], learnCount, throttleAngle;
    Value lambdaMv, lambdaFault, closedLoop;
    bool signedIdleConsistent;
};
inline Measurements decode(const Profile *p, Frame a, Frame b) {
    Measurements m{};
    if (!p) return m;
    const bool aok=valid(a,28), bok=valid(b,32);
    if (aok) {
        const auto r=a.bytes;
        m.ambient=constantFF(r[4]);
        // Existing temperature conversion maintained for confirmed FRT.
        m.fuel=p->fuelTemperature ? available(r[6],int(r[6])-55) : constantFF(r[6]);
        // 80/0B retains the profile-specific source bit; 80/0C and 80/1B are normalized inverted bits.
        m.pn=p->pnSourceMask && (r[11]==0 || r[11]==p->pnSourceMask)
            ? available(r[11],r[11]==0) : unverified(r[11]);
        m.ac=r[12]<=1
            ? available(r[12],r[12]==0) : unverified(r[12]);
        m.idleAmplitude=available(r[15],r[15]*6.25);
        m.idleHot=available(r[16],r[16]);
        m.learnVoltage=available(r[17],r[17]*0.1);
        m.throttleVoltage=available(r[9],r[9]*5.0/255.0);
        m.ignitionOff=r[27]<=1
            ? available(r[27],r[27]) : unverified(r[27]);
    }
    if (bok) {
        const auto r=b.bytes;
        m.throttleAngle=available(r[2],r[2]*0.5);
        m.injectionTime=available(r[11],(int(r[11])-128)*8);
        m.idleBase=available(r[15],r[15]);
        const unsigned err=word(b,20);
        m.idleError=available(err,int(err)-32768);
        for (unsigned i=0;i<4;++i) {
            const unsigned w=word(b,23+2*i);
            m.throttleReference[i]=available(w,w/256.0);
        }
        m.learnCount=available(r[31],r[31]);
        m.idleSigned=available(r[19],(int(r[19])-128)*25);
        m.lambdaMv=available(r[6],r[6]*5);
        m.lambdaFault=r[9]<=1 ? available(r[9],r[9]==0) : unverified(r[9]);
        m.closedLoop=available(r[10],r[10]!=0);
    }
    // Caller must also establish temporal coherence before displaying the
    // signed correction as a combined 80/0F and 7D/13 interpretation.
    if (aok && bok) {
        const double s=m.idleSigned.calculated;
        m.signedIdleConsistent=m.idleAmplitude.calculated==(s<0 ? -s : s);
    }
    return m;
}
// One qualified signed correction for both live pages and the V2 journal.
inline Value qualifiedIdleSigned(const Measurements &m) {
    if(m.idleSigned.availability==Availability::Missing)return missing();
    return m.signedIdleConsistent?m.idleSigned:unverified(m.idleSigned.raw);
}
enum class Group { Dtc0, Dtc1, Dtc2, Dtc3, Dtc4, Dtc5, Indirect7D10 };
struct DtcDefinition {
    std::uint8_t faultMask, stateMask;
    bool constant;
};
inline DtcDefinition definition(const Profile *p, Group g) {
    if (!p) return {0,0,false};
    switch(g) {
    case Group::Dtc0: return {p->dtc0Mask,0,false};
    case Group::Dtc1: return {0xBE,0,false};
    case Group::Dtc2: return {0x14,0x40,false};
    default: return {0,0,true};
    }
}
inline Circuit circuit(const Profile *p, Group g, unsigned mask) {
    if (!p) return Circuit::UnidentifiedIndicator;
    switch(g) {
    case Group::Dtc0:
        if(mask==1) return Circuit::CoolantSensor;
        if(mask==2) return Circuit::IntakeSensor;
        if(mask==0x20 && p->fuelTemperature) return Circuit::FuelTemperatureSensor;
        break;
    case Group::Dtc1:
        switch(mask) {
        case 2: return Circuit::FuelPump;
        case 4: return p->dtc1Bit04;
        case 8: return p->dtc1Bit08;
        case 0x10: return Circuit::Canister;
        case 0x20: return Circuit::MapSensor;
        case 0x80: return Circuit::ThrottlePot;
        default: break;
        }
        break;
    case Group::Dtc2:
        if(mask==4) return std::strcmp(p->firmware,"AANMP002")==0
            ? Circuit::LambdaHeater : Circuit::UnidentifiedFault;
        if(mask==0x10) return Circuit::EngineSync;
        if(mask==0x40) return Circuit::ThrottleSwitch;
        break;
    default: break;
    }
    return Circuit::UnidentifiedIndicator;
}
struct DtcResult {
    Availability availability;
    std::uint8_t raw, faultBits, stateBits, unknownBits;
    bool noFault() const {
        return availability==Availability::Available && !faultBits && !unknownBits;
    }
};
inline DtcResult decodeDtc(const Profile *p,Group g,Frame a,Frame b) {
    Frame f=b;
    unsigned offset=0;
    switch(g) {
    case Group::Dtc0: f=a; offset=13; break;
    case Group::Dtc1: f=a; offset=14; break;
    case Group::Dtc2: offset=5; break;
    case Group::Dtc3: offset=14; break;
    case Group::Dtc4: offset=17; break;
    case Group::Dtc5: offset=22; break;
    case Group::Indirect7D10: offset=16; break;
    default: return {Availability::Missing,0,0,0,0};
    }
    if (!valid(f,(g==Group::Dtc0||g==Group::Dtc1)?28:32))
        return {Availability::Missing,0,0,0,0};
    const std::uint8_t r=f.bytes[offset];
    if (!p) return {Availability::Unverified,r,0,0,r};
    const auto d=definition(p,g);
    if (d.constant) return {r==255?Availability::NotUsed:Availability::Unverified,
        r,0,0,static_cast<std::uint8_t>(r==255?0:r)};
    return {Availability::Available,r,
        static_cast<std::uint8_t>(r&d.faultMask),
        static_cast<std::uint8_t>(r&d.stateMask),
        static_cast<std::uint8_t>(r&~(d.faultMask|d.stateMask))};
}
} // namespace Mems16Na
