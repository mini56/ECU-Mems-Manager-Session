#ifndef PROTOCOLCONTEXT_H
#define PROTOCOLCONTEXT_H

#include <cstdint>

enum class MemsEcuFamily : int
{
    Unknown = 0,
    Rosco13_16 = 1,
    Mems19 = 2,
    Mems2J = 3,
    Mems3 = 4
};

enum class MemsDiagnosticMode : int
{
    Unknown = 0,
    Normal = 1,
    Mode3 = 2,
    Mode4 = 3,
    Transition = 4
};

namespace MemsProtocolSafety
{
inline bool familyKnown(MemsEcuFamily family)
{
    return family != MemsEcuFamily::Unknown;
}

inline bool allowsGenericCommand(MemsEcuFamily family,
                                 MemsDiagnosticMode mode,
                                 std::uint8_t command)
{
    (void)family;

    // F0 is a read-only mode query. It is needed specifically to recover from
    // an unknown mode, therefore it remains available independently of family.
    if (command == 0xF0u)
        return true;

    if (mode != MemsDiagnosticMode::Normal)
        return false;

    // D0/D1/D2 are identification/status reads in the confirmed normal
    // diagnostic session. D1 is refused in Mode 4 above because there it can
    // program calibration RAM to ROM. These reads are also what allow the
    // application to establish the exact ECU family after connection.
    return command == 0xD0u || command == 0xD1u || command == 0xD2u;
}

inline bool isKnownLegacyMutation(std::uint8_t command)
{
    switch (command)
    {
    case 0x81u: // save adjustments, LeopoldG original DLL
    case 0xCCu: // clear faults
    case 0xAFu: // original LeopoldG ECU reset
    case 0xFAu: // original LeopoldG adjustment reset
    case 0x0Fu: // original LeopoldG emissions reset
    case 0x11u: case 0x01u: // fuel pump
    case 0x12u: case 0x02u: // PTC
    case 0x13u: case 0x03u: // A/C
    case 0x18u: case 0x08u: // purge
    case 0x19u: case 0x09u: // O2 heater
    case 0x1Bu: case 0x0Bu: // boost/wastegate
    case 0x1Du: case 0x0Du: // fan 1
    case 0x1Eu: case 0x0Eu: // fan 2
    case 0x6Fu: case 0x67u: // fan 3
    case 0xF8u:             // fire coil in normal diagnostic mode
    case 0xFDu: case 0xFEu: // IAC step
    case 0xF4u:             // all actuators off / heartbeat in normal context
    case 0x79u: case 0x7Au: // fuel trim
    case 0x7Bu: case 0x7Cu: // secondary fuel trim, normal ROSCO only
    case 0x89u: case 0x8Au: // idle decay
    case 0x91u: case 0x92u: // idle speed
    case 0x93u: case 0x94u: // ignition advance offset
        return true;
    default:
        return false;
    }
}

inline bool allowsMutation(MemsEcuFamily family,
                           MemsDiagnosticMode mode,
                           std::uint8_t command)
{
    // A completed ECU handshake selects the protocol family in connectToECU().
    // No documentary lookup or exact ECU part number is required.
    if (mode != MemsDiagnosticMode::Normal)
        return false;

    // MEMS 1.9 uses the same single-byte fault/reset commands but its K-Line
    // transport must consume an optional extra local cable echo.
    if (family == MemsEcuFamily::Mems19)
        return command == 0xCCu || command == 0x0Fu || command == 0xFAu || command == 0x81u;

    if (family != MemsEcuFamily::Rosco13_16)
        return false;

    // Injector commands are not sent without subtype-specific routing.
    if (command == 0xF7u || command == 0xEFu)
        return false;

    return isKnownLegacyMutation(command);
}
}

#endif // PROTOCOLCONTEXT_H
