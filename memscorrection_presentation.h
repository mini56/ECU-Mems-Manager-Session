#pragma once
#include "mems219_units.h"
#include "i18n.h"
#include <QString>
namespace MemsCorrection {
using Mems219::CorrectionKind;
using Mems219::CorrectionReading;
inline QString number(const CorrectionReading &r){
    return r.kind==CorrectionKind::Qualified||r.kind==CorrectionKind::Historical?QString::number(r.value):QStringLiteral("--");
}
inline QString unit(const CorrectionReading &r,bool lambda){
    if(r.kind==CorrectionKind::Qualified)return lambda?QStringLiteral("%"):QStringLiteral("µs");
    if(r.kind==CorrectionKind::Historical)return I18n::text(2973901);
    return QStringLiteral("--");
}
inline QString compact(const CorrectionReading &r,bool lambda){
    if(r.kind==CorrectionKind::Missing||r.kind==CorrectionKind::Unqualified)return QStringLiteral("--");
    return number(r)+QStringLiteral(" ")+unit(r,lambda);
}
inline QString detail(const CorrectionReading &r,bool lambda){
    if(r.kind==CorrectionKind::Missing)return I18n::text(29048);
    if(r.kind==CorrectionKind::Unqualified)return I18n::text(29002);
    return compact(r,lambda);
}
}
