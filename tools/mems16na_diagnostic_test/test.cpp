#include "diagnosticpanel.h"
#include "summarytab.h"
#include "mems16na_presentation.h"
#include "expert/IaMemsNaContext.h"
#include <QPushButton>
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
                               unsigned f1,unsigned f2,bool ready=true,std::uint8_t suffix=2) {
    SessionEvidence s;
    tx(s,0xd0,{0xd0,p0,p1,p2,suffix});
    std::vector<std::uint8_t> reply{0xd1};
    for(int i=0;i<8;++i)reply.push_back(name[i]);
    reply.insert(reply.end(),{p0,p1,p2,suffix});tx(s,0xd1,reply);
    if(!ready)return s;
    std::vector<std::uint8_t> a(29,0),b(33,0);
    a[0]=0x80;a[1]=28;a[5]=255;a[7]=255;a[14]=0;a[15]=f1;a[17]=32;
    b[0]=0x7d;b[1]=32;b[6]=f2;b[10]=1;b[12]=129;b[15]=255;b[17]=255;b[18]=255;b[21]=0x7f;b[22]=0xff;b[23]=255;
    a[16]=8;b[20]=126;b[7]=168;b[11]=4;
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
    CHECK(panel.naDiagnosticReport().contains(I18n::text(29019)));
    auto observed=na;
    observed.observeCoolantFault(true,1000);
    panel.updateData(&d,&observed);
    CHECK(report->toPlainText().contains(I18n::text(2972904)));
    CHECK(!report->toPlainText().contains(I18n::text(2972901)));
    observed.observeCoolantFault(false,2000);
    observed.observeCoolantFault(true,3000);
    panel.updateData(&d,&observed);
    CHECK(report->toPlainText().contains(I18n::text(2972901)));
    CHECK(!table->findItems(I18n::text(2972901),Qt::MatchExactly).isEmpty());
    panel.endNaSession();
    panel.updateData(&d,&na);
    CHECK(!report->toPlainText().contains(I18n::text(2972901)));
    const auto ai=IaMemsNa::observation(na,d,1000);
    CHECK(ai.values.value(QStringLiteral("injection_time_us"))==8);
    CHECK(ai.values.value(QStringLiteral("idle_error_rpm"))==-1);
    CHECK(ai.values.value(QStringLiteral("idle_signed_rpm"))==-50);
    CHECK(ai.values.value(QStringLiteral("na_dtc2_fault"))==0);
    CHECK(ai.values.value(QStringLiteral("na_dtc2_state"))==64);
    CHECK(!ai.values.contains(QStringLiteral("long_term_trim_pct")));
    CHECK(!ai.values.contains(QStringLiteral("idle_error_hot_corrected")));
    CHECK(!ai.values.contains(QStringLiteral("ambient_c")));
    CHECK(!ai.values.contains(QStringLiteral("fuel_c")));
    CHECK(IaMemsNa::dtcAnswer(4,na,false).contains(I18n::text(29001)));
    CHECK(IaMemsNa::dtcAnswer(2,na,false).contains(I18n::text(29019)));
    CHECK(IaMemsNa::requestedDtc(QStringLiteral("DTC 4"))==4);
    CHECK(IaMemsNa::requestedDtc(QStringLiteral("7D/10"))==6);
    CHECK(IaMemsNa::otherRoute(QStringLiteral("dépose du papillon")));
    CHECK(IaMemsNa::requestedProfile(QStringLiteral("AANMP002 DTC 1"))==na.identifiedProfile());
    CHECK(IaMemsNa::referenceAnswer(QStringLiteral("AANMP002 DTC 4")).contains(I18n::text(29001)));
    CHECK(IaMemsNa::measurementDetails(na).contains(QStringLiteral("8 µs")));
    CHECK(!IaMemsNa::measurementDetails(na).contains(QStringLiteral("255 °C")));
    CHECK(documentedUnusedDtc(Group::Dtc3)&&documentedUnusedDtc(Group::Dtc4)&&documentedUnusedDtc(Group::Dtc5)&&documentedUnusedDtc(Group::Indirect7D10));
    CHECK(!documentedUnusedDtc(Group::Dtc0)&&!documentedUnusedDtc(Group::Dtc1)&&!documentedUnusedDtc(Group::Dtc2));
    CHECK(documentation(dtcDocumentation(Group::Dtc1))==dtcDocumentation(Group::Dtc1));
    const QByteArray sameId=QByteArray::fromHex("98000202");
    panel.setEcuId(sameId);panel.updateData(&d,&na);
    auto buttons=panel.findChildren<QPushButton*>();
    for(auto *button:buttons)if(button->text()==I18n::text(6805))button->click();
    const QString referencePrefix=I18n::text(6889).section('%',0,0).trimmed();
    CHECK(!referencePrefix.isEmpty()&&report->toPlainText().contains(referencePrefix));
    panel.setEcuId(sameId);
    CHECK(table->rowCount()>8&&report->toPlainText().contains(referencePrefix));
    auto different=evidence("AANMP002",0x98,0,2,0,0x40,true,3);
    panel.updateData(&d,&different);
    CHECK(!report->toPlainText().contains(referencePrefix));
    app.setProperty("ecuDarkTheme",false);panel.updateData(&d,&na);
    CHECK(table->styleSheet().contains(QStringLiteral("#202936")));
    app.setProperty("ecuDarkTheme",true);panel.updateData(&d,&na);
    CHECK(!table->styleSheet().contains(QStringLiteral("#202936")));
    auto ek=evidence("EKDMF002",0x90,0,2,0x0c,0);
    CHECK(ek.ready());
    panel.updateData(&d,&ek);
    CHECK(table->item(2,3)->text().contains(I18n::text(29010)));
    CHECK(table->item(2,3)->text().contains(I18n::text(29011)));
    CHECK(table->item(2,2)->text()==I18n::text(29051));
    na.discardPair();panel.updateData(&d,&na);
    CHECK(IaMemsNa::observation(na,d,1001).values.isEmpty());
    CHECK(table->rowCount()==8);
    CHECK(report->toPlainText().contains(I18n::text(29048)));
    CHECK(!report->toPlainText().contains(QStringLiteral("-1 RPM")));
    panel.endNaSession();CHECK(table->rowCount()==0&&report->toPlainText().isEmpty());
    panel.updateData(&d);CHECK(table->item(0,0)->text()==I18n::text(6813));
    // The manually started coolant observation is independent of lambda validity.
    auto coolantFrame=[&](unsigned raw,unsigned rpm,unsigned fault){
        auto sample=evidence("AANMP002",0x98,0,2,0,0,false);
        std::vector<std::uint8_t> a(29,0),b(33,0);
        a[0]=0x80;a[1]=28;a[2]=rpm>>8;a[3]=rpm&255;a[4]=raw;a[14]=fault;
        b[0]=0x7d;b[1]=32;b[10]=255;b[11]=255;
        tx(sample,0x80,a);tx(sample,0x7d,b);return sample;
    };
    auto *start=panel.findChild<QPushButton*>(QStringLiteral("coolantTestStart"));
    auto *stop=panel.findChild<QPushButton*>(QStringLiteral("coolantTestStop"));
    auto *values=panel.findChild<QLabel*>(QStringLiteral("coolantTestValues"));
    auto *status=panel.findChild<QLabel*>(QStringLiteral("coolantTestStatus"));
    CHECK(start&&stop&&values&&status);
    auto cold=coolantFrame(75,900,0);
    panel.updateData(&d,&cold);CHECK(start->isEnabled()&&!stop->isEnabled());start->click();
    CHECK(!start->isEnabled()&&stop->isEnabled()&&values->text().contains(QStringLiteral("20 °C")));
    auto warmer=coolantFrame(80,900,0);panel.updateData(&d,&warmer);stop->click();
    CHECK(status->text().contains(I18n::text(2971820))&&values->text().contains(QStringLiteral("5 °C")));
    CHECK(report->toPlainText().contains(I18n::text(2971820)));
    start->click();auto stopped=coolantFrame(80,0,0);panel.updateData(&d,&stopped);
    CHECK(!start->isEnabled()&&!stop->isEnabled()&&status->text().contains(I18n::text(2971822)));
    panel.endNaSession();CHECK(!start->isEnabled()&&!stop->isEnabled());
    auto extended=evidence("KBHBG009",0x90,0,9,0,0,false);
    std::vector<std::uint8_t> a19(28),b19(33);
    a19[0]=28;a19[16]=32;a19[17]=143;
    b19[0]=33;b19[19]=126;b19[20]=0x7f;b19[21]=0xff;b19[31]=7;
    for(unsigned j=0;j<4;++j){b19[23+2*j]=42;b19[24+2*j]=128;}
    extended.setExtended80(a19);extended.setExtended7d(b19);
    panel.updateData(&d,&extended);
    CHECK(report->toPlainText().contains(QStringLiteral("14.3 V")));
    CHECK(report->toPlainText().contains(QStringLiteral("-1 RPM")));
    CHECK(report->toPlainText().contains(QStringLiteral("42.50 ")+I18n::text(2972003)));
    extended.discardPair();panel.updateData(&d,&extended);
    CHECK(!report->toPlainText().contains(QStringLiteral("14.3 V")));
    CHECK(!report->toPlainText().contains(QStringLiteral("42.50 ")+I18n::text(2972003)));
    extended.setExtended80(a19);extended.setExtended7d(b19);
    panel.endNaSession();
    // Multi-ECU: a generic reference belongs to this identity and connection.
    auto capture=[&](){for(auto *button:buttons)if(button->text()==I18n::text(6805))button->click();};
    auto generic19=evidence("KBHBG009",0x90,0,9,0,0,false);
    generic19.setExtended80(a19);generic19.setExtended7d(b19);
    auto other19=evidence("KBHBV001",0xc2,0,1,0,0,false);
    other19.setExtended80(a19);other19.setExtended7d(b19);
    d.dtc0=0;d.dtc1=0;d.dtc2=0;
    panel.updateData(&d,&generic19);capture();
    CHECK(report->toPlainText().contains(referencePrefix));
    CHECK(report->toPlainText().contains(I18n::text(2973301)));
    panel.updateData(&d,&generic19);
    CHECK(report->toPlainText().contains(referencePrefix));
    panel.updateData(&d,&other19);
    CHECK(!report->toPlainText().contains(referencePrefix));
    capture();CHECK(report->toPlainText().contains(referencePrefix));
    panel.endNaSession();CHECK(report->toPlainText().isEmpty());
    panel.updateData(&d,&other19);CHECK(!report->toPlainText().contains(referencePrefix));
    panel.setEcuId(QByteArray::fromHex("90000102"));
    panel.updateData(&d,&other19);capture();
    panel.setEcuId(QByteArray::fromHex("90000902"));
    CHECK(report->toPlainText().isEmpty());
    panel.updateData(&d,&generic19);CHECK(!report->toPlainText().contains(referencePrefix));
    panel.updateData(&d);capture();CHECK(report->toPlainText().contains(referencePrefix));
    panel.endNaSession();panel.updateData(&d);CHECK(!report->toPlainText().contains(referencePrefix));
    QVariantMap structured;structured.insert(QStringLiteral("rpm"),900);
    panel.update2JData(structured,QStringList(),QStringLiteral("MEMS 2J"));capture();
    CHECK(report->toPlainText().contains(QStringLiteral("Référence enregistrée le")));
    panel.update2JData(structured,QStringList(),QStringLiteral("MEMS 2J"));
    CHECK(report->toPlainText().contains(QStringLiteral("Référence enregistrée le")));
    panel.update2JData(structured,QStringList(),QStringLiteral("MEMS 3"));
    CHECK(!report->toPlainText().contains(QStringLiteral("Référence enregistrée le")));
    capture();panel.endNaSession();
    panel.update2JData(structured,QStringList(),QStringLiteral("MEMS 3"));
    CHECK(!report->toPlainText().contains(QStringLiteral("Référence enregistrée le")));
    panel.endNaSession();
    // The last report is not a live diagnosis after loss of communications.
    auto captureButton=[&]()->QPushButton*{for(auto *button:buttons)if(button->text()==I18n::text(6805))return button;return nullptr;};
    auto *cap=captureButton();CHECK(cap&&!cap->isEnabled());
    for(const QString &family:{QStringLiteral("MEMS 2J"),QStringLiteral("MEMS 3")}){
        panel.update2JData(structured,QStringList(),family);CHECK(cap->isEnabled());
        panel.invalidateCoolantTest();
        CHECK(!cap->isEnabled()&&table->rowCount()==0);
        CHECK(report->toPlainText().startsWith(I18n::text(2974001)));
        const auto staleReport=report->toPlainText();
        panel.invalidateCoolantTest();CHECK(report->toPlainText()==staleReport);
        panel.update2JData(structured,QStringList(),family);CHECK(cap->isEnabled());
        CHECK(!report->toPlainText().startsWith(I18n::text(2974001)));
        panel.endNaSession();
    }
    panel.updateData(&d,&generic19);CHECK(cap->isEnabled());
    panel.invalidateCoolantTest();CHECK(!cap->isEnabled()&&table->rowCount()==0);
    CHECK(report->toPlainText().startsWith(I18n::text(2974001)));
    panel.updateData(&d,&generic19);CHECK(cap->isEnabled());
    panel.endNaSession();
    // Non-NA unknown DTC2 must not be presented as a proven TPS or lambda fault.
    auto unknown=evidence("KBHBV001",0xc2,0,1,0,0,false);
    auto rawUnknown=b19;rawUnknown[5]=255;unknown.setExtended80(a19);unknown.setExtended7d(rawUnknown);
    d.dtc0=0;d.dtc1=0;d.dtc2=255;
    panel.updateData(&d,&unknown);
    auto stateOf=[&](int id)->QString{
        for(int row=0;row<table->rowCount();++row)
            if(table->item(row,0)->text()==I18n::text(id))return table->item(row,2)->text();
        return QString();
    };
    CHECK(stateOf(6835)==I18n::text(6849)&&stateOf(6838)==I18n::text(6849));
    CHECK(report->toPlainText().contains(I18n::text(2972310))&&report->toPlainText().contains(QStringLiteral("0xFF")));
    auto known=generic19;auto rawKnown=b19;rawKnown[5]=4;known.setExtended80(a19);known.setExtended7d(rawKnown);
    d.dtc2=4;panel.updateData(&d,&known);CHECK(stateOf(6838)==I18n::text(6840));
    known.discardPair();panel.updateData(&d,&known);CHECK(stateOf(6838)!=I18n::text(6840));
    CHECK(report->toPlainText().contains(I18n::text(29048)));
    for(int row=0;row<table->rowCount();++row)
        if(table->item(row,0)->text()==QStringLiteral("DTC 2"))
            CHECK(table->item(row,1)->text()==QStringLiteral("--"));
    d.dtc2=0;panel.endNaSession();
    // Real SummaryTab widget: both themes, all six languages, target widths.
    for(const char *lang:{"fr","en","de","es","it","pt"}) {
        CHECK(I18n::load(QString::fromLatin1(lang)));
        SummaryTab summary;
        auto good=evidence("AANMP002",0x98,0,2,0xA0,0x40);
        auto find=[&](int id)->QTableWidgetItem* {
            for(auto *tbl:summary.findChildren<QTableWidget*>())
                for(int i=0;i<tbl->rowCount();++i)
                    if(tbl->item(i,0)&&tbl->item(i,0)->text()==I18n::text(id))
                        return tbl->item(i,0);
            return nullptr;
        };
        auto value=[&](int id,int column)->QString {
            auto *it=find(id);return it?it->tableWidget()->item(it->row(),column)->text():QString();
        };
        // Regression: initial disconnected view must show the documented
        // fields and all permanent DTC bubbles, with no fake measurements.
        auto checkOffline=[&]()->bool {
            for(int id:{29022,29023,29024,29025,29026,29027,29028,29029,29030,29031,29033}) {
                auto *it=find(id);if(!it||value(id,2)!=QStringLiteral("--")||value(id,3)!=QStringLiteral("--"))return false;
            }
            for(int i=1;i<=4;++i) {
                bool found=false;
                for(auto *tbl:summary.findChildren<QTableWidget*>())
                    for(int row=0;row<tbl->rowCount();++row)
                        if(tbl->item(row,0)->text()==I18n::text(29032).arg(i)&&!tbl->isRowHidden(row)
                           &&!tbl->item(row,1)->icon().isNull())found=true;
                if(!found)return false;
            }
            for(int id:{6657,6658,6630,6639,6642,6647,29081,5198,29079,29080,6632,6633}) {
                auto *it=find(id);if(!it)return false;
                auto *bubble=it->tableWidget()->item(it->row(),1);
                if(bubble->icon().isNull()||bubble->toolTip().isEmpty())return false;
            }
            for(int id:{6639,6642,6647,29081})
                if(value(id,2)!=QStringLiteral("--")||value(id,3)!=I18n::text(29082))return false;
            auto *dtc=find(6658);
            return dtc&&dtc->toolTip().contains(I18n::text(29010))&&dtc->toolTip().contains(I18n::text(29012))
                &&dtc->toolTip().contains(I18n::text(29013))&&dtc->toolTip().contains(I18n::text(29015));
        };
        CHECK(checkOffline());
        auto historical=evidence("UNKNOWN0",0x90,0,1,0,0,false);
        auto rawHistorical=b19;rawHistorical[11]=129;rawHistorical[12]=103;
        historical.setExtended80(a19);historical.setExtended7d(rawHistorical);
        summary.updateData(&d,&historical);
        CHECK(value(2971701,2)==QStringLiteral("129"));
        CHECK(value(2971701,3).contains(QStringLiteral("1"))&&value(2971701,3).contains(I18n::text(2973901)));
        CHECK(value(6637,2)==QStringLiteral("103"));
        CHECK(value(6637,3).contains(QStringLiteral("3"))&&value(6637,3).contains(I18n::text(2973901)));
        CHECK(!value(6637,3).contains(QStringLiteral("%")));
        historical.discardPair();summary.updateData(&d,&historical);
        CHECK(value(2971701,2)==QStringLiteral("--")&&value(2971701,3)==I18n::text(29048));
        auto lost19=extended;lost19.discardPair();
        summary.updateData(&d,&lost19);
        CHECK(value(6630,2)==QStringLiteral("--"));
        CHECK(value(6630,3).contains(I18n::text(29048)));
        summary.updateData(&d,&extended);
        CHECK(value(29026,3)==QStringLiteral("14.3 V"));
        CHECK(value(29031,3)==QStringLiteral("-1 RPM"));
        CHECK(value(2972001,3)==QStringLiteral("42.50 ")+I18n::text(2972003));
        CHECK(value(29033,3)==QStringLiteral("7"));
        summary.updateData(&d,&good);
        CHECK(!find(2972001));
        summary.showNaDocumentation();CHECK(checkOffline());
        for(bool dark:{false,true}) {
            summary.applyVisualTheme(dark);
            for(int width:{1200,1366,1920}) {
                summary.resize(width,600);summary.show();app.processEvents();
                for(auto *tbl:summary.findChildren<QTableWidget*>())CHECK(tbl->iconSize()==QSize(14,14));
                summary.updateData(&d,&good);
                CHECK(value(6644,3)==QStringLiteral("-50 RPM"));
                CHECK(value(6631,3)==QStringLiteral("840 mV"));
                CHECK(value(6635,3)==I18n::text(6673));
                for(int id:{6657,6658,6630,6639,6642,6647,29081}) {
                    auto *it=find(id);CHECK(it);
                    auto *bubble=it->tableWidget()->item(it->row(),1);
                    CHECK(!bubble->icon().isNull()&&!bubble->toolTip().isEmpty());
                }
                auto *dtc=find(6658);CHECK(dtc);
                CHECK(dtc->toolTip().contains(I18n::text(29015))&&dtc->toolTip().contains(I18n::text(29016)));
                CHECK(value(6658,2)==QStringLiteral("160"));
                CHECK(value(6658,3)==I18n::text(29015)+QStringLiteral(" ; ")+I18n::text(29016));
                CHECK(!dtc->toolTip().contains(QStringLiteral("0x"),Qt::CaseInsensitive));
                CHECK(!dtcDocumentation(Group::Dtc1).contains(QStringLiteral("0x"),Qt::CaseInsensitive));
                auto simple=evidence("AANMP002",0x98,0,2,0x20,0);
                summary.updateData(&d,&simple);
                CHECK(value(6658,2)==QStringLiteral("32")&&value(6658,3)==I18n::text(29015));
                auto none=evidence("AANMP002",0x98,0,2,0,0);
                summary.updateData(&d,&none);
                CHECK(value(6658,3)==I18n::text(29003));
                summary.updateData(&d,&good);
                for(int id:{5198,29079,29080,6632,6633,6639,29081,6642,6647})CHECK(find(id));
                auto missing=good;missing.discardPair();summary.updateData(&d,&missing);
                CHECK(value(6603,2)==QStringLiteral("--")&&value(6603,3)==I18n::text(29048));
                CHECK(value(6631,3)==I18n::text(29048)&&value(6644,3)==I18n::text(29048));
                CHECK(value(6657,3)==I18n::text(29048));
                summary.updateData(&d);CHECK(value(6603,2)==QStringLiteral("900"));
                summary.showNaDocumentation();CHECK(checkOffline());
            }
        }
    }
    std::cout<<"MEMS16NA_SUMMARY_QT_OK: six languages, two themes, three widths, seven permanent DTC icons, invalid and legacy transitions\n";
    std::cout<<"MEMS16NA_DIAGNOSTIC_QT_OK: profile-specific faults, state 40, units, invalid frames, disconnect and legacy fallback\n";
}
