#include "lucas5asplipmonitor.h"
#include <QCoreApplication>
#include <QDebug>

int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv);
    Lucas5AsPlip::Monitor mon;
    QVariantMap first;
    for(int n=1;n<=4;++n)
        first.insert(QStringLiteral("plip%1").arg(n),QStringLiteral("0x00"));
    if(!mon.observe(first).isEmpty())return 1;
    if(!mon.observe(first).isEmpty())return 2;
    QVariantMap second=first;
    second[QStringLiteral("plip2")]=QStringLiteral("0xFF");
    second[QStringLiteral("plip4")]=QStringLiteral("0xAB");
    const QVector<int> changed=mon.observe(second);
    if(changed.size()!=2 || changed.at(0)!=2 || changed.at(1)!=4 ||
       mon.slot(1).changes!=1 || mon.slot(3).changes!=1)return 3;
    second.remove(QStringLiteral("plip2"));
    if(!mon.observe(second).isEmpty() || mon.slot(1).available)return 4;
    second[QStringLiteral("plip2")]=QStringLiteral("0xFF");
    if(!mon.observe(second).isEmpty())return 5;
    second[QStringLiteral("plip1")]=QStringLiteral("0xGG");
    if(!mon.observe(second).isEmpty() || mon.slot(0).available)return 6;
    mon.reset();
    if(mon.slot(1).changes!=0 || mon.slot(1).available)return 7;
    qInfo() << "LUCAS_5AS_PLIP_MONITOR_SELFTEST_PASS";
    return 0;
}
