#pragma once
#include <QtGlobal>
#include <QString>
// Shared with the standard mapped-injection reader. No duplicated firmware tables.
struct InjectionRapidPlan {
    quint16 base=0, source=0, counter=0;
    bool incremental=false, additive=false;
    double baseScale=0, transientScale=0;
    bool permitsBlock(quint8 b) const {return b==(base>>8)||b==(source>>8)||b==(counter>>8);}
    bool permitsWord(quint16 a) const {return a==base||a==source||a==(counter&0xFFFEu);}
    static quint8 command(quint16 a) {return quint8((a&0xFEu)>>1);}
    quint8 counterByte(quint16 w) const {return quint8((counter&1u)?w>>8:w);}
    bool event(quint8 now,bool have,quint8 prev) const {
        return incremental?(have&&now!=0&&now!=prev):now!=0;
    }
};
bool injectionRapidPlanForNa(const QString &firmware,InjectionRapidPlan *plan);
