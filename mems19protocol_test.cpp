#include "mems19protocol.h"
#include "serialadapterdetector.h"

#include <cstdlib>
#include <iostream>

static void require(bool pass, const char *message)
{
    if (!pass)
    {
        std::cerr << "MEMS19 TEST FAILED: " << message << '\n';
        std::exit(1);
    }
}

int main()
{
    // ISO 9141 slow init 0x16: start, 8 bits LSB-first, stop.
    const bool expected[10] = {
        false, false, true, true, false, true, false, false, false, true
    };
    require(Mems19Protocol::kSlowBitCount == 10, "ten bits");
    require(Mems19Protocol::kSlowBitDurationMs == 200, "five baud");
    for (int i = 0; i < Mems19Protocol::kSlowBitCount; ++i)
        require(Mems19Protocol::slowBitHigh(i) == expected[i],
                "incorrect 0x16 slow-init bit order");

    // The serial read may be fragmented, echoed, or preceded by zeroes.
    quint8 complementedKey = 0;
    require(!Mems19Protocol::keyComplement(QByteArray(), &complementedKey),
            "empty reply must not pass");
    require(!Mems19Protocol::keyComplement(QByteArray::fromHex("005576"),
                                           &complementedKey),
            "partial reply must not pass");
    require(Mems19Protocol::keyComplement(QByteArray::fromHex("0000557683"),
                                          &complementedKey),
            "expected ISO9141 sync was not found");
    require(complementedKey == 0x7Cu,
            "complement of 0x83 must be 0x7C");

    require(Mems19Protocol::keyComplement(QByteArray::fromHex("556512"),
                                          &complementedKey),
            "alternative key bytes must not be hardcoded away");
    require(complementedKey == 0xEDu, "alternative key complement wrong");
    require(!Mems19Protocol::keyComplement(QByteArray::fromHex("557683"),
                                           nullptr),
            "null output argument must not be used");
    require(!Mems19Protocol::hasAddressConfirmation(
                QByteArray::fromHex("007CE8")), "wrong address ack");
    require(Mems19Protocol::hasAddressConfirmation(
                QByteArray::fromHex("007CE9")), "E9 address confirmation");

    // Confirmed user cables: KKL CH340 first, legacy FT232 ROSCO first.
    DetectedSerialAdapter cable;
    cable.hasVendorId = true;
    cable.hasProductId = true;
    cable.vendorId = 0x1A86u;
    cable.productId = 0x7523u;
    require(SerialAdapterDetector::prefersKLineFirst(cable),
            "known CH340 KKL adapter must try MEMS 1.9 first");
    cable.vendorId = 0x0403u;
    cable.productId = 0x6001u;
    require(!SerialAdapterDetector::prefersKLineFirst(cable),
            "FT232 historical ROSCO adapter must retain its priority");
    cable.vendorId = 0x1A86u;
    cable.productId = 0x7522u;
    require(!SerialAdapterDetector::prefersKLineFirst(cable),
            "USB manufacturer alone must not classify a KKL cable");
    cable.productId = 0x7523u;
    cable.hasProductId = false;
    require(!SerialAdapterDetector::prefersKLineFirst(cable),
            "incomplete USB identity must not be assumed");

    // K-Line cable echo must be removed without touching ECU data.
    require(Mems19Protocol::normalizedReply(
                QByteArray::fromHex("D1D1414243"), 0xD1u, true) ==
                QByteArray::fromHex("D1414243"),
            "K-Line local command echo should be removed");
    require(Mems19Protocol::normalizedReply(
                QByteArray::fromHex("D1414243"), 0xD1u, false) ==
                QByteArray::fromHex("D1414243"),
            "no-echo cable must preserve ECU response");
    require(Mems19Protocol::normalizedReply(
                QByteArray::fromHex("D1D1414243"), 0xD1u, false) ==
                QByteArray::fromHex("D1D1414243"),
            "disabled cable echo must not strip bytes");
    require(Mems19Protocol::normalizedReply(
                QByteArray::fromHex("D1"), 0xD1u, true) ==
                QByteArray::fromHex("D1"),
            "incomplete reply must remain unchanged");

    std::cout << "PASS MEMS 1.9 slow init, fragmented ISO9141, cable preference\n";
    return 0;
}
