#include "MemsDatabaseUniversalAccess.h"
#include "../i18n.h"

#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include <array>

namespace {

bool expect(bool condition,const char *message)
{
    if(!condition) qCritical().noquote() << "FAIL MEMS database content:" << message;
    return condition;
}

}

int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv);
    bool ok=true;

    const QString multilingual=QStringLiteral(
        "title_fr: Sonde temperature air\n"
        "title_en: Intake air sensor\n"
        "title_es: Sensor de aire\n"
        "title_it: Sensore aria\n"
        "title_pt: Sensor de ar\n"
        "title_de: Ansaugluftsensor\n"
        "description_fr: Mesure de l'air\n"
        "description_en: A & B <C>\n"
        "description_es: Detalle ES\n"
        "description_it: Dettaglio IT\n"
        "description_pt: Detalhe PT\n"
        "description_de: Detail DE");

    struct LanguageSample {const char *code;const char *expectedTitle;const char *otherTitle;};
    const std::array<LanguageSample,6> languages={{{"fr","Sonde temperature air","Intake air sensor"},
                                                   {"en","Intake air sensor","Sonde temperature air"},
                                                   {"es","Sensor de aire","Intake air sensor"},
                                                   {"it","Sensore aria","Intake air sensor"},
                                                   {"pt","Sensor de ar","Intake air sensor"},
                                                   {"de","Ansaugluftsensor","Intake air sensor"}}};
    for(const LanguageSample &language:languages)
    {
        I18n::load(QString::fromLatin1(language.code));
        const QString expected=QString::fromUtf8(language.expectedTitle);
        const QString other=QString::fromUtf8(language.otherTitle);
        const QString title=MemsDatabaseContent::localizedTitle(multilingual,QStringLiteral("fallback"));
        ok=expect(title==expected,"active-language title not selected")&&ok;

        const QString html=MemsDatabaseContent::renderResultHtml(
            title,multilingual,QStringLiteral("documentation"),QStringLiteral("1.6"),
            QStringLiteral("xml_index"),QString());
        ok=expect(html.contains(expected),"active-language title missing from rendered sheet")&&ok;
        ok=expect(!html.contains(other),"another language leaked into rendered sheet")&&ok;
        ok=expect(html.contains(QStringLiteral("MEMS 1.6")),"ECU generation lost")&&ok;
        if(QString::fromLatin1(language.code)==QStringLiteral("en"))
        {
            ok=expect(html.contains(QStringLiteral("A &amp; B &lt;C&gt;")),
                      "HTML text not escaped")&&ok;
            ok=expect(!html.contains(QStringLiteral("Mesure de l'air")),
                      "French description leaked into English sheet")&&ok;
        }
    }

    I18n::load(QStringLiteral("en"));
    const QString missingTitle=MemsDatabaseContent::localizedTitle(
        QStringLiteral("description_en: text only"),QStringLiteral("Fallback title"));
    ok=expect(missingTitle==QStringLiteral("Fallback title"),"fallback title lost")&&ok;

    const QString diagram=QStringLiteral("<p id='trusted-diagram'>SCHEMA</p>");
    const QString wiring=MemsDatabaseContent::renderResultHtml(
        QStringLiteral("Wire <A>"),QStringLiteral("description_en: Red & Green"),
        QStringLiteral("wiring"),QStringLiteral("1.9"),QStringLiteral("wiring_index"),diagram);
    ok=expect(wiring.contains(diagram),"trusted wiring diagram lost")&&ok;
    ok=expect(wiring.contains(QStringLiteral("Wire &lt;A&gt;")),"title HTML escaping broken")&&ok;
    ok=expect(wiring.contains(QStringLiteral("Red &amp; Green")),"wiring description escaping broken")&&ok;

    const QString escapedAsset=MemsDatabaseContent::renderResultHtml(
        QStringLiteral("Protected asset"),QStringLiteral("relative_path: ../../private.svg\n"
                                                         "description_en: Unavailable asset"),
        QStringLiteral("wiring"),QStringLiteral("1.2"),QStringLiteral("wiring_assets"),QString());
    ok=expect(escapedAsset.contains(QStringLiteral("Unavailable asset")),
              "missing asset fallback not shown")&&ok;
    ok=expect(!escapedAsset.contains(QStringLiteral("<img")),
              "unsafe or missing image unexpectedly rendered")&&ok;

    if(ok) qInfo().noquote() << "PASS MEMS database: six languages, safe HTML, wiring assets and fallback";
    return ok?0:1;
}
