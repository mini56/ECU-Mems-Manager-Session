#include "diagnosticpanel.h"
#include "i18n.h"
#include <QApplication>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <QLabel>
#include <iostream>
using namespace Mems16Na;
#define CHECK(x) do{if(!(x)){std::cerr<<"FAIL "<<__LINE__<<" "<<#x<<"\n";return 1;}}while(0)
static void tx(SessionEvidence &s,unsigned c,const std::vector<std::uint8_t> &b) {
    s.tx({static_cast<std::uint8_t>(c)});s.rx(b);s.finish();
}
static SessionEvidence evidence(const char *name,std::uint8_t p0,std::uint8_t p1,std::uint8_t p2,
                               unsigned f1,unsigned f2,bool ready=true) {
    SessionEvidence s;
    tx(s,0xd0,{0xd0,p0,p1,p2,2});
    std::vector<std::uint8_t> reply{0xd1};
    for(int i=0;i<8;++i)reply.push_back(name[i]);
    reply.insert(reply.end(),{p0,p1,p2,2});tx(s,0xd1,reply);
    if(!ready)return s;
    std::vector<std::uint8_t> a(29,0),b(33,0);
    a[0]=0x80;a[1]=28;a[5]=255;a[7]=255;a[14]=0;a[15]=f1;a[17]=32;
    b[0]=0x7d;b[1]=32;b[6]=f2;b[10]=1;b[12]=129;b[15]=255;b[17]=255;b[18]=255;b[21]=0x7f;b[22]=0xff;b[23]=255;
    tx(s,0x80,a);tx(s,0x7d,b);
    return s;
}
int main(int argc,char **argv) {
    QApplication app(argc,argv);
    CHECK(I18n::load(QStringLiteral("fr")));
    DiagnosticPanel panel;
    auto *table=panel.findChild<QTableWidget*>();
    auto *report=panel.findChild<QPlainTextEdit*>();
    CHECK(table&&report);
    mems_data d{};d.battery_voltage=138;d.coolant_temp=100;d.engine_rpm=900;
    d.map_kpa=40;d.short_term_fuel_trim=100;d.coil_time=1250;d.dtc2=0x40;
    auto na=evidence("AANMP002",0x98,0,2,0,0x40);
    CHECK(na.ready());
    panel.updateData(&d,&na);
    QString t=report->toPlainText();
    CHECK(t.contains(QStringLiteral("AANMP002")));
    CHECK(t.contains(I18n::text(29019))&&t.contains(I18n::text(29003)));
    CHECK(!t.contains(I18n::text(6884).section('%',0,0)));
    CHECK(t.contains(QStringLiteral("-1 RPM"))&&t.contains(QStringLiteral("8 µs")));
    CHECK(table->item(3,2)->text()==I18n::text(6841));
    auto ek=evidence("EKDMF002",0x90,0,2,0x0c,0);
    CHECK(ek.ready());
    panel.updateData(&d,&ek);
    CHECK(table->item(2,3)->text().contains(I18n::text(29010)));
    CHECK(table->item(2,3)->text().contains(I18n::text(29011)));
    CHECK(table->item(2,2)->text()==I18n::text(29051));
    na.discardPair();panel.updateData(&d,&na);
    CHECK(table->rowCount()==8);
    CHECK(report->toPlainText().contains(I18n::text(29048)));
    CHECK(!report->toPlainText().contains(QStringLiteral("-1 RPM")));
    panel.endNaSession();CHECK(table->rowCount()==0&&report->toPlainText().isEmpty());
    panel.updateData(&d);CHECK(table->item(0,0)->text()==I18n::text(6813));
    std::cout<<"MEMS16NA_DIAGNOSTIC_QT_OK: profile-specific faults, state 40, units, invalid frames, disconnect and legacy fallback\n";
}
