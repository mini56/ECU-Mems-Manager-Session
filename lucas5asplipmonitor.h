#ifndef LUCAS5ASPLIPMONITOR_H
#define LUCAS5ASPLIPMONITOR_H

// Only observes Lucas 5AS normal-mode status bytes; not a PLIP programmer.
// The bytes may vary without a key-fob press on some module generations.
#include <QString>
#include <QVariantMap>
#include <QVector>
#include <array>

namespace Lucas5AsPlip {
struct Slot {
    QString raw;
    int changes=0;
    bool available=false;
};
class Monitor {
public:
    void reset() {for(Slot &slot:m_slots)slot=Slot();}
    Slot slot(int index) const {
        return index>=0 && index<4?m_slots[std::size_t(index)]:Slot();
    }
    QVector<int> observe(const QVariantMap &fields)
    {
        QVector<int> changed;
        for(int i=0;i<4;++i){
            Slot &entry=m_slots[std::size_t(i)];
            const QString raw=fields.value(QStringLiteral("plip%1").arg(i+1))
                                  .toString().trimmed();
            bool valid=false;
            const uint number=raw.size()==4 &&
                raw.startsWith(QStringLiteral("0x"),Qt::CaseInsensitive)
                ?raw.mid(2).toUInt(&valid,16):0;
            if(!valid || number>255){
                entry.available=false;
                entry.raw.clear();
                continue; // Missing data are not zero.
            }
            const QString value=QStringLiteral("0x")+raw.mid(2).toUpper();
            if(entry.available && entry.raw!=value){
                if(entry.changes<999999)++entry.changes;
                changed.append(i+1);
            }
            entry.raw=value;
            entry.available=true;
        }
        return changed;
    }
private:
    std::array<Slot,4> m_slots;
};
}
#endif // LUCAS5ASPLIPMONITOR_H
