#pragma once
// Historical ROSCO display/log convention, not a firmware-independent physical
// measurement. Dedicated NA and qualified 1.9 decoders remain independent.
namespace LegacyHotIdle {
inline int raw(unsigned high,unsigned low){return int(((high&255u)<<8)|(low&255u));}
inline int adjustment(unsigned hot){return int(hot)-35;}
inline int corrected(int rawValue,unsigned hot){return rawValue-32768-adjustment(hot);}
}
