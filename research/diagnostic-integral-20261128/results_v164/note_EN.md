# Research V1.64 — memory readers: complete static coverage

Corrected V1.63 run 46135232161 succeeded. It retained the previous 345 legacy proofs and linked 134 service-23 readers (9 MEMS 2J, 95 MEMS 3, 30 Td5). The seven V1.62 identifiers are Git blob SHA-1 values, not MD5 hashes.

The remaining four L-Series Di images were examined directly in ASM and binary: mode 1E, selector range 00–7F, word reader, DC command, state 8 and storage in the same variable read by the memory reader. DRAG0522 has a distinct block variable. Comparison constant 001A was observed; do not extrapolate bounds or dialogue conditions. The evidence includes addresses and bytes. The cumulative matrix retains each firmware and its status.

483/483 memory readers statically linked: 345 legacy MEMS + 134 service 23 + 4 L-Series Di. This is not exhaustive coverage of all diagnostic functions, nor validated ECU dialogue or hardware testing. Next: service-21 group tables, coolant fallback and per-firmware data availability. No Manager or DLL changes.
