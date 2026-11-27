#pragma once
#include "mems220_measurements.h"
#include "mems16na_presentation.h"
namespace Mems220 {
inline QString voltage(const Measurements &m){return Mems16Na::numeric(m.learnVoltage,1,QStringLiteral(" V"));}
inline QString idleError(const Measurements &m){return Mems16Na::numeric(m.idleError,0,QStringLiteral(" RPM"));}
inline QString reference(const Measurements &m){return Mems16Na::numeric(m.throttleReference,2,QStringLiteral(" ")+I18n::text(2972003));}
inline QString count(const Measurements &m){return Mems16Na::numeric(m.learnCount,0);}
inline QString inputState(const Mems16Na::Value &v){return v.availability==Mems16Na::Availability::Available?I18n::text(v.calculated?2972305:2972306):Mems16Na::statusText(v);}
}
