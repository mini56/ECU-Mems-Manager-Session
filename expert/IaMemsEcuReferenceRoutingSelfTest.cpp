#include "IaMemsEcuReferenceRouting.h"

#include <QCoreApplication>
#include <QVariantMap>
#include <QVariantList>
#include <QStringList>
#include <cstdio>

// Tests deliberately cover categories rather than a single ECU code.
// No real database is modified; this validates the routing gate and the
// evidence-only formatter independently of the installer and RAVE corpus.
static bool check(bool condition,const char *name)
{
    if(!condition)std::fprintf(stderr,"ECU_REFERENCE_ROUTING_FAIL: %s\n",name);
    return condition;
}

int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv);
    using namespace IaMemsEcuReferenceRouting;
    bool ok=true;
    const QStringList codeSamples={
        QStringLiteral("MNE101351"),QStringLiteral("MKC102122"),
        QStringLiteral("NNN100743"),QStringLiteral("AANMP002"),
        QStringLiteral("ZXY901234"),QStringLiteral("ABC1234"),
        QStringLiteral("RVB654321")
    };
    for(const QString &code:codeSamples){
        ok=check(referenceFromQuestion(code.toLower())==code,
                 "bare alphanumeric ECU-like identifier") && ok;
        ok=check(referenceFromQuestion(
                     QStringLiteral("Que sait-on sur %1 ?").arg(code))==code,
                 "query suggested by the IA must extract the code") && ok;
        ok=check(referenceFromQuestion(
                     QStringLiteral("référence ECU : %1").arg(code))==code,
                 "explicit ECU reference phrasing") && ok;
        ok=check(referenceFromQuestion(
                     QStringLiteral("infos calculateur %1").arg(code))==code,
                 "ECU description phrasing") && ok;
        ok=check(exactCodeMention(
                     QStringLiteral("ECU %1 compatible").arg(code),code),
                 "exact token match in source evidence") && ok;
        ok=check(!exactCodeMention(
                     QStringLiteral("ECU X%1Y compatible").arg(code),code),
                 "no substring evidence allowed") && ok;
    }
    // All existing documentary, procedural, menu and diagnostic messages
    // must still go to their original specialised routes. This is a
    // *negative* regression suite rather than a collection of hardcoded
    // positive examples for a single failing question.
    const QStringList excluded={
        QStringLiteral("FREIN"),
        QStringLiteral("BIELLE ET PISTON"),
        QStringLiteral("INJECTEUR SPi"),
        QStringLiteral("VOLANT MOTEUR"),
        QStringLiteral("DEPOSE ETRIER DE FREIN"),
        QStringLiteral("Réglages moteur SPi boîte manuelle"),
        QStringLiteral("brochage ECU MNE101351"),
        QStringLiteral("couleur du fil ECU MKC102122"),
        QStringLiteral("diagnostic ECU AANMP002"),
        QStringLiteral("schéma électrique AANMP002"),
        QStringLiteral("réparation ECU MNE101351"),
        QStringLiteral("couple serrage 70.40.03"),
        QStringLiteral("purge frein MNE101351"),
        QStringLiteral("AANMP002 et NNN100743"),
        QStringLiteral("11"),
        QStringLiteral("70.40.03"),
        QStringLiteral("MEMS 1.9"),
        QStringLiteral("Quelle est la pression du collecteur ?"),
        QStringLiteral("Que sait-on sur les fusibles ?"),
        QStringLiteral("DÉPOSE"),
        QStringLiteral("Aide logiciel")
    };
    for(const QString &question:excluded){
        ok=check(referenceFromQuestion(question).isEmpty(),
                 "previous documentary/diagnostic/menu routes are unchanged") && ok;
    }
    // Publication identifiers are not ECU identities. The full service
    // additionally checks exact database matches before routing.
    ok=check(!recognizedEcuNamespace(QStringLiteral("RCL0193FRE")) &&
             !recognizedEcuNamespace(QStringLiteral("AKM7169ENG")),
             "RAVE publication IDs must stay documentary") && ok;
    for(const QString &code:codeSamples.mid(0,4))
        ok=check(recognizedEcuNamespace(code),"known ECU namespace") && ok;
    ok=check(!recognizedEcuNamespace(QStringLiteral("ZXY901234")) &&
             explicitEcuIdentityIntent(QStringLiteral("ECU ZXY901234")),
             "unknown prefix requires exact ECU record or explicit ECU intent") && ok;

    QVariantMap ecu;
    ecu.insert(QStringLiteral("part_number"),QStringLiteral("ZXY901234"));
    ecu.insert(QStringLiteral("brand"),QStringLiteral("Rover"));
    ecu.insert(QStringLiteral("system_family"),QStringLiteral("MEMS 1.3"));
    ecu.insert(QStringLiteral("injection"),QStringLiteral("SPi"));
    ecu.insert(QStringLiteral("models"),QStringLiteral("Mini"));
    ecu.insert(QStringLiteral("year_from"),1993);
    ecu.insert(QStringLiteral("year_to"),1996);
    ecu.insert(QStringLiteral("notes"),QStringLiteral("Référence vérifiée dans la base de test"));
    QVariantMap fitment;
    fitment.insert(QStringLiteral("make"),QStringLiteral("Rover"));
    fitment.insert(QStringLiteral("model"),QStringLiteral("Mini"));
    fitment.insert(QStringLiteral("engine"),QStringLiteral("1275"));
    QVariantMap file;
    file.insert(QStringLiteral("filename"),QStringLiteral("firmware.bin"));
    const QString grounded=exactEcuSummary(
        QStringLiteral("ZXY901234"),ecu,
        QVariantList{fitment},QVariantList{file});
    ok=check(grounded.contains(QStringLiteral("ZXY901234")) &&
             grounded.contains(QStringLiteral("MEMS 1.3")) &&
             grounded.contains(QStringLiteral("SPi")) &&
             grounded.contains(QStringLiteral("1993")) &&
             grounded.contains(QStringLiteral("1996")) &&
             grounded.contains(QStringLiteral("Source : base ECU locale")) &&
             grounded.contains(QStringLiteral("Rover — Mini — 1275")),
             "identified ECU must show only grounded fields and provenance") && ok;
    ok=check(!grounded.contains(QStringLiteral("MPi")) &&
             !grounded.contains(QStringLiteral("RCL0193FRE")) &&
             exactEcuSummary(QStringLiteral("ZXY901234"),QVariantMap{},
                             QVariantList{},QVariantList{}).isEmpty(),
             "do not invent an injection variant, manual or unknown ECU data") && ok;
    ok=check(notFound(QStringLiteral("ZXY901234"),true).contains(
                 QStringLiteral("références ECU")) &&
             !notFound(QStringLiteral("ZXY901234"),true).contains(
                 QStringLiteral("aucune rubrique")) &&
             notFound(QStringLiteral("ZXY901234"),false).contains(
                 QStringLiteral("pas pu être ouverte")),
             "missing references must not produce the RAVE failure answer") && ok;
    if(ok){
        std::puts("MEMS_ECU_REFERENCE_ROUTING_NON_REGRESSION_PASS");
        return 0;
    }
    return 1;
}
